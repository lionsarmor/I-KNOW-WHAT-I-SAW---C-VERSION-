/* ============================================================================
 * main_term.c -- the TERMINAL platform layer. The game, in ASCII.
 *
 * Same portable core, zero graphics libraries: every frame the 240x160
 * framebuffer is downsampled into 120x40 terminal cells (one character
 * covers a 2x4 pixel block -- terminal glyphs are about twice as tall as
 * they are wide, so the picture keeps its shape). Each cell becomes an
 * ASCII character picked by brightness:
 *
 *        dark  " .,:;i+*x%#@"  bright
 *
 * colored with ANSI truecolor escapes. `--mono` (or a NO_COLOR env var)
 * drops the colors for 100% pure printable ASCII.
 *
 * Run it in a terminal at least 120x40 (maximize; zoom out with Ctrl+minus
 * if needed). No audio in this port -- the visitors are silent here.
 *
 * Controls: arrows/WASD move - Z or space = A - X = B - Enter = START -
 *           Q or a lone Esc quits.
 *
 * A terminal cannot report keys being HELD, only presses + autorepeat,
 * so each keypress counts as "held" for KEY_HOLD_TICKS and autorepeat
 * keeps it alive. If walking feels sticky or stuttery, tune that number.
 *
 * Flags:  --mono       no colors, pure ASCII
 *         --frames N   exit after N frames (smoke tests / CI)
 * ==========================================================================*/
#define _POSIX_C_SOURCE 200809L   /* clock_nanosleep & friends */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "game.h"

/* one terminal cell = this many framebuffer pixels */
#define CELL_W 2
#define CELL_H 4
#define COLS (SCREEN_W / CELL_W)   /* 120 */
#define ROWS (SCREEN_H / CELL_H)   /*  40 */

/* how long one keypress counts as "held", in 60 Hz ticks. Big enough to
 * bridge the keyboard's autorepeat delay, small enough to stop soon after
 * you let go. */
#define KEY_HOLD_TICKS 20

static const char RAMP[] = " .,:;i+*x%#@";
#define RAMP_LEN ((int)sizeof(RAMP) - 2)   /* last index into RAMP */

static struct termios s_term_orig;
static int s_term_raw = 0;
static int s_color = 1;

static void term_restore(void)
{
    /* colors off, cursor back on, leave the picture on screen */
    (void)!write(STDOUT_FILENO, "\x1b[0m\x1b[?25h\n", 11);
    if (s_term_raw)
        tcsetattr(STDIN_FILENO, TCSANOW, &s_term_orig);
}

static void on_signal(int sig)
{
    (void)sig;
    exit(0);   /* runs term_restore via atexit */
}

static void term_setup(void)
{
    if (isatty(STDIN_FILENO)) {
        struct termios raw;
        tcgetattr(STDIN_FILENO, &s_term_orig);
        raw = s_term_orig;
        raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
        raw.c_cc[VMIN]  = 0;   /* read() returns immediately ... */
        raw.c_cc[VTIME] = 0;   /* ... with whatever is buffered  */
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        s_term_raw = 1;
    } else {
        /* stdin is a pipe/file (tests, demos): never block on it */
        fcntl(STDIN_FILENO, F_SETFL,
              fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
    }
    atexit(term_restore);
    signal(SIGINT,  on_signal);
    signal(SIGTERM, on_signal);

    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 &&
        (ws.ws_col < COLS || ws.ws_row < ROWS))
        fprintf(stderr, "warning: terminal is %dx%d, the game wants %dx%d "
                "-- maximize or Ctrl+minus\n",
                ws.ws_col, ws.ws_row, COLS, ROWS);

    /* clear screen, hide cursor */
    (void)!write(STDOUT_FILENO, "\x1b[2J\x1b[?25l", 10);
}

/* ---- input -----------------------------------------------------------------
 * Keypresses arm a per-button countdown; a button reads "held" while its
 * countdown is alive and autorepeat keeps re-arming it. */
static int s_hold[7];   /* indexed by BTN_* bit number */

static void press(uint16_t btn)
{
    for (int b = 0; b < 7; b++)
        if (btn & (1u << b))
            s_hold[b] = KEY_HOLD_TICKS;
}

static uint16_t read_buttons(void)
{
    unsigned char buf[64];
    ssize_t n = read(STDIN_FILENO, buf, sizeof buf);
    for (ssize_t i = 0; i < n; i++) {
        unsigned char c = buf[i];
        if (c == 0x1b) {                       /* escape sequence or Esc  */
            if (i + 2 < n && buf[i + 1] == '[') {
                switch (buf[i + 2]) {
                case 'A': press(BTN_UP);    break;
                case 'B': press(BTN_DOWN);  break;
                case 'C': press(BTN_RIGHT); break;
                case 'D': press(BTN_LEFT);  break;
                }
                i += 2;
            } else if (i + 1 >= n) {
                exit(0);                       /* a lone Esc = quit       */
            }
            continue;
        }
        switch (c) {
        case 'w': case 'W':           press(BTN_UP);    break;
        case 's': case 'S':           press(BTN_DOWN);  break;
        case 'a': case 'A':           press(BTN_LEFT);  break;
        case 'd': case 'D':           press(BTN_RIGHT); break;
        case 'z': case 'Z': case ' ': press(BTN_A);     break;
        case 'x': case 'X':           press(BTN_B);     break;
        case '\r': case '\n':         press(BTN_START); break;
        case 'q': case 'Q': case 3:   exit(0);          /* q / Ctrl-C */
        }
    }

    uint16_t held = 0;
    for (int b = 0; b < 7; b++)
        if (s_hold[b] > 0) { s_hold[b]--; held |= (uint16_t)(1u << b); }
    return held;
}

/* ---- render ---------------------------------------------------------------*/
/* worst case ~20 bytes per cell (color escape + char) + row overhead */
static char s_out[COLS * ROWS * 20 + ROWS * 8 + 64];

static void present(void)
{
    const uint16_t *fb = game_framebuffer();
    int o = 0;
    int lr = -1, lg = -1, lb = -1;   /* last emitted color */

    memcpy(s_out + o, "\x1b[H", 3); o += 3;   /* cursor home */

    for (int cy = 0; cy < ROWS; cy++) {
        for (int cx = 0; cx < COLS; cx++) {
            /* average the cell's 2x4 pixel block (RGB565 -> 8-bit) */
            int r = 0, g = 0, b = 0;
            for (int py = 0; py < CELL_H; py++) {
                const uint16_t *row =
                    fb + (cy * CELL_H + py) * SCREEN_W + cx * CELL_W;
                for (int px = 0; px < CELL_W; px++) {
                    uint16_t p = row[px];
                    r += (p >> 11) << 3;
                    g += ((p >> 5) & 0x3f) << 2;
                    b += (p & 0x1f) << 3;
                }
            }
            r /= CELL_W * CELL_H; g /= CELL_W * CELL_H; b /= CELL_W * CELL_H;

            int lum = (2 * r + 5 * g + b) / 8;             /* 0..255 */
            char ch = RAMP[(lum * RAMP_LEN + 127) / 255];

            if (s_color && ch != ' ') {
                /* quantize so flat areas reuse one escape code */
                int qr = r & ~15, qg = g & ~15, qb = b & ~15;
                if (qr != lr || qg != lg || qb != lb) {
                    o += snprintf(s_out + o, 24, "\x1b[38;2;%d;%d;%dm",
                                  qr, qg, qb);
                    lr = qr; lg = qg; lb = qb;
                }
            }
            s_out[o++] = ch;
        }
        memcpy(s_out + o, "\x1b[0m\r\n", 6); o += 6;
        lr = lg = lb = -1;
    }
    (void)!write(STDOUT_FILENO, s_out, (size_t)o);
}

/* ---- main ------------------------------------------------------------------*/
int main(int argc, char **argv)
{
    long max_frames = -1;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--mono"))
            s_color = 0;
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc)
            max_frames = atol(argv[++i]);
        else {
            fprintf(stderr, "usage: %s [--mono] [--frames N]\n", argv[0]);
            return 2;
        }
    }
    if (getenv("NO_COLOR"))
        s_color = 0;

    int errors = game_init();
    if (errors)
        fprintf(stderr, "%d malformed asset(s) -- look for bright cells\n",
                errors);

    term_setup();

    /* fixed 60 Hz, same pacing scheme as every other platform */
    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);
    for (long frame = 0; max_frames < 0 || frame < max_frames; frame++) {
        game_update(read_buttons());
        present();

        next.tv_nsec += 1000000000L / TICKS_PER_SEC;
        if (next.tv_nsec >= 1000000000L) {
            next.tv_nsec -= 1000000000L;
            next.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }
    return 0;
}
