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
    /* HOLY WATER, THE THIRD OF THREE -- in the house of the man they took
     * out through the roof. He kept it by the bed. It didn't help him;
     * it can still help somebody. Never restocks. */
    { ENT_ITEM, 13, 8, ITEM_HOLYWATER, 0 },
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

/* ============================================================================
 *                                PART 1
 * ============================================================================*/

/* ============================ MAIN STREET ==================================
 * THE CITY. You come out of the church into this. Sirens somewhere, every
 * TV saying the same thing, and everybody on the street doing the math on
 * how far home is.
 *
 * Layout: church and the office tower along the north side, the street
 * through the middle, and a little park on the south -- which is where IT
 * is. The service alley between the buildings (cols 10-11) is the only way
 * into the tower: the lobby locked itself when the power went.
 *
 * THIS IS A CITY, so it is built like one:
 *   S  skyscraper facade (floors of windows -- the tower keeps going up
 *      past the top of the screen)
 *   Z  a shorter building's roof line (the shops)
 *   z  the map border: smaller towers a block away, where the farm maps
 *      have trees
 *   C/W/+/U  the CATHOLIC CHURCH: dressed stone, stained glass, the
 *      gilded cross over the arched doors
 *   G  the tower's glass lobby doors    p  sidewalk    a  asphalt
 *   =  centre line                      L  street lamp
 */
static const char *const MAP_ROWS_CITY[20] = {
    "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
    "zCC+CCCCCCaaSSSSSSSSZZZZZZZZZz",
    "zCWCCCCWCCaaDSSSSSSSSSSSSSSSSz",
    "zCCCCCCCCCaaSSSSSSSSSSSSSSSSSz",
    "zCCUCCCCCCaaSSSGSSSSSSSSDSSSSz",
    "zppppppppppppppppppppppppppppz",
    "zppppppppppppppppppppppppppppz",
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
    "a=aa=aa=aa=aa=aa=aa=aa=aa=aa=a",
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
    "zpLpppppppppLpppppppppLppppppz",
    "zppppppppppppppppppppppppppppz",
    "z....T..*......T.....*.....T.z",
    "z.T......ww.........T....*...z",
    "z....*...ww....T.........T...z",
    "z..T............*...T........z",
    "z.....T...###............T...z",
    "zzzzzzzzzzz#zzzzzzzzzzzzzzzzzz",
};

static const spawn_t MAP_SPAWNS_CITY[] = {
    /* THE PRIEST, out on the steps with his flock bolted in behind him. */
    { ENT_NPC,    5,  5, LOOK_PRIEST,
      "GO HOME, MY SON. BE WITH YOUR FAMILY -- THAT'S THE WHOLE SERMON "
      "NOW. LOCK THE DOORS, AND PRAY LOUD ENOUGH THAT THEY CAN HEAR IT "
      "OVER THE SIRENS." },

    /* DEACON CHARLIE -- almost ordained, and holding the sacristy's last
     * blessed vial. His gift is the game's whole lesson on holy water:
     * there are THREE in existence, it never restocks, and one throw ENDS
     * one of those things. The player who spends it on an ant deserves
     * the rest of their night. */
    { ENT_NPC,    2,  5, LOOK_DEACON,
      "DEACON CHARLIE. WELL -- ALMOST. THEY'D HAVE ORDAINED ME IN JUNE.\n"
      "FATHER BLESSED EVERY DROP IN THE SACRISTY THIS MORNING, AND THIS "
      "IS THE LAST VIAL OF IT. TAKE IT.\n"
      "IT DOESN'T HURT THOSE THINGS, ~. IT ENDS THEM. ONE THROW, ONE "
      "MIRACLE -- SO SPEND IT LIKE THE LAST PRAYER ON EARTH, BECAUSE IT "
      "VERY NEARLY IS.",
      GIFT(ITEM_HOLYWATER),
      "SPEND IT WELL, ~. WHEN IT'S GONE THERE IS NO MORE -- I'D KNOW, "
      "I CARRIED THE CRATE IN MYSELF." },

    /* MRS. ABERNATHY. The librarian, by the pond, and the pond did not do
     * this to her. Her rosary is still in her hand -- taking it comes with
     * a prayer he only half remembers (see the LOOK_DEADLADY monologue
     * special-case in try_talk). */
    { ENT_NPC,    8, 16, LOOK_DEADLADY,
      "MRS. ABERNATHY. THE LIBRARIAN. SHE STAMPED MY BOOKS WHEN I WAS "
      "NINE AND SHE STAMPED MY KIDS' BOOKS LAST TUESDAY.\n"
      "SOMETHING OPENED HER UP AND LEFT HER BY THE POND. HER ROSARY IS "
      "STILL IN HER HAND.\n"
      "...FORGIVE ME, MRS. A. OUR FATHER, WHO ART -- I DON'T REMEMBER "
      "THE REST. I'M A BAD CATHOLIC. BUT I HOPE TO GOD THIS GIVES ME "
      "STRENGTH.\n"
      "THE BEADS ARE WARM. EQUIP THEM FROM THE PACK.",
      GIFT(ITEM_ROSARY),
      "SHE IS PAST HELPING. THE PRAYER WILL HAVE TO DO." },

    /* THE COPS. They block the ends of the street, which makes the alley
     * the only way forward -- that's the puzzle, and they'll even tell
     * you so if you ask. */
    { ENT_NPC,    2,  8, LOOK_COP,
      "ROAD'S CLOSED, COUNSELOR. EVERYTHING WEST OF HERE IS... LOOK, IT'S "
      "CLOSED.\nYOU WANT OFF THIS STREET? THE TOWERS LOCKED THEIR LOBBIES "
      "WHEN THE POWER WENT. TRY THE SERVICE ALLEY BY THE CHURCH -- "
      "MAINTENANCE NEVER LOCKS UP." },
    { ENT_NPC,   27,  9, LOOK_COP,
      "TURN AROUND. THE BRIDGE IS CARS AS FAR AS YOU CAN SEE, AND NOBODY "
      "IS IN THE CARS.\nI'M NOT SAYING IT AGAIN. TURN AROUND." },
    { ENT_NPC,   11, 13, LOOK_COP,
      "STAY OUT OF THE PARK. IT TOOK LENNY AT THE FOUNTAIN, AND LENNY "
      "HAD A SHOTGUN.\nWHATEVER YOU'VE GOT TO DO, IT ISN'T IN THE PARK." },

    /* the cruisers, parked where they stopped mattering */
    { ENT_NPC,    3, 10, LOOK_COPCAR,
      "THE RADIO IS STILL SQUAWKING. UNITS CALLING FOR UNITS THAT DON'T "
      "ANSWER. NOBODY IS SAYING 'OVER' ANY MORE." },
    { ENT_NPC,   21,  7, LOOK_COPCAR,
      "BOTH DOORS HANG OPEN. THE ENGINE IS RUNNING. THE LIGHTS TURN, AND "
      "TURN, AND TURN." },

    /* THE PEOPLE. Every one of them is somebody specific. */
    { ENT_NPC,    8,  6, LOOK_CITYWOMAN,
      "THE TV SAID STAY INSIDE. THEN THE TV SHOWED ONE OF THEM. THEN THE "
      "TV STOPPED.\nI KEEP TURNING THE KNOB LIKE IT'S GOING TO APOLOGIZE." },
    { ENT_NPC,   17,  5, LOOK_SKEPTIC,
      "MY CAR WON'T START. NOBODY'S CAR STARTS.\nI HAVE A DEPOSITION "
      "MONDAY MORNING. THINGS WILL BE NORMAL BY MONDAY. SAY THEY WILL." },
    { ENT_NPC,   24,  6, LOOK_WITNESS,
      "I SAW IT DRAG A DOG UNDER THE EL PLATFORM. A BIG DOG.\nI'M NOT "
      "GOING TO TELL YOU WHAT IT LOOKED LIKE, BECAUSE THEN YOU'D KNOW "
      "TOO." },
    { ENT_NPC,   25,  5, LOOK_STOREKEEP,
      "EVERY SET IN MY WINDOW SAYS THE SAME THING. CREATURES. IN THE "
      "CITY. STAY INDOORS.\nTHEN COLOR BARS, ON EVERY CHANNEL. I SELL "
      "TVS, FRIEND. COLOR BARS ON CHANNEL 5 IS THE END OF THE WORLD." },
    { ENT_NPC,    6, 13, LOOK_CITYWOMAN,
      "MY HUSBAND WENT BACK FOR THE CAR. I TOLD HIM THE CAR DOESN'T "
      "MATTER.\nIF YOU SEE A MAN IN A GREEN COAT -- TELL HIM THE CAR "
      "DOESN'T MATTER." },
    { ENT_NPC,   20, 13, LOOK_MA,
      "THE PIGEONS KNEW FIRST. WHOLE FLOCK GOT UP AN HOUR AGO AND WENT "
      "NORTH, ALL AT ONCE, LIKE A CURTAIN.\nI JUST LIKE THIS BENCH, DEAR. "
      "AT MY AGE YOU DON'T RUN, YOU WATCH." },
    { ENT_NPC,   14, 12, LOOK_DOG,
      "WOOF! WOOF WOOF! A CITY DOG, DRAGGING ITS LEASH. NOBODY IS "
      "HOLDING THE OTHER END." },

    /* IT HIDES. A dogman in the mouth of the alley, waiting for somebody
     * to need the only way through. */
    { ENT_ALIEN, 10,  1, SPECIES_DOGMAN, 0 },

    /* LE CHUPACABRA. The thing in the park, standing square on the path
     * south -- the path that goes HOME. A boss: it waits, it doesn't
     * wander, and once it's down it stays down. */
    { ENT_ALIEN, 11, 18, SPECIES_CHUPA_BOSS, 0 },

    /* what the street has to offer */
    { ENT_ITEM,  13, 16, ITEM_HERB,    0 },  /* growing wild by the pond  */
    { ENT_ITEM,  26, 12, ITEM_MEDKIT,  0 },  /* dropped outside the shop  */
    { ENT_ITEM,  11,  1, ITEM_BULLETS, 0 },  /* deep in the alley. earn it */
};

static const warp_t MAP_WARPS_CITY[] = {
    /* the service door in the alley -- the way into the dark tower */
    { 12,  2, MAP_OFFICE, 2, 6 },

    /* every door that ISN'T a way forward says exactly why (FLAG_NEVER:
     * a flag nobody sets, so the door never opens and the line always
     * plays -- see check_door_bump) */
    {  3,  4, MAP_CITY, 0, 0, 0, FLAG_NEVER,
      "YOU CAN HEAR THEM PRAYING THROUGH THE DOOR. THEY BOLTED IT "
      "BEHIND YOU." },
    { 15,  4, MAP_CITY, 0, 0, 0, FLAG_NEVER,
      "THE LOBBY DOORS ARE BOLTED. THROUGH THE GLASS: PITCH DARK, AND "
      "THE FERNS ARE KNOCKED OVER. ALL OF THEM." },
    { 24,  4, MAP_CITY, 0, 0, 0, FLAG_NEVER,
      "SHUTTERED. THE SIGN SAYS BACK IN FIVE MINUTES. THE SIGN HAS BEEN "
      "SAYING THAT SINCE THIS MORNING." },

    /* THE PATH HOME. South, past the park, past the thing standing in it.
     * This is where Part 1 stops -- for now. */
    { 11, 19, MAP_CITY, 0, 0, 0, FLAG_NEVER,
      "SOUTH. HOME. MILES OF STREETS BETWEEN HERE AND THE HOUSE, AND "
      "EVERY ONE OF THEM SOUNDS LIKE THIS ONE.\nYOU'LL GET THERE. NOT "
      "TONIGHT. (PART 1 ENDS HERE -- FOR NOW.)" },
};

/* ============================ THE DARK OFFICE ==============================
 * Fourteen floors of tower and the only one that matters is this one. The
 * power is out (`dark` in the map table), so the room is DARK_BRIGHT black
 * until the flashlight is in your hand -- which is the first thing lying
 * near the door, because the game is dark, not cruel.
 */
static const char *const MAP_ROWS_OFFICE[12] = {
    "HHHHHHHHHHHHHHHHHH",
    "Hx______t______xxH",
    "H_tt____t__tt___xH",
    "H_tt_______tt____H",
    "H________________H",
    "Hx__t____x___t__xH",
    "HM_______________H",
    "H___tt_____tt____H",
    "H___tt_____tt___xH",
    "Hx_______x_______H",
    "H__t__________ttxH",
    "HHHHHHHHHHHHHHHHHH",
};

static const spawn_t MAP_SPAWNS_OFFICE[] = {
    /* THE NIGHT GUARD, under his desk, going nowhere until sunrise. */
    { ENT_NPC,   2,  4, LOOK_WITNESS,
      "DON'T SHINE THAT AT ME -- OH. OH, YOU'RE A PERSON.\nSOMETHING'S "
      "BEEN WALKING THE FLOOR ABOVE US FOR AN HOUR. WALKING, AND THEN "
      "STOPPING. THE STOPPING IS WORSE.\nMR. KOWALSKI IN THE BACK OFFICE "
      "KEPT A PISTOL IN HIS DESK. BACK WALL. TAKE IT, HE'D WANT YOU TO. "
      "I'M NOT MOVING TILL SUNRISE." },

    { ENT_ITEM,  4,  6, ITEM_FLASHLIGHT, 0 },  /* the maintenance torch,
                                                   right inside the door  */
    { ENT_ITEM, 13, 10, ITEM_HANDGUN,    0 },  /* Kowalski's desk drawer  */
    { ENT_ITEM, 14,  1, ITEM_BULLETS,    0 },  /* ...and the carton under
                                                   it. Pistol BULLETS --
                                                   shells are for shotguns */
    { ENT_ITEM,  8,  9, ITEM_MEDKIT,     0 },  /* the break-room kit      */
    /* HOLY WATER, THE SECOND OF THREE. Somebody on this floor was more
     * scared than they let on. It never restocks. */
    { ENT_ITEM, 10, 10, ITEM_HOLYWATER,  0 },

    /* IT GOT IN. The second dogman is already inside, between you and
     * the pistol, and you will hear it before your little light finds it. */
    { ENT_ALIEN, 10,  4, SPECIES_DOGMAN, 0 },
};

static const warp_t MAP_WARPS_OFFICE[] = {
    { 1, 6, MAP_CITY, 11, 2 },   /* the mat by the service door -> alley */
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
    /* ---- PART 1 ---- */
    [MAP_CITY]  = { "MAIN STREET", MAP_ROWS_CITY, 30, 20,
                    MAP_SPAWNS_CITY,  N(MAP_SPAWNS_CITY),
                    MAP_WARPS_CITY,   N(MAP_WARPS_CITY),   1 },
    /* the tower: indoors, and DARK -- the last field is what kills the
     * lights (see `dark` in map_t, and DARK_BRIGHT in config.h) */
    [MAP_OFFICE] = { "HALLORAN AND WEEKS", MAP_ROWS_OFFICE, 18, 12,
                    MAP_SPAWNS_OFFICE, N(MAP_SPAWNS_OFFICE),
                    MAP_WARPS_OFFICE,  N(MAP_WARPS_OFFICE), 0, 1 },
};

#undef N
#endif /* MAPS_H */
