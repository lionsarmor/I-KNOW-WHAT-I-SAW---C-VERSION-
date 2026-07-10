/* ============================================================================
 * sprites.h -- every character in the game, drawn in ASCII.
 *
 * One character = one pixel. The palette (what each letter means) is
 * defined at the top of assets.c:
 *
 *   .  transparent      s  skin / S shaded    a  alien gray / A shadow
 *   0  near-black       r  red  / R dark      p  purple / P dark purple
 *   1  white            b  denim / B dark     k  gray / K dark gray
 *   y  straw / Y dark   d  wood / D dark      g  grass / G dark
 *   w  water / W light  o  orange
 *
 * 16-BIT STYLE RULES used here (follow them and new art will fit in):
 *   - every character has a '0' (near-black) outline
 *   - one shade color on the lit form ('S' under the chin, 'Y' hat brim,
 *     'P' skirt hem, 'A' along the alien's edges)
 *   - faces read at 1x: eyes are single '0' pixels, keep 1px gaps
 *
 * Every sprite is exactly 16 rows of exactly 16 characters
 * (assets_init() checks this and reports mistakes as magenta pixels).
 *
 * TO ADD A SPRITE:
 *   1. Draw it here as  static const char *SPR_ART_MYTHING[16] = {...};
 *   2. Add an id to the sprite enum in assets.h
 *   3. Register it in the sprite_art[] list in assets.c
 * ==========================================================================*/
#ifndef SPRITES_H
#define SPRITES_H

/* ---- THE FARMER -- our hero. Straw hat, red shirt, denim overalls. -------
 * Three frames per direction: stand, step-left, step-right.
 * The walk cycle plays them 0,1,0,2.
 */

static const char *SPR_ART_FARMER_DOWN_0[16] = {
    "................",
    ".....00000......",
    "....0yyyyy0.....",
    "...0yyyyyyy0....",
    "..0YYYYYYYYY0...",
    "...0ssssss0.....",
    "...0s0ss0s0.....",
    "...0ssSSss0.....",
    "....0ssss0......",
    "...0rrrrrr0.....",
    "..0rr0bb0rr0....",
    "..0sr0bb0rs0....",
    "...0bbbbbb0.....",
    "...0bb00bb0.....",
    "...0DD00DD0.....",
    "....00..00......",
};

/* walking: left foot forward */
static const char *SPR_ART_FARMER_DOWN_1[16] = {
    "................",
    ".....00000......",
    "....0yyyyy0.....",
    "...0yyyyyyy0....",
    "..0YYYYYYYYY0...",
    "...0ssssss0.....",
    "...0s0ss0s0.....",
    "...0ssSSss0.....",
    "....0ssss0......",
    "...0rrrrrr0.....",
    "..0rr0bb0rr0....",
    "..0sr0bb0rs0....",
    "...0bbbbbb0.....",
    "...0bb00bb0.....",
    "...0DD00bb0.....",
    ".......0DD0.....",
};

/* walking: right foot forward */
static const char *SPR_ART_FARMER_DOWN_2[16] = {
    "................",
    ".....00000......",
    "....0yyyyy0.....",
    "...0yyyyyyy0....",
    "..0YYYYYYYYY0...",
    "...0ssssss0.....",
    "...0s0ss0s0.....",
    "...0ssSSss0.....",
    "....0ssss0......",
    "...0rrrrrr0.....",
    "..0rr0bb0rr0....",
    "..0sr0bb0rs0....",
    "...0bbbbbb0.....",
    "...0bb00bb0.....",
    "...0bb00DD0.....",
    "...0DD0.........",
};

/* back view: hat from behind, overall straps crossing the shirt */
static const char *SPR_ART_FARMER_UP_0[16] = {
    "................",
    ".....00000......",
    "....0yyyyy0.....",
    "...0yyyyyyy0....",
    "..0YYYYYYYYY0...",
    "...0yyYyyy0.....",
    "...0ssssss0.....",
    "...0rrrrrr0.....",
    "..0rrbbbbrr0....",
    "..0srbbbbrs0....",
    "...0bbbbbb0.....",
    "...0bbbbbb0.....",
    "...0bb00bb0.....",
    "...0bb00bb0.....",
    "...0DD00DD0.....",
    "....00..00......",
};

static const char *SPR_ART_FARMER_UP_1[16] = {
    "................",
    ".....00000......",
    "....0yyyyy0.....",
    "...0yyyyyyy0....",
    "..0YYYYYYYYY0...",
    "...0yyYyyy0.....",
    "...0ssssss0.....",
    "...0rrrrrr0.....",
    "..0rrbbbbrr0....",
    "..0srbbbbrs0....",
    "...0bbbbbb0.....",
    "...0bbbbbb0.....",
    "...0bb00bb0.....",
    "...0DD00bb0.....",
    ".......0DD0.....",
    "................",
};

static const char *SPR_ART_FARMER_UP_2[16] = {
    "................",
    ".....00000......",
    "....0yyyyy0.....",
    "...0yyyyyyy0....",
    "..0YYYYYYYYY0...",
    "...0yyYyyy0.....",
    "...0ssssss0.....",
    "...0rrrrrr0.....",
    "..0rrbbbbrr0....",
    "..0srbbbbrs0....",
    "...0bbbbbb0.....",
    "...0bbbbbb0.....",
    "...0bb00bb0.....",
    "...0bb00DD0.....",
    "...0DD0.........",
    "................",
};

/* Side view faces LEFT. The renderer mirrors it when walking right. */
static const char *SPR_ART_FARMER_SIDE_0[16] = {
    "................",
    ".....00000......",
    "....0yyyyy0.....",
    "...0yyyyyyy0....",
    "..0YYYYYYYYY0...",
    "...0ssssss0.....",
    "...0s0ssss0.....",
    "...0ssssSs0.....",
    "....0ssss0......",
    "...0rrrrrr0.....",
    "...0rrrrrr0.....",
    "...0srrrrs0.....",
    "...0bbbbbb0.....",
    "....0bbbb0......",
    "....0DDDD0......",
    ".....0000.......",
};

/* striding: legs scissor open */
static const char *SPR_ART_FARMER_SIDE_1[16] = {
    "................",
    ".....00000......",
    "....0yyyyy0.....",
    "...0yyyyyyy0....",
    "..0YYYYYYYYY0...",
    "...0ssssss0.....",
    "...0s0ssss0.....",
    "...0ssssSs0.....",
    "....0ssss0......",
    "...0rrrrrr0.....",
    "...0rrrrrr0.....",
    "...0srrrrs0.....",
    "...0bbbbbb0.....",
    "...0bbb0bbb0....",
    "..0DD0..0DD0....",
    "................",
};

/* legs passing each other */
static const char *SPR_ART_FARMER_SIDE_2[16] = {
    "................",
    ".....00000......",
    "....0yyyyy0.....",
    "...0yyyyyyy0....",
    "..0YYYYYYYYY0...",
    "...0ssssss0.....",
    "...0s0ssss0.....",
    "...0ssssSs0.....",
    "....0ssss0......",
    "...0rrrrrr0.....",
    "...0rrrrrr0.....",
    "...0srrrrs0.....",
    "...0bbbbbb0.....",
    "....0bbbb0......",
    "....0bb0DD0.....",
    ".....00.........",
};

/* ---- THE VISITORS ---------------------------------------------------------
 * Gaunt greys. Huge black wraparound eyes angled toward a lipless slit,
 * a red glint deep in each. Outlined in their own shadow color 'A'.
 */

static const char *SPR_ART_ALIEN_0[16] = {
    "................",
    ".....AAAAAA.....",
    "....AaaaaaaA....",
    "...AaaaaaaaaA...",
    "..Aa000aa000aA..",
    "..A00r0aa0r00A..",
    "..Aa000aa000aA..",
    "...AaaaaaaaaA...",
    "....Aa0000aA....",
    ".....AaaaaA.....",
    "......AaaA......",
    "....AaaaaaaA....",
    "...Aa.AaaA.aA...",
    "...A..AaaA..A...",
    ".....Aa..aA.....",
    ".....AA..AA.....",
};

/* second frame: the long arms lift. it has noticed you. */
static const char *SPR_ART_ALIEN_1[16] = {
    "................",
    ".....AAAAAA.....",
    "....AaaaaaaA....",
    "...AaaaaaaaaA...",
    "..Aa000aa000aA..",
    "..A00r0aa0r00A..",
    "..Aa000aa000aA..",
    "...AaaaaaaaaA...",
    "....Aa0000aA....",
    ".....AaaaaA.....",
    "..A..AaaaaA..A..",
    "..Aa.AaaaaA.aA..",
    ".....AaaaaA.....",
    ".....Aa..aA.....",
    ".....AA..AA.....",
    "................",
};

/* ---- TOWNSFOLK ------------------------------------------------------------
 * NPC "looks" are pairs of idle frames, registered in npc_looks[] in
 * assets.c. Spawn one on any map with { ENT_NPC, x, y, LOOK_*, "TEXT" }.
 */

/* LOOK_VILLAGER: gray hair, purple dress */
static const char *SPR_ART_NPC[16] = {
    "................",
    ".....00000......",
    "....0kkkkk0.....",
    "...0kkkkkkk0....",
    "...0ssssss0.....",
    "...0s0ss0s0.....",
    "...0ssssss0.....",
    "....0ssss0......",
    "...0pppppp0.....",
    "..0pppppppp0....",
    "..0spppppps0....",
    "..0pppppppp0....",
    "..0pPPppPPp0....",
    "..0pppppppp0....",
    "...0D0..0D0.....",
    "................",
};

/* idle frame 2: a slow nervous sway. everyone in town is on edge. */
static const char *SPR_ART_NPC_1[16] = {
    "................",
    ".....00000......",
    "....0kkkkk0.....",
    "...0kkkkkkk0....",
    "...0ssssss0.....",
    "...0s0ss0s0.....",
    "...0ssssss0.....",
    "....0ssss0......",
    "....0pppppp0....",
    "...0pppppppp0...",
    "...0spppppps0...",
    "...0pppppppp0...",
    "...0pPPppPPp0...",
    "...0pppppppp0...",
    "....0D0..0D0....",
    "................",
};

/* LOOK_ELDER: Pa. straw hat, gray beard, the same overalls he's worn
 * for thirty years. */
static const char *SPR_ART_ELDER[16] = {
    "................",
    ".....00000......",
    "....0yyyyy0.....",
    "...0yyyyyyy0....",
    "..0YYYYYYYYY0...",
    "...0ssssss0.....",
    "...0s0ss0s0.....",
    "...0kssssk0.....",
    "...0kkkkkk0.....",
    "...0rrrrrr0.....",
    "..0rr0bb0rr0....",
    "..0sr0bb0rs0....",
    "...0bbbbbb0.....",
    "...0bb00bb0.....",
    "...0DD00DD0.....",
    "....00..00......",
};

/* idle frame 2: he leans on nothing, looking at the sky */
static const char *SPR_ART_ELDER_1[16] = {
    "................",
    ".....00000......",
    "....0yyyyy0.....",
    "...0yyyyyyy0....",
    "..0YYYYYYYYY0...",
    "...0ssssss0.....",
    "...0s0ss0s0.....",
    "...0kssssk0.....",
    "...0kkkkkk0.....",
    "...0rrrrrr0.....",
    "..0rr0bb0r00....",
    "..0sr0bb0r0.....",
    "...0bbbbbb0.....",
    "...0bb00bb0.....",
    "...0DD00DD0.....",
    "....00..00......",
};

/* ---- THE FACE (intro screen, 32x32, drawn scaled up and very dim) --------
 * A gaunt grey: huge angled wraparound eyes with a red glint, hollow
 * cheeks ('A' shading), slit nostrils, a long thin mouth. You never see
 * it at full brightness. That's the point.
 */
static const char *SPR_ART_FACE_BIG[32] = {
    "................................",
    "..........aaaaaaaaaaaa..........",
    "........aaaaaaaaaaaaaaaa........",
    "......Aaaaaaaaaaaaaaaaaaaa......",
    ".....AaaaaaaaaaaaaaaaaaaaaA.....",
    "....AaaaaaaaaaaaaaaaaaaaaaaA....",
    "...AaaaaaaaaaaaaaaaaaaaaaaaaA...",
    "...AaaaaaaaaaaaaaaaaaaaaaaaaA...",
    "..AaaaaaaaaaaaaaaaaaaaaaaaaaaA..",
    "..Aaa0000000aaaaaaaa0000000aaA..",
    "..Aa000000000aaaaaa000000000aA..",
    "..Aa0000000000aaaa0000000000aA..",
    "..Aa0000000r00aaaa00r0000000aA..",
    "..Aaa000000000aaaa000000000aaA..",
    "..Aaaa00000000aaaa00000000aaaA..",
    "..Aaaaaa000000aaaa000000aaaaaA..",
    "...AaaaaAAAAaaaaaaaaAAAAaaaaA...",
    "...AaaAAAaaaaaaaaaaaaaaAAAaaA...",
    "....Aaaaaaaaaa0aa0aaaaaaaaaA....",
    ".....AaaaaaaaaaaaaaaaaaaaaA.....",
    "......AaaaaaaaaaaaaaaaaaaA......",
    ".......AaaaaaaaaaaaaaaaaA.......",
    "........AaaaaaaaaaaaaaA.........",
    "........Aaa0000000000aaA........",
    ".........AaaaaaaaaaaaaA.........",
    "..........AaaaaaaaaaaA..........",
    "...........AaaaaaaaA............",
    "............AaaaaaA.............",
    ".............AaaaaA.............",
    "..............AaaA..............",
    "................................",
    "................................",
};

#endif /* SPRITES_H */
