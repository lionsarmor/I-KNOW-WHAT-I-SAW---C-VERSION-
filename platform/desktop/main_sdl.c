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

/* ANDROID uses this same file. The platform's job is identical -- a 60Hz
 * loop, buttons, a framebuffer, audio -- and SDL papers over the rest. The
 * only real differences are (a) a phone has no buttons, so we draw some, and
 * (b) it has no window to resize and no ESC key. Both are handled below. */
#ifdef __ANDROID__
#include "touch.h"
#endif

/* THE WEB. A browser will not let you own the main loop -- it owns it, and it
 * calls YOU once per animation frame. So the body of the loop becomes a
 * function (frame_step) and everything the loop used to keep on its stack
 * becomes an `app` struct. On every other platform the while() below calls
 * exactly the same function, so there is one game loop, not two. */
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

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
    /* ...but ALT+ENTER is the fullscreen shortcut, not a START press. */
    if (k[SDL_SCANCODE_RETURN] && !(SDL_GetModState() & KMOD_ALT))
        b |= BTN_START;
    return b;
}

/* ---- GAMEPADS --------------------------------------------------------------
 * SDL's GameController layer is the whole reason this is short. It ships a
 * mapping database for hundreds of pads -- Xbox, Switch Pro, DualShock,
 * no-name USB -- and hands them all to us as ONE abstract layout. We never
 * ask what brand it is, and there is no per-brand code anywhere in the game.
 *
 * A note on Nintendo pads specifically: SDL reports buttons by POSITION, not
 * by the letter printed on them. So SDL_CONTROLLER_BUTTON_A is always the
 * BOTTOM face button -- on a Switch Pro pad that's the one physically
 * labelled "B". That is the right default (the bottom button is "confirm"
 * nearly everywhere), but people have strong feelings, so OPTIONS -> CONTROLS
 * -> SWAP A/B flips it.
 */
static SDL_GameController *pad = NULL;

static void pad_open_first(void)
{
    if (pad)
        return;
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (!SDL_IsGameController(i))
            continue;
        pad = SDL_GameControllerOpen(i);
        if (pad) {
            const char *n = SDL_GameControllerName(pad);
            game_set_pad(n ? n : "GAMEPAD");
            SDL_Log("pad: %s", n ? n : "?");
            return;
        }
    }
}

static void pad_close(void)
{
    if (!pad)
        return;
    SDL_GameControllerClose(pad);
    pad = NULL;
    game_set_pad(NULL);
    SDL_Log("pad: disconnected");
}

/* dead zone: sticks rest a long way from centre and we do not want to walk */
#define STICK_DEADZONE 12000

static uint16_t read_pad(void)
{
    if (!pad || !SDL_GameControllerGetAttached(pad))
        return 0;

    uint16_t b = 0;
    struct { int btn; uint16_t bit; } map[] = {
        { SDL_CONTROLLER_BUTTON_DPAD_UP,    BTN_UP    },
        { SDL_CONTROLLER_BUTTON_DPAD_DOWN,  BTN_DOWN  },
        { SDL_CONTROLLER_BUTTON_DPAD_LEFT,  BTN_LEFT  },
        { SDL_CONTROLLER_BUTTON_DPAD_RIGHT, BTN_RIGHT },
        { SDL_CONTROLLER_BUTTON_START,      BTN_START },
    };
    for (unsigned i = 0; i < sizeof map / sizeof map[0]; i++)
        if (SDL_GameControllerGetButton(pad, map[i].btn))
            b |= map[i].bit;

    /* the face buttons, in whichever order the player prefers */
    int a_btn = SDL_CONTROLLER_BUTTON_A;      /* bottom */
    int b_btn = SDL_CONTROLLER_BUTTON_B;      /* right  */
    if (game_swap_ab()) {
        a_btn = SDL_CONTROLLER_BUTTON_B;
        b_btn = SDL_CONTROLLER_BUTTON_A;
    }
    if (SDL_GameControllerGetButton(pad, a_btn)) b |= BTN_A;
    if (SDL_GameControllerGetButton(pad, b_btn)) b |= BTN_B;

    /* X shoots too -- the shotgun deserves its own button */
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_X)) b |= BTN_B;
    /* either shoulder opens the pack */
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_BACK) ||
        SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) ||
        SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
        b |= BTN_START;

    /* the left stick works as a d-pad, because half of everyone will try it */
    int lx = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX);
    int ly = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY);
    if (lx < -STICK_DEADZONE) b |= BTN_LEFT;
    if (lx >  STICK_DEADZONE) b |= BTN_RIGHT;
    if (ly < -STICK_DEADZONE) b |= BTN_UP;
    if (ly >  STICK_DEADZONE) b |= BTN_DOWN;

    return b;
}

/* the game asked for a shake (a gunshot, dynamite, a hit landing on you) */
static void service_rumble(void)
{
    int strength = game_rumble_take();
    if (!strength || !pad)
        return;
    Uint16 s = (Uint16)(strength * 257);        /* 0..255 -> 0..65535 */
    SDL_GameControllerRumble(pad, s, s, 140);
}

/* ---- SAVE GAMES ------------------------------------------------------------
 * The core hands us bytes; where they go is OUR problem (see the protocol
 * at the bottom of game.h). On the ESP32 it's NVS. Neither the game nor the
 * other platform needs to know or care.
 *
 * On the desktop it must NOT be a file next to the binary. A shipped game
 * lives somewhere the user cannot write to -- C:\Program Files\... on
 * Windows, the Steam library on Linux -- so a relative path means saving
 * silently fails for everyone who didn't build the game themselves.
 *
 * SDL_GetPrefPath() gives us the right per-user directory on every OS and
 * creates it if it isn't there:
 *
 *   Windows   C:\Users\<you>\AppData\Roaming\Weeks\I Know What I Saw\
 *   Linux     ~/.local/share/Weeks/I Know What I Saw/
 *   macOS     ~/Library/Application Support/Weeks/I Know What I Saw/
 */
#define SAVE_ORG      "Weeks"
#define SAVE_APP      "I Know What I Saw"
#define SAVE_FILE     "iknowwhatisaw.sav"
#define SETTINGS_FILE "settings.cfg"

/* Resolved once at startup. Empty string = we never found anywhere to
 * write, in which case saving fails honestly instead of pretending. */
static char save_path[1024];

/* Both the save and the settings live in that same per-user directory. */
static char settings_path[1024];

static int save_read(uint8_t *buf, int len);

/* ---- THE WEB'S FILESYSTEM IS A LIE ----------------------------------------
 * Emscripten gives you a filesystem, and it is entirely in RAM. Write a save
 * to it and it is gone the instant the tab reloads -- silently, with fopen()
 * happily returning success the whole time. So on the web we mount IDBFS (the
 * browser's IndexedDB, which actually persists) at /save, pull it in at boot,
 * and push it back after every write.
 *
 * The syncs are asynchronous and we deliberately do not wait for them: the
 * game must not stall for a browser API. A save landing a few milliseconds
 * late is fine; a frame hitch is not. */
#ifdef __EMSCRIPTEN__
static void web_storage_init(void)
{
    EM_ASM({
        FS.mkdir('/save');
        FS.mount(IDBFS, {}, '/save');
        /* true = load what's already in IndexedDB into the in-memory FS */
        FS.syncfs(true, function (err) {
            if (err) console.warn('save load failed:', err);
            ccall('web_storage_ready', null, [], []);
        });
    });
}

static void web_storage_flush(void)
{
    EM_ASM({
        FS.syncfs(false, function (err) {
            if (err) console.warn('save write failed:', err);
        });
    });
}

/* IndexedDB arrives a beat after boot, so the title screen can't offer
 * CONTINUE until it does. This is called back from the JS above. */
EMSCRIPTEN_KEEPALIVE
void web_storage_ready(void)
{
    uint8_t blob[GAME_SAVE_SIZE];
    if (save_read(blob, GAME_SAVE_SIZE) &&
        game_save_load(blob, GAME_SAVE_SIZE))
        SDL_Log("save loaded from IndexedDB");
}
#endif

static void save_path_init(void)
{
#ifdef __EMSCRIPTEN__
    /* SDL_GetPrefPath on the web hands back a RAM directory. Ignore it. */
    SDL_strlcpy(save_path,     "/save/iknowwhatisaw.sav", sizeof save_path);
    SDL_strlcpy(settings_path, "/save/settings.cfg",      sizeof settings_path);
    return;
#else
    char *pref = SDL_GetPrefPath(SAVE_ORG, SAVE_APP);
    save_path[0] = settings_path[0] = '\0';
    if (!pref) {
        SDL_Log("no writable save directory: %s", SDL_GetError());
        return;
    }
    /* SDL_GetPrefPath already ends in the platform's separator */
    SDL_snprintf(save_path,     sizeof save_path,     "%s%s", pref, SAVE_FILE);
    SDL_snprintf(settings_path, sizeof settings_path, "%s%s", pref,
                 SETTINGS_FILE);
    SDL_free(pref);
    SDL_Log("saves: %s", save_path);
#endif
}

static int save_read(uint8_t *buf, int len)
{
    if (!save_path[0])
        return 0;
    FILE *f = fopen(save_path, "rb");
    if (!f)
        return 0;
    size_t n = fread(buf, 1, (size_t)len, f);
    fclose(f);
    return n == (size_t)len;
}

static int save_write(const uint8_t *buf, int len)
{
    if (!save_path[0])
        return 0;
    FILE *f = fopen(save_path, "wb");
    if (!f)
        return 0;
    size_t n = fwrite(buf, 1, (size_t)len, f);
    fclose(f);
#ifdef __EMSCRIPTEN__
    web_storage_flush();     /* ...and actually make it survive a reload */
#endif
    return n == (size_t)len;
}

/* ---- SETTINGS --------------------------------------------------------------
 * Display preferences are NOT part of the save game -- they belong to this
 * machine, not to this playthrough, and they have to survive "NEW GAME" and
 * exist before any save does. So: their own tiny file, next to the save.
 *
 * One line, "fullscreen=0|1". If it's missing or unreadable we default to
 * windowed, which is the safe thing to hand somebody on first launch. */
/* everything the player can set, in one place */
typedef struct { int fullscreen, swap_ab, rumble, music, sfx; } settings_t;

static settings_t settings_read(void)
{
    /* windowed, xbox button order, rumble on, volumes at the default notch */
    settings_t s = { 0, 0, 1, VOL_DEFAULT, VOL_DEFAULT };
    if (!settings_path[0])
        return s;
    FILE *f = fopen(settings_path, "r");
    if (!f)
        return s;                       /* first run: the defaults above */
    char key[32];
    int  val;
    while (fscanf(f, "%31[^=]=%d\n", key, &val) == 2) {
        if      (!strcmp(key, "fullscreen")) s.fullscreen = val ? 1 : 0;
        else if (!strcmp(key, "swap_ab"))    s.swap_ab    = val ? 1 : 0;
        else if (!strcmp(key, "rumble"))     s.rumble     = val ? 1 : 0;
        else if (!strcmp(key, "music"))      s.music      = val;
        else if (!strcmp(key, "sfx"))        s.sfx        = val;
    }
    fclose(f);
    return s;
}

static void settings_write(settings_t s)
{
    if (!settings_path[0])
        return;
    FILE *f = fopen(settings_path, "w");
    if (!f)
        return;
    fprintf(f, "fullscreen=%d\n", s.fullscreen);
    fprintf(f, "swap_ab=%d\n",    s.swap_ab);
    fprintf(f, "rumble=%d\n",     s.rumble);
    fprintf(f, "music=%d\n",      s.music);
    fprintf(f, "sfx=%d\n",        s.sfx);
    fclose(f);
#ifdef __EMSCRIPTEN__
    web_storage_flush();
#endif
}

/* FULLSCREEN_DESKTOP, not FULLSCREEN: it borrows the desktop's current
 * resolution instead of yanking the monitor into a mode change. It's
 * instant, it doesn't rearrange the player's other windows, and alt-tab
 * behaves. Our 240x160 gets scaled to fit it either way (see the logical
 * size in main), so there is nothing to gain from a real mode switch. */
static void apply_fullscreen(SDL_Window *win, int on)
{
    if (SDL_SetWindowFullscreen(win, on ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)
        != 0)
        SDL_Log("fullscreen failed: %s", SDL_GetError());
    SDL_ShowCursor(on ? SDL_DISABLE : SDL_ENABLE);
    SDL_Log("display: %s", on ? "fullscreen" : "window");
}

/* SDL pulls audio from us on its own thread; we just forward to the core */
static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    (void)userdata;
    game_audio_fill((int16_t *)stream, len / 2);
}

/* Everything the loop used to hold on its stack. It has to outlive main()
 * on the web, because on the web main() RETURNS and the browser keeps
 * calling frame_step() afterwards. */
static struct {
    SDL_Window       *win;
    SDL_Renderer     *ren;
    SDL_Texture      *tex;
    SDL_AudioDeviceID adev;
    settings_t        cfg;
    int               fullscreen;
    int               running;
    long              frame, max_frames, shot_frame;
    const char       *shot_path;
    double            next_ms;     /* web only: the 60Hz gate. See frame_step. */
} app;

static void frame_step(void);

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

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO |
                 SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    save_path_init();   /* somewhere the player can actually write */
#ifdef __EMSCRIPTEN__
    web_storage_init(); /* ...which on the web means IndexedDB, not RAM */
#endif

    /* Nearest-neighbour = crisp fat pixels. This hint is read when the
     * texture and the renderer are created, so it has to be set FIRST. */
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

#ifdef __ANDROID__
    /* a phone is already "fullscreen"; ask for the whole panel and let SDL
     * pick the resolution the device actually has */
    SDL_Window *win = SDL_CreateWindow("I KNOW WHAT I SAW", 0, 0, 0, 0,
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI);
#else
    SDL_Window *win = SDL_CreateWindow("I KNOW WHAT I SAW",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W * WINDOW_SCALE, SCREEN_H * WINDOW_SCALE,
        SDL_WINDOW_RESIZABLE);
#endif
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    /* THE WHOLE OF THE SCALING PROBLEM, SOLVED ONCE.
     *
     * We draw a 240x160 picture and SDL fits it to whatever the window
     * happens to be -- windowed, fullscreen, resized, any monitor -- and
     * letterboxes the leftovers. Nothing downstream has to know.
     *
     * IntegerScale means it only ever scales by a whole number (5x, 6x...).
     * Without it, a 240x160 image stretched to fill 1080p lands on a 6.75x
     * scale, so some game pixels come out 7 screen pixels wide and their
     * neighbours 6 -- the picture shimmers and crawls as you walk. Better a
     * slightly smaller image made of perfect squares. */
    SDL_RenderSetLogicalSize(ren, SCREEN_W, SCREEN_H);
    SDL_RenderSetIntegerScale(ren, SDL_TRUE);

    /* the bars around the picture are BLACK, not whatever was there before */
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);

    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);

    /* whatever the player chose LAST time they ran the game */
    settings_t cfg = settings_read();
    int fullscreen = cfg.fullscreen;
    apply_fullscreen(win, fullscreen);

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

    /* AFTER game_init(), so a fresh core can't clobber them: tell the core
     * the truth about the machine, so OPTIONS opens showing it */
    game_set_fullscreen(fullscreen);
    game_set_swap_ab(cfg.swap_ab);
    game_set_rumble(cfg.rumble);
    game_set_music_volume(cfg.music);
    game_set_sfx_volume(cfg.sfx);
    game_enable_controls_menu(1);   /* pads work here (bluetooth, on a phone) */
#ifdef __EMSCRIPTEN__
    /* You cannot close a browser tab from inside it. Say so, and the pause
     * screen drops QUIT and offers SAVE GAME instead of SAVE AND QUIT. */
    game_enable_quit(0);
#endif
#ifdef __ANDROID__
    /* No window to resize -- a DISPLAY row that does nothing is a lie. */
    game_enable_display_menu(0);
    {
        int ww, wh;
        SDL_GetWindowSize(win, &ww, &wh);
        touch_layout(ren, ww, wh);
    }
#else
    game_enable_display_menu(1);
#endif
    pad_open_first();               /* one may already be plugged in */

    if (adev) SDL_PauseAudioDevice(adev, 0);

    /* ---- the loop: 60 updates per second, forever --------------------- */
    /* if a save is sitting there, offer it: the title will say CONTINUE */
    {
        uint8_t blob[GAME_SAVE_SIZE];
        if (save_read(blob, GAME_SAVE_SIZE) &&
            game_save_load(blob, GAME_SAVE_SIZE))
            printf("save loaded\n");
    }

    app.win = win; app.ren = ren; app.tex = tex; app.adev = adev;
    app.fullscreen = fullscreen;
    app.cfg = cfg;
    app.max_frames = max_frames;
    app.shot_frame = shot_frame;
    app.shot_path  = shot_path;
    app.frame = 0;
    app.running = 1;

#ifdef __EMSCRIPTEN__
    /* the browser drives. 0 = use requestAnimationFrame's own rate. */
    emscripten_set_main_loop(frame_step, 0, 1);
    return 0;
#else
    Uint64 next_tick = SDL_GetPerformanceCounter();
    const Uint64 tick_len = SDL_GetPerformanceFrequency() / TICKS_PER_SEC;

    while (app.running) {
        frame_step();

        /* fixed 60 Hz pacing (vsync usually handles it; this is backup) */
        next_tick += tick_len;
        Uint64 now = SDL_GetPerformanceCounter();
        if (now < next_tick)
            SDL_Delay((Uint32)((next_tick - now) * 1000
                               / SDL_GetPerformanceFrequency()));
        else
            next_tick = now;   /* running behind: don't spiral */
    }

    if (app.adev) SDL_CloseAudioDevice(app.adev);
    SDL_DestroyTexture(app.tex);
    SDL_DestroyRenderer(app.ren);
    SDL_DestroyWindow(app.win);
    SDL_Quit();
    return 0;
#endif
}

/* ONE FRAME. Called by the while() above on desktop/android, and by the
 * browser's animation callback on the web. Identical either way. */
static void frame_step(void)
{
    SDL_Window   *win  = app.win;
    SDL_Renderer *ren  = app.ren;
    SDL_Texture  *tex  = app.tex;

#ifdef __EMSCRIPTEN__
    /* THE BROWSER CALLS US AT THE MONITOR'S REFRESH RATE, NOT AT 60Hz.
     *
     * requestAnimationFrame fires once per display frame -- which on a 144Hz
     * screen is 144 times a second. This game is a FIXED-TIMESTEP machine:
     * one game_update() IS one 60th of a second, everywhere, and every
     * duration in the code (day length, stun ticks, the whole intro) is
     * counted in those. Let the browser drive it directly and the game runs
     * at 2.4x speed on exactly the monitors gamers own.
     *
     * So: the browser can call us as often as it likes; we only do work when
     * a 60th of a second has actually elapsed. */
    double now = emscripten_get_now();
    if (app.next_ms == 0.0)
        app.next_ms = now;
    if (now < app.next_ms)
        return;                                  /* too soon. do nothing. */
    app.next_ms += 1000.0 / TICKS_PER_SEC;
    if (app.next_ms < now - 250.0)
        app.next_ms = now;      /* tab was backgrounded: don't try to catch up
                                   on a thousand frames at once */
#endif
    {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            /* The window's X button is an unambiguous "close it" from the
             * OS -- we honour it. ESC is NOT: it hands over to the core,
             * which raises the pause screen and lets the player choose.
             * An hour of play should not die to a misplaced thumb. */
            if (ev.type == SDL_QUIT)
                app.running = 0;

            if (ev.type == SDL_KEYDOWN && !ev.key.repeat &&
                ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                game_request_quit();

#ifdef __ANDROID__
            /* the phone's BACK gesture is this platform's ESC */
            if (ev.type == SDL_KEYDOWN && !ev.key.repeat &&
                ev.key.keysym.scancode == SDL_SCANCODE_AC_BACK)
                game_request_quit();

            touch_event(&ev);

            if (ev.type == SDL_WINDOWEVENT &&
                (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                 ev.window.event == SDL_WINDOWEVENT_RESIZED))
                touch_layout(ren, ev.window.data1, ev.window.data2);
#endif

            /* pads come and go while the game is running */
            if (ev.type == SDL_CONTROLLERDEVICEADDED)
                pad_open_first();
            if (ev.type == SDL_CONTROLLERDEVICEREMOVED)
                pad_close();

            /* F11 / Alt+Enter -- the shortcut everyone reaches for without
             * being told. It goes through the same core flag as the OPTIONS
             * menu, so the menu never disagrees with the window. */
            if (ev.type == SDL_KEYDOWN && !ev.key.repeat &&
                (ev.key.keysym.scancode == SDL_SCANCODE_F11 ||
                 (ev.key.keysym.scancode == SDL_SCANCODE_RETURN &&
                  (ev.key.keysym.mod & KMOD_ALT))))
                game_set_fullscreen(!game_want_fullscreen());
        }

        /* keyboard OR pad OR fingers -- all of them, always, no mode to pick.
         * A touch is just another controller as far as the core knows. */
        uint16_t btn = read_buttons(SDL_GetKeyboardState(NULL)) | read_pad();
#ifdef __ANDROID__
        btn |= touch_buttons();
#endif
        game_update(btn);
        service_rumble();

        /* Did the player change DISPLAY in OPTIONS (or hit F11)? The core
         * only ever records the wish -- making it so is our job. */
        if (game_want_fullscreen() != app.fullscreen) {
            app.fullscreen = game_want_fullscreen();
            apply_fullscreen(win, app.fullscreen);
        }
        /* ...and remember anything the player changed in OPTIONS */
        if (app.fullscreen        != app.cfg.fullscreen ||
            game_swap_ab()        != app.cfg.swap_ab    ||
            game_rumble_enabled() != app.cfg.rumble     ||
            game_music_volume()   != app.cfg.music      ||
            game_sfx_volume()     != app.cfg.sfx) {
            app.cfg.fullscreen = app.fullscreen;
            app.cfg.swap_ab    = game_swap_ab();
            app.cfg.rumble     = game_rumble_enabled();
            app.cfg.music      = game_music_volume();
            app.cfg.sfx        = game_sfx_volume();
            settings_write(app.cfg);
        }

        /* did the player just pick SAVE or LOAD in the pack? */
        if (game_save_pending()) {
            uint8_t blob[GAME_SAVE_SIZE];
            game_save_write(blob);
            game_save_done(save_write(blob, GAME_SAVE_SIZE));
        }
        if (game_load_pending()) {
            uint8_t blob[GAME_SAVE_SIZE];
            int ok = save_read(blob, GAME_SAVE_SIZE) &&
                     game_save_load(blob, GAME_SAVE_SIZE);
            game_load_done(ok);
        }

        /* SAVE AND QUIT / QUIT on the pause screen. Checked AFTER the save
         * above, so a save-and-quit's bytes are on disk before we go. */
        if (game_quit_pending())
            app.running = 0;

        SDL_UpdateTexture(tex, NULL, game_framebuffer(),
                          SCREEN_W * (int)sizeof(uint16_t));
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);

#ifdef __ANDROID__
        /* The controls are drawn in REAL WINDOW PIXELS, not game pixels --
         * that's the only way to reach the letterbox bars beside the
         * picture, which is exactly where thumbs should be. Drop the logical
         * size for the overlay, and put it back for next frame's blit. */
        SDL_RenderSetLogicalSize(ren, 0, 0);
        touch_draw(ren);
        SDL_RenderSetLogicalSize(ren, SCREEN_W, SCREEN_H);
        SDL_RenderSetIntegerScale(ren, SDL_TRUE);
#endif

        SDL_RenderPresent(ren);

        if (app.frame == app.shot_frame && app.shot_path) {
            SDL_Surface *s = SDL_CreateRGBSurfaceWithFormatFrom(
                (void *)game_framebuffer(), SCREEN_W, SCREEN_H, 16,
                SCREEN_W * 2, SDL_PIXELFORMAT_RGB565);
            SDL_SaveBMP(s, app.shot_path);
            SDL_FreeSurface(s);
            printf("saved %s\n", app.shot_path);
        }
        app.frame++;
        if (app.max_frames >= 0 && app.frame >= app.max_frames)
            app.running = 0;
    }
}
