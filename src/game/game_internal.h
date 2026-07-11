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
    ST_OVERWORLD,   /* walking around                (overworld.c) */
    ST_DIALOG,      /* textbox over the overworld    (overworld.c) */
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
    uint16_t items_taken[NUM_MAPS]; /* bit i = spawn i pocketed    */

    /* dialog scene */
    const char *dialog_text;
    int dialog_shown;    /* typewriter: chars revealed so far */

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
void battle_update(void);     void battle_render(void);
void gameover_update(void);   void gameover_render(void);

/* scene transitions */
void overworld_start_game(void);              /* fresh game from title  */
void overworld_enter_map(int map, int tx, int ty);
void dialog_start(const char *text);
void battle_start(int ent_index);

#endif /* GAME_INTERNAL_H */
