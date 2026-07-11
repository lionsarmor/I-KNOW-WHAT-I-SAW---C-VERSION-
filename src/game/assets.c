/* ============================================================================
 * assets.c -- turns the ASCII art in assets/ into pixels, and owns the
 * game's color PALETTE and tile rules (what's solid, what map characters
 * mean).
 * ==========================================================================*/
#include "assets.h"
#include "gfx.h"

#include "assets/font.h"      /* defines font8[][]      */
#include "assets/sprites.h"   /* defines SPR_ART_*      */
#include "assets/tiles.h"     /* defines TILE_ART_*     */
#include "assets/maps.h"      /* defines maps[]         */

/* ---------------------------------------------------------------------------
 * THE PALETTE -- what each ASCII-art character means, as RGB565.
 * Change a color here and it changes everywhere at once.
 * Add a new letter here to use it in any sprite or tile.
 * -------------------------------------------------------------------------*/
static uint16_t pal(char c)
{
    switch (c) {
    case '.': return COLOR_KEY;            /* transparent (sprites only) */
    case '0': return RGB565( 16,  16,  24); /* near-black                 */
    case '1': return RGB565(240, 240, 240); /* white                      */
    case 'g': return RGB565( 88, 160,  72); /* grass green                */
    case 'G': return RGB565( 48, 112,  48); /* dark green                 */
    case 'd': return RGB565(168, 128,  88); /* dirt / light wood          */
    case 'D': return RGB565(104,  72,  48); /* dark brown                 */
    case 'y': return RGB565(232, 200,  96); /* straw yellow               */
    case 'Y': return RGB565(180, 140,  56); /* straw shadow (hat brim)    */
    case 's': return RGB565(232, 176, 136); /* skin                       */
    case 'S': return RGB565(196, 132,  96); /* skin shadow                */
    case 'r': return RGB565(200,  56,  48); /* red                        */
    case 'R': return RGB565(136,  40,  40); /* dark red                   */
    case 'b': return RGB565( 72, 104, 176); /* denim blue                 */
    case 'B': return RGB565( 48,  64, 120); /* dark blue                  */
    case 'a': return RGB565(158, 166, 174); /* alien skin: pale corpse gray*/
    case 'A': return RGB565( 84,  92, 104); /* alien shadow gray          */
    case 'p': return RGB565(144,  88, 168); /* purple                     */
    case 'P': return RGB565( 96,  56, 116); /* purple shadow (dress hem)  */
    case 'o': return RGB565(224, 120,  48); /* orange                     */
    case 'w': return RGB565( 56,  96, 176); /* water                      */
    case 'W': return RGB565(136, 176, 224); /* water highlight            */
    case 'k': return RGB565(150, 150, 158); /* gray                       */
    case 'K': return RGB565( 90,  90,  98); /* dark gray                  */
    default:  return RGB565(255,   0, 255); /* loud magenta = "typo here" */
    }
}

/* ---------------------------------------------------------------------------
 * Which tiles block walking. 1 = solid wall, 0 = walkable.
 * -------------------------------------------------------------------------*/
const uint8_t tile_solid[NUM_TILES] = {
    [TILE_GRASS]  = 0, [TILE_GRASS2] = 0, [TILE_PATH]   = 0,
    [TILE_WATER]  = 1, [TILE_FENCE]  = 1, [TILE_TREE]   = 1,
    [TILE_WALL]   = 1, [TILE_ROOF]   = 1, [TILE_DOOR]   = 1,
    [TILE_CROP]   = 0, [TILE_FLOWER] = 0, [TILE_WATER2] = 1,
    [TILE_FLOOR]  = 0, [TILE_MAT]    = 0, [TILE_BED]    = 1,
    [TILE_TABLE]  = 1, [TILE_VOID]   = 1,
};
/* NOTE: TILE_DOOR stays SOLID -- walking INTO a door is what triggers
 * its warp (see check_door_bump in overworld.c). The doormat is the
 * walk-on kind of warp tile. */

/* map character -> tile id (see the legend in assets/maps.h) */
static int char_to_tile(char c)
{
    switch (c) {
    case '.': return TILE_GRASS;
    case ',': return TILE_GRASS2;
    case '#': return TILE_PATH;
    case 'w': return TILE_WATER;
    case 'f': return TILE_FENCE;
    case 'T': return TILE_TREE;
    case 'H': return TILE_WALL;
    case 'R': return TILE_ROOF;
    case 'D': return TILE_DOOR;
    case 'c': return TILE_CROP;
    case '*': return TILE_FLOWER;
    case '_': return TILE_FLOOR;
    case 'M': return TILE_MAT;
    case 'B': return TILE_BED;
    case 't': return TILE_TABLE;
    case 'x': return TILE_VOID;
    default:  return TILE_GRASS;   /* unknown char = grass, harmless */
    }
}

int map_tile(const map_t *m, int tx, int ty)
{
    if (tx < 0 || ty < 0 || tx >= m->w || ty >= m->h)
        return TILE_TREE;          /* the world edge is always solid */
    return char_to_tile(m->rows[ty][tx]);
}

/* ---------------------------------------------------------------------------
 * Decoding: ASCII art -> RGB565 pixel arrays (done once at boot).
 * -------------------------------------------------------------------------*/
sprite16_t sprites[NUM_SPRITES];
sprite16_t tiles[NUM_TILES];
uint16_t   face_big[FACE_W * FACE_H];

/* returns 1 if the art was malformed (wrong row length) */
static int decode(const char *const *art, int w, int h, uint16_t *out)
{
    int bad = 0;
    for (int y = 0; y < h; y++) {
        const char *row = art[y];
        int x = 0;
        for (; x < w && row[x]; x++)
            out[y * w + x] = pal(row[x]);
        if (x != w || row[x] != '\0') {   /* too short or too long */
            bad = 1;
            for (; x < w; x++)
                out[y * w + x] = RGB565(255, 0, 255);
        }
    }
    return bad;
}

int assets_init(void)
{
    int errors = 0;

    /* Register every sprite here, in the same order as the enum
     * in assets.h. This is the list to extend when you add art. */
    static const struct { int id; const char *const *art; } sprite_art[] = {
        { SPR_FARMER_DOWN_0, SPR_ART_FARMER_DOWN_0 },
        { SPR_FARMER_DOWN_1, SPR_ART_FARMER_DOWN_1 },
        { SPR_FARMER_DOWN_2, SPR_ART_FARMER_DOWN_2 },
        { SPR_FARMER_UP_0,   SPR_ART_FARMER_UP_0   },
        { SPR_FARMER_UP_1,   SPR_ART_FARMER_UP_1   },
        { SPR_FARMER_UP_2,   SPR_ART_FARMER_UP_2   },
        { SPR_FARMER_SIDE_0, SPR_ART_FARMER_SIDE_0 },
        { SPR_FARMER_SIDE_1, SPR_ART_FARMER_SIDE_1 },
        { SPR_FARMER_SIDE_2, SPR_ART_FARMER_SIDE_2 },
        { SPR_ALIEN_0,       SPR_ART_ALIEN_0       },
        { SPR_ALIEN_1,       SPR_ART_ALIEN_1       },
        { SPR_NPC,           SPR_ART_NPC           },
        { SPR_NPC_1,         SPR_ART_NPC_1         },
        { SPR_ELDER,         SPR_ART_ELDER         },
        { SPR_ELDER_1,       SPR_ART_ELDER_1       },
        { SPR_ITEM_HERB,     SPR_ART_ITEM_HERB     },
        { SPR_ITEM_MEDKIT,   SPR_ART_ITEM_MEDKIT   },
        { SPR_ITEM_SHELLS,   SPR_ART_ITEM_SHELLS   },
        { SPR_ITEM_SHOTGUN,  SPR_ART_ITEM_SHOTGUN  },
        { SPR_GUN_AIM,       SPR_ART_GUN_AIM       },
    };
    for (unsigned i = 0; i < sizeof sprite_art / sizeof sprite_art[0]; i++)
        errors += decode(sprite_art[i].art, TILE, TILE,
                         sprites[sprite_art[i].id].px);

    static const struct { int id; const char *const *art; } tile_art[] = {
        { TILE_GRASS,  TILE_ART_GRASS  }, { TILE_GRASS2, TILE_ART_GRASS2 },
        { TILE_PATH,   TILE_ART_PATH   }, { TILE_WATER,  TILE_ART_WATER  },
        { TILE_FENCE,  TILE_ART_FENCE  }, { TILE_TREE,   TILE_ART_TREE   },
        { TILE_WALL,   TILE_ART_WALL   }, { TILE_ROOF,   TILE_ART_ROOF   },
        { TILE_DOOR,   TILE_ART_DOOR   }, { TILE_CROP,   TILE_ART_CROP   },
        { TILE_FLOWER, TILE_ART_FLOWER }, { TILE_WATER2, TILE_ART_WATER2 },
        { TILE_FLOOR,  TILE_ART_FLOOR  }, { TILE_MAT,    TILE_ART_MAT    },
        { TILE_BED,    TILE_ART_BED    }, { TILE_TABLE,  TILE_ART_TABLE  },
        { TILE_VOID,   TILE_ART_VOID   },
    };
    for (unsigned i = 0; i < sizeof tile_art / sizeof tile_art[0]; i++)
        errors += decode(tile_art[i].art, TILE, TILE,
                         tiles[tile_art[i].id].px);

    errors += decode(SPR_ART_FACE_BIG, FACE_W, FACE_H, face_big);

    /* sanity-check the maps: every row must be exactly w chars */
    for (int m = 0; m < NUM_MAPS; m++)
        for (int y = 0; y < maps[m].h; y++) {
            int len = 0;
            while (maps[m].rows[y][len]) len++;
            if (len != maps[m].w)
                errors++;
        }

    return errors;
}

/* ===========================================================================
 * THE CAST & THE BESTIARY
 *
 * One row here = one kind of creature. See "THE CAST" in assets.h for
 * the step-by-step recipes. Stats reference: the player starts with
 * 20 HP and deals 4-6 damage at level 1.
 * =========================================================================*/

const species_t species[NUM_SPECIES] = {
    /*                 name       hp atk xp  frames                  bright */
    [SPECIES_GREY] = { "VISITOR", 10, 2,  5, SPR_ALIEN_0, SPR_ALIEN_1, 256 },
    /* the TALL ONE reuses the grey's art drawn darker (a classic
     * palette-swap villain). give it its own sprites when you draw them. */
    [SPECIES_TALL] = { "TALL ONE", 16, 4, 12, SPR_ALIEN_0, SPR_ALIEN_1, 150 },
};

const npc_look_t npc_looks[NUM_LOOKS] = {
    [LOOK_VILLAGER] = { SPR_NPC,   SPR_NPC_1   },
    [LOOK_ELDER]    = { SPR_ELDER, SPR_ELDER_1 },
};

/* ============================ THE ITEMS ====================================
 * One row = one thing you can pocket. See "THE ITEMS" in assets.h.
 * HERB heals a little, MEDKIT heals everything, SHELLS feed the SHOTGUN,
 * and the SHOTGUN unlocks SHOOT in battle.
 * =========================================================================*/

const item_info_t item_info[NUM_ITEMS] = {
    [ITEM_HERB]    = { "HERB",    SPR_ITEM_HERB,
        "A BITTER FIELD HERB. PA CHEWS IT WHEN HIS BACK GIVES OUT." },
    [ITEM_MEDKIT]  = { "MEDKIT",  SPR_ITEM_MEDKIT,
        "A FIRST AID KIT. GAUZE, IODINE, AND A PRAYER." },
    [ITEM_SHELLS]  = { "SHELLS",  SPR_ITEM_SHELLS,
        "A BOX OF SHOTGUN SHELLS. HEAVY. HONEST." },
    [ITEM_SHOTGUN] = { "SHOTGUN", SPR_ITEM_SHOTGUN,
        "THE FAMILY SHOTGUN. PA KEPT IT LOADED SINCE THE LIGHTS CAME." },
};
