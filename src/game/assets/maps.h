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
 *   { ENT_ALIEN, x, y, SPECIES_ANT, 0 }              an enemy
 *   { ENT_NPC,   x, y, LOOK_ELDER, "WHAT THEY SAY" }  a person
 *   { ENT_ITEM,  x, y, ITEM_HERB, 0 }                 a pickup
 *   { ENT_NPC,   x, y, LOOK_COW, "MOO." }              a cow (animals are
 *                                                      just NPCs that moo)
 *
 * Write a '~' anywhere in a line and it becomes the player's NAME, so the
 * folks who know you can use it.
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
    { ENT_NPC,    8,  3, LOOK_PA,
      "PA: THE CROPS WERE FLAT IN CIRCLES AGAIN THIS MORNING, ~. "
      "AND THE COWS WON'T GO NEAR THE POND. YOU BE CAREFUL OUT THERE." },
    /* The farm is HOME. It is where you feed the dog and get mooed at, and
     * it should feel like that most of the time -- so there is exactly one
     * ant wandering it, and exactly one hill making more. If you want the
     * place to feel besieged again, add a second hill here rather than more
     * loose ants: the hill is the thing the player can actually solve. */
    { ENT_ALIEN, 17,  8, SPECIES_ANT, 0 },   /* open ground by the pond   */

    /* THE HILL. Shoved right into the far south-east corner, hard against
     * the queen -- and deliberately NOT next to the livestock. That corner
     * is the nest, and it's where the farm turns dangerous; the pasture and
     * the yard stay somewhere you can stand and talk to a cow.
     *
     * It does not move. It just keeps making more, and every ant on this
     * farm comes out of it -- the only way to stop that is to put the mound
     * itself down. */
    { ENT_ALIEN, 26, 16, SPECIES_ANTHILL, 0 },

    /* HER. Down in the far corner, past everything, where the grass is dead.
     * She does not come at you until you make her -- and then she is faster
     * than you are. */
    { ENT_ALIEN, 24, 18, SPECIES_QUEEN, 0 },
    { ENT_ITEM,  12, 16, ITEM_HERB,    0 },   /* growing in the south field */
    { ENT_ITEM,  26,  2, ITEM_SHELLS,  0 },   /* dropped behind the house   */

    /* The neighbour, down by the south road. He hands your flashlight
     * back the first time you talk to him -- and points you at the ants. */
    { ENT_NPC,    3, 13, LOOK_NEIGHBOR,
      "OH -- AND HERE. YOUR FLASHLIGHT. BEEN MEANING TO BRING IT ROUND. "
      "SAY... THE ANTS. THEY'RE HUGE THIS YEAR. BIG AS DOGS, SOME OF THEM. "
      "SOMETHING GOT INTO THEM SINCE THE LIGHTS CAME. SOMEBODY OUGHT TO "
      "PUT THEM DOWN BEFORE THEY GET IN THE HOUSES.",
      GIFT(ITEM_FLASHLIGHT),
      "THEM ANTS WON'T CLEAR THEMSELVES, ~. AND DON'T GO OUT THERE DARK -- "
      "THAT'S WHAT THE LIGHT'S FOR." },

    /* ---- THE LIVESTOCK. They are just NPCs that moo. ---- */
    { ENT_NPC,   10, 14, LOOK_COW,
      "MOO. ...MOOOOO. SHE CHEWS. SHE STARES RIGHT THROUGH YOU. "
      "SHE HAS SEEN THINGS AND SHE IS NOT GOING TO TALK ABOUT THEM." },
    { ENT_NPC,   13, 15, LOOK_COW,
      "MOO! THIS ONE WON'T FACE NORTH. NOT SINCE TUESDAY." },
    { ENT_NPC,   16, 17, LOOK_GOAT,
      "MAAAAAA! THE GOAT HEADBUTTS YOUR KNEE. LOVINGLY, YOU THINK. "
      "IT HAS ALSO EATEN PART OF THE FENCE." },
    { ENT_NPC,    7,  5, LOOK_DOG,
      "WOOF! WOOF WOOF WOOF! HE HAS BEEN BARKING AT THE SKY ALL WEEK "
      "AND HE IS NOT SORRY." },
    { ENT_NPC,    8,  2, LOOK_CAT,
      "MRRRP? THE CAT BLINKS SLOWLY AT YOU. THE CAT KNOWS EXACTLY WHAT "
      "IS HAPPENING. THE CAT DOES NOT CARE." },
};

static const warp_t MAP_WARPS_FARM[] = {
    { 29, 12, MAP_TOWN, 1, 12 },   /* east road -> town main street  */
    {  5,  4, MAP_HOME, 7,  8 },   /* the farmhouse door -> inside   */
};

/* ============================ THE TOWN ====================================
 * Three houses on the north side of Main Street; only the middle one
 * answers. The general store is south. Not everyone thinks you're crazy.
 */
/* The road out of the north end of town runs up column 10. Something is
 * standing in it. */
static const char *const MAP_ROWS_TOWN[20] = {
    "TTTTTTTTTT#TTTTTTTTTTTTTTTTTTT",
    "T.........#..................T",
    "T..RRRRR..#..RRRRR.....RRRRR.T",
    "T..RRRRR..#..RRRRR.....RRRRR.T",
    "T..HHHHH..#..HHHHH.....HHHHH.T",
    "T..HHDHH..#..HHDHH.....HHDHH.T",
    "T....#....#....#.........#...T",
    "T....#....#....#.........#...T",
    "T....#....#....#.........#...T",
    "T....#....#....#.........#...T",
    "T....#....#....#.........#...T",
    "T....#....#....#.........#...T",
    "##############################",   /* <- the road EAST, once it's open */
    "T....*...............*.......T",
    "T............................T",
    "T...RRRRRRR.........,........T",
    "T...RRRRRRR..................T",
    "T...HHHDHHH..........T.......T",
    "T......#.....................T",
    "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",
};

static const spawn_t MAP_SPAWNS_TOWN[] = {
    { ENT_NPC,   10, 13, LOOK_WITNESS,
      "YOU SAW IT TOO, DIDN'T YOU? THE LIGHTS OVER THE RIDGE. "
      "YOU'VE GOT THAT LOOK. NOBODY BELIEVES US." },
    { ENT_NPC,   20, 14, LOOK_SKEPTIC,
      "SHERIFF SAYS IT WAS SWAMP GAS. SWAMP GAS DON'T HUM, FRIEND." },
    { ENT_ALIEN, 26, 17, SPECIES_SOLDIER, 0 },   /* something worse in town */
    { ENT_ITEM,  17, 13, ITEM_HERB,    0 },   /* by the flowerbeds       */
    { ENT_NPC,   18, 13, LOOK_DOG,
      "WOOF! A TOWN DOG. HE IS DELIGHTED TO SEE YOU AND HAS NO NEWS." },

    /* IT STANDS IN THE ROAD. You cannot leave north until it is dealt
     * with -- walk into it and the fight starts. It never wanders, and
     * once it's down it stays down (see the `boss` flag in assets.c). */
    { ENT_ALIEN, 10,  1, SPECIES_GOBLIN, 0 },
};

static const warp_t MAP_WARPS_TOWN[] = {
    {  0, 12, MAP_FARM,  28, 12 },  /* west end of Main St -> the farm */
    { 15,  5, MAP_HOUSE,  7,  8 },  /* middle house door                 */
    {  5,  5, MAP_WRECK,  7,  8, 1 },  /* WEST HOUSE -- needs the brass key */
    /* THE ROAD EAST. Shut until the thing in the north road is dealt with. */
    { 29, 12, MAP_VANLOT,  1, 11, 0, FLAG_GOBLIN_DEAD,
      "THE ROAD EAST IS STILL BLOCKED. SOMETHING CAME DOWN ACROSS IT IN "
      "THE NIGHT, AND NOBODY WILL GO NEAR IT WHILE THAT THING IS STANDING "
      "IN THE NORTH ROAD." },
    {  7, 17, MAP_STORE,  7,  8 },  /* the general store */
    { 10,  0, MAP_RIDGE,  7,  8 },  /* north road -- past the goblin   */
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
    { ENT_NPC, 4, 3, LOOK_MA,
      "MA: YOUR FATHER WON'T SAY IT, ~, BUT HE'S SCARED TOO. "
      "GET SOME SLEEP. THE BED'S RIGHT THERE, AND YOU LOOK AWFUL." },
    { ENT_ITEM, 12, 3, ITEM_SHOTGUN, 0 },   /* propped by the table        */
    { ENT_ITEM,  3, 5, ITEM_KEY,     0 },   /* on the dresser. for what?   */
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
    { ENT_NPC, 11, 4, LOOK_NEIGHBOR,
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
    { ENT_NPC, 7, 1, LOOK_STOREKEEP,
      "STOREKEEP: FOLKS ARE BUYING LANTERNS AND PADLOCKS, ~. "
      "I STOPPED ASKING QUESTIONS A WEEK AGO. HELP YOURSELF, ON THE HOUSE." },
    { ENT_NPC, 11, 7, LOOK_CAT,
      "MEOW. THE STORE CAT OWNS THIS STORE. THE STOREKEEP JUST WORKS HERE." },
    { ENT_ITEM,  3, 4, ITEM_SHELLS, 0 },   /* on the house. times are bad */
    { ENT_ITEM, 11, 4, ITEM_MEDKIT, 0 },
};

static const warp_t MAP_WARPS_STORE[] = {
    { 7, 9, MAP_TOWN, 7, 18 },
};

/* ============================ THE NORTH RIDGE ==============================
 * What the goblin was standing in front of.
 *
 * *** THE BESTIARY / TEST ARENA ***
 * Every creature in the game is here, one of each, spaced out so you can
 * walk up and look at them -- or shoot them, or let them come at you.
 * A crate of shells and a medkit sit by the gate so you can actually
 * fight. This is a testbed: thin it out (or wall it off) when the real
 * content for the ridge arrives.
 */
static const char *const MAP_ROWS_RIDGE[20] = {
    "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",
    "T............................T",
    "T..,......,.......,......,...T",
    "T............................T",
    "T....*..........*...........,T",
    "T............................T",
    "T.,.......,.........,........T",
    "T............................T",
    "T............................T",
    "T......,.........,...........T",
    "T............................T",
    "T...*..........,.........*...T",
    "T............................T",
    "T........,.........,.........T",
    "T............................T",
    "T....,...........,..........,T",
    "T............................T",
    "T..........,.................T",
    "T.............##.............T",
    "TTTTTTTTTTTTTT##TTTTTTTTTTTTTT",
};

static const spawn_t MAP_SPAWNS_RIDGE[] = {
    /* supplies, right by the gate -- you'll want them */
    { ENT_ITEM,  12, 17, ITEM_SHELLS, 0 },
    { ENT_ITEM,  17, 17, ITEM_MEDKIT, 0 },

    /* THE BESTIARY. One of everything. */
    { ENT_ALIEN,  3,  3, SPECIES_ANT,        0 },
    { ENT_ALIEN,  8,  3, SPECIES_SOLDIER,    0 },
    { ENT_ALIEN, 14,  3, SPECIES_DOVER,      0 },
    { ENT_ALIEN, 20,  3, SPECIES_GREY,       0 },
    { ENT_ALIEN, 25,  3, SPECIES_CHUPACABRA, 0 },
    { ENT_ALIEN,  3, 10, SPECIES_REPTOID,    0 },
    { ENT_ALIEN,  9, 10, SPECIES_DOGMAN,     0 },
    { ENT_ALIEN, 15, 10, SPECIES_MOTHMAN,    0 },
    { ENT_ALIEN, 21, 10, SPECIES_SASQUATCH,  0 },
    { ENT_ALIEN, 26, 10, SPECIES_NESSIE,     0 },
    { ENT_ALIEN, 15, 14, SPECIES_GOBLIN,     0 },
    /* the two new ones, for inspection. The hill will start filling the
     * arena with ants the moment you walk in, so don't dawdle. */
    { ENT_ALIEN,  5, 14, SPECIES_ANTHILL,    0 },
    { ENT_ALIEN, 25, 14, SPECIES_QUEEN,      0 },
    { ENT_ALIEN, 20, 14, SPECIES_TALL,       0 },
};

static const warp_t MAP_WARPS_RIDGE[] = {
    { 14, 19, MAP_TOWN, 10, 1 },   /* back down the road into town */
    { 15, 19, MAP_TOWN, 10, 1 },
};


/* ============================ THE WRECKED HOUSE ============================
 * The west house on Main Street. The brass key opens it. Nobody has been
 * inside in a while, and it shows: the floor is holed, the furniture is
 * where it landed, and the man who owns the place is still sitting in it.
 *
 * >>> THE LANDOWNER'S NAME IS A PLACEHOLDER -- you left it blank. Change
 * >>> "ELIAS HOLLIS" below to whatever you actually want him called.
 */
static const char *const MAP_ROWS_WRECK[10] = {
    "HHHHHHHHHHHHHHH",
    "HB_x____xtt__xH",
    "H__x___x__t__xH",
    "H___t____x___xH",
    "Hx__x_x____x__H",
    "H__t____x__t__H",
    "Hx____x____x__H",
    "H__x__t__x____H",
    "H_____________H",
    "HHHHHHHMHHHHHHH",
};

static const spawn_t MAP_SPAWNS_WRECK[] = {
    /* He is sitting in the wreckage of his own front room. He has been
     * sitting there for some time. */
    { ENT_NPC, 5, 4, LOOK_PA,   /* NOTE: shares Pa's sprite for now */
      "...OH. YOU FOUND THE KEY. I WONDERED WHO'D COME.\n"
      "ELIAS HOLLIS. I OWN THIS. OWNED IT. THEY TOOK ME OUT THROUGH THAT "
      "ROOF, ~, AND THEY PUT ME BACK IN THE WRONG ORDER.",
      0,
      "THE CEILING IS STILL OPEN. I DON'T LOOK UP ANY MORE. "
      "YOU SHOULDN'T EITHER." },

    { ENT_ITEM, 11, 8, ITEM_MEDKIT, 0 },
    { ENT_ITEM,  2, 8, ITEM_SHELLS, 0 },
};

static const warp_t MAP_WARPS_WRECK[] = {
    { 7, 9, MAP_TOWN, 5, 6 },   /* the doormat -> back out onto Main St */
};


/* ============================ THE PARKING LOT ==============================
 * East of town, once the goblin is off the north road.
 *
 * A grocery store and a lot of empty asphalt. The store lights are on and
 * nobody is shopping. Four sodium lamps, and they all buzz.
 *
 * Your van is in a space. So is the owner, and he is not going inside.
 *
 *   a  asphalt      -  a painted line      L  a street lamp (lit at night)
 */
static const char *const MAP_ROWS_VANLOT[14] = {
    "TTTTTTTTTTTTTTTTTTTTTTTT",
    "T......RRRRRRRRR.......T",
    "T......RRRRRRRRR.......T",
    "T......HHHHDHHHH.......T",
    "T.aaaaaaaaaaaaaaaaaaaa.T",
    "TLaaaaaaaaaaaaaaaaaaaaLT",
    "T.aaaa-aaaa-aaaa-aaaaa.T",
    "T.aaaa-aaaa-aaaa-aaaaa.T",
    "T.aaaa-aaaa-aaaa-aaaaa.T",
    "T.aaaaaaaaaaaaaaaaaaaa.T",
    "TLaaaaaaaaaaaaaaaaaaaaLT",
    "#.aaaaaaaaaaaaaaaaaaaa.T",
    "T......................T",
    "TTTTTTTTTTTTTTTTTTTTTTTT",
};

static const spawn_t MAP_SPAWNS_VANLOT[] = {
    /* THE OWNER. He is standing in his own car park at night with the
     * lights on behind him, and he will not go back inside. */
    { ENT_NPC, 11, 4, LOOK_STOREKEEP,
      "PLEASE. PLEASE -- YOU'VE GOT A VAN.\n"
      "MY GIRL WENT OUT AT NINE TO BRING THE CARTS IN. THAT'S ALL. THE "
      "CARTS ARE STILL OUT THERE, ~. HER SHOES ARE STILL OUT THERE, SET "
      "SIDE BY SIDE LIKE SHE MEANT TO COME BACK FOR THEM.\n"
      "THE PHONE JUST CLICKS. IT'S BEEN CLICKING ALL NIGHT. NOBODY PICKS "
      "UP IN THE WHOLE COUNTY.\n"
      "DRIVE TO THE CITY. GET THE POLICE. TELL THEM SOMETHING TOOK MY "
      "DAUGHTER AND TELL THEM TO HURRY.",
      0,
      "GO. PLEASE GO. I'LL WAIT BY THE DOOR IN CASE SHE COMES BACK.\n"
      "...I'M NOT GOING INSIDE. I CAN'T HEAR THE LOT FROM INSIDE." },

    /* Not a person. Get in it and the prologue is over. */
    { ENT_NPC, 17, 7, LOOK_VAN,
      "YOUR VAN. IT'S BEEN SITTING HERE SINCE THE LIGHTS CAME.\n"
      "THE STATION IS FORTY MINUTES DOWN THE HIGHWAY. SOMEBODY HAS TO SAY "
      "THIS OUT LOUD TO SOMEBODY, ~. AND HE'S RIGHT -- SOMEBODY HAS TO "
      "GO GET HER." },

    { ENT_ITEM,  3, 8, ITEM_SHELLS, 0 },
    /* NOT at (20,8) any more -- the van is four tiles wide now and that spot
     * is underneath it. An item you cannot reach is an item that isn't there. */
    { ENT_ITEM,  6, 10, ITEM_MEDKIT, 0 },
};

static const warp_t MAP_WARPS_VANLOT[] = {
    { 0, 11, MAP_TOWN, 28, 12 },   /* back west into town */
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
    [MAP_RIDGE] = { "THE NORTH RIDGE", MAP_ROWS_RIDGE, 30, 20,
                    MAP_SPAWNS_RIDGE, N(MAP_SPAWNS_RIDGE),
                    MAP_WARPS_RIDGE,  N(MAP_WARPS_RIDGE), 1 },
    [MAP_WRECK] = { "THE HOLLIS PLACE", MAP_ROWS_WRECK, 15, 10,
                    MAP_SPAWNS_WRECK, N(MAP_SPAWNS_WRECK),
                    MAP_WARPS_WRECK,  N(MAP_WARPS_WRECK), 0 },
    [MAP_VANLOT] = { "THE PARKING LOT", MAP_ROWS_VANLOT, 24, 14,
                    MAP_SPAWNS_VANLOT, N(MAP_SPAWNS_VANLOT),
                    MAP_WARPS_VANLOT,  N(MAP_WARPS_VANLOT), 1 },
};

#undef N
#endif /* MAPS_H */
