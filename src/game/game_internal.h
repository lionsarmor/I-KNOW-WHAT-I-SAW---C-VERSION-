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
    ST_OVERWORLD,   /* walking around                (overworld.c) */
    ST_DIALOG,      /* textbox over the overworld    (overworld.c) */
    ST_PACK,        /* the item menu (START)         (overworld.c) */
    ST_SLEEP,       /* you got into bed              (overworld.c) */
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
#define MAX_ENTITIES 16

typedef struct {
    int type;            /* ENT_NONE / ENT_ALIEN / ENT_NPC           */
    int kind;            /* ENT_ALIEN: SPECIES_*  ENT_NPC: LOOK_*    */
    int x, y;            /* map pixels                               */
    int dir;
    int timer;           /* alien AI: ticks until it re-decides      */
    const char *dialog;  /* NPC speech                               */
    int gift;            /* NPC: ITEM_*+1 to hand over; 0 = nothing  */
    const char *after;   /* NPC: line to say once the gift is given  */
} entity_t;

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

    uint32_t daytime;        /* overworld clock driving day/night;
                                see DAY_LEN_TICKS in config.h      */
    /* Bit i = spawn slot i on that map is GONE FOR GOOD this run: an item
     * you pocketed, a gift an NPC already handed over, a boss you killed.
     * Ordinary aliens are NOT marked -- they come back, so you can grind. */
    uint16_t spawns_gone[NUM_MAPS];

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
    int title_sel;       /* title menu: 0 = CONTINUE, 1 = NEW GAME      */

    /* the little banner that says SAVED. / LOADED. / NO SAVE FOUND */
    const char *toast;
    int toast_ticks;
    int toast_good;

    /* When the world last restocked its items. See ITEM_RESPAWN_TICKS. */
    uint32_t last_restock;

    /* battle scene */
    struct {
        int ent;                 /* index into G.ents we're fighting */
        int kind;                /* its SPECIES_* (stats + sprites)  */
        int enemy_hp, enemy_max;
        int menu;                /* 0 FIGHT  1 SHOOT  2 ITEM  3 RUN  */
        int item_sel;            /* ITEM submenu: 0 herb, 1 medkit   */
        int phase;               /* see battle.c */
        int timer;
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
void sleep_update(void);      void sleep_render(void);
void battle_update(void);     void battle_render(void);
void gameover_update(void);   void gameover_render(void);

/* scene transitions */
void overworld_start_game(void);              /* fresh game from title  */
void overworld_resume(void);                  /* CONTINUE from a save   */
void overworld_enter_map(int map, int tx, int ty);
void dialog_start(const char *text);
void battle_start(int ent_index);
void pack_discover(int item_kind);            /* it enters the pack     */
void name_start(void);                        /* the name screen        */

#endif /* GAME_INTERNAL_H */
