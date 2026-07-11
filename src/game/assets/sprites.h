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

/* ---- THE FARMER -- our hero. Worn cap, red shirt, denim overalls. --------
 * Three frames per direction: stand, step-left, step-right.
 * The walk cycle plays them 0,1,0,2.
 */

static const char *SPR_ART_FARMER_DOWN_0[16] = {
    "................",
    ".....000000.....",
    "....0dddddd0....",
    "...0ddDddDdd0...",
    "..000DDDDDD000..",
    "...0ssssssss0...",
    "...0s0ssss0s0...",
    "...0sssSSsss0...",
    "....0ssssss0....",
    "...0RrrrrrrR0...",
    "..0rr0bbbb0rr0..",
    "..0sr0bbbb0rs0..",
    "...0bBbbbbB0....",
    "...0bbbbbbbb0...",
    "...0bb00bb0.....",
    "..0DD0..0DD0....",
};

/* walking: left foot forward */
static const char *SPR_ART_FARMER_DOWN_1[16] = {
    "................",
    ".....000000.....",
    "....0dddddd0....",
    "...0ddDddDdd0...",
    "..000DDDDDD000..",
    "...0ssssssss0...",
    "...0s0ssss0s0...",
    "...0sssSSsss0...",
    "....0ssssss0....",
    "...0RrrrrrrR0...",
    "..0rr0bbbb0rr0..",
    "..0sr0bbbb0rs0..",
    "...0bbbbbbbb0...",
    "....0bb0bb0.....",
    "...0DD0..0bb0...",
    ".........0DD0...",
};

/* walking: right foot forward */
static const char *SPR_ART_FARMER_DOWN_2[16] = {
    "................",
    ".....000000.....",
    "....0dddddd0....",
    "...0ddDddDdd0...",
    "..000DDDDDD000..",
    "...0ssssssss0...",
    "...0s0ssss0s0...",
    "...0sssSSsss0...",
    "....0ssssss0....",
    "...0RrrrrrrR0...",
    "..0rr0bbbb0rr0..",
    "..0sr0bbbb0rs0..",
    "...0bbbbbbbb0...",
    ".....0bb0bb0....",
    "...0bb0..0DD0...",
    "...0DD0.........",
};

/* back view: hat from behind, overall straps crossing the shirt */
static const char *SPR_ART_FARMER_UP_0[16] = {
    "................",
    ".....000000.....",
    "....0dddddd0....",
    "...0ddDddDdd0...",
    "..000DDDDDD000..",
    "...0DD0000DD0...",
    "...0D000000D0...",
    "...0RrrrrrrR0...",
    "..0rrb0bb0brr0..",
    "..0srb0bb0brs0..",
    "...0bb0bb0bb0...",
    "...0bbb00bbb0...",
    "...0bbbbbbbb0...",
    "...0bb00bb0.....",
    "...0bb00bb0.....",
    "..0DD0..0DD0....",
};

static const char *SPR_ART_FARMER_UP_1[16] = {
    "................",
    ".....000000.....",
    "....0dddddd0....",
    "...0ddDddDdd0...",
    "..000DDDDDD000..",
    "...0DD0000DD0...",
    "...0D000000D0...",
    "...0RrrrrrrR0...",
    "..0rrb0bb0brr0..",
    "..0srb0bb0brs0..",
    "...0bb0bb0bb0...",
    "...0bbbbbbbb0...",
    "....0bb0bb0.....",
    "...0DD0..0bb0...",
    ".........0DD0...",
    "................",
};

static const char *SPR_ART_FARMER_UP_2[16] = {
    "................",
    ".....000000.....",
    "....0dddddd0....",
    "...0ddDddDdd0...",
    "..000DDDDDD000..",
    "...0DD0000DD0...",
    "...0D000000D0...",
    "...0RrrrrrrR0...",
    "..0rrb0bb0brr0..",
    "..0srb0bb0brs0..",
    "...0bb0bb0bb0...",
    "...0bbbbbbbb0...",
    ".....0bb0bb0....",
    "...0bb0..0DD0...",
    "...0DD0.........",
    "................",
};

/* Side view faces LEFT. The renderer mirrors it when walking right. */
static const char *SPR_ART_FARMER_SIDE_0[16] = {
    "................",
    ".....000000.....",
    "....0dddddd0....",
    "..000ddDDddd0...",
    ".0DD00000000....",
    "...0ssssss0.....",
    "...0s0sssss0....",
    "...0sssSSss0....",
    "...0Rrrrrrr0....",
    "..0sr0bbbbrr0...",
    "...0r0bbbbb0....",
    "...0s0bBbbb0....",
    "....0bbbbbb0....",
    "....0bb0bb0.....",
    "...0DD0.0DD0....",
    "....00...00.....",
};

/* striding: legs scissor open */
static const char *SPR_ART_FARMER_SIDE_1[16] = {
    "................",
    ".....000000.....",
    "....0dddddd0....",
    "..000ddDDddd0...",
    ".0DD00000000....",
    "...0ssssss0.....",
    "...0s0sssss0....",
    "...0sssSSss0....",
    "...0Rrrrrrr0....",
    "...0r0bbbbrr0...",
    "...0s0bbbbb0....",
    "....0bBbbbb0....",
    "...0bbb0bbbb0...",
    "..0bbb0..0bbb0..",
    ".0DD0......0DD0.",
    "................",
};

/* legs passing each other */
static const char *SPR_ART_FARMER_SIDE_2[16] = {
    "................",
    ".....000000.....",
    "....0dddddd0....",
    "..000ddDDddd0...",
    ".0DD00000000....",
    "...0ssssss0.....",
    "...0s0sssss0....",
    "...0sssSSss0....",
    "...0Rrrrrrr0....",
    "..0sr0bbbbrr0...",
    "...0r0bbbbb0....",
    "...0s0bBbbb0....",
    "....0bbbbbb0....",
    ".....0bbb0......",
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

/* LOOK_VILLAGER: bundled-up townswoman, gray hair and a dark dress */
static const char *SPR_ART_NPC[16] = {
    "................",
    ".....000000.....",
    "....0kkkkkk0....",
    "...0kKkkkkKk0...",
    "...0KssssssK0...",
    "...0s0ssss0s0...",
    "...0sssSSsss0...",
    "....0ssssss0....",
    "...0PPppppPP0...",
    "..0pppppppppp0..",
    "..0spPppppPps0..",
    "...0pPPppPPp0...",
    "..0pppppppppp0..",
    "..0PPPppppPPP0..",
    "...0DD0..0DD0...",
    "....00....00....",
};

/* idle frame 2: a slow nervous sway. everyone in town is on edge. */
static const char *SPR_ART_NPC_1[16] = {
    "................",
    "......000000....",
    ".....0kkkkkk0...",
    "....0kKkkkkKk0..",
    "....0KssssssK0..",
    "....0s0ssss0s0..",
    "....0sssSSsss0..",
    ".....0ssssss0...",
    "....0PPppppPP0..",
    "...0pppppppppp0.",
    "...0spPppppPps0.",
    "....0pPPppPPp0..",
    "...0pppppppppp0.",
    "...0PPPppppPPP0.",
    "....0DD0..0DD0..",
    ".....00....00...",
};

/* LOOK_ELDER: Pa. battered cap, gray beard, the same overalls he's worn
 * for thirty years. */
static const char *SPR_ART_ELDER[16] = {
    "................",
    ".....000000.....",
    "....0dddddd0....",
    "...0dDDddDDd0...",
    "..000DDDDDD000..",
    "...0ssssssss0...",
    "...0s0ssss0s0...",
    "...0kkkSSkkk0...",
    "....0kkkkkk0....",
    "...0RrrrrrrR0...",
    "..0rr0bbbb0rr0..",
    "..0sr0bbbb0rs0..",
    "...0bBbbbbB0....",
    "...0bb00bb0.....",
    "..0DD0..0DD0....",
    "....00....00....",
};

/* idle frame 2: he leans on nothing, looking at the sky */
static const char *SPR_ART_ELDER_1[16] = {
    "................",
    "......000000....",
    ".....0dddddd0...",
    "....0dDDddDDd0..",
    "...000DDDDDD000.",
    "....0ssssssss0..",
    "....0s0ssss0s0..",
    "....0kkkSSkkk0..",
    ".....0kkkkkk0...",
    "....0RrrrrrrR0..",
    "...0rr0bbbb0r00.",
    "...0sr0bbbb0r0..",
    "....0bBbbbbB0...",
    "....0bb00bb0....",
    "...0DD0..0DD0...",
    ".....00....00...",
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

/* ---- THE ITEMS -- pickups lying on the ground. ---------------------------
 * Drawn small and low in the frame so they read as "on the floor".
 */

/* a tuft of the bitter field herb, roots and all */
static const char *SPR_ART_ITEM_HERB[16] = {
    "................",
    "................",
    "................",
    "................",
    ".....0..0..0....",
    "....0g00g00g0...",
    "....0g00g00g0...",
    "....0gg0g0gg0...",
    ".....0ggggg0....",
    "......0ggg0.....",
    "......0gGg0.....",
    "......0gG0......",
    ".....00gG00.....",
    "....0dDDDd0.....",
    ".....00000......",
    "................",
};

/* white first-aid box, red cross */
static const char *SPR_ART_ITEM_MEDKIT[16] = {
    "................",
    "................",
    "................",
    "................",
    "..00000000000...",
    "..01111111110...",
    "..0111rrr1110...",
    "..0111rrr1110...",
    "..01rrrrrrr10...",
    "..0111rrr1110...",
    "..0111rrr1110...",
    "..0KKKKKKKKK0...",
    "..00000000000...",
    "................",
    "................",
    "................",
};

/* an open box of shotgun shells, brass tips up */
static const char *SPR_ART_ITEM_SHELLS[16] = {
    "................",
    "................",
    "................",
    "................",
    "................",
    "...0y0y0y0y0....",
    "...0r0r0r0r0....",
    "..0RRRRRRRRRR0..",
    "..0RrrrrrrrrR0..",
    "..0RrrrrrrrrR0..",
    "..0RRRRRRRRRR0..",
    "..000000000000..",
    "................",
    "................",
    "................",
    "................",
};

/* the family shotgun, propped on its stock */
static const char *SPR_ART_ITEM_SHOTGUN[16] = {
    "................",
    "................",
    "...........00...",
    "..........0kk0..",
    ".........0kk0...",
    "........0kk0....",
    ".......0kk0.....",
    "......0kk0......",
    ".....0kk0.......",
    "....0KK0........",
    "...0dd0.........",
    "..0ddD0.........",
    "..0dD0..........",
    "..0D0...........",
    "...0............",
    "................",
};

/* the shotgun RAISED, seen from behind, pointing up-right at whatever
 * is floating there -- overlaid on the farmer during a battle SHOOT */
static const char *SPR_ART_GUN_AIM[16] = {
    "..........00....",
    ".........0kk00..",
    "........0kkkk0..",
    ".......0kkkk0...",
    "......0kkkk0....",
    ".....0kkkk0.....",
    "....0kkkk0......",
    "...0kKkK0.......",
    "..0dKkK0........",
    ".0dddD0.........",
    ".0ddD0..........",
    ".0dD0...........",
    ".0D0............",
    "..0.............",
    "................",
    "................",
};

#endif /* SPRITES_H */
