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
    ST_CHURCH,      /* PART 1 opens at mass          (cutscene.c)  */
    ST_PART1END,    /* ...and closes in an apartment (cutscene.c)  */
    ST_JOURNAL,     /* WHAT I SAW -- the bestiary    (intro.c)     */
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
    int rosary;          /* rosary EQUIPPED? (owning it isn't wearing
                            it -- see ROSARY_* in config.h). Saved.  */
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

    /* COLLISION / INTERACTION BOX, in pixels from (x,y). Almost everything
     * is one tile, but the VAN is a van -- it is drawn four tiles wide and
     * has to be solid across all of it, or you walk through the bodywork. */
    int cw, ch;

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

/* A POOL OF BLOOD -- the part that doesn't fly away. Every kill leaves
 * one where the body dropped (a boss, or YOU, leaves a big one), and it
 * stays there, chunky and drying, across map exits and re-entries, until
 * the day/night cycle completes and the world restocks -- the same clock
 * that regrows the herbs washes the streets (see restock_items).
 * Not saved: a loaded game starts with clean ground. */
typedef struct {
    int map;             /* which map it soaked into; -1 = free slot  */
    int x, y;            /* map pixels, center of the pool            */
    int big;             /* boss/player pools spread wider            */
    uint32_t seed;       /* the shape, deterministic per pool         */
} blood_t;
#define MAX_BLOOD 48

/* species_seen (below) is one bit per species. If the bestiary ever
 * outgrows the mask, this line stops compiling instead of the seventeenth
 * creature silently never being seen. */
typedef char journal_mask_fits[(NUM_SPECIES <= 16) ? 1 : -1];

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
    blood_t  blood[MAX_BLOOD];   /* the pools. see blood_t above     */
    int      blood_head;         /* ring cursor: oldest pool goes first
                                    when all the slots are wet       */
    int      shoot_cd;       /* ticks until you can fire again   */
    int      recoil;         /* ticks of visible kick            */

    /* PA'S TNT, mid-air and then mid-blast. boom_t counts DOWN from
     * TNT_BOOM_TICKS; the fireball is drawn while it's nonzero. */
    int      boom_t, boom_x, boom_y;

    /* A THROWN THING IN FLIGHT (TNT or holy water, field mode). It arcs
     * from (lob_x0,y0) to (lob_x1,y1) over LOB_TICKS, and only THEN does
     * anything explode or sizzle. Not saved: a reload lands mid-nothing. */
    int      lob_active, lob_kind, lob_t;
    int      lob_x0, lob_y0, lob_x1, lob_y1;

    /* THE SIZZLING MIST holy water leaves behind. Counts down from
     * MIST_TICKS; drawn (and glowing in the dark) while nonzero. */
    int      mist_t, mist_x, mist_y;

    /* The reunion played; when HER dialog closes, the END OF PART 1 card
     * comes up (see try_talk and dialog_update). Not saved. */
    int      part1end_after;

    /* Ticks before a WALK-ON warp may fire after arriving on a map. Without
     * this, a doorway whose arrival tile sits beside the exit mat ping-pongs
     * anyone who keeps holding the direction they entered with -- in one
     * door, straight back out, forever. Doors you BUMP are exempt: pushing
     * into a door is already deliberate. */
    int      warp_cd;

    /* SCREEN SHAKE. shake_t counts down; the displacement decays with it. */
    int      shake_t, shake_len, shake_mag;

    /* THE WEATHER. wx is a WX_*; wx_t counts down to the next roll. wx_x is
     * where the tornado is, in world pixels. */
    int      wx, wx_t, wx_x, wx_flash;

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
    int dialog_style;    /* 0 = spoken (white box). 1 = INTERNAL MONOLOGUE:
                            black plate, red border, red italic text -- the
                            lawyer talking to himself. Not saved: a reload
                            never lands mid-sentence. */

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
    int  can_quit;        /* ...and can it be closed at all? (not a tab) */
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
    int toast_good;      /* 0 = bad (red), 1 = good (green),
                            2 = GOLD -- reserved for leveling up */

    /* THE JOURNAL -- "WHAT I SAW". Nobody believes you, but you wrote it
     * down. A species is SEEN once you've fought it or dropped it in the
     * field (walking past a shape in the dark doesn't count -- you have to
     * have LOOKED at it). Kills cap at 255 and the journal shows 99+.
     * Both ride the save blob (v10). */
    uint16_t species_seen;                  /* bitmask by SPECIES_*      */
    uint8_t  species_kills[NUM_SPECIES];
    int      journal_sel;                   /* which page you're on      */
    state_t  journal_from;                  /* who opened it: title/pack */

    /* MA AT THE EAST GATE (overworld.c): the little scripted scene where
     * she runs the length of Main Street to hand you the journal. Phases:
     * 0 off, 1 she's running to you, 2 the dialog is up, 3 she's running
     * home. Not saved: you can't save mid-scene (input is locked). */
    int ma_scene;
    int ma_ent;                             /* her runtime entity slot   */

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
        int item_sel;            /* ITEM submenu: index into the list of
                                    items you actually HAVE (battle.c) */
        int item_top;            /* first visible row of that list --
                                    it scrolls when the pockets do    */
        int phase;               /* see battle.c */
        int timer;
        int stunned;             /* a special move cost you your turn */
        int move_idx;            /* which special move was announced   */
        int hits_left;           /* MV_MULTI: blows still to land      */
        int hit_power;           /* MV_MULTI: damage per blow          */
        int lob_kind;            /* PH_LOB: what's in the air (ITEM_*) */
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
void shake(int mag, int ticks); /* ...and shake the SCREEN */
void shake_offset(int *x, int *y);
void audio_apply_volumes(void);
void render_scene(int state);   /* draw a scene without advancing it */
void sleep_update(void);      void sleep_render(void);
void drive_update(void);      void drive_render(void);
void prologue_update(void);   void prologue_render(void);
void drive_start(void);       /* the van pulls out */
void battle_update(void);     void battle_render(void);
void gameover_update(void);   void gameover_render(void);
void journal_update(void);    void journal_render(void);
void journal_start(state_t from);  /* ST_TITLE or ST_PACK: where B goes  */
void journal_saw(int kind);        /* you got a LOOK at it (battle/kill) */
void journal_kill(int kind);       /* ...and you put it down             */
void species_colors(int kind, uint16_t *edge, uint16_t *text);
                                   /* the strength tiers (battle.c)      */
void church_update(void);     void church_render(void);
void part1_start(void);       /* new man, new city: begins at mass */
void part1end_update(void);   void part1end_render(void);
void part1end_start(void);    /* he found them. roll the card.     */

/* scene transitions */
void overworld_start_game(void);              /* fresh game from title  */
void overworld_resume(void);                  /* CONTINUE from a save   */
void overworld_enter_map(int map, int tx, int ty);
void dialog_start(const char *text);
void monologue_start(const char *text);       /* the voice in his head */
void draw_toast(void);   /* the little corner popup (SAVED. and friends) --
                            every scene that can show one calls this, so
                            they all agree on where and how */

/* does the player have SOMETHING that fires? (Part 1's pistol counts) */
#define PLAYER_HAS_GUN() \
    (G.player.items[ITEM_SHOTGUN] || G.player.items[ITEM_HANDGUN])

/* ...and what feeds the gun he'd actually raise. The shotgun wins if he
 * somehow carries both, and each gun eats ONLY its own ammunition:
 * SHELLS for the shotgun, BULLETS for the pistol. */
#define GUN_AMMO() \
    (G.player.items[ITEM_SHOTGUN] ? ITEM_SHELLS : ITEM_BULLETS)
void battle_start(int ent_index);
void pack_discover(int item_kind);            /* it enters the pack     */
void mark_dead(int ent_index);                /* it stays dead till dawn */
void add_blood(int x, int y, int big);        /* a kill soaks the ground
                                                 of the CURRENT map     */
void blood_clear(void);                       /* the cycle washes it    */
int  map_music(int map);                      /* the song a place plays  */
void name_start(void);                        /* the name screen        */

#endif /* GAME_INTERNAL_H */
