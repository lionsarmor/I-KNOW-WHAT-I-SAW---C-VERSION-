/* ============================================================================
 * assets.c -- turns the ASCII art in assets/ into pixels, and owns the
 * game's color PALETTE and tile rules (what's solid, what map characters
 * mean).
 * ==========================================================================*/
#include "assets.h"
#include "gfx.h"
#include "audio.h"      /* melee_info[] names the SFX_* each weapon makes */

#include "assets/font.h"      /* defines font8[][]      */
#include "assets/sprites.h"   /* defines SPR_ART_*      */
#include "assets/tiles.h"     /* defines TILE_ART_*     */
#include "assets/maps.h"      /* defines maps[]         */

/* ---------------------------------------------------------------------------
 * THE PALETTE -- what each ASCII-art character means, as RGB565.
 * Change a color here and it changes everywhere at once.
 * Add a new letter here to use it in any sprite or tile.
 * -------------------------------------------------------------------------*/
/* Counts art characters that aren't in the palette below. decode() watches
 * this so a typo'd letter is reported by game_init() instead of quietly
 * painting magenta pixels on somebody's face for the rest of time. */
static int pal_unknown;

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
    case 'O': return RGB565(150,  72,  28); /* orange shadow              */
    case 'w': return RGB565( 56,  96, 176); /* water                      */
    case 'W': return RGB565(136, 176, 224); /* water highlight            */
    case 'k': return RGB565(150, 150, 158); /* gray                       */
    case 'K': return RGB565( 90,  90,  98); /* dark gray                  */
    /* bright chibi-sprite colors (kept separate from the map palette) */
    case 'c': return RGB565(220, 142,  78); /* cap / warm brown highlight */
    case 'C': return RGB565(132,  70,  44); /* cap shadow                 */
    case 'h': return RGB565( 72,  38,  30); /* hair midtone               */
    case 'H': return RGB565( 38,  22,  24); /* hair shadow                */
    case 'i': return RGB565(140, 184, 248); /* bright clothing blue       */
    case 'I': return RGB565( 68,  92, 176); /* blue shadow                */
    case 'm': return RGB565(255,  32, 112); /* hot pink                   */
    case 'M': return RGB565(156,  16,  72); /* pink shadow                */
    case 'e': return RGB565(112, 224, 208); /* visitor mint               */
    case 'E': return RGB565( 40, 128, 128); /* visitor shadow             */
    /* the Hopkinsville goblin: silvery, self-lit, wrong */
    case 'v': return RGB565(198, 226, 255); /* goblin glow (silver-blue)  */
    case 'V': return RGB565( 96, 140, 200); /* goblin shadow              */
    /* THE TAN ONE (Communion cover): a yellow-green olive hide, three
     * tones, so the big skull reads as smooth and modelled up close       */
    case 'n': return RGB565(172, 166, 102); /* olive-tan mid              */
    case 'N': return RGB565(112, 110,  64); /* olive-tan shadow           */
    case 'j': return RGB565(208, 202, 146); /* olive-tan highlight        */
    default:
        pal_unknown++;                      /* game_init() will report it */
        return RGB565(255, 0, 255);         /* loud magenta = "typo here" */
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
    [TILE_ASPHALT]= 0, [TILE_LINE]   = 0, [TILE_LAMP] = 1,
    [TILE_SIDEWALK] = 0, [TILE_LINE_H] = 0,
    /* the city's architecture is all solid -- the two DOOR kinds included:
     * bumping a door is what opens it, same rule as TILE_DOOR */
    [TILE_SKY_WALL] = 1, [TILE_SKY_TOP] = 1, [TILE_SKY_FAR] = 1,
    [TILE_CHURCH_WALL] = 1, [TILE_CHURCH_WIN] = 1, [TILE_CHURCH_CROSS] = 1,
    [TILE_CHURCH_DOOR] = 1, [TILE_DOOR_GLASS] = 1,
    /* the ship: walls and consoles stop you; the deck and its symbols
     * don't. The POD is SOLID -- like every door, you WALK INTO it to
     * use it (its warp fires on the bump, and only once the tan is down). */
    [TILE_UFOWALL] = 1, [TILE_UFOFLOOR] = 0, [TILE_COMPUTER] = 1,
    [TILE_SYMBOL] = 0,  [TILE_POD] = 1,
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
    case 'a': return TILE_ASPHALT;
    case '-': return TILE_LINE;
    case 'L': return TILE_LAMP;
    case 'p': return TILE_SIDEWALK;
    case '=': return TILE_LINE_H;
    case 'S': return TILE_SKY_WALL;
    case 'Z': return TILE_SKY_TOP;
    case 'z': return TILE_SKY_FAR;
    case 'C': return TILE_CHURCH_WALL;
    case 'W': return TILE_CHURCH_WIN;
    case '+': return TILE_CHURCH_CROSS;
    case 'U': return TILE_CHURCH_DOOR;
    case 'G': return TILE_DOOR_GLASS;
    /* ---- PART 2: THE SHIP ---- */
    case '%': return TILE_UFOWALL;
    case '/': return TILE_UFOFLOOR;
    case '&': return TILE_COMPUTER;
    case '@': return TILE_SYMBOL;
    case 'O': return TILE_POD;
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
uint16_t   van_big[2][VAN_W * VAN_H];
uint16_t   van_top[VANTOP_W * VANTOP_H];
uint16_t   copcar[COPCAR_W * COPCAR_H];

/* returns 1 if the art was malformed: a row of the wrong length, OR a
 * character that isn't in the palette (which would silently decode to
 * magenta -- easy to miss on a small sprite) */
static int decode(const char *const *art, int w, int h, uint16_t *out)
{
    int bad = 0;
    int unknown_before = pal_unknown;

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
    if (pal_unknown != unknown_before)
        bad = 1;                          /* a letter we don't have a color for */
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
        { SPR_ANT_DOWN_0,    SPR_ART_ANT_DOWN_0    },
        { SPR_ANT_DOWN_1,    SPR_ART_ANT_DOWN_1    },
        { SPR_ANT_UP_0,      SPR_ART_ANT_UP_0      },
        { SPR_ANT_UP_1,      SPR_ART_ANT_UP_1      },
        { SPR_ANT_SIDE_0,    SPR_ART_ANT_SIDE_0    },
        { SPR_ANT_SIDE_1,    SPR_ART_ANT_SIDE_1    },
        { SPR_NPC,           SPR_ART_NPC           },
        { SPR_NPC_1,         SPR_ART_NPC_1         },
        { SPR_ELDER,         SPR_ART_ELDER         },
        { SPR_ELDER_1,       SPR_ART_ELDER_1       },
        { SPR_SKEPTIC,       SPR_ART_SKEPTIC       },
        { SPR_SKEPTIC_1,     SPR_ART_SKEPTIC_1     },
        { SPR_MA,            SPR_ART_MA             },
        { SPR_MA_1,          SPR_ART_MA_1           },
        { SPR_NEIGHBOR,      SPR_ART_NEIGHBOR       },
        { SPR_NEIGHBOR_1,    SPR_ART_NEIGHBOR_1     },
        { SPR_STOREKEEP,     SPR_ART_STOREKEEP      },
        { SPR_STOREKEEP_1,   SPR_ART_STOREKEEP_1    },
        { SPR_ITEM_HERB,     SPR_ART_ITEM_HERB     },
        { SPR_ITEM_MEDKIT,   SPR_ART_ITEM_MEDKIT   },
        { SPR_ITEM_SHELLS,   SPR_ART_ITEM_SHELLS   },
        { SPR_ITEM_SHOTGUN,  SPR_ART_ITEM_SHOTGUN  },
        { SPR_ITEM_FLASHLIGHT, SPR_ART_ITEM_FLASHLIGHT },
        { SPR_GUN_AIM,       SPR_ART_GUN_AIM       },
        { SPR_GUN_READY,     SPR_ART_GUN_READY     },
        { SPR_BOSS_0,        SPR_ART_BOSS_0        },
        { SPR_BOSS_1,        SPR_ART_BOSS_1        },
        { SPR_ITEM_KEY,      SPR_ART_ITEM_KEY      },
        { SPR_ITEM_TNT,      SPR_ART_ITEM_TNT      },
        { SPR_TALL_0,        SPR_ART_TALL_0        },
        { SPR_TALL_1,        SPR_ART_TALL_1        },
        { SPR_ANTHILL_0,     SPR_ART_ANTHILL_0     },
        { SPR_ANTHILL_1,     SPR_ART_ANTHILL_1     },
        { SPR_QUEEN_0,       SPR_ART_QUEEN_0       },
        { SPR_QUEEN_1,       SPR_ART_QUEEN_1       },
        { SPR_COW,           SPR_ART_COW           },
        { SPR_COW_1,         SPR_ART_COW_1         },
        { SPR_GOAT,          SPR_ART_GOAT          },
        { SPR_GOAT_1,        SPR_ART_GOAT_1        },
        { SPR_DOG,           SPR_ART_DOG           },
        { SPR_DOG_1,         SPR_ART_DOG_1         },
        { SPR_CAT,           SPR_ART_CAT           },
        { SPR_CAT_1,         SPR_ART_CAT_1         },
        { SPR_UFO,           SPR_ART_UFO           },
        { SPR_UFO_1,         SPR_ART_UFO_1         },
        { SPR_CARRY_DOWN,    SPR_ART_CARRY_DOWN    },
        { SPR_CARRY_UP,      SPR_ART_CARRY_UP      },
        { SPR_CARRY_SIDE,    SPR_ART_CARRY_SIDE    },
        { SPR_DOVER_0,       SPR_ART_DOVER_0       },
        { SPR_DOVER_1,       SPR_ART_DOVER_1       },
        { SPR_MOTH_0,        SPR_ART_MOTH_0        },
        { SPR_MOTH_1,        SPR_ART_MOTH_1        },
        { SPR_CHUPA_0,       SPR_ART_CHUPA_0       },
        { SPR_CHUPA_1,       SPR_ART_CHUPA_1       },
        { SPR_GREY_0,        SPR_ART_GREY_0        },
        { SPR_GREY_1,        SPR_ART_GREY_1        },
        { SPR_REPTOID_0,     SPR_ART_REPTOID_0     },
        { SPR_REPTOID_1,     SPR_ART_REPTOID_1     },
        { SPR_NESSIE_0,      SPR_ART_NESSIE_0      },
        { SPR_NESSIE_1,      SPR_ART_NESSIE_1      },
        { SPR_SQUATCH_0,     SPR_ART_SQUATCH_0     },
        { SPR_SQUATCH_1,     SPR_ART_SQUATCH_1     },
        { SPR_DOGMAN_0,      SPR_ART_DOGMAN_0      },
        { SPR_DOGMAN_1,      SPR_ART_DOGMAN_1      },
        { SPR_VAN,           SPR_ART_VAN           },
        { SPR_VAN_1,         SPR_ART_VAN_1         },
        /* ---- PART 1 ---- */
        { SPR_LAWYER_DOWN_0, SPR_ART_LAWYER_DOWN_0 },
        { SPR_LAWYER_DOWN_1, SPR_ART_LAWYER_DOWN_1 },
        { SPR_LAWYER_DOWN_2, SPR_ART_LAWYER_DOWN_2 },
        { SPR_LAWYER_UP_0,   SPR_ART_LAWYER_UP_0   },
        { SPR_LAWYER_UP_1,   SPR_ART_LAWYER_UP_1   },
        { SPR_LAWYER_UP_2,   SPR_ART_LAWYER_UP_2   },
        { SPR_LAWYER_SIDE_0, SPR_ART_LAWYER_SIDE_0 },
        { SPR_LAWYER_SIDE_1, SPR_ART_LAWYER_SIDE_1 },
        { SPR_LAWYER_SIDE_2, SPR_ART_LAWYER_SIDE_2 },
        { SPR_PRIEST,        SPR_ART_PRIEST        },
        { SPR_PRIEST_1,      SPR_ART_PRIEST_1      },
        { SPR_COP,           SPR_ART_COP           },
        { SPR_COP_1,         SPR_ART_COP_1         },
        { SPR_CITYWOMAN,     SPR_ART_CITYWOMAN     },
        { SPR_CITYWOMAN_1,   SPR_ART_CITYWOMAN_1   },
        { SPR_CRUCIFIX,      SPR_ART_CRUCIFIX      },
        { SPR_ITEM_HANDGUN,  SPR_ART_ITEM_HANDGUN  },
        { SPR_ITEM_BULLETS,  SPR_ART_ITEM_BULLETS  },
        { SPR_ITEM_HOLYWATER, SPR_ART_ITEM_HOLYWATER },
        { SPR_ITEM_ROSARY,   SPR_ART_ITEM_ROSARY   },
        { SPR_DEACON,        SPR_ART_DEACON        },
        { SPR_DEACON_1,      SPR_ART_DEACON_1      },
        /* the dead don't animate: both frames decode the same art */
        { SPR_DEADLADY,      SPR_ART_DEADLADY      },
        { SPR_DEADLADY_1,    SPR_ART_DEADLADY      },
        { SPR_WIFE,          SPR_ART_WIFE          },
        { SPR_WIFE_1,        SPR_ART_WIFE_1        },
        { SPR_BOY,           SPR_ART_BOY           },
        { SPR_BOY_1,         SPR_ART_BOY_1         },
        { SPR_BRAD,          SPR_ART_BRAD          },
        { SPR_BRAD_1,        SPR_ART_BRAD_1        },
        { SPR_CHICKEN,       SPR_ART_CHICKEN       },
        { SPR_CHICKEN_1,     SPR_ART_CHICKEN_1     },
        { SPR_PIG,           SPR_ART_PIG           },
        { SPR_PIG_1,         SPR_ART_PIG_1         },
        { SPR_JENNA,         SPR_ART_JENNA         },
        { SPR_JENNA_1,       SPR_ART_JENNA_1       },
        { SPR_DAN,           SPR_ART_DAN           },
        { SPR_DAN_1,         SPR_ART_DAN_1         },
        /* ---- PART 2: THE SHIP ---- */
        { SPR_SILVER_DOWN_0, SPR_ART_SILVER_DOWN_0 },
        { SPR_SILVER_DOWN_1, SPR_ART_SILVER_DOWN_1 },
        { SPR_SILVER_DOWN_2, SPR_ART_SILVER_DOWN_2 },
        { SPR_SILVER_UP_0,   SPR_ART_SILVER_UP_0   },
        { SPR_SILVER_UP_1,   SPR_ART_SILVER_UP_1   },
        { SPR_SILVER_UP_2,   SPR_ART_SILVER_UP_2   },
        { SPR_SILVER_SIDE_0, SPR_ART_SILVER_SIDE_0 },
        { SPR_SILVER_SIDE_1, SPR_ART_SILVER_SIDE_1 },
        { SPR_SILVER_SIDE_2, SPR_ART_SILVER_SIDE_2 },
        { SPR_TAN_0,         SPR_ART_TAN_0         },
        { SPR_TAN_1,         SPR_ART_TAN_1         },
        { SPR_ITEM_LASER,    SPR_ART_ITEM_LASER    },
        { SPR_ITEM_BATTERY,  SPR_ART_ITEM_BATTERY  },
        { SPR_ITEM_NUKE,     SPR_ART_ITEM_NUKE     },
        { SPR_ITEM_GEL,      SPR_ART_ITEM_GEL      },
        { SPR_ITEM_GOO,      SPR_ART_ITEM_GOO      },
        { SPR_CLONETANK,     SPR_ART_CLONETANK     },
        { SPR_CLONETANK_1,   SPR_ART_CLONETANK_1   },
        { SPR_SADIE,         SPR_ART_SADIE         },
        { SPR_SADIE_1,       SPR_ART_SADIE_1       },
        { SPR_SADIE_DN0,     SPR_ART_SADIE_DN0     },
        { SPR_SADIE_DN1,     SPR_ART_SADIE_DN1     },
        { SPR_SADIE_UP0,     SPR_ART_SADIE_UP0     },
        { SPR_SADIE_UP1,     SPR_ART_SADIE_UP1     },
        { SPR_SADIE_SD0,     SPR_ART_SADIE_SD0     },
        { SPR_SADIE_SD1,     SPR_ART_SADIE_SD1     },
        { SPR_ITEM_SPADE,    SPR_ART_ITEM_SPADE    },
        { SPR_ITEM_PIPE,     SPR_ART_ITEM_PIPE     },
        { SPR_ITEM_PROD,     SPR_ART_ITEM_PROD     },
        { SPR_HOLLIS,        SPR_ART_HOLLIS        },
        { SPR_HOLLIS_1,      SPR_ART_HOLLIS_1      },
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
        { TILE_ASPHALT, TILE_ART_ASPHALT },
        { TILE_LINE,   TILE_ART_LINE   },
        { TILE_LAMP,   TILE_ART_LAMP   },
        { TILE_SIDEWALK, TILE_ART_SIDEWALK },
        { TILE_LINE_H, TILE_ART_LINE_H },
        { TILE_SKY_WALL,     TILE_ART_SKY_WALL     },
        { TILE_SKY_TOP,      TILE_ART_SKY_TOP      },
        { TILE_SKY_FAR,      TILE_ART_SKY_FAR      },
        { TILE_CHURCH_WALL,  TILE_ART_CHURCH_WALL  },
        { TILE_CHURCH_WIN,   TILE_ART_CHURCH_WIN   },
        { TILE_CHURCH_CROSS, TILE_ART_CHURCH_CROSS },
        { TILE_CHURCH_DOOR,  TILE_ART_CHURCH_DOOR  },
        { TILE_DOOR_GLASS,   TILE_ART_DOOR_GLASS   },
        /* ---- PART 2: THE SHIP ---- */
        { TILE_UFOWALL,      TILE_ART_UFOWALL      },
        { TILE_UFOFLOOR,     TILE_ART_UFOFLOOR     },
        { TILE_COMPUTER,     TILE_ART_COMPUTER     },
        { TILE_SYMBOL,       TILE_ART_SYMBOL       },
        { TILE_POD,          TILE_ART_POD          },
    };
    for (unsigned i = 0; i < sizeof tile_art / sizeof tile_art[0]; i++)
        errors += decode(tile_art[i].art, TILE, TILE,
                         tiles[tile_art[i].id].px);

    errors += decode(SPR_ART_FACE_BIG, FACE_W, FACE_H, face_big);
    errors += decode(SPR_ART_VAN_BIG_0, VAN_W, VAN_H, van_big[0]);
    errors += decode(SPR_ART_VAN_BIG_1, VAN_W, VAN_H, van_big[1]);
    errors += decode(SPR_ART_VANTOP,  VANTOP_W, VANTOP_H, van_top);
    errors += decode(SPR_ART_COPCAR,  COPCAR_W, COPCAR_H, copcar);

    /* sanity-check the maps: every row must be exactly w chars */
    for (int m = 0; m < NUM_MAPS; m++)
        for (int y = 0; y < maps[m].h; y++) {
            int len = 0;
            while (maps[m].rows[y][len]) len++;
            if (len != maps[m].w)
                errors++;
        }

    /* Sanity-check every SPOKEN LINE: once '~' becomes the player's name it
     * still has to fit in the dialog buffer, or the end of the sentence is
     * silently lost. Worst case is the longest possible name. */
    for (int m = 0; m < NUM_MAPS; m++)
        for (int i = 0; i < maps[m].nspawns; i++) {
            const char *lines[2] = { maps[m].spawns[i].dialog,
                                     maps[m].spawns[i].after };
            for (int k = 0; k < 2; k++) {
                if (!lines[k])
                    continue;
                int len = 0, tildes = 0;
                for (const char *c = lines[k]; *c; c++) {
                    len++;
                    if (*c == '~') tildes++;
                }
                /* each '~' becomes up to PLAYER_NAME_MAX chars */
                if (len + tildes * (PLAYER_NAME_MAX - 1) >= DIALOG_BUF_MAX)
                    errors++;
            }
        }

    /* Sanity-check the SPAWNS: nothing may start life inside a wall.
     * An alien spawned on a fence tile can never step off it (it only
     * moves onto walkable ground), so it stands there forever, embedded
     * in the fence. Same for an NPC or an item you could never reach. */
    for (int m = 0; m < NUM_MAPS; m++)
        for (int i = 0; i < maps[m].nspawns; i++) {
            const spawn_t *sp = &maps[m].spawns[i];
            if (tile_solid[map_tile(&maps[m], sp->tx, sp->ty)])
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
/* Each row: name, hp, atk, xp, frame0, frame1, bright, boss, dirs, ow_hits,
 * then up to TWO SPECIAL MOVES taken straight from the creature's legend.
 * A move rolls its `chance` each turn; the first one that lands is used.
 *
 * ow_hits = shotgun blasts to drop it out in the field (Zelda mode). */

    /* --- the ants: the only things here that walk in four directions --- */
    [SPECIES_ANT] = { "GIANT ANT", 10, 2, 5,
        SPR_ANT_DOWN_0, SPR_ANT_DOWN_1, 256, 0, 1, 1,
        { { "FORMIC ACID SPRAY", MV_HEAVY, 6, 25 } } },

    [SPECIES_SOLDIER] = { "SOLDIER ANT", 16, 4, 12,
        SPR_ANT_DOWN_0, SPR_ANT_DOWN_1, 180, 0, 1, 2,
        { { "MANDIBLE LOCK", MV_MULTI, 3, 30 } } },

    /* --- KELLY, KENTUCKY, 1955. They shot at it all night. It kept coming.
     * NINE blasts out in the field, and it does not stagger and it does not
     * get knocked back (see BOSS_STUN_TICKS in config.h). Shooting it is the
     * alternative to the brutal turn-based fight -- it is not the easy way. */
    [SPECIES_GOBLIN] = { "HOPKINSVILLE GOBLIN", 55, 6, 60,
        SPR_BOSS_0, SPR_BOSS_1, 256, 1, 0, 9,
        { { "IMPOSSIBLE LEAP", MV_STUN, 0, 22 },
          { "THE BULLETS DO NOTHING", MV_HEAL, 9, 20 } } },

    /* --- DOVER, MASSACHUSETTS, 1977. Two orange eyes, and it just stares. */
    [SPECIES_DOVER] = { "DOVER DEMON", 30, 5, 30,
        SPR_DOVER_0, SPR_DOVER_1, 256, 0, 0, 2,
        { { "GLASS-EYED STARE", MV_STUN, 0, 30 },
          { "SPINDLE GRASP", MV_HEAVY, 8, 22 } } },

    /* --- POINT PLEASANT, 1966. It shows up before the bridge comes down. */
    [SPECIES_MOTHMAN] = { "MOTHMAN", 45, 7, 55,
        SPR_MOTH_0, SPR_MOTH_1, 256, 0, 0, 3,
        { { "PROPHECY OF DISASTER", MV_HEAVY, 14, 25 },
          { "WING GUST", MV_MULTI, 4, 25 } } },

    /* --- it drains what it catches, and leaves the rest. */
    [SPECIES_CHUPACABRA] = { "CHUPACABRA", 35, 6, 40,
        SPR_CHUPA_0, SPR_CHUPA_1, 256, 0, 0, 2,
        { { "EXSANGUINATE", MV_DRAIN, 9, 35 } } },

    /* --- the classic. You lose time around them and never get it back. */
    [SPECIES_GREY] = { "THE GREY", 32, 5, 38,
        SPR_GREY_0, SPR_GREY_1, 256, 0, 0, 2,
        { { "MISSING TIME", MV_STUN, 0, 32 },
          { "THE PROBE", MV_HEAVY, 9, 20 } } },

    /* --- it sheds, and what's underneath is fine. */
    [SPECIES_REPTOID] = { "REPTOID", 48, 7, 58,
        SPR_REPTOID_0, SPR_REPTOID_1, 256, 0, 0, 3,
        { { "SHED SKIN", MV_HEAL, 11, 25 },
          { "TAIL WHIP", MV_HEAVY, 10, 25 } } },

    /* --- most of it is under the water. */
    [SPECIES_NESSIE] = { "LOCH NESS MONSTER", 60, 8, 70,
        SPR_NESSIE_0, SPR_NESSIE_1, 256, 0, 0, 3,
        { { "TIDAL SURGE", MV_HEAVY, 16, 30 },
          { "SOUND THE DEPTHS", MV_HEAL, 10, 18 } } },

    /* --- eight feet of shoulders. It throws what's lying around. */
    [SPECIES_SASQUATCH] = { "SASQUATCH", 65, 9, 75,
        SPR_SQUATCH_0, SPR_SQUATCH_1, 256, 0, 0, 3,
        { { "TIMBER SLAM", MV_HEAVY, 17, 28 },
          { "STONE THROW", MV_HEAVY, 9, 22 } } },

    /* --- Michigan, 1887. The wrong part is that it STANDS UP. */
    [SPECIES_DOGMAN] = { "DOGMAN", 50, 8, 62,
        SPR_DOGMAN_0, SPR_DOGMAN_1, 256, 0, 0, 3,
        { { "FRENZY", MV_MULTI, 6, 30 },
          { "HOWL AT THE RIDGE", MV_STUN, 0, 20 } } },

    /* --- THE TALL ONE. Nobody who saw it agrees on the face. Everybody
     * agrees on the height, and on the arms. It does not charge and it does
     * not hurry -- it moves at exactly your speed, and it is simply closer
     * every time you look up. Very tough: five shells, and it hits like a
     * falling tree. */
    [SPECIES_TALL] = { "THE TALL ONE", 78, 10, 95,
        SPR_TALL_0, SPR_TALL_1, 256, 0, 0, 5,
        { { "IT IS ALREADY BEHIND YOU", MV_STUN, 0, 30 },
          { "THOSE ARMS", MV_HEAVY, 15, 32 } },
        2, 0, 0 },

    /* --- THE ANT HILL. It is not an animal. It is a hole in the ground that
     * things come out of, and it will keep making them until you close it.
     * ROOTED: it never takes a step. Shoot it and it seethes. Walk into it
     * and the whole mound comes up your leg.
     *
     * The last three numbers are ow_speed / rooted / brood -- brood is the
     * gap in ticks between ants, so a smaller number is a worse night. */
    [SPECIES_ANTHILL] = { "ANT HILL", 26, 3, 45,
        SPR_ANTHILL_0, SPR_ANTHILL_1, 256, 0, 0, 4,
        { { "THE MOUND ERUPTS", MV_MULTI, 4, 40 },
          { "SWARM", MV_DRAIN, 5, 25 } },
        0, 1, ANTHILL_BROOD_TICKS },

    /* --- THE MUTANT QUEEN. Whatever came down did this to her first, and it
     * did it worst. Bloated, wet, winged, and FASTER THAN YOU CAN RUN
     * (ow_speed 3 against your 2) -- you cannot simply walk away from her.
     * She births soldiers mid-fight and she does not stop.
     *
     * A boss: she waits where she stands, she's drawn huge in battle, and
     * once she's down she is down for good. */
    [SPECIES_QUEEN] = { "MUTANT QUEEN", 62, 7, 75,
        SPR_QUEEN_0, SPR_QUEEN_1, 256, 1, 0, 7,
        { { "BIRTH A SOLDIER", MV_MULTI, 5, 30 },
          { "FESTERING BITE", MV_DRAIN, 11, 30 } },
        3, 0, 0 },

    /* --- LE CHUPACABRA. The one from the park -- bigger, older, and it has
     * been eating well since the lights came. A BOSS: it stands square on
     * the path home and waits, it keeps pace when it charges (ow_speed 2),
     * and once it's down it stays down. Drawn a shade darker than the
     * ridge-country chupacabra: this one hunts under street lights. */
    [SPECIES_CHUPA_BOSS] = { "LE CHUPACABRA", 66, 8, 85,
        SPR_CHUPA_0, SPR_CHUPA_1, 200, 1, 0, 8,
        { { "EXSANGUINATE", MV_DRAIN, 12, 35 },
          { "THE SHRIEK", MV_STUN, 0, 22 } },
        2, 0, 0 },

    /* --- THE VISITOR. A grey, alone, standing in the Starlight Diner with
     * its hands at its sides. It has been standing there since midnight.
     * The coffee is still warm. A BOSS: it does not wander, it does not
     * blink first, and when it's gone it's gone. Brighter than the ridge
     * greys -- it is not trying to hide. */
    [SPECIES_GREY_BOSS] = { "THE VISITOR", 82, 10, 120,
        SPR_GREY_0, SPR_GREY_1, 256, 1, 0, 9,
        { { "MISSING TIME", MV_STUN, 0, 30 },
          { "THE PROBE", MV_HEAVY, 13, 30 } },
        2, 0, 0 },

    /* --- THE TAN ONE. Part 2's boss, and the last face in the book. The
     * one from the Communion cover: taller than the greys, the color of
     * old paper, and it does not hurry because it does not have to. It
     * stands between you and the pod. It hits like the thing that put you
     * on the table. Twelve laser bolts in the field, and it keeps pace. */
    [SPECIES_TAN] = { "THE TAN ONE", 95, 12, 150,
        SPR_TAN_0, SPR_TAN_1, 256, 1, 0, 12,
        { { "IT REMEMBERS YOU", MV_STUN, 0, 30 },
          { "THE LONG NEEDLE", MV_DRAIN, 15, 30 } },
        2, 0, 0 },

    /* --- THE CLONING TANK. Not a creature, a MACHINE that makes them: a
     * rooted, solid vat that pushes a north-ridge horror out onto the deck
     * every `brood` ticks. It never moves and never fights -- but it takes
     * a dozen laser bolts to shatter the glass. XP 0: the reward is that
     * it stops. See lab_pool()/hill_spawn() and the kill in overworld.c. */
    [SPECIES_CLONETANK] = { "CLONING TANK", 40, 0, 0,
        SPR_CLONETANK, SPR_CLONETANK_1, 256, 0, 0, 8,
        { { 0 } },
        0, 1, 300 },        /* rooted; 8 bolts to shatter; decants HALF as
                               often (every 300 ticks) so the lab doesn't
                               bury you before you reach the girl */
};

/* ---- THE LEGENDS, FOR THE JOURNAL ------------------------------------------
 * One line each, the way you'd write it down by flashlight. The journal
 * (intro.c) prints these under the sprite once you've seen the thing.
 * Kept apart from species[] so the stat rows stay positional. */
const char *species_lore[NUM_SPECIES] = {
    [SPECIES_ANT]        = "THEY CAME UP OUT OF THE FIELDS. THE HILLS ARE "
                           "STILL PRODUCING.",
    [SPECIES_SOLDIER]    = "THE BIG ONES. THEY LOCK ON AND THEY DO NOT "
                           "LET GO.",
    [SPECIES_GOBLIN]     = "KELLY, KENTUCKY, 1955. THEY SHOT AT IT ALL "
                           "NIGHT. IT KEPT COMING.",
    [SPECIES_DOVER]      = "DOVER, MASSACHUSETTS, 1977. TWO ORANGE EYES, "
                           "AND IT JUST STARES.",
    [SPECIES_MOTHMAN]    = "POINT PLEASANT, 1966. IT SHOWS UP BEFORE THE "
                           "BRIDGE COMES DOWN.",
    [SPECIES_CHUPACABRA] = "IT DRAINS WHAT IT CATCHES, AND LEAVES THE "
                           "REST.",
    [SPECIES_GREY]       = "THE CLASSIC. YOU LOSE TIME AROUND THEM AND "
                           "YOU NEVER GET IT BACK.",
    [SPECIES_REPTOID]    = "IT SHEDS, AND WHAT'S UNDERNEATH IS FINE.",
    [SPECIES_NESSIE]     = "MOST OF IT IS UNDER THE WATER.",
    [SPECIES_SASQUATCH]  = "EIGHT FEET OF SHOULDERS. IT THROWS WHAT'S "
                           "LYING AROUND.",
    [SPECIES_DOGMAN]     = "SOME NIGHTS THE RIDGE HOWLS. SOME NIGHTS THE "
                           "RIDGE HOWLS BACK.",
    [SPECIES_ANTHILL]    = "THE MOUND IS NOT DIRT. THE MOUND IS A DOOR.",
    [SPECIES_QUEEN]      = "WHERE THE ANTS COME FROM. SHE IS FASTER THAN "
                           "YOU ARE.",
    [SPECIES_TALL]       = "EVERY STORY AGREES ON THE HEIGHT, AND ON THE "
                           "ARMS. IT IS CLOSER EVERY TIME YOU LOOK UP.",
    [SPECIES_CHUPA_BOSS] = "IT HAS BEEN EATING WELL SINCE THE LIGHTS "
                           "CAME. IT HUNTS UNDER THE STREET LIGHTS NOW.",
    [SPECIES_GREY_BOSS]  = "THE STARLIGHT DINER. IT HAS BEEN STANDING "
                           "THERE SINCE MIDNIGHT. THE COFFEE IS STILL "
                           "WARM.",
    [SPECIES_TAN]        = "THE ONE FROM THE COVER OF THAT BOOK. IT WAS "
                           "IN THE ROOM WHEN YOU WOKE UP. IT WAS THERE "
                           "THE FIRST TIME, TOO -- ON THE HIGHWAY.",
    [SPECIES_CLONETANK]  = "THEY DON'T CATCH THEM. THEY GROW THEM. EVERY "
                           "THING I EVER SAW IN A FIELD WAS DECANTED IN A "
                           "ROOM LIKE THIS ONE.",
};

const npc_look_t npc_looks[NUM_LOOKS] = {
    [LOOK_WITNESS]   = { SPR_NPC,       SPR_NPC_1       },
    [LOOK_SKEPTIC]   = { SPR_SKEPTIC,   SPR_SKEPTIC_1   },
    [LOOK_PA]        = { SPR_ELDER,      SPR_ELDER_1     },
    [LOOK_MA]        = { SPR_MA,         SPR_MA_1         },
    [LOOK_NEIGHBOR]  = { SPR_NEIGHBOR,   SPR_NEIGHBOR_1   },
    [LOOK_STOREKEEP] = { SPR_STOREKEEP,  SPR_STOREKEEP_1  },
    [LOOK_COW]       = { SPR_COW,        SPR_COW_1        },
    [LOOK_GOAT]      = { SPR_GOAT,       SPR_GOAT_1       },
    [LOOK_DOG]       = { SPR_DOG,        SPR_DOG_1        },
    [LOOK_CAT]       = { SPR_CAT,        SPR_CAT_1        },
    [LOOK_VAN]       = { SPR_VAN,        SPR_VAN_1        },
    /* ---- PART 1 ---- */
    [LOOK_PRIEST]    = { SPR_PRIEST,     SPR_PRIEST_1     },
    [LOOK_COP]       = { SPR_COP,        SPR_COP_1        },
    [LOOK_CITYWOMAN] = { SPR_CITYWOMAN,  SPR_CITYWOMAN_1  },
    /* the cruiser never uses these two frames -- the renderer special-cases
     * it (see overworld_render) -- but the row has to exist. */
    [LOOK_COPCAR]    = { SPR_VAN,        SPR_VAN_1        },
    [LOOK_DEACON]    = { SPR_DEACON,     SPR_DEACON_1     },
    [LOOK_DEADLADY]  = { SPR_DEADLADY,   SPR_DEADLADY_1   },
    [LOOK_WIFE]      = { SPR_WIFE,       SPR_WIFE_1       },
    [LOOK_BOY]       = { SPR_BOY,        SPR_BOY_1        },
    [LOOK_BRAD]      = { SPR_BRAD,       SPR_BRAD_1       },
    [LOOK_CHICKEN]   = { SPR_CHICKEN,    SPR_CHICKEN_1    },
    [LOOK_PIG]       = { SPR_PIG,        SPR_PIG_1        },
    [LOOK_JENNA]     = { SPR_JENNA,      SPR_JENNA_1      },
    [LOOK_DAN]       = { SPR_DAN,        SPR_DAN_1        },
    [LOOK_SADIE]     = { SPR_SADIE,      SPR_SADIE_1      },
    [LOOK_HOLLIS]    = { SPR_HOLLIS,     SPR_HOLLIS_1     },
};

/* THE MELEE LADDER (see melee_t in assets.h). Fists first, always; each
 * weapon you own outranks it and outranks the ones before it. */
const melee_t melee_info[NUM_MELEE] = {
    [MELEE_FIST]  = { -1,         "YOU THROW A PUNCH! ",       0, -1,            SFX_PUNCH },
    [MELEE_SPADE] = { ITEM_SPADE, "YOU SWING THE SPADE! ",     3, SPR_ITEM_SPADE, SFX_SPADE },
    [MELEE_PIPE]  = { ITEM_PIPE,  "YOU SWING THE PIPE! ",      6, SPR_ITEM_PIPE,  SFX_PIPE },
    [MELEE_PROD]  = { ITEM_PROD,  "YOU JAB THE STUN-PROD! ",  10, SPR_ITEM_PROD,  SFX_PROD },
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
    /* not consumed: using it toggles the beam. See items_update(). */
    [ITEM_FLASHLIGHT] = { "LIGHT", SPR_ITEM_FLASHLIGHT,
        "YOUR OWN FLASHLIGHT, BACK AT LAST. THE BATTERIES HAVE SOME LIFE "
        "IN THEM YET." },
    [ITEM_KEY] = { "KEY", SPR_ITEM_KEY,
        "A HEAVY BRASS KEY. YOU DON'T KNOW WHAT IT OPENS. NOT YET." },
    [ITEM_TNT] = { "TNT", SPR_ITEM_TNT,
        "PA'S TNT, FROM WHEN HE WAS CLEARING STUMPS. THE FUSE IS SHORT.\n"
        "HE ALWAYS SAID DON'T RUN WITH IT. HE ALSO SAID THERE'S NOTHING "
        "OUT THERE." },
    /* The pickup line here is the fallback; in Part 1 the lawyer picks it
     * up in his own words -- see the monologue override in pick_up(). */
    [ITEM_HANDGUN] = { "PISTOL", SPR_ITEM_HANDGUN,
        "A REVOLVER FROM SOMEBODY'S DESK DRAWER. LOADED." },
    [ITEM_BULLETS] = { "BULLETS", SPR_ITEM_BULLETS,
        "A CARTON OF PISTOL ROUNDS. THEY ROLL AROUND IN THE BOX LIKE "
        "LOOSE TEETH." },
    [ITEM_HOLYWATER] = { "H.WATER", SPR_ITEM_HOLYWATER,
        "A STOPPERED VIAL OF HOLY WATER. IT IS COLD IN YOUR POCKET AND IT "
        "DOES NOT WARM UP.\n"
        "THERE IS ALMOST NONE OF THIS LEFT IN THE WORLD. ONE THROW ENDS "
        "ONE OF THOSE THINGS -- ANY OF THEM. CHOOSE THE THROW." },
    [ITEM_ROSARY] = { "ROSARY", SPR_ITEM_ROSARY,
        "MRS. ABERNATHY'S ROSARY. EQUIP IT FROM THE PACK: THE BEADS TURN "
        "ASIDE WHAT HUNTS YOU, AND LEND YOU A STRENGTH THAT ISN'T YOURS." },
    /* ---- PART 2: THE SHIP ---- */
    [ITEM_LASER] = { "LASER", SPR_ITEM_LASER,
        "IT WASN'T BUILT FOR A HAND LIKE YOURS. IT FITS ANYWAY. HOLD THE "
        "TRIGGER AND IT DOESN'T STOP -- AS LONG AS THE CELL HOLDS." },
    [ITEM_BATTERY] = { "BATTERY", SPR_ITEM_BATTERY,
        "A GREEN CELL FOR THE LASER. THEY ARE EVERYWHERE ON THIS SHIP, "
        "WHICH TELLS YOU HOW MUCH THEY USE IT." },
    [ITEM_NUKE] = { "A.NUKE", SPR_ITEM_NUKE,
        "YOU DON'T KNOW WHAT IT IS. YOU KNOW WHAT IT'S FOR. THROW IT AND "
        "GET SMALL -- THE HOLE IT LEAVES IS BIGGER THAN PA'S, AND THE "
        "LIGHT IS GREEN." },
    [ITEM_GEL] = { "GEL", SPR_ITEM_GEL,
        "A TUBE OF COOL WHITE GEL. IT CLOSES A WOUND WHILE YOU WATCH, AND "
        "IT DOESN'T HURT. THAT'S THE PART THAT SCARES YOU." },
    [ITEM_GOO] = { "GOO?", SPR_ITEM_GOO,
        "WEIRD GREEN GOO IN A CAN. THERE IS NO OTHER WAY TO SAY IT. IT IS "
        "WARM THROUGH THE METAL AND IT MOVES WHEN YOU HOLD STILL.\n"
        "DON'T OPEN IT. NOT YET. NOT HERE." },
    /* ---- MELEE WEAPONS (see melee_info[]). Not used from a menu -- just
     * carrying one is what the FIGHT button swings. ---- */
    [ITEM_SPADE] = { "SPADE", SPR_ITEM_SPADE,
        "PA'S SPADE. THE HANDLE IS WORN SMOOTH WHERE HIS HANDS WENT. IT "
        "ISN'T MUCH, BUT IT'S BETTER THAN YOUR BARE KNUCKLES." },
    [ITEM_PIPE] = { "PIPE", SPR_ITEM_PIPE,
        "A LENGTH OF LEAD PIPE, PRIED OFF SOMETHING IN THE DARK. IT HAS A "
        "GOOD, UGLY WEIGHT TO IT." },
    [ITEM_PROD] = { "PROD", SPR_ITEM_PROD,
        "AN ALIEN STUN-PROD. THE TIP STILL SPARKS GREEN. YOU'VE BEEN ON "
        "THE WRONG END OF ONE OF THESE. NOW YOU'RE NOT." },
};
