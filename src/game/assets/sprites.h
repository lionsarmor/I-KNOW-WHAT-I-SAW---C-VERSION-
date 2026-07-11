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
 *   c/C cap brown       h/H hair       i/I bright blue
 *   m/M hot pink        e/E visitor mint
 *
 * CHIBI STYLE RULES used here:
 *   - oversized head and face: roughly half the sprite height
 *   - strong near-black outline and bright, flat color blocks
 *   - single-pixel eyes/features with one deliberate shadow ramp
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

/* ---- HERO: compact rural adventure-game proportions. -------------------- */
static const char *SPR_ART_FARMER_DOWN_0[16] = {
    "................",
    ".....00000......",
    "...00ccccc00....",
    "..0ccCccCccc0...",
    "..0c000000cc0...",
    "..0csssssscc0...",
    "..0cs0ss0sc0....",
    "...0sssSsss0....",
    "....0ssss0......",
    "...0rryyrr0.....",
    "..0sr0ii0rs0....",
    "...0iIiIi0......",
    "...0iiiiii0.....",
    "...0i0..0i0.....",
    "..0DD0..0DD0....",
    "...00....00.....",
};

static const char *SPR_ART_FARMER_DOWN_1[16] = {
    "................",
    ".....00000......",
    "...00ccccc00....",
    "..0ccCccCccc0...",
    "..0c000000cc0...",
    "..0csssssscc0...",
    "..0cs0ss0sc0....",
    "...0sssSsss0....",
    "....0ssss0......",
    "...0rryyrr0.....",
    "..0sr0ii0rs0....",
    "...0iIiIi0......",
    "...0iiiiii0.....",
    "....0i0.0i0.....",
    "...0DD0.0i0.....",
    ".........0DD0...",
};

static const char *SPR_ART_FARMER_DOWN_2[16] = {
    "................",
    ".....00000......",
    "...00ccccc00....",
    "..0ccCccCccc0...",
    "..0c000000cc0...",
    "..0csssssscc0...",
    "..0cs0ss0sc0....",
    "...0sssSsss0....",
    "....0ssss0......",
    "...0rryyrr0.....",
    "..0sr0ii0rs0....",
    "...0iIiIi0......",
    "...0iiiiii0.....",
    "....0i0.0i0.....",
    "...0i0.0DD0.....",
    "..0DD0..........",
};

static const char *SPR_ART_FARMER_UP_0[16] = {
    "................",
    ".....00000......",
    "...00ccccc00....",
    "..0ccCccCccc0...",
    "..0c000000cc0...",
    "..0cccccccc0....",
    "...0cCCCCc0.....",
    "...0rryyrr0.....",
    "..0rri00irr0....",
    "..0sri00irs0....",
    "...0ii00ii0.....",
    "...0iIiiIi0.....",
    "...0iiiiii0.....",
    "...0i0..0i0.....",
    "..0DD0..0DD0....",
    "...00....00.....",
};

static const char *SPR_ART_FARMER_UP_1[16] = {
    "................",
    ".....00000......",
    "...00ccccc00....",
    "..0ccCccCccc0...",
    "..0c000000cc0...",
    "..0cccccccc0....",
    "...0cCCCCc0.....",
    "...0rryyrr0.....",
    "..0rri00irr0....",
    "..0sri00irs0....",
    "...0ii00ii0.....",
    "...0iIiiIi0.....",
    "...0iiiiii0.....",
    "....0i0.0i0.....",
    "...0DD0.0i0.....",
    ".........0DD0...",
};

static const char *SPR_ART_FARMER_UP_2[16] = {
    "................",
    ".....00000......",
    "...00ccccc00....",
    "..0ccCccCccc0...",
    "..0c000000cc0...",
    "..0cccccccc0....",
    "...0cCCCCc0.....",
    "...0rryyrr0.....",
    "..0rri00irr0....",
    "..0sri00irs0....",
    "...0ii00ii0.....",
    "...0iIiiIi0.....",
    "...0iiiiii0.....",
    "....0i0.0i0.....",
    "...0i0.0DD0.....",
    "..0DD0..........",
};

static const char *SPR_ART_FARMER_SIDE_0[16] = {
    "................",
    "......0000......",
    "....00cccc00....",
    "...0ccCccccc0...",
    ".000c000000c0...",
    "0CC0cssssssc0...",
    "...0cs0ssssc0...",
    "....0sssSss0....",
    ".....0ssss0.....",
    "...0rryyrr0.....",
    "..0sr0iiiirr0...",
    "...0s0iIiIi0....",
    "....0iiiiii0....",
    "....0i0.0i0.....",
    "...0DD0.0DD0....",
    "....00...00.....",
};

static const char *SPR_ART_FARMER_SIDE_1[16] = {
    "................",
    "......0000......",
    "....00cccc00....",
    "...0ccCccccc0...",
    ".000c000000c0...",
    "0CC0cssssssc0...",
    "...0cs0ssssc0...",
    "....0sssSss0....",
    ".....0ssss0.....",
    "...0rryyrr0.....",
    "...0r0iiiirr0...",
    "...0s0iIiIi0....",
    "...0iii0iiii0...",
    "..0iii0..0iii0..",
    ".0DD0....0DD0...",
    "..00......00....",
};

static const char *SPR_ART_FARMER_SIDE_2[16] = {
    "................",
    "......0000......",
    "....00cccc00....",
    "...0ccCccccc0...",
    ".000c000000c0...",
    "0CC0cssssssc0...",
    "...0cs0ssssc0...",
    "....0sssSss0....",
    ".....0ssss0.....",
    "...0rryyrr0.....",
    "..0sr0iiiirr0...",
    "...0s0iIiIi0....",
    "....0iiiiii0....",
    ".....0iii0......",
    "....0i0DD0......",
    ".....00.........",
};

/* ---- GIANT ANTS: mandibles, antennae, segmented bodies and six legs. ---- */
static const char *SPR_ART_ALIEN_0[16] = {
    "..0..........0..",
    "...0..0000..0...",
    "....00RRRR00....",
    "...0RrRRRRrR0...",
    "..0RR000000RR0..",
    "..0R0m0RR0m0R0..",
    "...0RRR00RRR0...",
    "....00rrrr00....",
    "..00..0RR0..00..",
    ".0RR0.0RR0.0RR0.",
    "0RR0..0RR0..0RR0",
    ".....0RRRR0.....",
    "....0R0RR0R0....",
    "...0R0.00.0R0...",
    "..00......00....",
    "................",
};

static const char *SPR_ART_ALIEN_1[16] = {
    ".0............0.",
    "..0....00....0..",
    "...0.0RRRR0.0...",
    "....0RrRRrR0....",
    "...0RR0000RR0...",
    "..0RR0m00m0RR0..",
    "...0RRR00RRR0...",
    "..00.0rrrr0.00..",
    ".0RR00RRRR00RR0.",
    "0RR0.0RRRR0.0RR0",
    "....0RR00RR0....",
    "...0RRRRRRRR0...",
    "..0RR0RRRR0RR0..",
    ".0R0..0RR0..0R0.",
    "00....0..0....00",
    "................",
};

/* ---- INDIVIDUAL NPC LOOKS: each pair is a subtle idle sway. ------------- */
static const char *SPR_ART_NPC[16] = {
    "................",
    "....00mmmm0.....",
    "...0mmmmmmm0....",
    "..0mMmmmmMmm0...",
    "..0m0000000m0...",
    "..0msssssssm0...",
    "..0ms0ss0ssm0...",
    "...0sssSsss0....",
    "....0ssss0......",
    "...0iiiiii0.....",
    "..0si0II0is0....",
    "...0iiiiii0.....",
    "...0i0..0i0.....",
    "...0i0..0i0.....",
    "..0mm0..0mm0....",
    "...00....00.....",
};

static const char *SPR_ART_NPC_1[16] = {
    "................",
    ".....00mmmm0....",
    "....0mmmmmmm0...",
    "...0mMmmmmMmm0..",
    "...0m0000000m0..",
    "...0msssssssm0..",
    "...0ms0ss0ssm0..",
    "....0sssSsss0...",
    ".....0ssss0.....",
    "....0iiiiii0....",
    "...0si0II0is0...",
    "....0iiiiii0....",
    "....0i0..0i0....",
    "....0i0..0i0....",
    "...0mm0..0mm0...",
    "....00....00....",
};

static const char *SPR_ART_SKEPTIC[16] = {
    "................",
    "....00000000....",
    "...0yyyyyyyy0...",
    "..000YYYYYY000..",
    "...0HssssssH0...",
    "...0s0ssss0s0...",
    "...0ssS00Sss0...",
    "....0HHHHHH0....",
    ".....0ssss0.....",
    "...0kkkkkk0.....",
    "..0sk0KK0ks0....",
    "...0kkkkkk0.....",
    "...0K0..0K0.....",
    "...0K0..0K0.....",
    "..0DD0..0DD0....",
    "...00....00.....",
};

static const char *SPR_ART_SKEPTIC_1[16] = {
    "................",
    ".....00000000...",
    "....0yyyyyyyy0..",
    "...000YYYYYY000.",
    "....0HssssssH0..",
    "....0s0ssss0s0..",
    "....0ssS00Sss0..",
    ".....0HHHHHH0...",
    "......0ssss0....",
    "....0kkkkkk0....",
    "...0sk0KK0ks0...",
    "....0kkkkkk0....",
    "....0K0..0K0....",
    "....0K0..0K0....",
    "...0DD0..0DD0...",
    "....00....00....",
};

static const char *SPR_ART_ELDER[16] = {
    "................",
    "....00kkkk00....",
    "...0kKssssKk0...",
    "..0kssssssssk0..",
    "..0ss0ssss0ss0..",
    "..0sssS00Ssss0..",
    "...0ss0000ss0...",
    "...0kHHHHHHk0...",
    "....0kkkkkk0....",
    "...0RrRrrRr0....",
    "..0sr0bb0rs0....",
    "...0bBbbBb0.....",
    "...0bbbbbb0.....",
    "...0b0..0b0.....",
    "..0DD0..0DD0....",
    "...00....00.....",
};

static const char *SPR_ART_ELDER_1[16] = {
    "................",
    ".....00kkkk00...",
    "....0kKssssKk0..",
    "...0kssssssssk0.",
    "...0ss0ssss0ss0.",
    "...0sssS00Ssss0.",
    "....0ss0000ss0..",
    "....0kHHHHHHk0..",
    ".....0kkkkkk0...",
    "....0RrRrrRr0...",
    "...0sr0bb0rs0...",
    "....0bBbbBb0....",
    "....0bbbbbb0....",
    "....0b0..0b0....",
    "...0DD0..0DD0...",
    "....00....00....",
};

static const char *SPR_ART_MA[16] = {
    "................",
    "......000.......",
    "....00kkkk0.....",
    "...0kKkkkkk0....",
    "..0Kssssssss0...",
    "..0ss0ssss0ss0..",
    "..0sssS00Ssss0..",
    "...0ss0kk0ss0...",
    "....0kkkkkk0....",
    "...0ppPpppp0....",
    "..0sp0iiii0s0...",
    "...0iIiiIi0.....",
    "...0iiiiii0.....",
    "...0i0..0i0.....",
    "..0DD0..0DD0....",
    "...00....00.....",
};

static const char *SPR_ART_MA_1[16] = {
    "................",
    ".......000......",
    ".....00kkkk0....",
    "....0kKkkkkk0...",
    "...0Kssssssss0..",
    "...0ss0ssss0ss0.",
    "...0sssS00Ssss0.",
    "....0ss0kk0ss0..",
    ".....0kkkkkk0...",
    "....0ppPpppp0...",
    "...0sp0iiii0s0..",
    "....0iIiiIi0....",
    "....0iiiiii0....",
    "....0i0..0i0....",
    "...0DD0..0DD0...",
    "....00....00....",
};

static const char *SPR_ART_NEIGHBOR[16] = {
    "................",
    ".....000000.....",
    "....0kkkkkk0....",
    "...0kKkkkkKk0...",
    "..0K0ssssss0K0..",
    "..0ss0ssss0ss0..",
    "..0sssSssSsss0..",
    "...0ss0000ss0...",
    "....0ssssss0....",
    "...0pPppppP0....",
    "..0sp0pppp0s0...",
    "...0ppPPpp0.....",
    "...0pppppp0.....",
    "...0p0..0p0.....",
    "..0DD0..0DD0....",
    "...00....00.....",
};

static const char *SPR_ART_NEIGHBOR_1[16] = {
    "................",
    "......000000....",
    ".....0kkkkkk0...",
    "....0kKkkkkKk0..",
    "...0K0ssssss0K0.",
    "...0ss0ssss0ss0.",
    "...0sssSssSsss0.",
    "....0ss0000ss0..",
    ".....0ssssss0...",
    "....0pPppppP0...",
    "...0sp0pppp0s0..",
    "....0ppPPpp0....",
    "....0pppppp0....",
    "....0p0..0p0....",
    "...0DD0..0DD0...",
    "....00....00....",
};

static const char *SPR_ART_STOREKEEP[16] = {
    "................",
    ".....000000.....",
    "....0ssssss0....",
    "...0ssSssSss0...",
    "..0ss000000ss0..",
    "..0s010ss010s0..",
    "..0ss00ss00ss0..",
    "...0sss00sss0...",
    "....0HHHHHH0....",
    "...0oOooooO0....",
    "..0so0kkkk0s0...",
    "...0kKkkKk0.....",
    "...0kkkkkk0.....",
    "...0K0..0K0.....",
    "..0DD0..0DD0....",
    "...00....00.....",
};

static const char *SPR_ART_STOREKEEP_1[16] = {
    "................",
    "......000000....",
    ".....0ssssss0...",
    "....0ssSssSss0..",
    "...0ss000000ss0.",
    "...0s010ss010s0.",
    "...0ss00ss00ss0.",
    "....0sss00sss0..",
    ".....0HHHHHH0...",
    "....0oOooooO0...",
    "...0so0kkkk0s0..",
    "....0kKkkKk0....",
    "....0kkkkkk0....",
    "....0K0..0K0....",
    "...0DD0..0DD0...",
    "....00....00....",
};

/* ---- ITEMS: bold outlines and simple icon-like color blocks. ------------ */
static const char *SPR_ART_ITEM_HERB[16] = {
    "................",
    "................",
    "................",
    ".......0........",
    "....0.0g0.0.....",
    "...0g00g00g0....",
    "...0gg0g0gg0....",
    "....0ggggg0.....",
    ".....0gGg0......",
    ".....0gGg0......",
    "....00gG00......",
    "...0ggGGgg0.....",
    "..0gggGGggg0....",
    "...00000000.....",
    "................",
    "................",
};

static const char *SPR_ART_ITEM_MEDKIT[16] = {
    "................",
    "................",
    "................",
    ".....000000.....",
    "....01111110....",
    "...0111111110...",
    "...0111rr1110...",
    "...01rrrrr110...",
    "...01rrrrr110...",
    "...0111rr1110...",
    "...0111111110...",
    "...0KKKKKKKK0...",
    "....00000000....",
    "................",
    "................",
    "................",
};

static const char *SPR_ART_ITEM_SHELLS[16] = {
    "................",
    "................",
    "................",
    "....0y0y0y0y....",
    "....0r0r0r0r....",
    "...0000000000...",
    "..0RRRRRRRRRR0..",
    "..0RrrrrrrrrR0..",
    "..0RrYrYrYrRr0..",
    "..0RrrrrrrrrR0..",
    "..0RRRRRRRRRR0..",
    "...0000000000...",
    "................",
    "................",
    "................",
    "................",
};

static const char *SPR_ART_ITEM_SHOTGUN[16] = {
    "................",
    "................",
    "............00..",
    "...........0kk0.",
    "..........0kk0..",
    ".........0kk0...",
    "........0kk0....",
    ".......0kk0.....",
    "......0kk0......",
    ".....0KK0.......",
    "....0dd0........",
    "...0ddd0........",
    "..0ddD0.........",
    "..0dD0..........",
    "...00...........",
    "................",
};

static const char *SPR_ART_GUN_AIM[16] = {
    "...........00...",
    "..........0kk0..",
    ".........0kk0...",
    "........0kk0....",
    ".......0kk0.....",
    "......0kk0......",
    ".....0kk0.......",
    "....0kK0........",
    "...0dd0.........",
    "..0ddd0.........",
    ".0ddD0..........",
    ".0dD0...........",
    ".00.............",
    "................",
    "................",
    "................",
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
