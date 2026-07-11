/* ============================================================================
 * assets.h -- ids and access to every piece of art & data in the game.
 *
 * ALL art is authored as ASCII text in the headers under src/game/assets/:
 *
 *   assets/font.h     the 8x8 text font (1 bit per pixel)
 *   assets/sprites.h  characters: farmer, alien, townsfolk, big intro face
 *   assets/tiles.h    the 16x16 map tiles (grass, water, house, ...)
 *   assets/maps.h     the maps themselves (one character = one tile),
 *                     plus who lives on them (spawns) and the doors
 *                     between them (warps)
 *
 * At boot, assets_init() decodes the ASCII art into RGB565 pixel arrays
 * using the color palette at the top of assets.c.
 * ==========================================================================*/
#ifndef ASSETS_H
#define ASSETS_H

#include <stdint.h>
#include "config.h"

/* ---- Sprites (16x16 characters) ------------------------------------------
 * To add a sprite: draw it in assets/sprites.h, add an id here, and add
 * it to the decode list in assets.c. Left-facing art is mirrored at draw
 * time for right-facing, so you only draw one side.
 */
/* Farmer frames come in threes per direction: stand, step-left,
 * step-right. The walk cycle plays 0,1,0,2 (see overworld.c).
 * KEEP each direction's three frames consecutive -- the renderer
 * indexes them as SPR_FARMER_DIR_0 + frame. */
enum {
    SPR_FARMER_DOWN_0, SPR_FARMER_DOWN_1, SPR_FARMER_DOWN_2,
    SPR_FARMER_UP_0,   SPR_FARMER_UP_1,   SPR_FARMER_UP_2,
    SPR_FARMER_SIDE_0, SPR_FARMER_SIDE_1, SPR_FARMER_SIDE_2, /* faces LEFT */
    SPR_ALIEN_0,       SPR_ALIEN_1,       /* the visitors...             */
    SPR_NPC,           SPR_NPC_1,         /* villager (2 idle frames)    */
    SPR_ELDER,         SPR_ELDER_1,       /* old farmhand (2 frames)     */
    SPR_SKEPTIC,       SPR_SKEPTIC_1,
    SPR_MA,            SPR_MA_1,
    SPR_NEIGHBOR,      SPR_NEIGHBOR_1,
    SPR_STOREKEEP,     SPR_STOREKEEP_1,
    SPR_ITEM_HERB,     SPR_ITEM_MEDKIT,   /* pickups lying on maps       */
    SPR_ITEM_SHELLS,   SPR_ITEM_SHOTGUN,
    SPR_GUN_AIM,                          /* the shotgun raised (battle) */
    NUM_SPRITES
};

/* ============================ THE CAST =====================================
 * Everything that lives on a map is data, not code. The tables live in
 * assets.c under "THE CAST & THE BESTIARY".
 *
 * TO ADD AN ENEMY SPECIES:
 *   1. add an id below            (e.g. SPECIES_MANTIS)
 *   2. add a row to species[] in assets.c: name, stats, sprites
 *      (draw new sprites, or reuse existing ones with a different
 *       `bright` for a cheap palette-swap villain)
 *   3. spawn it on any map:  { ENT_ALIEN, x, y, SPECIES_MANTIS, 0 }
 *
 * TO ADD AN NPC:
 *   1. pick (or add) a look below (e.g. LOOK_SHERIFF + sprites)
 *   2. spawn it with its dialog:
 *      { ENT_NPC, x, y, LOOK_SHERIFF, "BACK INSIDE, SON." }
 * That's the whole job -- talking, blocking, animation are automatic.
 */
enum { SPECIES_GREY, SPECIES_TALL, NUM_SPECIES };

typedef struct {
    const char *name;      /* shown in battle ("A <NAME> BLOCKS...")     */
    int base_hp;           /* + rng(0..4) + 2 per player level           */
    int atk;               /* + rng(0..2) per hit                        */
    int xp;                /* awarded on defeat                          */
    int spr0, spr1;        /* two animation frames                       */
    int bright;            /* 256 = as drawn; lower = darker variant     */
} species_t;

extern const species_t species[NUM_SPECIES];

enum {
    LOOK_WITNESS, LOOK_SKEPTIC, LOOK_PA, LOOK_MA,
    LOOK_NEIGHBOR, LOOK_STOREKEEP, NUM_LOOKS
};

typedef struct { int spr0, spr1; } npc_look_t;   /* two idle frames */

extern const npc_look_t npc_looks[NUM_LOOKS];

/* ============================ THE ITEMS ====================================
 * TO ADD AN ITEM: add an id here, draw its sprite in sprites.h, add a
 * row to item_info[] in assets.c, then drop it on any map:
 *     { ENT_ITEM, x, y, ITEM_HERB, 0 }
 * The player walks over it: chime, pocket, message. A taken item stays
 * gone for the rest of the run, even if you leave and re-enter the map.
 *
 * HERB and MEDKIT are usable from the battle ITEM menu. SHELLS feed the
 * SHOTGUN; the SHOTGUN itself unlocks SHOOT in battle (see battle.c).
 */
enum { ITEM_HERB, ITEM_MEDKIT, ITEM_SHELLS, ITEM_SHOTGUN, NUM_ITEMS };

typedef struct {
    const char *name;         /* shown in the battle ITEM menu       */
    int spr;                  /* how it looks lying on the ground    */
    const char *pickup_msg;   /* the dialog shown when you grab it   */
} item_info_t;

extern const item_info_t item_info[NUM_ITEMS];

typedef struct { uint16_t px[TILE * TILE]; } sprite16_t;
extern sprite16_t sprites[NUM_SPRITES];

/* The big 32x32 alien face used by the intro (drawn scaled up). */
#define FACE_W 32
#define FACE_H 32
extern uint16_t face_big[FACE_W * FACE_H];

/* ---- Tiles (16x16 map squares) --------------------------------------------
 * To add a tile: draw it in assets/tiles.h, add an id here, decode it in
 * assets.c, give it a map character in char_to_tile(), and mark whether
 * you can walk through it in tile_solid[].
 */
enum {
    TILE_GRASS, TILE_GRASS2, TILE_PATH, TILE_WATER, TILE_FENCE,
    TILE_TREE,  TILE_WALL,   TILE_ROOF, TILE_DOOR,  TILE_CROP,
    TILE_FLOWER,
    TILE_FLOOR, TILE_MAT,    TILE_BED,  TILE_TABLE, TILE_VOID, /* interiors */
    TILE_WATER2,   /* animation frame only -- maps never use it directly */
    NUM_TILES
};

extern sprite16_t tiles[NUM_TILES];
extern const uint8_t tile_solid[NUM_TILES];  /* 1 = blocks walking */

/* ---- Maps -----------------------------------------------------------------*/
enum { MAP_FARM, MAP_TOWN, MAP_HOME, MAP_HOUSE, MAP_STORE, NUM_MAPS };

enum { ENT_NONE, ENT_ALIEN, ENT_NPC, ENT_ITEM };  /* entity types on a map */

typedef struct {          /* something living (or lying) on a map */
    int type;             /* ENT_ALIEN / ENT_NPC / ENT_ITEM            */
    int tx, ty;           /* spawn tile                                */
    int kind;             /* ALIEN: SPECIES_*  NPC: LOOK_*  ITEM: ITEM_* */
    const char *dialog;   /* NPCs: what they say (0 otherwise)         */
} spawn_t;

/* Warps fire two ways (both handled automatically):
 *   - on a WALKABLE tile (an exit mat): step on it, you teleport
 *   - on a SOLID door tile: WALK INTO it, you teleport ("entering")
 * A solid door with no warp is locked. */
typedef struct {
    int tx, ty;           /* trigger tile on this map   */
    int dest_map;
    int dest_tx, dest_ty; /* arrival tile               */
} warp_t;

typedef struct {
    const char *name;           /* shown when you enter        */
    const char *const *rows;    /* the ASCII map, h strings    */
    int w, h;                   /* size in tiles               */
    const spawn_t *spawns; int nspawns;
    const warp_t  *warps;  int nwarps;
    int outdoor;                /* 1 = night darkens this map  */
} map_t;

extern const map_t maps[NUM_MAPS];

/* tile id at tile coords (safely returns TILE_TREE outside the map,
 * so the world edge is always solid) */
int map_tile(const map_t *m, int tx, int ty);

/* ---- Font -----------------------------------------------------------------*/
extern const unsigned char font8[96][8];   /* ASCII 32..127, 8 bytes/glyph */

/* Decode all ASCII art -> pixels. Returns count of malformed assets
 * (wrong row length etc.); 0 means everything is clean. */
int assets_init(void);

#endif /* ASSETS_H */
