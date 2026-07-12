/* ============================================================================
 * game_internal.h -- shared state + entry points between the game's
 * scenes. Platforms never include this; it is core-private.
 *
 * The whole game is ONE state machine (G.state). Each scene is a pair
 * of functions, xxx_update() and xxx_render(), that game.c dispatches
 * to every tick. To add a scene (inventory? shop? pause menu?):
 *
 *   1. add a value to state_t below
 *   2. write yourscene_update()/yourscene_render() in a new .c file
 *   3. add the dispatch case in game.c
 *   4. jump to it from anywhere:  G.state = ST_YOURSCENE; G.t = 0;
 * ==========================================================================*/
#ifndef GAME_INTERNAL_H
#define GAME_INTERNAL_H

#include "game.h"
#include "gfx.h"
#include "audio.h"
#include "assets.h"

typedef enum {
    ST_INTRO,       /* the creepy cinematic          (intro.c)     */
    ST_TITLE,       /* logo + PRESS START            (intro.c)     */
    ST_NAME,        /* pick your name (new game)     (intro.c)     */
    ST_OPTIONS,     /* the OPTIONS menu              (intro.c)     */
    ST_PAUSE,       /* ESC: save & quit / quit / back (intro.c)     */
    ST_CONTROLS,    /* OPTIONS -> CONTROLS            (intro.c)     */
    ST_OVERWORLD,   /* walking around                (overworld.c) */
    ST_DIALOG,      /* textbox over the overworld    (overworld.c) */
    ST_PACK,        /* the item menu (START)         (overworld.c) */
    ST_SLEEP,       /* you got into bed              (overworld.c) */
    ST_DRIVE,       /* the highway. the cutscene.    (cutscene.c)  */
    ST_PROLOGUE,    /* END OF PROLOGUE + menu        (cutscene.c)  */
    ST_BATTLE,      /* turn-based fight              (battle.c)    */
    ST_GAMEOVER,    /* you blacked out...            (battle.c)    */
} state_t;

enum { DIR_DOWN, DIR_UP, DIR_LEFT, DIR_RIGHT };


/* ---- the player ----------------------------------------------------------*/
typedef struct {
    int x, y;            /* position in map PIXELS (sprite top-left) */
    int dir;             /* DIR_* facing                             */
    int walking;         /* moved this tick? drives leg animation    */
    int anim;            /* animation clock                          */
    int hp, max_hp;
    int level, xp;
    int items[NUM_ITEMS];/* the pockets: count per ITEM_* (SHELLS =
                            ammo count, SHOTGUN nonzero = owned)     */
    int lamp;            /* flashlight switched on?                  */
    char name[PLAYER_NAME_MAX + 1];  /* what people call you. "PLAYER" if unset */
} player_t;

/* ---- things living on the current map ------------------------------------*/
/* Enough for every scripted spawn on the busiest map PLUS room for the
 * things an ANT HILL produces at run time -- those take the slots above
 * the map's spawn count. */
#define MAX_ENTITIES 24

typedef struct {
    int type;            /* ENT_NONE / ENT_ALIEN / ENT_NPC           */
    int kind;            /* ENT_ALIEN: SPECIES_*  ENT_NPC: LOOK_*    */
    int x, y;            /* map pixels                               */
    int dir;
    int timer;           /* alien AI: ticks until it re-decides      */
    const char *dialog;  /* NPC speech                               */
    int gift;            /* NPC: ITEM_*+1 to hand over; 0 = nothing  */
    const char *after;   /* NPC: line to say once the gift is given  */

    /* ZELDA MODE (aliens only) */
    int hp;              /* shotgun blasts left in it                */
    int stun;            /* ticks staggered -- can't move or grab you */
    int hurt;            /* ticks of white flash after a hit          */
    int aggro;           /* it has seen you and is coming             */
} entity_t;

/* a shotgun blast in flight */
typedef struct {
    int active;
    int x, y;            /* map pixels           */
    int dx, dy;          /* pixels per tick      */
    int travelled;
} shot_t;

/* what's left of something that came apart */
typedef struct {
    int active;
    int x, y;
    int vx, vy;          /* 1/16th pixels per tick */
    int life;
    uint16_t col;
} gore_t;

/* ---- everything, in one struct you can inspect in a debugger -------------*/
typedef struct {
    state_t  state;
    uint32_t frame;      /* ticks since boot                     */
    uint32_t t;          /* ticks since entering current state   */
    uint16_t held;       /* buttons currently down               */
    uint16_t pressed;    /* buttons that went down THIS tick     */

    player_t player;
    int      map_id;
    entity_t ents[MAX_ENTITIES];
    int      battle_grace;   /* ticks of no-battle after fleeing */

    /* ZELDA MODE */
    shot_t   shots[MAX_SHOTS];
    gore_t   gore[MAX_GORE];
    int      shoot_cd;       /* ticks until you can fire again   */
    int      recoil;         /* ticks of visible kick            */

    /* PA'S TNT, mid-air and then mid-blast. boom_t counts DOWN from
     * TNT_BOOM_TICKS; the fireball is drawn while it's nonzero. */
    int      boom_t, boom_x, boom_y;

    /* Ticks left on the "THE FARM" banner. Set ONLY by overworld_enter_map,
     * so it fires when you actually walk into a place -- not every time a
     * dialog or a menu closes and hands control back to the overworld. */
    int      banner;

    uint32_t daytime;        /* overworld clock driving day/night;
                                see DAY_LEN_TICKS in config.h      */
    /* Bit i = spawn slot i on that map is GONE FOR GOOD this run: an item
     * you pocketed, a gift an NPC already handed over, a boss you killed.
     * Ordinary aliens are NOT marked -- they come back, so you can grind. */
    uint32_t spawns_gone[NUM_MAPS];   /* one bit per spawn slot (24 of them) */

    /* THE ONE GENEROUS SOUL. On each map, one random person is holding
     * something for you (see BOON_HEAL in config.h). boon_ent is their
     * entity index on the map you're standing on, or -1 if nobody here has
     * anything -- rerolled every time you walk in. boon_item is what
     * they'd hand over.
     *
     * boons_done has a bit per MAP: once you've taken that map's kindness
     * it stays taken, so walking back out and in again gets you nothing.
     * The whole mask clears when the world restocks. */
    int      boon_ent;
    int      boon_item;
    uint16_t boons_done;

    /* dialog scene.
     * dialog_text points into dialog_buf: NPC lines are written with a '~'
     * wherever your name goes, and dialog_start() expands it once, here,
     * so the typewriter and the word-wrap never have to know about it. */
    char dialog_buf[DIALOG_BUF_MAX];
    const char *dialog_text;
    int dialog_page;     /* index into dialog_text where THIS page starts */
    int dialog_shown;    /* typewriter: chars of this page revealed so far */

    /* the name-entry screen */
    int name_sel;        /* which cell of the letter grid    */
    int name_len;

    /* the sleep sequence (bed in Ma's house) */
    int sleep_t;

    /* the PACK (overworld item menu) */
    int pack_sel;        /* highlighted row (index into the VISIBLE list) */
    int pack_top;        /* first visible row -- the scroll window       */

    /* Which items the player has ever held. The pack lists ONLY these, so
     * it starts empty and fills in as you find things: it never spoils an
     * item you haven't met. An item you found and used up still shows
     * (at x0) -- you already know it exists. */
    uint16_t items_seen;

    /* save games -- see the protocol at the bottom of game.h */
    int save_pending;    /* player picked SAVE; platform must write     */
    int load_pending;    /* player picked LOAD; platform must read      */
    int has_save;        /* a save exists -> title offers CONTINUE, and
                            LOAD is selectable in the pack               */
    int title_sel;       /* title menu row (see title_rows in intro.c)  */

    /* OPTIONS. `fullscreen` is what the PLAYER asked for; the platform
     * reads it via game_want_fullscreen() and makes the window match.
     * The core itself has no idea what a window is. */
    int opt_sel;
    int fullscreen;

    /* GAMEPAD. All of it is the platform's business except the menu.
     * pad_name is whatever SDL says is plugged in ("" = nothing). */
    char pad_name[24];
    int  ctl_sel;
    int  controls_menu;   /* does this platform even have pads?   */
    int  display_menu;    /* ...and a window it can resize?        */
    int  swap_ab;         /* the player prefers the other button order */
    int  rumble_on;
    int  rumble_req;      /* 0..255, set by the game, taken by the platform */

    /* VOLUMES, 0..VOL_STEPS. Kept as small integers because that's what the
     * menu shows; audio.c gets them scaled to 0..255. */
    int  music_vol;
    int  sfx_vol;
    int  opt_from;        /* OPTIONS was opened from here: title, or pause */

    /* THE PAUSE / QUIT SCREEN (ESC).
     *   pause_from  the scene we interrupted, and go back to on CANCEL.
     *   quit_after_save  SAVE AND QUIT was chosen: the save is in flight,
     *                    and we exit the moment the platform says it landed.
     *   quit_pending     the platform must now close the game. It polls this
     *                    with game_quit_pending().
     *   pause_msg   a line shown on the pause panel, e.g. a failed save. */
    int pause_sel;
    int pause_from;
    int quit_after_save;
    int quit_pending;
    const char *pause_msg;

    /* the little banner that says SAVED. / LOADED. / NO SAVE FOUND */
    const char *toast;
    int toast_ticks;
    int toast_good;

    /* When the world last restocked its items. See ITEM_RESPAWN_TICKS. */
    uint32_t last_restock;

    /* STORY FLAGS -- things that have happened, and stay happened.
     * Saved, so they survive a reload. */
    uint32_t flags;

    /* the driving cutscene */
    int drive_t;
    int end_sel;             /* END OF PROLOGUE menu: 0 save 1 load 2 cont */

    /* battle scene */
    struct {
        int ent;                 /* index into G.ents we're fighting */
        int kind;                /* its SPECIES_* (stats + sprites)  */
        int enemy_hp, enemy_max;
        int menu;                /* 0 FIGHT  1 SHOOT  2 ITEM  3 RUN  */
        int item_sel;            /* ITEM submenu: 0 herb, 1 medkit   */
        int phase;               /* see battle.c */
        int timer;
        int stunned;             /* a special move cost you your turn */
        int move_idx;            /* which special move was announced   */
        int hits_left;           /* MV_MULTI: blows still to land      */
        int hit_power;           /* MV_MULTI: damage per blow          */
        char msg[80];
    } battle;

    uint32_t rng;
} game_t;

extern game_t G;

/* helper: did this button just get pressed this tick? */
#define PRESSED(btn) ((G.pressed & (btn)) != 0)

/* deterministic xorshift RNG -- same everywhere, no libc */
uint32_t rng_next(void);
int      rng_range(int lo, int hi);   /* inclusive */

/* scene entry points */
void intro_update(void);      void intro_render(void);
void title_update(void);      void title_render(void);
void overworld_update(void);  void overworld_render(void);
void dialog_update(void);     void dialog_render(void);
void pack_update(void);       void pack_render(void);
void name_update(void);       void name_render(void);
void options_update(void);    void options_render(void);
void pause_update(void);      void pause_render(void);
void controls_update(void);   void controls_render(void);
void options_start(int from);   /* ST_TITLE or ST_PAUSE */
void rumble(int strength);    /* ask the platform to shake something */
void audio_apply_volumes(void);
void render_scene(int state);   /* draw a scene without advancing it */
void sleep_update(void);      void sleep_render(void);
void drive_update(void);      void drive_render(void);
void prologue_update(void);   void prologue_render(void);
void drive_start(void);       /* the van pulls out */
void battle_update(void);     void battle_render(void);
void gameover_update(void);   void gameover_render(void);

/* scene transitions */
void overworld_start_game(void);              /* fresh game from title  */
void overworld_resume(void);                  /* CONTINUE from a save   */
void overworld_enter_map(int map, int tx, int ty);
void dialog_start(const char *text);
void battle_start(int ent_index);
void pack_discover(int item_kind);            /* it enters the pack     */
void mark_dead(int ent_index);                /* it stays dead till dawn */
int  map_music(int map);                      /* the song a place plays  */
void name_start(void);                        /* the name screen        */

#endif /* GAME_INTERNAL_H */
