/* ============================================================================
 * maps.h -- the world, drawn one character per tile.
 *
 *  OUTDOORS                          INDOORS
 *   .  grass        H  house wall     _  floorboards
 *   ,  grass (alt)  R  roof           M  doormat (exit -- put a warp on it)
 *   T  tree         D  door           B  bed
 *   #  path         c  crops          t  table
 *   w  water        *  flowers        x  darkness
 *   f  fence
 *
 * (The character -> tile mapping lives in char_to_tile() in assets.c.)
 *
 * Every row of a map must be exactly `w` characters -- assets_init()
 * checks and complains if not. The world edge behaves as solid trees,
 * so you can't walk off a map except through a WARP.
 *
 * ---------------------------------------------------------------------------
 * TO ADD A MAP:
 *   1. Draw the rows here + define its spawns[] and warps[]
 *   2. Add an id to the map enum in assets.h
 *   3. Add one line to the maps[] table at the bottom of this file
 *   4. Connect it: put a warp on an existing map that leads to it
 *
 * TO MAKE A BUILDING ENTERABLE (like the farmhouse below):
 *   1. Draw the interior as its own map (interiors must be at least
 *      15x10 tiles -- one full screen -- or the camera shows the void)
 *   2. Put a doormat 'M' in the gap in its bottom wall
 *   3. Add TWO warps:
 *      outside map:  { doorX, doorY,  MAP_INSIDE, matX, matY - 1 }
 *      inside map:   { matX,  matY,   MAP_OUTSIDE, doorX, doorY + 1 }
 *   A door with no warp is simply locked ("nobody's answering").
 *
 * SPAWNS place living things (see "THE CAST" in assets.h):
 *   { ENT_ALIEN, x, y, SPECIES_GREY, 0 }              an enemy
 *   { ENT_NPC,   x, y, LOOK_ELDER, "WHAT THEY SAY" }  a person
 *   { ENT_ITEM,  x, y, ITEM_HERB, 0 }                 a pickup
 * Coordinates are in tiles. Enemies wander and attack; NPCs stand,
 * breathe, block your path, and talk when you face them and press A.
 * Items get pocketed when you walk into them, and stay taken for the
 * rest of the run.
 * ==========================================================================*/
#ifndef MAPS_H
#define MAPS_H

/* ============================ THE FARM ====================================
 * Home. Farmhouse top-left (you can go inside), fenced crop field, a
 * pond the cows now refuse to go near. The road east leads to town.
 */
static const char *const MAP_ROWS_FARM[20] = {
    "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",
    "T..RRRRR.....................T",
    "T..RRRRR..*.........,........T",
    "T..HHHHH........,............T",
    "T..HHDHH....,................T",
    "T....#.......................T",
    "T....#...ffffff...........,..T",
    "T....#...fccccf..........ww..T",
    "T....#...fccccf.........wwww.T",
    "T....#...fccccf.........wwww.T",
    "T....#...ffffff..........ww..T",
    "T....#........,..............T",
    "T....#########################",
    "T......*...........,.........T",
    "T...T....................T...T",
    "T............,...............T",
    "T.....,..............*.......T",
    "T..T......................T..T",
    "T............................T",
    "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",
};

static const spawn_t MAP_SPAWNS_FARM[] = {
    { ENT_NPC,    8,  3, LOOK_ELDER,
      "PA: THE CROPS WERE FLAT IN CIRCLES AGAIN THIS MORNING... "
      "AND THE COWS WON'T GO NEAR THE POND. YOU BE CAREFUL OUT THERE." },
    { ENT_ALIEN, 20, 15, SPECIES_GREY, 0 },
    { ENT_ALIEN,  7, 16, SPECIES_GREY, 0 },
    { ENT_ALIEN, 14,  8, SPECIES_GREY, 0 },
    { ENT_ITEM,  12, 16, ITEM_HERB,    0 },   /* growing in the south field */
    { ENT_ITEM,  26,  2, ITEM_SHELLS,  0 },   /* dropped behind the house   */
};

static const warp_t MAP_WARPS_FARM[] = {
    { 29, 12, MAP_TOWN, 1, 12 },   /* east road -> town main street  */
    {  5,  4, MAP_HOME, 7,  8 },   /* the farmhouse door -> inside   */
};

/* ============================ THE TOWN ====================================
 * Three houses on the north side of Main Street; only the middle one
 * answers. The general store is south. Not everyone thinks you're crazy.
 */
static const char *const MAP_ROWS_TOWN[20] = {
    "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",
    "T............................T",
    "T..RRRRR.....RRRRR.....RRRRR.T",
    "T..RRRRR.....RRRRR.....RRRRR.T",
    "T..HHHHH.....HHHHH.....HHHHH.T",
    "T..HHDHH.....HHDHH.....HHDHH.T",
    "T....#.........#.........#...T",
    "T....#.........#.........#...T",
    "T....#.........#.........#...T",
    "T....#.........#.........#...T",
    "T....#.........#.........#...T",
    "T....#.........#.........#...T",
    "#############################T",
    "T....*...............*.......T",
    "T............................T",
    "T...RRRRRRR.........,........T",
    "T...RRRRRRR..................T",
    "T...HHHDHHH..........T.......T",
    "T......#.....................T",
    "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",
};

static const spawn_t MAP_SPAWNS_TOWN[] = {
    { ENT_NPC,   10, 13, LOOK_VILLAGER,
      "YOU SAW IT TOO, DIDN'T YOU? THE LIGHTS OVER THE RIDGE. "
      "YOU'VE GOT THAT LOOK. NOBODY BELIEVES US." },
    { ENT_NPC,   20, 14, LOOK_VILLAGER,
      "SHERIFF SAYS IT WAS SWAMP GAS. SWAMP GAS DON'T HUM, FRIEND." },
    { ENT_ALIEN, 26, 17, SPECIES_TALL, 0 },   /* something worse in town */
    { ENT_ITEM,  17, 13, ITEM_HERB,    0 },   /* by the flowerbeds       */
};

static const warp_t MAP_WARPS_TOWN[] = {
    {  0, 12, MAP_FARM,  28, 12 },  /* west end of Main St -> the farm */
    { 15,  5, MAP_HOUSE,  7,  8 },  /* middle house door (others locked) */
    {  7, 17, MAP_STORE,  7,  8 },  /* the general store */
};

/* ============================ THE FARMHOUSE ================================
 * Interiors are just maps. This one is exactly one screen (15x10).
 */
static const char *const MAP_ROWS_HOME[10] = {
    "HHHHHHHHHHHHHHH",
    "HBB________tt_H",
    "HBB________tt_H",
    "H_____________H",
    "H_____________H",
    "H_____________H",
    "H_____________H",
    "H_____________H",
    "H_____________H",
    "HHHHHHHMHHHHHHH",
};

static const spawn_t MAP_SPAWNS_HOME[] = {
    { ENT_NPC, 4, 3, LOOK_VILLAGER,
      "MA: YOUR FATHER WON'T SAY IT, BUT HE'S SCARED TOO. "
      "EAT SOMETHING BEFORE YOU GO BACK OUT THERE." },
    { ENT_ITEM, 12, 3, ITEM_SHOTGUN, 0 },   /* propped by the table        */
};

static const warp_t MAP_WARPS_HOME[] = {
    { 7, 9, MAP_FARM, 5, 5 },   /* the doormat -> outside the front door */
};

/* ============================ A TOWN HOUSE =================================*/
static const char *const MAP_ROWS_HOUSE[10] = {
    "HHHHHHHHHHHHHHH",
    "H_tt________BBH",
    "H_tt________BBH",
    "H_____________H",
    "H_____________H",
    "H_____________H",
    "H_____________H",
    "H_____________H",
    "H_____________H",
    "HHHHHHHMHHHHHHH",
};

static const spawn_t MAP_SPAWNS_HOUSE[] = {
    { ENT_NPC, 11, 4, LOOK_VILLAGER,
      "I DON'T OPEN THE DOOR AFTER DARK. NOT SINCE THE HUM. "
      "YOU SHOULDN'T BE OUT WALKING, NEIGHBOR." },
    { ENT_ITEM, 3, 6, ITEM_MEDKIT, 0 },   /* they keep it by the door */
};

static const warp_t MAP_WARPS_HOUSE[] = {
    { 7, 9, MAP_TOWN, 15, 6 },
};

/* ============================ THE GENERAL STORE ============================*/
static const char *const MAP_ROWS_STORE[10] = {
    "HHHHHHHHHHHHHHH",
    "H_____________H",
    "H__ttttttttt__H",
    "H_____________H",
    "H_____________H",
    "H_____________H",
    "Htt_________ttH",
    "Htt_________ttH",
    "H_____________H",
    "HHHHHHHMHHHHHHH",
};

static const spawn_t MAP_SPAWNS_STORE[] = {
    { ENT_NPC, 7, 1, LOOK_ELDER,
      "STOREKEEP: FOLKS ARE BUYING LANTERNS AND PADLOCKS. "
      "I STOPPED ASKING QUESTIONS A WEEK AGO. CASH ONLY." },
    { ENT_ITEM,  3, 4, ITEM_SHELLS, 0 },   /* on the house. times are bad */
    { ENT_ITEM, 11, 4, ITEM_MEDKIT, 0 },
};

static const warp_t MAP_WARPS_STORE[] = {
    { 7, 9, MAP_TOWN, 7, 18 },
};

/* ============================ THE MAP TABLE ===============================*/
#define N(a) (int)(sizeof(a) / sizeof((a)[0]))

const map_t maps[NUM_MAPS] = {                       /* outdoor? (night) */
    [MAP_FARM]  = { "THE FARM",  MAP_ROWS_FARM, 30, 20,
                    MAP_SPAWNS_FARM,  N(MAP_SPAWNS_FARM),
                    MAP_WARPS_FARM,   N(MAP_WARPS_FARM),  1 },
    [MAP_TOWN]  = { "TOWN",      MAP_ROWS_TOWN, 30, 20,
                    MAP_SPAWNS_TOWN,  N(MAP_SPAWNS_TOWN),
                    MAP_WARPS_TOWN,   N(MAP_WARPS_TOWN),  1 },
    [MAP_HOME]  = { "HOME",      MAP_ROWS_HOME, 15, 10,
                    MAP_SPAWNS_HOME,  N(MAP_SPAWNS_HOME),
                    MAP_WARPS_HOME,   N(MAP_WARPS_HOME),  0 },
    [MAP_HOUSE] = { "SOMEONE'S HOUSE", MAP_ROWS_HOUSE, 15, 10,
                    MAP_SPAWNS_HOUSE, N(MAP_SPAWNS_HOUSE),
                    MAP_WARPS_HOUSE,  N(MAP_WARPS_HOUSE), 0 },
    [MAP_STORE] = { "GENERAL STORE",   MAP_ROWS_STORE, 15, 10,
                    MAP_SPAWNS_STORE, N(MAP_SPAWNS_STORE),
                    MAP_WARPS_STORE,  N(MAP_WARPS_STORE), 0 },
};

#undef N
#endif /* MAPS_H */
