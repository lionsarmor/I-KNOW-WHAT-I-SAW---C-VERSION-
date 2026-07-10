/* ============================================================================
 * main_sdl.c -- the DESKTOP platform layer (SDL2). ~200 lines, and this
 * is the whole job of a platform: window, 60 Hz loop, keyboard -> BTN_*
 * bits, blit the framebuffer, feed the audio callback.
 *
 * Build & run:   make run       (needs libsdl2-dev / SDL2.framework)
 *
 * KEYBOARD MAPPING lives in read_buttons() below -- edit freely.
 *   arrows / WASD  move        Z / space  A (confirm)
 *   X / left-shift B (back)    Enter      START        Esc quits
 *
 * Debug flags (handy for testing and CI):
 *   --frames N          quit automatically after N frames
 *   --shot N file.bmp   save a screenshot of frame N
 * ==========================================================================*/
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

#define WINDOW_SCALE 4   /* 240x160 -> 960x640 window */

static uint16_t read_buttons(const Uint8 *k)
{
    uint16_t b = 0;
    if (k[SDL_SCANCODE_UP]    || k[SDL_SCANCODE_W]) b |= BTN_UP;
    if (k[SDL_SCANCODE_DOWN]  || k[SDL_SCANCODE_S]) b |= BTN_DOWN;
    if (k[SDL_SCANCODE_LEFT]  || k[SDL_SCANCODE_A]) b |= BTN_LEFT;
    if (k[SDL_SCANCODE_RIGHT] || k[SDL_SCANCODE_D]) b |= BTN_RIGHT;
    if (k[SDL_SCANCODE_Z]     || k[SDL_SCANCODE_SPACE])  b |= BTN_A;
    if (k[SDL_SCANCODE_X]     || k[SDL_SCANCODE_LSHIFT]) b |= BTN_B;
    if (k[SDL_SCANCODE_RETURN]) b |= BTN_START;
    return b;
}

/* SDL pulls audio from us on its own thread; we just forward to the core */
static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    (void)userdata;
    game_audio_fill((int16_t *)stream, len / 2);
}

int main(int argc, char **argv)
{
    long max_frames = -1, shot_frame = -1;
    const char *shot_path = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--frames") && i + 1 < argc)
            max_frames = atol(argv[++i]);
        else if (!strcmp(argv[i], "--shot") && i + 2 < argc) {
            shot_frame = atol(argv[++i]);
            shot_path  = argv[++i];
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("I KNOW WHAT I SAW",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W * WINDOW_SCALE, SCREEN_H * WINDOW_SCALE, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    /* nearest-neighbour scaling = crisp fat pixels */
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);

    SDL_AudioSpec want = {0}, have;
    want.freq     = AUDIO_RATE;
    want.format   = AUDIO_S16SYS;
    want.channels = 1;
    want.samples  = 512;
    want.callback = audio_callback;
    SDL_AudioDeviceID adev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);

    int errors = game_init();
    if (errors)
        fprintf(stderr, "WARNING: %d malformed asset(s) -- look for "
                        "magenta pixels / check row lengths\n", errors);

    if (adev) SDL_PauseAudioDevice(adev, 0);

    /* ---- the loop: 60 updates per second, forever --------------------- */
    long frame = 0;
    int running = 1;
    Uint64 next_tick = SDL_GetPerformanceCounter();
    const Uint64 tick_len = SDL_GetPerformanceFrequency() / TICKS_PER_SEC;

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT ||
                (ev.type == SDL_KEYDOWN &&
                 ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE))
                running = 0;
        }

        game_update(read_buttons(SDL_GetKeyboardState(NULL)));

        SDL_UpdateTexture(tex, NULL, game_framebuffer(),
                          SCREEN_W * (int)sizeof(uint16_t));
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);

        if (frame == shot_frame && shot_path) {
            SDL_Surface *s = SDL_CreateRGBSurfaceWithFormatFrom(
                (void *)game_framebuffer(), SCREEN_W, SCREEN_H, 16,
                SCREEN_W * 2, SDL_PIXELFORMAT_RGB565);
            SDL_SaveBMP(s, shot_path);
            SDL_FreeSurface(s);
            printf("saved %s\n", shot_path);
        }
        frame++;
        if (max_frames >= 0 && frame >= max_frames)
            running = 0;

        /* fixed 60 Hz pacing (vsync usually handles it; this is backup) */
        next_tick += tick_len;
        Uint64 now = SDL_GetPerformanceCounter();
        if (now < next_tick)
            SDL_Delay((Uint32)((next_tick - now) * 1000
                               / SDL_GetPerformanceFrequency()));
        else
            next_tick = now;   /* running behind: don't spiral */
    }

    if (adev) SDL_CloseAudioDevice(adev);
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
