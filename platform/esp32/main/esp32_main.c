/* ============================================================================
 * esp32_main.c -- the ESP32 platform layer.
 *
 * STATUS: skeleton, written against ESP-IDF v5.x. It compiles the same
 * portable core as the desktop build; pick your BOARD below, check the
 * pins, and iterate. (Developed and tested on desktop first -- that's the
 * workflow this repo is built for.)
 *
 * TWO WIRINGS ARE SUPPORTED -- pick one with the BOARD_* define below:
 *
 * BOARD_C3_SUPER_MINI = 1   ESP32-C3 Super Mini handheld
 *
 *   1.69" ST7789 240x280 SPI LCD      MAX98357A I2S amp
 *     SCL  -> GPIO 4                    BCLK -> GPIO 21
 *     SDA  -> GPIO 6  (= MOSI)          LRC  -> GPIO 0
 *     RES  -> GPIO 7                    DIN  -> GPIO 20
 *     DC   -> GPIO 2                    SD   -> GPIO 5  (high = amp on)
 *     CS   -> GPIO 10                   GAIN -> NC
 *     BLK  -> 3V3 (always on)
 *
 *   PCF8574 I2C button expander       extras
 *     SDA -> GPIO 8, SCL -> GPIO 9      vibration IN -> GPIO 3
 *     P0 UP    P1 DOWN  P2 LEFT         IR blaster   -> GPIO 1
 *     P3 RIGHT P4 A     P5 B            IR receiver  -> PCF8574 P7
 *     P6 free (wired = START)
 *
 *   No START button is wired, so this layer reports START when A and B
 *   are held TOGETHER (needed on the title screen and game over).
 *
 *   NOTE: GPIO 20/21 are normally UART0 -- logs go over the USB port
 *   instead (see sdkconfig.defaults.esp32c3). GPIO 2/8/9 are strapping
 *   pins; the LCD DC line and the I2C pull-ups leave them high at boot,
 *   which is what the chip wants -- just don't add pull-downs there.
 *
 * BOARD_C3_SUPER_MINI = 0   classic ESP32 devkit (the original skeleton)
 *
 *   ILI9341 320x240 SPI LCD           buttons (one GPIO each, to GND,
 *     SCLK -> GPIO 18                  internal pull-ups enabled)
 *     MOSI -> GPIO 23                   UP    -> GPIO 32
 *     DC   -> GPIO 2                    DOWN  -> GPIO 33
 *     CS   -> GPIO 15                   LEFT  -> GPIO 25
 *     RST  -> GPIO 4                    RIGHT -> GPIO 26
 *     BL   -> 3V3 (or a GPIO)           A     -> GPIO 27
 *                                       B     -> GPIO 14
 *                                       START -> GPIO 12
 *
 * Either way the 240x160 framebuffer is drawn centered on the panel with
 * a black frame around it.
 * ==========================================================================*/

/* ---- BOARD SELECT: EDIT ME ------------------------------------------------*/
#define BOARD_C3_SUPER_MINI 1   /* 1 = C3 Super Mini handheld, 0 = devkit */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

#if BOARD_C3_SUPER_MINI
#include "esp_lcd_panel_vendor.h"   /* ST7789 driver ships inside ESP-IDF   */
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#else
#include "esp_lcd_ili9341.h"        /* managed component, see idf_component.yml */
#endif

#include "game.h"

static const char *TAG = "iknowwhatisaw";

/* ---- pins & panel geometry -------------------------------------------------*/
#if BOARD_C3_SUPER_MINI

#define PIN_LCD_SCLK   4
#define PIN_LCD_MOSI   6
#define PIN_LCD_DC     2
#define PIN_LCD_CS    10
#define PIN_LCD_RST    7

#define PIN_I2C_SDA    8            /* PCF8574 button expander */
#define PIN_I2C_SCL    9
#define PCF8574_ADDR   0x20         /* A0..A2 to GND; a PCF8574*A* is 0x38 */

#define PIN_I2S_BCLK  21            /* MAX98357A */
#define PIN_I2S_LRC    0
#define PIN_I2S_DIN   20
#define PIN_AMP_SD     5            /* drive high: amp on, plays LEFT slot */

#define PIN_VIBRATOR   3            /* not used by the core yet; held off  */
#define PIN_IR_TX      1            /* not used by the core yet; held off  */
/* IR receiver OUT is PCF8574 bit P7 -- read it from pcf_state() if you
 * ever want it; the game core doesn't know about IR. */

/* 1.69" ST7789 panels are 240x280, portrait. The ST7789 controller RAM
 * is 240x320, and these panels sit 20 rows in -- hence the Y gap. */
#define LCD_W 240
#define LCD_H 280
#define LCD_GAP_X 0
#define LCD_GAP_Y 20
#define LCD_RGB_ORDER LCD_RGB_ELEMENT_ORDER_RGB  /* flip if colors wrong  */
#define LCD_SPI_HZ (40 * 1000 * 1000)

#else /* classic devkit ------------------------------------------------------*/

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
#define LCD_GAP_X 0
#define LCD_GAP_Y 0
#define LCD_RGB_ORDER LCD_RGB_ELEMENT_ORDER_BGR  /* flip if colors wrong  */
#define LCD_SPI_HZ (40 * 1000 * 1000)   /* most ILI9341s take 40 MHz fine */

#endif

/* where the 240x160 game screen sits on the panel */
#define OFF_X ((LCD_W - SCREEN_W) / 2)
#define OFF_Y ((LCD_H - SCREEN_H) / 2)

/* SPI LCDs want big-endian RGB565, the core renders little-endian,
 * so we byte-swap into this buffer each frame (76.8 KB). */
static uint16_t swapbuf[SCREEN_W * SCREEN_H];

/* ---- buttons ---------------------------------------------------------------*/
#if BOARD_C3_SUPER_MINI

/* All six buttons live on a PCF8574 behind I2C: write 0xFF once so every
 * pin is a (quasi-)input, then one 1-byte read per frame. Pressed = low. */
static i2c_master_dev_handle_t s_pcf;
static uint8_t s_pcf_last = 0xFF;   /* last good read (0xFF = nothing)    */

static void buttons_init(void)
{
    i2c_master_bus_config_t bus = {
        .i2c_port          = -1,   /* auto-pick */
        .sda_io_num        = PIN_I2C_SDA,
        .scl_io_num        = PIN_I2C_SCL,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t h;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus, &h));

    i2c_device_config_t dev = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = PCF8574_ADDR,
        .scl_speed_hz    = 100000,      /* PCF8574 tops out at 100 kHz */
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(h, &dev, &s_pcf));

    uint8_t all_inputs = 0xFF;
    if (i2c_master_transmit(s_pcf, &all_inputs, 1, 100) != ESP_OK)
        ESP_LOGW(TAG, "PCF8574 not answering at 0x%02x -- check SDA/SCL/addr",
                 PCF8574_ADDR);
}

/* raw expander byte; bit 7 is the IR receiver if you want it */
static uint8_t pcf_state(void)
{
    uint8_t v;
    if (i2c_master_receive(s_pcf, &v, 1, 20) == ESP_OK)
        s_pcf_last = v;
    return s_pcf_last;   /* on a bad read, coast on the last good state */
}

static uint16_t read_buttons(void)
{
    uint8_t v = pcf_state();
    uint16_t b = 0;
    if (!(v & (1 << 0))) b |= BTN_UP;
    if (!(v & (1 << 1))) b |= BTN_DOWN;
    if (!(v & (1 << 2))) b |= BTN_LEFT;
    if (!(v & (1 << 3))) b |= BTN_RIGHT;
    if (!(v & (1 << 4))) b |= BTN_A;
    if (!(v & (1 << 5))) b |= BTN_B;
    if (!(v & (1 << 6))) b |= BTN_START;   /* P6: wire a 7th button here  */
    /* no dedicated START on this pad: A+B held together = START.
     * (Reported INSTEAD of A and B so the chord can't also talk/cancel.) */
    if ((b & (BTN_A | BTN_B)) == (BTN_A | BTN_B))
        b = (uint16_t)((b & ~(BTN_A | BTN_B)) | BTN_START);
    return b;
}

#else /* devkit: one GPIO per button ----------------------------------------*/

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

#endif

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
        .rgb_ele_order  = LCD_RGB_ORDER,
        .bits_per_pixel = 16,
    };
#if BOARD_C3_SUPER_MINI
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io, &panel_cfg, &panel));
#else
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io, &panel_cfg, &panel));
#endif
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel, LCD_GAP_X, LCD_GAP_Y));
#if BOARD_C3_SUPER_MINI
    /* portrait panel, and these IPS ST7789s want inversion ON --
     * if your image looks like a photo negative, flip this to false */
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, true));
#else
    /* landscape orientation; toggle these if your image is mirrored */
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, true, false));
#endif
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

/* ---- audio (MAX98357A over I2S) ---------------------------------------------*/
#if BOARD_C3_SUPER_MINI

static i2s_chan_handle_t s_i2s_tx;

static void audio_task(void *arg)
{
    static int16_t buf[256];         /* ~11.6 ms of sound per write */
    size_t written;
    (void)arg;
    for (;;) {
        game_audio_fill(buf, 256);
        i2s_channel_write(s_i2s_tx, buf, sizeof buf, &written, portMAX_DELAY);
    }
}

static void audio_init(void)
{
    /* SD > 1.4 V = amp on, playing the LEFT slot -- which is where a
     * mono I2S stream lands. Tie it low (or drive it) to mute. */
    gpio_config_t sd = {
        .pin_bit_mask = 1ULL << PIN_AMP_SD,
        .mode         = GPIO_MODE_OUTPUT,
    };
    gpio_config(&sd);
    gpio_set_level(PIN_AMP_SD, 1);

    i2s_chan_config_t chan = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO,
                                                        I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan, &s_i2s_tx, NULL));

    i2s_std_config_t std = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                        I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,     /* MAX98357A needs no MCLK */
            .bclk = PIN_I2S_BCLK,
            .ws   = PIN_I2S_LRC,
            .dout = PIN_I2S_DIN,
            .din  = I2S_GPIO_UNUSED,
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(s_i2s_tx, &std));
    ESP_ERROR_CHECK(i2s_channel_enable(s_i2s_tx));

    xTaskCreate(audio_task, "audio", 2048, NULL, 5, NULL);
}

/* the vibration motor and IR blaster aren't used by the game core (yet);
 * park them off so they don't buzz/blast at boot */
static void extras_init(void)
{
    gpio_config_t out = {
        .pin_bit_mask = (1ULL << PIN_VIBRATOR) | (1ULL << PIN_IR_TX),
        .mode         = GPIO_MODE_OUTPUT,
    };
    gpio_config(&out);
    gpio_set_level(PIN_VIBRATOR, 0);
    gpio_set_level(PIN_IR_TX, 0);
}

#endif /* BOARD_C3_SUPER_MINI */

/* ---- save games (NVS) -------------------------------------------------------
 * The handheld has no filesystem, but it has NVS -- a wear-levelled
 * key/value store in flash. That is our save slot. The core doesn't know
 * or care; it just hands us GAME_SAVE_SIZE bytes (see game.h).
 * ---------------------------------------------------------------------------*/
#define SAVE_NS  "ikwis"
#define SAVE_KEY "save"

static void save_init(void)
{
    esp_err_t e = nvs_flash_init();
    if (e == ESP_ERR_NVS_NO_FREE_PAGES || e == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        e = nvs_flash_init();
    }
    ESP_ERROR_CHECK(e);
}

static int save_read(uint8_t *buf, int len)
{
    nvs_handle_t h;
    if (nvs_open(SAVE_NS, NVS_READONLY, &h) != ESP_OK)
        return 0;
    size_t n = (size_t)len;
    esp_err_t e = nvs_get_blob(h, SAVE_KEY, buf, &n);
    nvs_close(h);
    return e == ESP_OK && n == (size_t)len;
}

static int save_write(const uint8_t *buf, int len)
{
    nvs_handle_t h;
    if (nvs_open(SAVE_NS, NVS_READWRITE, &h) != ESP_OK)
        return 0;
    esp_err_t e = nvs_set_blob(h, SAVE_KEY, buf, (size_t)len);
    if (e == ESP_OK)
        e = nvs_commit(h);          /* not saved until this returns */
    nvs_close(h);
    return e == ESP_OK;
}

/* ---- main -------------------------------------------------------------------*/
void app_main(void)
{
    buttons_init();
    esp_lcd_panel_handle_t panel = lcd_init();
#if BOARD_C3_SUPER_MINI
    extras_init();
    audio_init();
#endif

    save_init();

    int errors = game_init();
    if (errors)
        ESP_LOGW(TAG, "%d malformed asset(s) -- look for magenta pixels",
                 errors);

    {   /* a save in flash? then the title offers CONTINUE */
        uint8_t blob[GAME_SAVE_SIZE];
        if (save_read(blob, GAME_SAVE_SIZE) &&
            game_save_load(blob, GAME_SAVE_SIZE))
            ESP_LOGI(TAG, "save loaded from NVS");
    }

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

        if (game_save_pending()) {      /* player picked SAVE in the pack */
            uint8_t blob[GAME_SAVE_SIZE];
            game_save_write(blob);
            int ok = save_write(blob, GAME_SAVE_SIZE);
            game_save_done(ok);
            ESP_LOGI(TAG, "save %s", ok ? "written to NVS" : "FAILED");
        }
        if (game_load_pending()) {      /* player picked LOAD in the pack */
            uint8_t blob[GAME_SAVE_SIZE];
            int ok = save_read(blob, GAME_SAVE_SIZE) &&
                     game_save_load(blob, GAME_SAVE_SIZE);
            game_load_done(ok);
            ESP_LOGI(TAG, "load %s", ok ? "from NVS" : "FAILED");
        }

        present(panel);
        /* 60 Hz pacing (needs CONFIG_FREERTOS_HZ=1000, see
         * sdkconfig.defaults, so one tick is 1 ms) */
        vTaskDelayUntil(&last, pdMS_TO_TICKS(1000 / TICKS_PER_SEC));
    }

#if !BOARD_C3_SUPER_MINI
    /* TODO(audio): the devkit build has no speaker wired. Copy audio_init()
     * from the C3 Super Mini block above -- any I2S DAC/amp works the
     * same way. The core side is already done. */
#endif
}
