/* ============================================================================
 * tiles.h -- the 16x16 tiles the maps are built from. Same ASCII format
 * and palette as sprites.h ('.' is NOT transparent here -- tiles are
 * opaque backgrounds, so use real colors everywhere).
 *
 * TO ADD A TILE:
 *   1. Draw it here
 *   2. Add an id to the tile enum in assets.h
 *   3. Register it in tile_art[] in assets.c
 *   4. Give it a map character in char_to_tile() in assets.c
 *   5. Set whether it blocks walking in tile_solid[] in assets.c
 * ==========================================================================*/
#ifndef TILES_H
#define TILES_H

static const char *TILE_ART_GRASS[16] = {
    "gggggggggggggggg",
    "gggggggggGgggggg",
    "gggggggggggggggg",
    "ggGggggggggggggg",
    "gggggggggggggggg",
    "gggggggggggGgggg",
    "gggggggggggggggg",
    "gggggggggggggggg",
    "gggGgggggggggggg",
    "gggggggggggggggg",
    "ggggggggggggggGg",
    "gggggggggggggggg",
    "gGgggggggggggggg",
    "ggggggggGggggggg",
    "gggggggggggggggg",
    "gggggggggggggggg",
};

/* a mossier variant so big lawns don't look like wallpaper */
static const char *TILE_ART_GRASS2[16] = {
    "gggggggggggggggg",
    "ggGGgggggggggggg",
    "ggggggggggGGgggg",
    "gggggggggggggggg",
    "ggggggGggggggggg",
    "gggggggggggggggg",
    "gggggggggggggGgg",
    "ggGggggggggggggg",
    "gggggggggggggggg",
    "gggggggGGggggggg",
    "gggggggggggggggg",
    "ggggGggggggggggg",
    "gggggggggggggggg",
    "ggggggggggggGggg",
    "ggGggggggggggggg",
    "gggggggggggggggg",
};

static const char *TILE_ART_PATH[16] = {
    "dddddddddddddddd",
    "ddddddDddddddddd",
    "dddddddddddddddd",
    "ddDddddddddddddd",
    "dddddddddddDdddd",
    "dddddddddddddddd",
    "dddddddddddddddd",
    "ddddddddDddddddd",
    "dddddddddddddddd",
    "ddddDddddddddddd",
    "dddddddddddddDdd",
    "dddddddddddddddd",
    "dddddddddddddddd",
    "ddDddddddddddddd",
    "ddddddddddDddddd",
    "dddddddddddddddd",
};

static const char *TILE_ART_WATER[16] = {
    "wwwwwwwwwwwwwwww",
    "wwwwwwwwwwwwwwww",
    "wwWWWwwwwwwwwwww",
    "wwwwwwwwwwwwwwww",
    "wwwwwwwwwwWWWwww",
    "wwwwwwwwwwwwwwww",
    "wwwwwwwwwwwwwwww",
    "wWWWwwwwwwwwwwww",
    "wwwwwwwwwwwwwwww",
    "wwwwwwwwWWWwwwww",
    "wwwwwwwwwwwwwwww",
    "wwwwwwwwwwwwwwww",
    "wwwWWWwwwwwwwwww",
    "wwwwwwwwwwwwWWww",
    "wwwwwwwwwwwwwwww",
    "wwwwwwwwwwwwwwww",
};

/* second water frame: the highlights drift. the overworld renderer
 * swaps WATER/WATER2 every ~half second (never draw WATER2 in a map). */
static const char *TILE_ART_WATER2[16] = {
    "wwwwwwwwwwwwwwww",
    "wwWWWwwwwwwwwwww",
    "wwwwwwwwwwwwwwww",
    "wwwwwwwwwwWWWwww",
    "wwwwwwwwwwwwwwww",
    "wwwwwwwwwwwwwwww",
    "wWWWwwwwwwwwwwww",
    "wwwwwwwwwwwwwwww",
    "wwwwwwwwWWWwwwww",
    "wwwwwwwwwwwwwwww",
    "wwwwwwwwwwwwwwww",
    "wwwWWWwwwwwwwwww",
    "wwwwwwwwwwwwWWww",
    "wwwwwwwwwwwwwwww",
    "wwwwwwwwwwwwwwww",
    "wwwwwwwwwwwwwwww",
};

static const char *TILE_ART_FENCE[16] = {
    "gggggggggggggggg",
    "ggDDggggggggDDgg",
    "ggDDggggggggDDgg",
    "gDDDDDDDDDDDDDDg",
    "gDDDDDDDDDDDDDDg",
    "ggDDggggggggDDgg",
    "ggDDggggggggDDgg",
    "gDDDDDDDDDDDDDDg",
    "gDDDDDDDDDDDDDDg",
    "ggDDggggggggDDgg",
    "ggDDggggggggDDgg",
    "ggDDggggggggDDgg",
    "gggggggggggggggg",
    "gggggggggggggggg",
    "gggggggggggggggg",
    "gggggggggggggggg",
};

static const char *TILE_ART_TREE[16] = {
    "ggggGGGGGGGGgggg",
    "ggGGGGGGGGGGGGgg",
    "gGGGGgGGGGGGGGGg",
    "gGGGGGGGGGgGGGGg",
    "GGGGGGGGGGGGGGGG",
    "GGgGGGGGGGGGGGGG",
    "GGGGGGGGgGGGGGGG",
    "gGGGGGGGGGGGGGGg",
    "gGGGGGGGGGGGGGgg",
    "ggGGGGGGGGGGGGgg",
    "gggGGGGGGGGGGggg",
    "ggggggDDDDgggggg",
    "ggggggDDDDgggggg",
    "ggggggDDDDgggggg",
    "gggggDDDDDDggggg",
    "gggggggggggggggg",
};

/* wooden house wall, horizontal planks */
static const char *TILE_ART_WALL[16] = {
    "DDDDDDDDDDDDDDDD",
    "dddddddddddddddd",
    "dddddddddddddddd",
    "dddddddddddddddd",
    "DDDDDDDDDDDDDDDD",
    "dddddddddddddddd",
    "dddddddddddddddd",
    "dddddddddddddddd",
    "DDDDDDDDDDDDDDDD",
    "dddddddddddddddd",
    "dddddddddddddddd",
    "dddddddddddddddd",
    "DDDDDDDDDDDDDDDD",
    "dddddddddddddddd",
    "dddddddddddddddd",
    "dddddddddddddddd",
};

static const char *TILE_ART_ROOF[16] = {
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "rrrrrrrrrrrrrrrr",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "rrrrrrrrrrrrrrrr",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "rrrrrrrrrrrrrrrr",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "rrrrrrrrrrrrrrrr",
    "RRRRRRRRRRRRRRRR",
    "RRRRRRRRRRRRRRRR",
    "rrrrrrrrrrrrrrrr",
    "RRRRRRRRRRRRRRRR",
};

static const char *TILE_ART_DOOR[16] = {
    "DDDDDDDDDDDDDDDD",
    "dddDDDDDDDDDDddd",
    "dddD00000000Dddd",
    "dddD00000000Dddd",
    "dddD00000000Dddd",
    "dddD00000000Dddd",
    "dddD00000000Dddd",
    "dddD00000000Dddd",
    "dddD00000y00Dddd",
    "dddD00000000Dddd",
    "dddD00000000Dddd",
    "dddD00000000Dddd",
    "dddD00000000Dddd",
    "dddD00000000Dddd",
    "dddD00000000Dddd",
    "dddDDDDDDDDDDddd",
};

/* rows of little green sprouts in tilled dirt */
static const char *TILE_ART_CROP[16] = {
    "dddddddddddddddd",
    "ddgggddgggddgggd",
    "ddgGgddgGgddgGgd",
    "ddgggddgggddgggd",
    "dddddddddddddddd",
    "dddddddddddddddd",
    "ddgggddgggddgggd",
    "ddgGgddgGgddgGgd",
    "ddgggddgggddgggd",
    "dddddddddddddddd",
    "dddddddddddddddd",
    "ddgggddgggddgggd",
    "ddgGgddgGgddgGgd",
    "ddgggddgggddgggd",
    "dddddddddddddddd",
    "dddddddddddddddd",
};

static const char *TILE_ART_FLOWER[16] = {
    "gggggggggggggggg",
    "gggrgggggggggggg",
    "ggryrggggggggggg",
    "gggrgggggggggggg",
    "gggggggggggggggg",
    "ggggggggggrggggg",
    "gggggggggryrgggg",
    "ggggggggggrggggg",
    "ggyggggggggggggg",
    "gyrygggggggggggg",
    "ggyggggggggggggg",
    "ggggggggggggyggg",
    "gggggggggggyrygg",
    "ggggggggggggyggg",
    "gggggggggggggggg",
    "gggggggggggggggg",
};

/* ---- INTERIOR TILES (houses, the store...) --------------------------------*/

/* wooden floorboards, vertical seams */
static const char *TILE_ART_FLOOR[16] = {
    "dddDdddddDdddddd",
    "dddDdddddDdddddd",
    "dddDdddddDdddddd",
    "dddDdddddDdddddd",
    "dddDdddddDdddddd",
    "dddDdddddDdddddd",
    "dddDdddddDdddddd",
    "DDDDDDDDDDDDDDDD",
    "ddddddDddddddDdd",
    "ddddddDddddddDdd",
    "ddddddDddddddDdd",
    "ddddddDddddddDdd",
    "ddddddDddddddDdd",
    "ddddddDddddddDdd",
    "ddddddDddddddDdd",
    "DDDDDDDDDDDDDDDD",
};

/* the woven doormat -- step on it to leave a building (put a warp here) */
static const char *TILE_ART_MAT[16] = {
    "dddDdddddDdddddd",
    "d00000000000000d",
    "d0yYyYyYyYyYyY0d",
    "d0YyYyYyYyYyYy0d",
    "d0yYyYyYyYyYyY0d",
    "d0YyYyYyYyYyYy0d",
    "d0yYyYyYyYyYyY0d",
    "d0YyYyYyYyYyYy0d",
    "d0yYyYyYyYyYyY0d",
    "d0YyYyYyYyYyYy0d",
    "d0yYyYyYyYyYyY0d",
    "d0YyYyYyYyYyYy0d",
    "d0yYyYyYyYyYyY0d",
    "d00000000000000d",
    "dddDdddddDdddddd",
    "DDDDDDDDDDDDDDDD",
};

/* a bed: white pillow, red quilt, wooden frame */
static const char *TILE_ART_BED[16] = {
    "d00000000000000d",
    "d01111111111110d",  /* pillow */
    "d01111111111110d",
    "d01111111111110d",
    "d00000000000000d",
    "d0rrrrrrrrrrrr0d",
    "d0rrRrrrrrRrrr0d",
    "d0rrrrrrrrrrrr0d",
    "d0rRrrrrRrrrrr0d",
    "d0rrrrrrrrrrrr0d",
    "d0rrrrRrrrrRrr0d",
    "d0rrrrrrrrrrrr0d",
    "d00000000000000d",
    "d0DD00000000DD0d",
    "d0DD00000000DD0d",
    "dddDdddddDdddddd",
};

/* a sturdy table */
static const char *TILE_ART_TABLE[16] = {
    "dddDdddddDdddddd",
    "d00000000000000d",
    "d0dddddddddddd0d",
    "d0ddDddddddDdd0d",
    "d0dddddddddddd0d",
    "d0dddddddddddd0d",
    "d00000000000000d",
    "dd0DD000000DD0dd",
    "dd0DD000000DD0dd",
    "dd0DD000000DD0dd",
    "dd0DD000000DD0dd",
    "dddDdddddDdddddd",
    "ddddddDddddddDdd",
    "ddddddDddddddDdd",
    "ddddddDddddddDdd",
    "DDDDDDDDDDDDDDDD",
};

/* solid darkness (fills space outside interior walls) */
static const char *TILE_ART_VOID[16] = {
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
};

/* ---- THE PARKING LOT -----------------------------------------------------*/

/* cracked asphalt */
static const char *TILE_ART_ASPHALT[16] = {
    "KKKKKKKKKKKKKKKK",
    "KKKKKKKKKkKKKKKK",
    "KKKkKKKKKKKKKKKK",
    "KKKKKKKKKKKKKkKK",
    "KKKKKKKKKKKKKKKK",
    "KkKKKKKKKKKKKKKK",
    "KKKKKKKKKkKKKKKK",
    "KKKKKKKKKKKKKKKK",
    "KKKKKKKKKKKKKKKk",
    "KKKKKkKKKKKKKKKK",
    "KKKKKKKKKKKKKKKK",
    "KKKKKKKKKKKkKKKK",
    "KKKKKKKKKKKKKKKK",
    "KkKKKKKKKKKKKKKK",
    "KKKKKKKKKKKKKKKK",
    "KKKKKKKkKKKKKKKK",
};

/* a painted parking line, going grey */
static const char *TILE_ART_LINE[16] = {
    "KKKKKKK11KKKKKKK",
    "KKKKKKK11KKKKKKK",
    "KKKKKKK11KKKKKKK",
    "KKKKKKKkkKKKKKKK",
    "KKKKKKK11KKKKKKK",
    "KKKKKKK11KKKKKKK",
    "KKKKKKK11KKKKKKK",
    "KKKKKKK11KKKKKKK",
    "KKKKKKKk1KKKKKKK",
    "KKKKKKK11KKKKKKK",
    "KKKKKKK11KKKKKKK",
    "KKKKKKK11KKKKKKK",
    "KKKKKKK11KKKKKKK",
    "KKKKKKKkkKKKKKKK",
    "KKKKKKK11KKKKKKK",
    "KKKKKKK11KKKKKKK",
};

/* ---- THE CITY -------------------------------------------------------------*/

/* sidewalk: pale slabs with expansion joints, and the odd crack */
static const char *TILE_ART_SIDEWALK[16] = {
    "kkkkkkkKkkkkkkkK",
    "kkkkkkkKkkkkkkkK",
    "kkKkkkkKkkkkkkkK",
    "kkkkkkkKkkkkKkkK",
    "kkkkkkkKkkkkkkkK",
    "kkkkkkkKkkkkkkkK",
    "kkkkKkkKkkkkkkkK",
    "KKKKKKKKKKKKKKKK",
    "kkkkkkkKkkkkkkkK",
    "kkkkkkkKkkKkkkkK",
    "kKkkkkkKkkkkkkkK",
    "kkkkkkkKkkkkkkkK",
    "kkkkkkkKkkkkkKkK",
    "kkkKkkkKkkkkkkkK",
    "kkkkkkkKkkkkkkkK",
    "KKKKKKKKKKKKKKKK",
};

/* the centre line of a street that runs ACROSS the screen -- TILE_LINE
 * turned on its side. Painted long ago, worn by everything since. */
static const char *TILE_ART_LINE_H[16] = {
    "KKKKKKKKKKKKKKKK",
    "KKKKKkKKKKKKKKKK",
    "KKKKKKKKKKKkKKKK",
    "KKKKKKKKKKKKKKKK",
    "KkKKKKKKKKKKKKKK",
    "KKKKKKKKKKKKKKKK",
    "KKKKKKKKKKKKKKKK",
    "111111111k111111",
    "11111k1111111111",
    "KKKKKKKKKKKKKKKK",
    "KKKKKKKKKkKKKKKK",
    "KKKKKKKKKKKKKKKK",
    "KKKkKKKKKKKKKKKK",
    "KKKKKKKKKKKKKKKK",
    "KKKKKKKKKKKKKkKK",
    "KKKKKKKkKKKKKKKK",
};

/* ---- THE CITY'S ARCHITECTURE ----------------------------------------------
 * Skyscrapers, and a catholic church. The facades are drawn front-on, the
 * way every building in this engine is -- what makes them read as TALL is
 * the window grid: floors of lit and dark offices instead of logs.
 */

/* skyscraper facade: concrete, and two floors of windows per tile. Some
 * windows are lit -- somebody is still up there, or left in a hurry. */
static const char *TILE_ART_SKY_WALL[16] = {
    "KKKKKKKKKKKKKKKK",
    "K0000KKK0000KKKK",
    "K0BBB0KK0yyy0KKK",
    "K0BbB0KK0y1y0KKK",
    "K0BBB0KK0yyy0KKK",
    "K0000KKK0000KKKK",
    "KKKKKKKKKKKKKKKK",
    "0000000000000000",
    "KKKKKKKKKKKKKKKK",
    "K0000KKK0000KKKK",
    "K0yyy0KK0BBB0KKK",
    "K0y1y0KK0BbB0KKK",
    "K0yyy0KK0BBB0KKK",
    "K0000KKK0000KKKK",
    "KKKKKKKKKKKKKKKK",
    "0000000000000000",
};

/* a shorter building's roof line: parapet cap, shadow, then windows */
static const char *TILE_ART_SKY_TOP[16] = {
    "kkkkkkkkkkkkkkkk",
    "kkkkkkkkkkkkkkkk",
    "0000000000000000",
    "KKKKKKKKKKKKKKKK",
    "KKKKKKKKKKKKKKKK",
    "K0000KKK0000KKKK",
    "K0BBB0KK0yyy0KKK",
    "K0BbB0KK0y1y0KKK",
    "K0BBB0KK0yyy0KKK",
    "K0000KKK0000KKKK",
    "KKKKKKKKKKKKKKKK",
    "KKKKKKKKKKKKKKKK",
    "K0000KKK0000KKKK",
    "K0BBB0KK0BBB0KKK",
    "K0BBB0KK0BbB0KKK",
    "K0000KKK0000KKKK",
};

/* THE EDGE OF THE MAP: smaller towers a block away, black against the
 * night, a few windows still burning. This is the city's tree line. */
static const char *TILE_ART_SKY_FAR[16] = {
    "00000000KKKKKK00",
    "00000000KyKKKK00",
    "00000000KKKKKK00",
    "00000000KKKyKK00",
    "0KKKKK00KKKKKK00",
    "0KKKyK00KKKKKK00",
    "0KKKKK00KyKKKK00",
    "0KyKKK00KKKKKK00",
    "0KKKKK00KKKKyK00",
    "0KKKKK00KKKKKK00",
    "0KKKyK00KyKKKK00",
    "0KKKKK00KKKKKK00",
    "0KyKKK00KKKyKK00",
    "0KKKKK00KKKKKK00",
    "0KKKKK00KyKKKK00",
    "0KKKKK00KKKKKK00",
};

/* dressed stone, laid in courses -- nothing else downtown is built of it */
static const char *TILE_ART_CHURCH_WALL[16] = {
    "kkkkkkkKkkkkkkkk",
    "kkkkkkkKkkkkkkkk",
    "kkkkkkkKkkkkkkkk",
    "KKKKKKKKKKKKKKKK",
    "kkkKkkkkkkkKkkkk",
    "kkkKkkkkkkkKkkkk",
    "kkkKkkkkkkkKkkkk",
    "KKKKKKKKKKKKKKKK",
    "kkkkkkkKkkkkkkkk",
    "kkkkkkkKkkkkkkkk",
    "kkkkkkkKkkkkkkkk",
    "KKKKKKKKKKKKKKKK",
    "kkkKkkkkkkkKkkkk",
    "kkkKkkkkkkkKkkkk",
    "kkkKkkkkkkkKkkkk",
    "KKKKKKKKKKKKKKKK",
};

/* a stained-glass arch set in that stone. The same window the cutscene
 * lights from inside. */
static const char *TILE_ART_CHURCH_WIN[16] = {
    "kkkkkkkKkkkkkkkk",
    "kkkkk000000kkkkk",
    "kkkk0rybgpr0kkkk",
    "kkk0rybKbgpr0kkk",
    "KKK0ybgKgpry0KKK",
    "kkk0bgpKpryb0kkk",
    "kkk0000K0000kkkk",
    "KKK0gprKrybg0KKK",
    "kkk0prybKybgp0kk",
    "kkk0rybgKbgpr0kk",
    "kkk0ybgpKgpry0kk",
    "KKK0bgprKpryb0KK",
    "kkk0000000000kkk",
    "kkkkkkkKkkkkkkkk",
    "kkkKkkkkkkkKkkkk",
    "KKKKKKKKKKKKKKKK",
};

/* the gilded cross in the gable, over the doors */
static const char *TILE_ART_CHURCH_CROSS[16] = {
    "kkkkkkkKkkkkkkkk",
    "kkkkkk0000kkkkkk",
    "kkkkkk0yY0kkkkkk",
    "kkkkkk0yY0kkkkkk",
    "kkk00000yY000kkk",
    "kkk0yyyyyyyyY0kk",
    "kkk0YyyyyyyyY0kk",
    "kkk00000yY000kkk",
    "kkkkkk0yY0kkkkkk",
    "kkkkkk0yY0kkkkkk",
    "kkkkkk0yY0kkkkkk",
    "KKKKKK0yY0KKKKKK",
    "kkkkkk0yY0kkkkkk",
    "kkkkkk0000kkkkkk",
    "kkkKkkkkkkkKkkkk",
    "KKKKKKKKKKKKKKKK",
};

/* THE CHURCH DOOR: an arched double door of old oak. It is a DOOR tile in
 * every way that matters -- solid, and walking into it fires its warp. */
static const char *TILE_ART_CHURCH_DOOR[16] = {
    "kkkkk000000kkkkk",
    "kkk00DDDDDD00kkk",
    "kk0DDdDDDDdDD0kk",
    "kk0DdDD00DDdD0kk",
    "k0DDdDD00DDdDD0k",
    "k0DDdDD00DDdDD0k",
    "k0DDdDD00DDdDD0k",
    "k0DDdDD00DDdDD0k",
    "k0DDdDy00yDdDD0k",
    "k0DDdDy00yDdDD0k",
    "k0DDdDD00DDdDD0k",
    "k0DDdDD00DDdDD0k",
    "k0DDdDD00DDdDD0k",
    "k0DDdDD00DDdDD0k",
    "k0DDdDD00DDdDD0k",
    "0000000000000000",
};

/* an office lobby's glass doors: aluminium frame, push bar, and nothing
 * moving on the other side. Also a DOOR tile. */
static const char *TILE_ART_DOOR_GLASS[16] = {
    "KKKKKKKKKKKKKKKK",
    "K00000000000000K",
    "K0BBBWB00BWBBB0K",
    "K0BBWBB00WBBBB0K",
    "K0BWBBB00BBBBB0K",
    "K0WBBBB00BBBBW0K",
    "K0BBBBB00BBBWB0K",
    "K0kkkkk00kkkkk0K",
    "K0BBBBB00BBWBB0K",
    "K0BBBBB00BWBBB0K",
    "K0BBBBB00WBBBB0K",
    "K0B0BBB00BBB0B0K",
    "K0B0BBB00BBB0B0K",
    "K00000000000000K",
    "KKKKKKKKKKKKKKKK",
    "0000000000000000",
};

/* a street light. The head is lit -- see the lights[] list in overworld.c,
 * which finds these tiles and puts a pool of light under each one. */
static const char *TILE_ART_LAMP[16] = {
    "....000000......",
    "...0yyyyyy0.....",
    "...0y1111y0.....",
    "...0yyyyyy0.....",
    "....00KK00......",
    "......KK........",
    "......KK........",
    "......KK........",
    "......KK........",
    "......KK........",
    "......KK........",
    "......KK........",
    "......KK........",
    ".....0KK0.......",
    "....0KKKK0......",
    "...0KKKKKK0.....",
};

/* ============================ THE SHIP =====================================
 * Part 2's architecture: white metal, consoles, and the symbols in the
 * deck. Drawn to read as ONE surface -- the seams line up tile to tile.
 */
static const char *TILE_ART_UFOWALL[16] = {
    "KKKKKKKKKKKKKKKK",
    "K11111111111111K",
    "K1kkkkkkkkkkkk1K",
    "K1k1111111111k1K",
    "K1k1111111111k1K",
    "K1k1111111111k1K",
    "K1k1111111111k1K",
    "K1k1111111111k1K",
    "K1k1111111111k1K",
    "K1k1111111111k1K",
    "K1k1111111111k1K",
    "K1k1111111111k1K",
    "K1k1111111111k1K",
    "K1kkkkkkkkkkkk1K",
    "K11111111111111K",
    "KKKKKKKKKKKKKKKK",
};

static const char *TILE_ART_UFOFLOOR[16] = {
    "kkkkkkkkkkkkkkkK",
    "k11111111111111K",
    "k11111111111111K",
    "k11111111111111K",
    "k11111111111111K",
    "k11111111111111K",
    "k11111111111111K",
    "k11111111111111K",
    "k11111111111111K",
    "k11111111111111K",
    "k11111111111111K",
    "k11111111111111K",
    "k11111111111111K",
    "k11111111111111K",
    "k11111111111111K",
    "KKKKKKKKKKKKKKKK",
};

/* a console: dark slab, mint screen, and lights that mean something to
 * somebody */
static const char *TILE_ART_COMPUTER[16] = {
    "0000000000000000",
    "0KKKKKKKKKKKKKK0",
    "0KeeeeeeeeeeeeK0",
    "0KegggegeggegeK0",
    "0KeeeeeeeeeeeeK0",
    "0KeggegeggeggeK0",
    "0KeeeeeeeeeeeeK0",
    "0KKKKKKKKKKKKKK0",
    "0KkkkkkkkkkkkkK0",
    "0Kk1k0k1k0k1kkK0",
    "0KkkkkkkkkkkkkK0",
    "0Kkrk0kgk0k1kkK0",
    "0KkkkkkkkkkkkkK0",
    "0KKKKKKKKKKKKKK0",
    "0000000000000000",
    "0000000000000000",
};

/* the deck symbol: a ring and a mark, pressed into the metal in a green
 * that has no business glowing without a light source */
static const char *TILE_ART_SYMBOL[16] = {
    "kkkkkkkkkkkkkkkK",
    "k11111111111111K",
    "k1111ggggg11111K",
    "k111gg111gg1111K",
    "k11gg11111gg111K",
    "k11g1111111g111K",
    "k11g1111111g111K",
    "k11g111g111g111K",
    "k11g111g111g111K",
    "k11g1111111g111K",
    "k11gg11111gg111K",
    "k111gg111gg1111K",
    "k1111ggggg11111K",
    "k11111g1g111111K",
    "k11111111111111K",
    "KKKKKKKKKKKKKKKK",
};

/* THE POD. A hatch in the deck with a green window, and the window is
 * the only thing on this ship that looks like it wants you to live. */
static const char *TILE_ART_POD[16] = {
    "KKKKKKKKKKKKKKKK",
    "K00000000000000K",
    "K01111111111110K",
    "K01kkkkkkkkkk10K",
    "K01k11111111k10K",
    "K01k1eeeeee1k10K",
    "K01k1eggggE1k10K",
    "K01k1egeegE1k10K",
    "K01k1egeegE1k10K",
    "K01k1eggggE1k10K",
    "K01k1EEEEEE1k10K",
    "K01k11111111k10K",
    "K01kkkkkkkkkk10K",
    "K01111111111110K",
    "K00000000000000K",
    "KKKKKKKKKKKKKKKK",
};

#endif /* TILES_H */
