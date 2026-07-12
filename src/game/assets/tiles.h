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

#endif /* TILES_H */
