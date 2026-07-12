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
    /* THE GIANT ANTS -- six frames, and the ORDER MATTERS: the renderer
     * indexes them as spr0 + {0,1 = down} {2,3 = up} {4,5 = side}. */
    SPR_ANT_DOWN_0, SPR_ANT_DOWN_1,
    SPR_ANT_UP_0,   SPR_ANT_UP_1,
    SPR_ANT_SIDE_0, SPR_ANT_SIDE_1,       /* faces LEFT; mirrored for right */
    SPR_NPC,           SPR_NPC_1,         /* villager (2 idle frames)    */
    SPR_ELDER,         SPR_ELDER_1,       /* old farmhand (2 frames)     */
    SPR_SKEPTIC,       SPR_SKEPTIC_1,
    SPR_MA,            SPR_MA_1,
    SPR_NEIGHBOR,      SPR_NEIGHBOR_1,
    SPR_STOREKEEP,     SPR_STOREKEEP_1,
    SPR_ITEM_HERB,     SPR_ITEM_MEDKIT,   /* pickups lying on maps       */
    SPR_ITEM_SHELLS,   SPR_ITEM_SHOTGUN,
    SPR_ITEM_FLASHLIGHT,
    SPR_GUN_AIM,                          /* the shotgun leveled (firing) */
    SPR_GUN_READY,                        /* the shotgun shouldered       */
    SPR_BOSS_0,        SPR_BOSS_1,        /* the Hopkinsville goblin      */
    SPR_ITEM_KEY,      SPR_ITEM_TNT,
    SPR_COW,     SPR_COW_1,               /* the livestock (2 frames each) */
    SPR_GOAT,    SPR_GOAT_1,
    SPR_DOG,     SPR_DOG_1,
    SPR_CAT,     SPR_CAT_1,
    SPR_UFO,     SPR_UFO_1,               /* the name-screen cursor        */
    /* the shotgun carried in the overworld, one per facing */
    SPR_CARRY_DOWN, SPR_CARRY_UP, SPR_CARRY_SIDE,
    /* THE BESTIARY -- two frames each */
    SPR_DOVER_0,   SPR_DOVER_1,
    SPR_MOTH_0,    SPR_MOTH_1,
    SPR_CHUPA_0,   SPR_CHUPA_1,
    SPR_GREY_0,    SPR_GREY_1,
    SPR_REPTOID_0, SPR_REPTOID_1,
    SPR_NESSIE_0,  SPR_NESSIE_1,
    SPR_SQUATCH_0, SPR_SQUATCH_1,
    SPR_DOGMAN_0,  SPR_DOGMAN_1,
    SPR_TALL_0,    SPR_TALL_1,            /* THE TALL ONE                 */
    SPR_ANTHILL_0, SPR_ANTHILL_1,         /* the mound, seething          */
    SPR_QUEEN_0,   SPR_QUEEN_1,           /* the MUTANT QUEEN             */
    SPR_VAN, SPR_VAN_1,                   /* the 1980s van               */
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
/* ---- SPECIAL MOVES --------------------------------------------------------
 * Every creature has up to two, taken from its legend. Each turn it rolls
 * for them in order; if none land it just hits you the ordinary way.
 */
enum {
    MV_NONE = 0,
    MV_HEAVY,   /* one big hit                                    */
    MV_MULTI,   /* 2-3 quick hits of `power` each                 */
    MV_DRAIN,   /* hits for `power` AND heals itself that much    */
    MV_HEAL,    /* heals itself `power`                           */
    MV_STUN,    /* you lose your next turn -- it hits you free    */
};

typedef struct {
    const char *name;   /* shouted in the message box   */
    int kind;           /* MV_*                         */
    int power;
    int chance;         /* percent, rolled each turn    */
} move_t;

enum {
    SPECIES_ANT, SPECIES_SOLDIER, SPECIES_GOBLIN,
    SPECIES_DOVER, SPECIES_MOTHMAN, SPECIES_CHUPACABRA, SPECIES_GREY,
    SPECIES_REPTOID, SPECIES_NESSIE, SPECIES_SASQUATCH, SPECIES_DOGMAN,
    SPECIES_ANTHILL,      /* a mound. it does not move. it makes more ants. */
    SPECIES_QUEEN,        /* the MUTANT QUEEN -- big, fast, winged, wrong   */
    SPECIES_TALL,         /* THE TALL ONE -- grey, red-eyed, arms too long  */
    NUM_SPECIES
};

typedef struct {
    const char *name;      /* shown in battle ("A <NAME> BLOCKS...")     */
    int base_hp;           /* + rng(0..4) + 2 per player level           */
    int atk;               /* + rng(0..2) per hit                        */
    int xp;                /* awarded on defeat                          */
    int spr0, spr1;        /* two animation frames                       */
    int bright;            /* 256 = as drawn; lower = darker variant     */

    /* A BOSS is different in three ways, all from this one flag:
     *   - it never wanders: it stands where it was spawned, guarding
     *   - it is drawn BIGGER in battle
     *   - once killed it STAYS killed, even after you leave the map
     * Everything else (stats, xp, art) is just a normal row. */
    int boss;

    /* 1 = this creature has FOUR DIRECTIONS. Its spr0 is then the first of
     * SIX consecutive sprites -- down0 down1 up0 up1 side0 side1 -- and it
     * is drawn facing the way it walks (side art mirrored for right).
     * 0 = it just idles between spr0 and spr1, wherever it's facing. */
    int dirs;

    /* ZELDA MODE: how many shotgun blasts it takes to drop this thing out
     * in the field (1-3). It gets stunned on every hit that doesn't kill. */
    int ow_hits;

    /* its legend, made mechanical */
    move_t moves[2];

    /* ---- ADDED AFTER THE FACT --------------------------------------------
     * These live at the END on purpose: every row written before they
     * existed simply zero-fills them, which is exactly the old behaviour.
     * Only the rows that care mention them. */

    /* field chase speed in pixels/tick. 0 = the default for its class
     * (CHASE_SPEED, or BOSS_CHASE_SPEED for a boss). The MUTANT QUEEN is
     * the reason this exists: she is FASTER than you can run. */
    int ow_speed;

    /* 1 = it is ROOTED. It never wanders, never charges, never takes a
     * step: it is a mound of earth. It just sits there and produces. */
    int rooted;

    /* rooted things with a brood > 0 SPAWN: one new creature every `brood`
     * ticks, up to BROOD_MAX alive at once (config.h). 0 = spawns nothing. */
    int brood;
} species_t;

extern const species_t species[NUM_SPECIES];

enum {
    LOOK_WITNESS, LOOK_SKEPTIC, LOOK_PA, LOOK_MA,
    LOOK_NEIGHBOR, LOOK_STOREKEEP,
    /* the animals. Same machinery as people: two idle frames, solid, and
     * they "talk" when you face them and press A. */
    LOOK_COW, LOOK_GOAT, LOOK_DOG, LOOK_CAT,
    LOOK_VAN,            /* not a person. talk to it and you drive away. */
    NUM_LOOKS
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
 * HERB and MEDKIT are usable from the battle ITEM menu AND from the PACK
 * (press START while walking). SHELLS feed the SHOTGUN; the SHOTGUN
 * unlocks SHOOT in battle (see battle.c). The FLASHLIGHT isn't consumed
 * -- using it toggles it on and off, and it lights your way at night.
 */
enum {
    ITEM_HERB, ITEM_MEDKIT, ITEM_SHELLS, ITEM_SHOTGUN, ITEM_FLASHLIGHT,
    ITEM_KEY,
    ITEM_TNT,      /* pa's dynamite: enormous damage, and out in the field
                      the blast leaves things reeling twice as long */
    NUM_ITEMS
};

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

/* The van, 32x32, seen from behind -- the whole driving cutscene is the
 * back of it, so it gets four times the pixels. [0] cruising, [1] braking. */
/* The van is drawn from this 32x32 art. VAN_SCALE is how big it is OUT IN
 * THE WORLD -- 2 makes it 64x64, four tiles across, which is a van. */
#define VAN_SCALE 2
#define VAN_W 32
#define VAN_H 32
extern uint16_t van_big[2][VAN_W * VAN_H];

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
    TILE_ASPHALT, TILE_LINE, TILE_LAMP,     /* the parking lot */
    NUM_TILES
};

extern sprite16_t tiles[NUM_TILES];
extern const uint8_t tile_solid[NUM_TILES];  /* 1 = blocks walking */

/* ---- Maps -----------------------------------------------------------------*/
enum {
    MAP_FARM, MAP_TOWN, MAP_HOME, MAP_HOUSE, MAP_STORE, MAP_RIDGE,
    MAP_WRECK,          /* the locked house. the key opens it. */
    MAP_VANLOT,         /* east of town. your van. opens after the goblin. */
    NUM_MAPS
};

enum { ENT_NONE, ENT_ALIEN, ENT_NPC, ENT_ITEM };  /* entity types on a map */

typedef struct {          /* something living (or lying) on a map */
    int type;             /* ENT_ALIEN / ENT_NPC / ENT_ITEM            */
    int tx, ty;           /* spawn tile                                */
    int kind;             /* ALIEN: SPECIES_*  NPC: LOOK_*  ITEM: ITEM_* */
    const char *dialog;   /* NPCs: what they say (0 otherwise)         */

    /* An NPC can hand you something the first time you talk to them.
     * Write it as GIFT(ITEM_FLASHLIGHT). The +1 inside GIFT() is what
     * keeps "gives nothing" as the natural 0 default, so every spawn
     * line that doesn't mention a gift keeps working untouched. */
    int gift;
    const char *after;    /* what they say once the gift is handed over
                             (0 = just repeat `dialog`)                */
} spawn_t;

#define GIFT(item) ((item) + 1)

/* ---- STORY FLAGS ---------------------------------------------------------
 * Set once, checked forever, and SAVED. A warp can require one
 * (warp_t.needs_flag below), which is how the road east of town stays shut
 * until the thing standing in the north road has been dealt with.
 */
#define FLAG_GOBLIN_DEAD (1u << 0)

/* Warps fire two ways (both handled automatically):
 *   - on a WALKABLE tile (an exit mat): step on it, you teleport
 *   - on a SOLID door tile: WALK INTO it, you teleport ("entering")
 * A solid door with no warp is locked. */
typedef struct {
    int tx, ty;           /* trigger tile on this map   */
    int dest_map;
    int dest_tx, dest_ty; /* arrival tile               */
    int needs_key;        /* 1 = the brass key, or it stays shut */
    unsigned needs_flag;  /* a FLAG_* that must be set, or it stays shut */
    const char *shut;     /* what to say when it won't open              */
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
