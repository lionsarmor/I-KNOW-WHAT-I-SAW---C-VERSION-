/* ============================================================================
 * esp32_main.c -- the ESP32 platform layer.
 *
 * STATUS: skeleton, written against ESP-IDF v5.x + the esp_lcd_ili9341
 * managed component. It compiles the same portable core as the desktop
 * build; wire up your display pins below and iterate. (Developed and
 * tested on desktop first -- that's the workflow this repo is built for.)
 *
 * HARDWARE (default wiring -- change the PIN_* defines to match yours):
 *
 *   ILI9341 320x240 SPI LCD          buttons (one GPIO each, to GND,
 *     SCLK -> GPIO 18                 internal pull-ups enabled)
 *     MOSI -> GPIO 23                  UP    -> GPIO 32
 *     DC   -> GPIO 2                   DOWN  -> GPIO 33
 *     CS   -> GPIO 15                  LEFT  -> GPIO 25
 *     RST  -> GPIO 4                   RIGHT -> GPIO 26
 *     BL   -> 3V3 (or a GPIO)          A     -> GPIO 27
 *                                      B     -> GPIO 14
 *                                      START -> GPIO 12
 *
 * The 240x160 framebuffer is drawn centered on the 320x240 panel with a
 * black frame around it.
 *
 * AUDIO: game_audio_fill() is ready to be wired to I2S (external DAC or
 * a PAM8302 + delta-sigma). See the TODO at the bottom.
 * ==========================================================================*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"
#include "esp_log.h"
#include <string.h>

#include "game.h"

static const char *TAG = "iknowwhatisaw";

/* ---- pins: EDIT ME --------------------------------------------------------*/
#define PIN_LCD_SCLK  18
#define PIN_LCD_MOSI  23
#define PIN_LCD_DC     2
#define PIN_LCD_CS    15
#define PIN_LCD_RST    4

#define PIN_BTN_UP    32
#define PIN_BTN_DOWN  33
#define PIN_BTN_LEFT  25
#define PIN_BTN_RIGHT 26
#define PIN_BTN_A     27
#define PIN_BTN_B     14
#define PIN_BTN_START 12

#define LCD_W 320
#define LCD_H 240
#define LCD_SPI_HZ (40 * 1000 * 1000)   /* most ILI9341s take 40 MHz fine */

/* where the 240x160 game screen sits on the 320x240 panel */
#define OFF_X ((LCD_W - SCREEN_W) / 2)
#define OFF_Y ((LCD_H - SCREEN_H) / 2)

/* SPI LCDs want big-endian RGB565, the core renders little-endian,
 * so we byte-swap into this buffer each frame (76.8 KB). */
static uint16_t swapbuf[SCREEN_W * SCREEN_H];

/* ---- buttons ---------------------------------------------------------------*/
static const struct { int pin; uint16_t btn; } button_map[] = {
    { PIN_BTN_UP,    BTN_UP    }, { PIN_BTN_DOWN,  BTN_DOWN  },
    { PIN_BTN_LEFT,  BTN_LEFT  }, { PIN_BTN_RIGHT, BTN_RIGHT },
    { PIN_BTN_A,     BTN_A     }, { PIN_BTN_B,     BTN_B     },
    { PIN_BTN_START, BTN_START },
};

static void buttons_init(void)
{
    for (unsigned i = 0; i < sizeof button_map / sizeof button_map[0]; i++) {
        gpio_config_t cfg = {
            .pin_bit_mask = 1ULL << button_map[i].pin,
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = GPIO_PULLUP_ENABLE,
        };
        gpio_config(&cfg);
    }
}

static uint16_t read_buttons(void)
{
    uint16_t b = 0;
    for (unsigned i = 0; i < sizeof button_map / sizeof button_map[0]; i++)
        if (gpio_get_level(button_map[i].pin) == 0)   /* active low */
            b |= button_map[i].btn;
    return b;
}

/* ---- display ----------------------------------------------------------------*/
static esp_lcd_panel_handle_t lcd_init(void)
{
    spi_bus_config_t bus = {
        .sclk_io_num = PIN_LCD_SCLK,
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SCREEN_W * SCREEN_H * 2,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num       = PIN_LCD_DC,
        .cs_gpio_num       = PIN_LCD_CS,
        .pclk_hz           = LCD_SPI_HZ,
        .lcd_cmd_bits      = 8,
        .lcd_param_bits    = 8,
        .spi_mode          = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_cfg, &io));

    esp_lcd_panel_handle_t panel;
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_BGR, /* flip if colors wrong */
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io, &panel_cfg, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    /* landscape orientation; toggle these if your image is mirrored */
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
    return panel;
}

static void present(esp_lcd_panel_handle_t panel)
{
    const uint16_t *fb = game_framebuffer();
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++)
        swapbuf[i] = (uint16_t)((fb[i] >> 8) | (fb[i] << 8));
    esp_lcd_panel_draw_bitmap(panel, OFF_X, OFF_Y,
                              OFF_X + SCREEN_W, OFF_Y + SCREEN_H, swapbuf);
}

/* ---- main -------------------------------------------------------------------*/
void app_main(void)
{
    buttons_init();
    esp_lcd_panel_handle_t panel = lcd_init();

    int errors = game_init();
    if (errors)
        ESP_LOGW(TAG, "%d malformed asset(s) -- look for magenta pixels",
                 errors);

    /* black out the whole panel once (the game only redraws its window) */
    memset(swapbuf, 0, sizeof swapbuf);
    for (int y = 0; y < LCD_H; y += SCREEN_H)
        for (int x = 0; x < LCD_W; x += SCREEN_W)
            esp_lcd_panel_draw_bitmap(panel, x, y,
                x + SCREEN_W > LCD_W ? LCD_W : x + SCREEN_W,
                y + SCREEN_H > LCD_H ? LCD_H : y + SCREEN_H, swapbuf);

    ESP_LOGI(TAG, "running");
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        game_update(read_buttons());
        present(panel);
        /* 60 Hz pacing (needs CONFIG_FREERTOS_HZ=1000, see
         * sdkconfig.defaults, so one tick is 1 ms) */
        vTaskDelayUntil(&last, pdMS_TO_TICKS(1000 / TICKS_PER_SEC));
    }

    /* TODO(audio): create an I2S TX channel (driver/i2s_std.h), then in a
     * second task loop:
     *     int16_t buf[512];
     *     game_audio_fill(buf, 512);
     *     i2s_channel_write(tx, buf, sizeof buf, &written, portMAX_DELAY);
     * The core side is already done -- game_audio_fill() is thread-safe
     * enough for this (worst case is a one-buffer click).
     */
}
