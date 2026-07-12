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

/* ---- THE GIANT ANTS ------------------------------------------------------
 * Head, thorax, abdomen; six fanned legs; two antennae. FOUR DIRECTIONS,
 * two frames each (the legs alternate, so they scuttle):
 *
 *   DOWN_0 DOWN_1  UP_0 UP_1  SIDE_0 SIDE_1     <- SIX CONSECUTIVE ids
 *
 * The renderer indexes them as spr0 + offset, so KEEP THEM IN THIS ORDER
 * (see the `dirs` flag in species[], assets.c). Left-facing side art is
 * mirrored for right, same trick as the farmer.
 */
static const char *SPR_ART_ANT_DOWN_0[16] = {
    "......0000......",
    ".....0RRRR0.....",
    ".0..0RRrrRR0..0.",
    "..0.0RRRRRR0.0..",
    "...00RRRRRR00...",
    "..0.00RRRR00.0..",
    "..0..0RRRR0..0..",
    "..0000RRRR0000..",
    "00..00RRRR00..00",
    "....00000000....",
    "...0.0rRRr0.0...",
    "..0.0R0RR0R0.0..",
    "..0.0RRRRRR0.0..",
    ".0...0RRRR0...0.",
    "....0.0000.0....",
    "....0......0....",
};

static const char *SPR_ART_ANT_DOWN_1[16] = {
    "......0000......",
    ".....0RRRR0.....",
    "....0RRrrRR0....",
    ".0..0RRRRRR0..0.",
    "..0.0RRRRRR0.0..",
    "..0000RRRR0000..",
    "..00.0RRRR0.00..",
    "00..00RRRR00..00",
    "....00RRRR00....",
    "....00000000....",
    "....00rRRr00....",
    "...00R0RR0R00...",
    "..0.0RRRRRR0.0..",
    "..0..0RRRR0..0..",
    ".0..0.0000.0..0.",
    "....0......0....",
};

static const char *SPR_ART_ANT_UP_0[16] = {
    "..0.0.0000.0.0..",
    "..0..0RRRR0..0..",
    ".0.00RRRRRR00.0.",
    "..0.0RRRRRR0.0..",
    "...000RRRR000...",
    "....00RRRR00....",
    ".....0RRRR0.....",
    "..0000RRRR0000..",
    "00...0RRRR0...00",
    "....0.0000.0....",
    "...0.0RRRR0.0...",
    "..0.0RRRRRR0.0..",
    "..0.0RRRRRR0.0..",
    ".0..0RRrrRR0..0.",
    ".....0RRRR0.....",
    "......0000......",
};

static const char *SPR_ART_ANT_UP_1[16] = {
    "..0.0.0000.0.0..",
    "...0.0RRRR0.0...",
    "....0RRRRRR0....",
    ".0..0RRRRRR0..0.",
    "..0.00RRRR00.0..",
    "...000RRRR000...",
    "..00.0RRRR0.00..",
    "00..00RRRR00..00",
    ".....0RRRR0.....",
    "....0.0000.0....",
    "....00RRRR00....",
    "...00RRRRRR00...",
    "..0.0RRRRRR0.0..",
    "..0.0RRrrRR0.0..",
    ".0...0RRRR0...0.",
    "......0000......",
};

static const char *SPR_ART_ANT_SIDE_0[16] = {
    "................",
    ".0..............",
    ".0..............",
    "..0.........0...",
    "..0.......00R00.",
    "00.000...0RRRRR0",
    "..0RrR000RRRRRRR",
    ".0R0RRRRRRRRRRRR",
    ".0RRRRRRRRRRRRR0",
    "..0RRR0RRR00R00.",
    "00.00000000.0...",
    ".....0.0.0......",
    ".....0.0.0......",
    ".....0.0.0......",
    "....0.0.0.......",
    "....0.0.0.......",
};

static const char *SPR_ART_ANT_SIDE_1[16] = {
    ".0..............",
    ".0..............",
    "..0.............",
    "..0.........0...",
    "...0......00R00.",
    "00.000...0RRRRR0",
    "..0RrR000RRRRRRR",
    ".0R0RRRRRRRRRRRR",
    ".0RRRRRRRRRRRRR0",
    "..0RRR0RRR00R00.",
    "00.00000000.0...",
    "......00..0.....",
    ".....0.0.0......",
    ".....00..0......",
    ".....00..0......",
    "....0...0.......",
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
    ".....000000.....",
    "....0kkkkkk0....",
    "...0kkkkkkkk0...",
    "...0kssssssk0...",
    "...0ks0ss0sk0...",
    "...0kssSSssk0...",
    "...0kssssssk0...",
    "....0ssssss0....",
    "...0pppppppp0...",
    "..0spp1111pps0..",
    "..0spp1111pps0..",
    "...0pp1111pp0...",
    "..0ppp1111ppp0..",
    ".0pppppppppppp0.",
    "..0DD0....0DD0..",
};

static const char *SPR_ART_MA_1[16] = {
    "................",
    "................",
    ".....000000.....",
    "....0kkkkkk0....",
    "...0kkkkkkkk0...",
    "...0kssssssk0...",
    "...0ks0ss0sk0...",
    "...0kssSSssk0...",
    "...0kssssssk0...",
    "....0ssssss0....",
    "...0pppppppp0...",
    "..0spp1111pps0..",
    "...0pp1111pp0...",
    "..0ppp1111ppp0..",
    ".0pppppppppppp0.",
    "..0DD0....0DD0..",
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


/* a battered tin flashlight, lens up */
static const char *SPR_ART_ITEM_FLASHLIGHT[16] = {
    "................",
    "................",
    "................",
    "................",
    ".....000000.....",
    "....0yyyyyy0....",
    "....0y1111y0....",
    "....0yy11yy0....",
    ".....0kkkk0.....",
    ".....0kKKk0.....",
    ".....0kkkk0.....",
    ".....0kKKk0.....",
    ".....0kkkk0.....",
    ".....0KKKK0.....",
    "......0000......",
    "................",
};

/* ---- THE HOPKINSVILLE GOBLIN -- the boss on the north road. --------------
 * Kelly, Kentucky, 1955: silver, self-lit, no neck, an egg of a head, huge
 * yellow lamp eyes, and those enormous floppy ears. It does not walk so
 * much as arrive. Shotguns reportedly did nothing. We'll see.
 */
static const char *SPR_ART_BOSS_0[16] = {
    ".0VV0.0000.0VV0.",
    "0VVV00vvvv00VVV0",
    "VVVV01yvv1y0VVVV",
    "0VVV0yyvvyy0VVV0",
    ".0VV0yyvvyy0VV0.",
    "..000vv00vv000..",
    ".....0vvvv0.....",
    "....0vvvvvv0....",
    "...0v0vvvv0v0...",
    "...0v0vvvv0v0...",
    "...0v0vvvv0v0...",
    "...0v0vvvv0v0...",
    "...0v0v00v0v0...",
    "..0v00v00v00v0..",
    "..0v00v00v00v0..",
    "...0.0v00v0.0...",
};

static const char *SPR_ART_BOSS_1[16] = {
    ".0VV0.0000.0VV0.",
    "0VVV00vvvv00VVV0",
    "VVVV01yvv1y0VVVV",
    "0VVV0yyvvyy0VVV0",
    ".0VV0yyvvyy0VV0.",
    "..000vv00vv000..",
    ".....0vvvv0.....",
    "....0vvvvvv0....",
    "...0v0vvvv0v0...",
    "...0v0vvvv0v0...",
    "...0v0vvvv0v0...",
    "...0v0vvvv0v0...",
    "...0v0v00v0v0...",
    "..0v00v00v00v0..",
    "...0.0v00v0.0...",
    ".....0v00v0.....",
};

/* ---- THE SHOTGUN, HELD AT THE READY -- worn over the shoulder, seen from
 * behind. Overlaid on the hero in battle once he owns it (battle.c). */
static const char *SPR_ART_GUN_READY[16] = {
    "..............0.",
    ".............0k0",
    "............0kk0",
    "...........0kk0.",
    "..........0kk0..",
    ".........0kk0...",
    "........0kk0....",
    ".......0kk0.....",
    "......0KK0......",
    ".....0dd0.......",
    "....0dDd0.......",
    "....0dD0........",
    "...0DD0.........",
    "...00...........",
    "................",
    "................",
};

/* ---- THE ANIMALS ---------------------------------------------------------
 * Two frames each: a small idle motion (a chewing jaw, a wagging tail, a
 * twitching ear). Nothing on the farm is in a hurry. They're ENT_NPCs, so
 * they're solid, they breathe, and they talk when you face them and press A.
 */

/* COW -- side on. White with black patches, pink nose and udder,
 * a horn and a flicking tail. You cannot mistake it for anything else. */
static const char *SPR_ART_COW[16] = {
    "................",
    "................",
    "..............0.",
    "..00.........010",
    ".011000000000001",
    "0110111111111111",
    "0111110001111110",
    "0110110001111110",
    "1111110001000110",
    "1mm111111100010.",
    "111111111111110.",
    "000001100mm110..",
    "....0110.00110..",
    "....0110..0110..",
    "....0DD0..0DD0..",
    ".....00....00...",
};

static const char *SPR_ART_COW_1[16] = {
    "................",
    "................",
    "..............00",
    "..00.........011",
    ".011000000000001",
    "0110111111111111",
    "0111110001111110",
    "0110110001111110",
    "1111110001000110",
    "1mm111111100010.",
    "111111111111110.",
    "000001100mm010..",
    "....0110.00010..",
    "....0110...010..",
    "....0DD0...0D0..",
    ".....00.....0...",
};

/* GOAT -- side on. Swept-back horns, a white beard, stubby tail. */
static const char *SPR_ART_GOAT[16] = {
    "................",
    ".....00.........",
    "....0yy0........",
    "...0y00y0.......",
    "..00y00y0.....0.",
    ".0kkkkk0000000k0",
    ".0kk0kkkkkkkkkk0",
    "0kkkkkkkkkkkkkk0",
    "0kkkkkkkkkkkkk0.",
    "0kkkkkkkkkkkkk0.",
    ".0110kkkkkkkkk0.",
    ".01100kk000kk0..",
    "..00.0kk0.0kk0..",
    ".....0kk0.0kk0..",
    ".....0KK0.0KK0..",
    "......00...00...",
};

static const char *SPR_ART_GOAT_1[16] = {
    "................",
    ".....00.........",
    "....0yy0........",
    "...0y00y0.....0.",
    "..00y00y0....0k0",
    ".0kkkkk0000000k0",
    ".0kk0kkkkkkkkkk0",
    "0kkkkkkkkkkkkkk0",
    "0kkkkkkkkkkkkk0.",
    "0kkkkkkkkkkkkk0.",
    ".0110kkkkkkkkk0.",
    ".01100kk0000kk0.",
    "..00.0kk0..0kk0.",
    ".....0kk0..0kk0.",
    ".....0KK0..0KK0.",
    "......00....00..",
};

/* DOG -- side on. Perky ear, black nose, and a tail that WAGS. */
static const char *SPR_ART_DOG[16] = {
    "................",
    "................",
    "....00........0.",
    "...0dd0......0d0",
    ".000dd0.....0d0.",
    "0ddddd0.....0d0.",
    "0dd0dd0000000d0.",
    "ddddddddddddd0..",
    "0dddddddddddd0..",
    "ddddddddddddd0..",
    "0000ddddddddd0..",
    "...0ddddddddd0..",
    "....0dd000dd0...",
    "....0dd0.0dd0...",
    "....0DD0.0DD0...",
    ".....00...00....",
};

static const char *SPR_ART_DOG_1[16] = {
    "................",
    "................",
    "....00..........",
    "...0dd0........0",
    ".000dd0......00d",
    "0ddddd0.....0dd0",
    "0dd0dd0000000d0.",
    "ddddddddddddd0..",
    "0dddddddddddd0..",
    "ddddddddddddd0..",
    "0000ddddddddd0..",
    "...0ddddddddd0..",
    "....0dd0000dd0..",
    "....0dd0..0dd0..",
    "....0DD0..0DD0..",
    ".....00....00...",
};

/* CAT -- side on. Pointed ears, tabby stripes, long curled-up tail. */
static const char *SPR_ART_CAT[16] = {
    "................",
    "................",
    "................",
    "..0..0..........",
    ".0o00o0....00...",
    "0oo00oo0..0oo0..",
    "0ooooo0....00o0.",
    "0oo0oo0000000o0.",
    "0ooooooooooo0o0.",
    ".0oOoooOoooo0o0.",
    "0ooooooooOooo0..",
    ".000oooooooo0...",
    "....0oo00oo0....",
    "....0oo00oo0....",
    "....0DD00DD0....",
    ".....00..00.....",
};

static const char *SPR_ART_CAT_1[16] = {
    "................",
    "................",
    "................",
    "..0..0.....00...",
    ".0o00o0...0oo0..",
    "0oo00oo0.0o000..",
    "0ooooo0...0.0o0.",
    "0oo0oo0000000o0.",
    "0ooooooooooo0o0.",
    ".0oOoooOoooo0o0.",
    "0ooooooooOooo0..",
    ".000oooooooo0...",
    "....0oo000oo0...",
    "....0oo0.0oo0...",
    "....0DD0.0DD0...",
    ".....00...00....",
};

/* ---- THE UFO -- the letter-picker cursor on the name screen. ------------
 * It hovers over the letter you're on. Lights blink via the two frames. */
static const char *SPR_ART_UFO[16] = {
    "................",
    "................",
    ".....000000.....",
    "....0eeeeee0....",
    "...0e111111e0...",
    "...0e1eeee1e0...",
    "..0eeeeeeeeee0..",
    ".0kkkkkkkkkkkk0.",
    "0kKyKkkKkkKyKkk0",
    "0kkkkkkkkkkkkkk0",
    ".0KKKKKKKKKKKK0.",
    "..00KKKKKKKK00..",
    "....0y0..0y0....",
    ".....0....0.....",
    "................",
    "................",
};

static const char *SPR_ART_UFO_1[16] = {
    "................",
    "................",
    ".....000000.....",
    "....0eeeeee0....",
    "...0e111111e0...",
    "...0e1eeee1e0...",
    "..0eeeeeeeeee0..",
    ".0kkkkkkkkkkkk0.",
    "0kyKkkKyKkkKykk0",
    "0kkkkkkkkkkkkkk0",
    ".0KKKKKKKKKKKK0.",
    "..00KKKKKKKK00..",
    "...0y0....0y0...",
    "..0y0......0y0..",
    "................",
    "................",
};

/* ---- THE KEY -- brass, toothed. Opens what's been locked. --------------- */
static const char *SPR_ART_ITEM_KEY[16] = {
    "................",
    "................",
    "................",
    "................",
    "......0000......",
    ".....0yyyy0.....",
    ".....0y00y0.....",
    ".....0y00y0.....",
    "......0yy0......",
    ".......0y0......",
    ".......0y0......",
    ".......0yy0.....",
    ".......0y0Y0....",
    ".......0yy0.....",
    ".......000......",
    "................",
};

/* ---- THE VAN, FROM ABOVE ---------------------------------------------------
 * This is the one that sits in the car park -- 48x64, drawn at 1:1 so its
 * pixels are the same size as the world around it. Three tiles wide and four
 * long: big enough that walking up to the side of it and getting in is a
 * thing a man could plausibly do.
 *
 * The REAR view (van_big) is a different drawing and belongs to the highway
 * cutscene, where you are looking at the back of it. Do not mix them up --
 * I did, and put the arse-end of the van on the tarmac seen from the sky.
 */
static const char *SPR_ART_VANTOP[64] = {
    "................................................",
    "................................................",
    "................................................",
    "......000000000000000000000000000000000000......",
    "......0dkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkd0......",
    "......0dkyyyyykkkkkkkkkkkkkkkkkkkkyyyyykd0......",
    "......0dkyyyyykkkkkkkkkkkkkkkkkkkkyyyyykd0......",
    "......0dddddddddddddddddddddddddddddddddd0......",
    "......0dDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDd0......",
    "......0dDD0DDDDD0DDDDD0DDDDD0DDDDD0DDDDDd0......",
    "......0dDD0DDDDD0DDDDD0DDDDD0DDDDD0DDDDDd0......",
    "......0dDD0DDDDD0DDDDD0DDDDD0DDDDD0DDDDDd0......",
    "...0000dDD0DDDDD0DDDDD0DDDDD0DDDDD0DDDDDd0000...",
    "...0000dDD0DDDDD0DDDDD0DDDDD0DDDDD0DDDDDd0000...",
    "...0000dDD0DDDDD0DDDDD0DDDDD0DDDDD0DDDDDd0000...",
    "...0000dDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDd0000...",
    "...0000dddddddddddddddddddddddddddddddddd0000...",
    "...0000dd000000000000000000000000000000dd0000...",
    "...0000dd0BbbbbbbbbbbbbBBBBBBBBBBBBBBB0dd0000...",
    "...0000dd0BbbbbbbbbbbbbBBBBBBBBBBBBBBB0dd0000...",
    "...0000dd0BbbbbbbbbbbbbBBBBBBBBBBBBBBB0dd0000...",
    "...0000dd0BBBBBBBBBBBBBBBBBBBBBBBBBBBB0dd0000...",
    "...0000dd0BBBBBBBBBBBBBBBBBBBBBBBBBBBB0dd0000...",
    "......0dd0BBBBBBBBBBBBBBBBBBBBBBBBBBBB0dd0......",
    "......0dd000000000000000000000000000000dd0......",
    "......0dddddddddddddddddddddddddddddddddd0......",
    "......0dDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDd0......",
    "......0dDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDd0......",
    "......0dddddddddddddddddddddddddddddddddd0......",
    "......0dddddddddddddddddddddddddddddddddd0......",
    "......0dddddddddddddddddddddddddddddddddd0......",
    "......0dddddd0000000000000000000000dddddd0......",
    "......0dddddd0DDDDDDDDDDDDDDDDDDDD0dddddd0......",
    "......0dddddd0000000000000000000000dddddd0......",
    "......0dddddddddddddddddddddddddddddddddd0......",
    "......0dddddddddddddddddddddddddddddddddd0......",
    "......0dddddddddddddddddddddddddddddddddd0......",
    "......0ddDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDdd0......",
    "......0dddddddddddddddddddddddddddddddddd0......",
    "......0dddddddddddddddddddddddddddddddddd0......",
    "......0d00000000000000000000000000000000d0......",
    "......0drrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrd0......",
    "......0d00000000000000000000000000000000d0......",
    "...0000dddddddddddddddddddddddddddddddddd0000...",
    "...0000dddddddddddddddddddddddddddddddddd0000...",
    "...0000ddDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDdd0000...",
    "...0000dddddddddddddddddddddddddddddddddd0000...",
    "...0000dddddddddddddddddddddddddddddddddd0000...",
    "...0000dddddddddddddddddddddddddddddddddd0000...",
    "...0000dddddddddddddddddddddddddddddddddd0000...",
    "...0000dddddddddddddddddddddddddddddddddd0000...",
    "...0000dddddddddddddddddddddddddddddddddd0000...",
    "...0000dddddddddddddddddddddddddddddddddd0000...",
    "...0000dddddddddddddddddddddddddddddddddd0000...",
    "...0000ddd0000000000000000000000000000ddd0000...",
    "......0ddd0BBBBBBBBBBBB00BBBBBBBBBBBB0ddd0......",
    "......0ddd0BBBBBBBBBBBB00BBBBBBBBBBBB0ddd0......",
    "......0ddd0000000000000000000000000000ddd0......",
    "......0dddddddddddddddddddddddddddddddddd0......",
    "......0dkrrrrkkkkkkkkkkkkkkkkkkkkkkrrrrkd0......",
    "......0dkrrrrkkkkkkkkkkkkkkkkkkkkkkrrrrkd0......",
    "......000000000000000000000000000000000000......",
    "................................................",
    "................................................",
};

/* ---- THE TALL ONE ---------------------------------------------------------
 * Witnesses never agree on the face. They agree on two things: the HEIGHT,
 * and the ARMS -- they hang past the knees, and they are always the last
 * thing anybody remembers. The eyes are red and they do not blink.
 * It doesn't run at you. It is just closer every time you look.
 */
static const char *SPR_ART_TALL_0[16] = {
    "......0aa0......",
    ".....0aaaa0.....",
    ".....0raar0.....",   /* the eyes. they do not blink. */
    ".....0aaaa0.....",
    "......0aa0......",   /* the neck is too long as well */
    "...0..0aa0..0...",
    "..0a00aaaa00a0..",
    "..0a0aaaaaa0a0..",
    "..0a0aaaaaa0a0..",
    "..0a0aAaaAa0a0..",
    "..0a0aaaaaa0a0..",
    "..0a00aaaa00a0..",
    "..0a0.0aa0.0a0..",   /* the hands hang well past the hips */
    "...0..0aa0..0...",
    "......0aa0......",
    ".....0A00A0.....",
};
static const char *SPR_ART_TALL_1[16] = {
    "......0aa0......",
    ".....0aaaa0.....",
    ".....0raar0.....",
    ".....0aaaa0.....",
    "......0aa0......",
    "..0...0aa0...0..",
    ".0a000aaaa000a0.",
    ".0a0aaaaaaaa0a0.",
    "..0a0aaaaaa0a0..",
    "..0a0aAaaAa0a0..",
    "...0aaaaaaaa0...",
    "...0a0aaaa0a0...",
    "...0a00aa00a0...",
    "....0.0aa0.0....",
    "......0aa0......",
    ".....0A00A0.....",
};

/* ---- THE ANT HILL ---------------------------------------------------------
 * A mound of turned earth with a hole in it. It does not move. Things come
 * out of it. Frame 1 has more of them out, and the hole is wider.
 */
static const char *SPR_ART_ANTHILL_0[16] = {
    "................",
    "................",
    "......0000......",
    ".....0DDDD0.....",
    "....0DDddDD0....",
    "...0DD0000DD0...",
    "...0Dd000000....",
    "..0DDd00000d0...",
    "..0DDDd000dDD0..",
    ".0DDdDDddDDdDD0.",
    ".0DdDDDDDDDDdD0.",
    "0DDDDdDDDDdDDDD0",
    "0DdDDDDDDDDDDdD0",
    "0DDDDDDDDDDDDDD0",
    ".00000000000000.",
    "................",
};
static const char *SPR_ART_ANTHILL_1[16] = {
    "................",
    "...0R0......0R0.",
    "..0R0.0000...R0.",
    "...0.0DDDD0.....",
    "....0DDddDD0....",
    "...0DD0000DD0...",
    "..0Dd0000000R0..",
    "..0DDd00000dR0..",
    ".0RDDDd000dDD0..",
    ".0DDdDDddDDdDD0.",
    "0RDdDDDDDDDDdD0.",
    "0DDDDdDDDDdDDDD0",
    "0DdDDDDDDDDDDdR0",
    "0DDDDDDDDDDDDDD0",
    ".00000000000000.",
    "..0R0.......0R0.",
};

/* ---- THE MUTANT QUEEN -----------------------------------------------------
 * Whatever the lights did to the ants, they did it to her first and they
 * did it worst. She is bloated, she is wet, and she has WINGS -- ragged,
 * useless-looking, and she is still faster than you. The pale 'y' is the
 * egg sac. It moves on its own.
 */
static const char *SPR_ART_QUEEN_0[16] = {
    "..0..0RRRR0..0..",
    "...00RRrrRR00...",
    "..0R0R1RR1R0R0..",
    ".0RR0RRRRRR0RR0.",
    "0aRR00RRRR00RRa0",
    "0aaR0.0RR0.0Raa0",
    ".0aa00RRRR00aa0.",
    "..0a0RRRRRR0a0..",
    "...0RRrRRrRR0...",
    "..0RyyRRRRyyR0..",
    ".0RyyyyRRyyyyR0.",
    ".0RyyyyyyyyyyR0.",
    "..0RyyyyyyyyR0..",
    "...0RRyyyyRR0...",
    "....0RR00RR0....",
    ".....0000000....",
};
static const char *SPR_ART_QUEEN_1[16] = {
    "0.0..0RRRR0..0.0",
    ".0.00RRrrRR00.0.",
    "..0R01RRRR1R0R..",
    "0aa0RRRRRRRR0aa0",
    "0aaR00RRRR00Raa0",
    ".0aaR.0RR0.Raa0.",
    "..0a00RRRR00a0..",
    "...00RRRRRR00...",
    "...0RRrRRrRR0...",
    "..0RyyRRRRyyR0..",
    ".0RyyyyRRyyyyR0.",
    ".0Ryyy1yy1yyyR0.",
    "..0RyyyyyyyyR0..",
    "...0RRyyyyRR0...",
    "...0RR0..0RR0...",
    "..0.00....00.0..",
};

/* PA'S TNT. Three sticks, taped, with a fuse he cut short on purpose.
 * It was for stumps. */
static const char *SPR_ART_ITEM_TNT[16] = {
    "................",
    "..........0y0...",
    ".........0y0....",
    "........0y0.....",
    ".......0y0......",
    "......00000.....",
    "....0rrrrrrr0...",
    "....0rRrRrRr0...",
    "....0rrrrrrr0...",
    "....0KKKKKKK0...",
    "....0rrrrrrr0...",
    "....0rRrRrRr0...",
    "....0rrrrrrr0...",
    "....0KKKKKKK0...",
    "....000000000...",
    "................",
};

/* ============================ THE BESTIARY ================================
 * Two frames each. Non-directional: these things face you.
 * ==========================================================================*/

/* DOVER DEMON -- Dover, Massachusetts, 1977. A watermelon head on a
 * body like a coat-hanger, and two orange eyes that do not blink. */
static const char *SPR_ART_DOVER_0[16] = {
    ".......00.......",
    ".....00ss00.....",
    "....00000000....",
    "....0yo00oy0....",
    "....0oo00oo0....",
    "....0oo00oo0....",
    "....00000000....",
    ".....000s00.....",
    ".....0SSSS0.....",
    "....0SSSSSS0....",
    "...0S0SSSS0S0...",
    "...0S0SSSS0S0...",
    "..0S00S00S00S0..",
    "..0S0S0..0S0S0..",
    "...00S0..0S00...",
    "....0S0..0S0....",
};

static const char *SPR_ART_DOVER_1[16] = {
    ".......00.......",
    ".....00ss00.....",
    "....00000000....",
    "....0yo00oy0....",
    "....0oo00oo0....",
    "....0oo00oo0....",
    "....00000000....",
    ".....000s00.....",
    ".....0SSSS0.....",
    "....0SSSSSS0....",
    "...0S0SSSS0S0...",
    "...0S0SSSS0S0...",
    "..0S00S00S00S0..",
    "..0S0S0..0S0S0..",
    "..0S0S0..0S0S0..",
    "...00S0..0S00...",
};

/* MOTHMAN -- Point Pleasant, 1966. Man-shaped, winged, and its eyes
 * are not reflecting your light: they are MAKING it. */
static const char *SPR_ART_MOTH_0[16] = {
    "................",
    ".00....00....00.",
    "0KK00.0kk0.00KK0",
    "KKKKK1rkkr1KKKK0",
    "KKKKKrrkkrrKKKKK",
    "KKKKK0kkkk0KKKKK",
    "KKKKK0KKKK0KKKKK",
    "0KKKK0KKKK0KKKK0",
    ".00KK0KKKK0KK00.",
    "...000KKKK000...",
    ".....0KKKK0.....",
    ".....0KKKK0.....",
    ".....0K00K0.....",
    ".....0K00K0.....",
    ".....0K00K0.....",
    ".....0K00K0.....",
};

static const char *SPR_ART_MOTH_1[16] = {
    ".0............0.",
    "0K0....00....0K0",
    "KKK00.0kk0.00KK0",
    "0KKKK1rkkr1KKKK0",
    "KKKKKrrkkrrKKKKK",
    "KKKKK0kkkk0KKKKK",
    "KKKKK0KKKK0KKKKK",
    "KKKKK0KKKK0KKKKK",
    "00KKK0KKKK0KKK00",
    "..0000KKKK0000..",
    ".....0KKKK0.....",
    ".....0KKKK0.....",
    ".....0K00K0.....",
    ".....0K00K0.....",
    ".....0K00K0.....",
    ".....0K00K0.....",
};

/* EL CHUPACABRA -- spined, upright, and it drains what it catches. */
static const char *SPR_ART_CHUPA_0[16] = {
    "................",
    ".....0000.......",
    "....0GGGG0......",
    "..00GrGrGG0.....",
    ".0GGGGGGGGg0....",
    "..1G0GGGG00g0...",
    "...10GGGGGG0g0..",
    "...0GGGGGGGG0g0.",
    "..0G0GGGGGG0G0..",
    "..0G0GGGGGG0G0..",
    ".0G00GGGGGG00G0.",
    ".0G00GGGGGG00G0.",
    "..0..0G00G0..0..",
    "....0G0..0G0....",
    "....0G0..0G0....",
    "....0G0..0G0....",
};

static const char *SPR_ART_CHUPA_1[16] = {
    "................",
    ".....0000.......",
    "....0GGGG0......",
    "..00GrGrGG00....",
    ".0GGGGGGGG0g0...",
    "..1G0GGGG000g0..",
    "...10GGGGGG00g0.",
    "...0GGGGGGGG00g0",
    "..0G0GGGGGG0G00.",
    "..0G0GGGGGG0G0..",
    ".0G00GGGGGG00G0.",
    ".0G00GGGGGG00G0.",
    ".0G0.0G00G0.0G0.",
    "..0.0G0..0G0.0..",
    "....0G0..0G0....",
    "....0G0..0G0....",
};

/* THE GREY -- you already know what it looks like. Everyone does.
 * That's the part that keeps you up. */
static const char *SPR_ART_GREY_0[16] = {
    "......0000......",
    ".....0aaaa0.....",
    "....0aaaaaa0....",
    "....00aaaa00....",
    "...0000aa0000...",
    "....000aa000....",
    "....0aaAAaa0....",
    ".....0aaaa0.....",
    ".....0aaaa0.....",
    "....0aaaaaa0....",
    "...0a0aaaa0a0...",
    "...0a0aaaa0a0...",
    "..0a00a00a00a0..",
    "..0a00a00a00a0..",
    "...0.0a00a0.0...",
    ".....0a00a0.....",
};

static const char *SPR_ART_GREY_1[16] = {
    "......0000......",
    ".....0aaaa0.....",
    "....0aaaaaa0....",
    "....00aaaa00....",
    "...0000aa0000...",
    "....000aa000....",
    "....0aaAAaa0....",
    ".....0aaaa0.....",
    ".....0aaaa0.....",
    "....0aaaaaa0....",
    "...0a0aaaa0a0...",
    "...0a0aaaa0a0...",
    "..0a00a00a00a0..",
    "..0a00a00a00a0..",
    "..0a00a00a00a0..",
    "...0.0a00a0.0...",
};

/* REPTOID -- upright, scaled, patient. It has been here longer
 * than you have. */
static const char *SPR_ART_REPTOID_0[16] = {
    ".......00.......",
    "......0gg0......",
    ".....0gggg0.....",
    "...00yggggy0....",
    "..01g0gggg00....",
    "...010gggg0.....",
    ".....0ygy0y0....",
    "....0gGGGGg0....",
    "...0gGGGGGGg0...",
    "..0g0GGGGGG0g0..",
    "..0g0GGGGGGGg0..",
    ".0g00GGGGGG0GG0.",
    ".0g00G0000G00gG0",
    "..0.0G0..0G0.00G",
    "....0G0..0G0..0G",
    "....0G0..0G0...0",
};

static const char *SPR_ART_REPTOID_1[16] = {
    ".......00.......",
    "......0gg0......",
    ".....0gggg0.....",
    "...00yggggy0....",
    "..01g0gggg00....",
    "...010gggg0.....",
    ".....0ygy0y0....",
    "....0gGGGGg0....",
    "...0gGGGGGGg0...",
    "..0g0GGGGGG0g0..",
    "..0g0GGGGGGGg0..",
    ".0g00GGGGGG0GG0.",
    ".0g00G0000G00gG0",
    ".0g00G0..0G00g0G",
    "..0.0G0..0G0.0.0",
    "....0G0..0G0....",
};

/* THE LOCH NESS MONSTER -- a long neck, and far more underneath. */
static const char *SPR_ART_NESSIE_0[16] = {
    "................",
    "..000...........",
    ".0yGy0..........",
    "0GGGGG0.........",
    ".1GGG0..........",
    "..0GG0..........",
    "..0GG0..........",
    "..0GG0..........",
    ".00GG0..000...0.",
    "0W0GG000GGG000W0",
    ".00GGGGGGGGGGG0G",
    "..0GGGGGGGGGGGG0",
    "...0GGGGGGGGGG0.",
    "...0GGGGGGGGGG0.",
    "...0GGGGGGGGGG0.",
    "....0000000000..",
};

static const char *SPR_ART_NESSIE_1[16] = {
    "................",
    "..000...........",
    ".0yGy0..........",
    "0GGGGG0.........",
    ".1GGG0..........",
    "..0GG0..........",
    "..0GG0..........",
    "..0GG0..000.....",
    ".00GG000GGG00000",
    "0W0GGGGGGGGGGGWG",
    ".00GGGGGGGGGGGG0",
    "..0GGGGGGGGGGGG0",
    "...0GGGGGGGGGG0.",
    "...0GGGGGGGGGG0.",
    "...0GGGGGGGGGG0.",
    "....0000000000..",
};

/* SASQUATCH -- eight feet of shoulders and a very small, very
 * interested head. */
static const char *SPR_ART_SQUATCH_0[16] = {
    "................",
    "......0000......",
    ".....0DDDD0.....",
    "....00yDDy00....",
    "...0DDDddDDD0...",
    "..0DDDDDDDDDD0..",
    "..0DDDDDDDDDD0..",
    ".0DDDDDDDDDDDD0.",
    ".0DDDDDDDDDDDD0.",
    "0D0DDDDDDDDDD0D0",
    "0D0DDDDDDDDDD0D0",
    "0D0DdDDDdDDDd0D0",
    "0D000Dd00Dd000D0",
    "0D0.0DD00DD0.0D0",
    ".0..0DD00DD0..0.",
    "....0DD00DD0....",
};

static const char *SPR_ART_SQUATCH_1[16] = {
    "................",
    "......0000......",
    ".....0DDDD0.....",
    "....00yDDy00....",
    "...0DDDddDDD0...",
    "..0DDDDDDDDDD0..",
    "..0DDDDDDDDDD0..",
    ".0DDDDDDDDDDDD0.",
    ".0DDDDDDDDDDDD0.",
    "0D0DDDDDDDDDD0D0",
    "0D0DDDDDDDDDD0D0",
    "0D0DDDdDDDdDD0D0",
    "0D00dDD0dDD0d0D0",
    "0D0.0DD00DD000D0",
    "0D0.0DD00DD0.0D0",
    ".0..0DD00DD0..0.",
};

/* THE DOGMAN -- Michigan, 1887. It stands up. That's the wrong part. */
static const char *SPR_ART_DOGMAN_0[16] = {
    ".....00.00......",
    "....0DD0DD0.....",
    "....0DDDD0......",
    "..00DrDrDD0.....",
    ".0DDDDDDDD0.....",
    "..1D0DDDD00.....",
    "...11DDDDDD0....",
    "...0DDDDDDDD0..0",
    "..0D0DDDDDD0D00D",
    "..0D0DDDDDDDD0D0",
    ".0D00DDDDDD0DD0.",
    ".0D00DDDDDD00D0.",
    "..0..0D00D0..0..",
    "....0D0..0D0....",
    "....0D0..0D0....",
    "....0D0..0D0....",
};

static const char *SPR_ART_DOGMAN_1[16] = {
    ".....00.00......",
    "....0DD0DD0.....",
    "....0DDDD0......",
    "..00DrDrDD0.....",
    ".0DDDDDDDD0.....",
    "..1D0DDDD00.....",
    "...11DDDDDD0...0",
    "...0DDDDDDDD0.0D",
    "..0D0DDDDDD0D0D0",
    "..0D0DDDDDDDD0D0",
    ".0D00DDDDDD0DD0.",
    ".0D00DDDDDD00D0.",
    ".0D0.0D00D0.0D0.",
    "..0.0D0..0D0.0..",
    "....0D0..0D0....",
    "....0D0..0D0....",
};

/* ---- THE SHOTGUN, CARRIED -------------------------------------------------
 * Overlaid on the farmer in the OVERWORLD once he owns it -- one per
 * facing, drawn on top of his walk frames.
 *
 * Facing toward or away from the camera he can't point it at you, so he
 * carries it SLUNG ACROSS HIS CHEST (and across his back going away):
 * barrel up over one shoulder, stock down by the opposite hip. Sideways
 * he holds it out level, because that's the way he's aiming.
 */
static const char *SPR_ART_CARRY_DOWN[16] = {
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "..........0k0...",
    ".........0kk0...",
    "........0kk0....",
    ".......0kk0.....",
    "......0kk0......",
    ".....0dd0.......",
    "....0dD0........",
    "....0D0.........",
};
static const char *SPR_ART_CARRY_UP[16] = {
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "...0k0..........",
    "...0kk0.........",
    "....0kk0........",
    ".....0kk0.......",
    "......0kk0......",
    ".......0dd0.....",
    "........0dD0....",
    ".........0D0....",
};
static const char *SPR_ART_CARRY_SIDE[16] = {
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    ".0000000........",
    "0kkkkkkk0.......",
    "0kKKKKKk0.......",
    "0dddddDd0.......",
    ".0DDDDD0........",
    "..00000.........",
    "................",
    "................",
    "................",
};

/* ---- THE VAN -- 1980s, rust-brown, and it still starts. ------------------
 * TOP-DOWN, because the overworld is: windscreen at the nose, the long
 * roof, the rear window, wheels poking out at the sides. Frame 2 shivers
 * (it's idling).
 */
static const char *SPR_ART_VAN[16] = {
    "....0DDDDDD0....",
    "...0yBBBBBBy0...",
    "...0DBiiiBBD0...",
    "..0KDBBBBBBDK0..",
    "..0KDddddddDK0..",
    "..0KDddddddDK0..",
    "...0DDDDDDDD0...",
    "...0DDddddDD0...",
    "...0DDddddDD0...",
    "...0DDddddDD0...",
    "...0DDddddDD0...",
    "..0KDDDDDDDDK0..",
    "..0KDBBBBBBDK0..",
    "..0KDBBBBBBDK0..",
    "...0rDDDDDDr0...",
    "....00000000....",
};
static const char *SPR_ART_VAN_1[16] = {
    "....0DDDDDD0....",
    "...0yBBBBBBy0...",
    "...0DBWiiBBD0...",
    "..0KDBBBBBBDK0..",
    "..0KDddddddDK0..",
    "..0KDddddddDK0..",
    "...0DDDDDDDD0...",
    "...0DDddddDD0...",
    "...0DDddddDD0...",
    "...0DDddddDD0...",
    "...0DDddddDD0...",
    "..0KDDDDDDDDK0..",
    "..0KDBBBBBBDK0..",
    "..0KDBBBBBBDK0..",
    "...0RDDDDDDR0...",
    "....00000000....",
};

/* ---- THE VAN AGAIN, 32x32, SEEN FROM BEHIND -----------------------------
 * The whole driving cutscene is the back of this thing, so it gets four
 * times the pixels: rear window with the sky in it, door seam and handles,
 * a lit plate, a chrome bumper, fat tyres. Frame 2 is BRAKING -- the tail
 * lights flare, because he has just seen what is behind him.
 */
static const char *SPR_ART_VAN_BIG_0[32] = {
    "......00000..........00000......",
    ".....0KKKKK0000000000KKKKK0.....",
    "....0DDDDDDDDDDDDDDDDDDDDDD0....",
    "...0DDDDDDDDDDDDDDDDDDDDDDDD0...",
    "...0DDDDDDDDDDDDDDDDDDDDDDDD0...",
    "...0DDDD0000000000000000DDDD0...",
    "...0DDDD0BBBBBBBBBBBBBB0DDDD0...",
    "...0DDDD0BiiiiiiiBBBBBB0DDDD0...",
    "..0DDDDD0BiiiiiiiBBBBBB0DDDDD0..",
    "..0DDDDD0BWWWWBBBBBBBBB0DDDDD0..",
    "..0DDDDD0BBBBBBBBBBBBBB0DDDDD0..",
    "..0DDDDD0BBBBBBBBBBBBBB0DDDDD0..",
    "..0DDDDD0BBBBBBBBBBBBBB0DDDDD0..",
    "..0DDDDD0000000000000000DDDDD0..",
    "..0DDDDDDDDDDDD0DDDDDDDDDDDDD0..",
    "..0D00000DDDDDD0DDDDDDD00000D0..",
    "..0D0RRR0DDDDDD0DDDDDDD0RRR0D0..",
    "..0D0RrR0DDDDDD0DDDDDDD0RrR0D0..",
    "..0D0RRR0DDDkkk0DkkkDDD0RRR0D0..",
    "..0D0RRR0DDDDDD0DDDDDDD0RRR0D0..",
    "..0D0RRR0DDDDDD0DDDDDDD0RRR0D0..",
    "..0D00000DDDkkkkkkkkDDD00000D0..",
    "..0DDDDDDDDDk1K1K11kDDDDDDDDD0..",
    "..0DDDDDDDDDk1111K1kDDDDDDDDD0..",
    "..0DDDDDDDDDkkkkkkkkDDDDDDDDD0..",
    ".0kkkkkkkkkkkkkkkkkkkkkkkkkkkk0.",
    ".0KKKKKKKKKKKKKKKKKKKKKKKKKKKK0.",
    ".0k00000000kkkkkkkkkkk00000000..",
    "..00KKKKKK0000000000000KKKKKK0..",
    "...0Kk11kK0...........0Kk11kK0..",
    "...0KkkkkK0...........0KkkkkK0..",
    "...00000000...........00000000..",
};
static const char *SPR_ART_VAN_BIG_1[32] = {
    "......00000..........00000......",
    ".....0KKKKK0000000000KKKKK0.....",
    "....0DDDDDDDDDDDDDDDDDDDDDD0....",
    "...0DDDDDDDDDDDDDDDDDDDDDDDD0...",
    "...0DDDDDDDDDDDDDDDDDDDDDDDD0...",
    "...0DDDD0000000000000000DDDD0...",
    "...0DDDD0BBBBBBBBBBBBBB0DDDD0...",
    "...0DDDD0BiiiiiiiBBBBBB0DDDD0...",
    "..0DDDDD0BiiiiiiiBBBBBB0DDDDD0..",
    "..0DDDDD0BWWWWBBBBBBBBB0DDDDD0..",
    "..0DDDDD0BBBBBBBBBBBBBB0DDDDD0..",
    "..0DDDDD0BBBBBBBBBBBBBB0DDDDD0..",
    "..0DDDDD0BBBBBBBBBBBBBB0DDDDD0..",
    "..0DDDDD0000000000000000DDDDD0..",
    "..0DDDDDDDDDDDD0DDDDDDDDDDDDD0..",
    "..0D00000DDDDDD0DDDDDDD00000D0..",
    "..0D0rrr0DDDDDD0DDDDDDD0rrr0D0..",
    "..0D0rmr0DDDDDD0DDDDDDD0rmr0D0..",
    "..0D0rmr0DDDkkk0DkkkDDD0rmr0D0..",
    "..0D0rmr0DDDDDD0DDDDDDD0rmr0D0..",
    "..0D0rrr0DDDDDD0DDDDDDD0rrr0D0..",
    "..0D00000DDDkkkkkkkkDDD00000D0..",
    "..0DDDDDDDDDk1K1K11kDDDDDDDDD0..",
    "..0DDDDDDDDDk1111K1kDDDDDDDDD0..",
    "..0DDDDDDDDDkkkkkkkkDDDDDDDDD0..",
    ".0kkkkkkkkkkkkkkkkkkkkkkkkkkkk0.",
    ".0KKKKKKKKKKKKKKKKKKKKKKKKKKKK0.",
    ".0k00000000kkkkkkkkkkk00000000..",
    "..00KKKKKK0000000000000KKKKKK0..",
    "...0Kk11kK0...........0Kk11kK0..",
    "...0KkkkkK0...........0KkkkkK0..",
    "...00000000...........00000000..",
};

/* ============================ PART 1: THE CITY =============================
 * THE LAWYER. The farmer's silhouette in a charcoal suit: same walk cycle,
 * same frame order, so player_sprite() only has to swap the base id.
 * Dark hair instead of the cap, white shirt, red tie, city shoes.
 */
static const char *SPR_ART_LAWYER_DOWN_0[16] = {
    "................",
    ".....00000......",
    "...00hhhhh00....",
    "..0hhHhhHhhh0...",
    "..0h000000hh0...",
    "..0hsssssshh0...",
    "..0hs0ss0sh0....",
    "...0sssSsss0....",
    "....0ssss0......",
    "...0K11r11K0....",
    "..0sK01r10Ks0...",
    "...0KKrKKK0.....",
    "...0KKKKKK0.....",
    "...0K0..0K0.....",
    "..0DD0..0DD0....",
    "...00....00.....",
};

static const char *SPR_ART_LAWYER_DOWN_1[16] = {
    "................",
    ".....00000......",
    "...00hhhhh00....",
    "..0hhHhhHhhh0...",
    "..0h000000hh0...",
    "..0hsssssshh0...",
    "..0hs0ss0sh0....",
    "...0sssSsss0....",
    "....0ssss0......",
    "...0K11r11K0....",
    "..0sK01r10Ks0...",
    "...0KKrKKK0.....",
    "...0KKKKKK0.....",
    "....0K0.0K0.....",
    "...0DD0.0K0.....",
    ".........0DD0...",
};

static const char *SPR_ART_LAWYER_DOWN_2[16] = {
    "................",
    ".....00000......",
    "...00hhhhh00....",
    "..0hhHhhHhhh0...",
    "..0h000000hh0...",
    "..0hsssssshh0...",
    "..0hs0ss0sh0....",
    "...0sssSsss0....",
    "....0ssss0......",
    "...0K11r11K0....",
    "..0sK01r10Ks0...",
    "...0KKrKKK0.....",
    "...0KKKKKK0.....",
    "....0K0.0K0.....",
    "...0K0.0DD0.....",
    "..0DD0..........",
};

static const char *SPR_ART_LAWYER_UP_0[16] = {
    "................",
    ".....00000......",
    "...00hhhhh00....",
    "..0hhHhhHhhh0...",
    "..0hhhhhhhh0....",
    "..0hhhhhhhh0....",
    "...0hHHHHh0.....",
    "...0KKKKKK0.....",
    "..0KKK00KKK0....",
    "..0sKK00KKs0....",
    "...0KK00KK0.....",
    "...0KBKKBK0.....",
    "...0KKKKKK0.....",
    "...0K0..0K0.....",
    "..0DD0..0DD0....",
    "...00....00.....",
};

static const char *SPR_ART_LAWYER_UP_1[16] = {
    "................",
    ".....00000......",
    "...00hhhhh00....",
    "..0hhHhhHhhh0...",
    "..0hhhhhhhh0....",
    "..0hhhhhhhh0....",
    "...0hHHHHh0.....",
    "...0KKKKKK0.....",
    "..0KKK00KKK0....",
    "..0sKK00KKs0....",
    "...0KK00KK0.....",
    "...0KBKKBK0.....",
    "...0KKKKKK0.....",
    "....0K0.0K0.....",
    "...0DD0.0K0.....",
    ".........0DD0...",
};

static const char *SPR_ART_LAWYER_UP_2[16] = {
    "................",
    ".....00000......",
    "...00hhhhh00....",
    "..0hhHhhHhhh0...",
    "..0hhhhhhhh0....",
    "..0hhhhhhhh0....",
    "...0hHHHHh0.....",
    "...0KKKKKK0.....",
    "..0KKK00KKK0....",
    "..0sKK00KKs0....",
    "...0KK00KK0.....",
    "...0KBKKBK0.....",
    "...0KKKKKK0.....",
    "....0K0.0K0.....",
    "...0K0.0DD0.....",
    "..0DD0..........",
};

static const char *SPR_ART_LAWYER_SIDE_0[16] = {
    "................",
    "......0000......",
    "....00hhhh00....",
    "...0hhhhhhhh0...",
    "...0hssssshh0...",
    "...0hs0ssshh0...",
    "...0hssssshh0...",
    "....0sssSs0.....",
    ".....0ssss0.....",
    "...0K1KKKK0.....",
    "..0sK0KKKKKK0...",
    "...0s0KBKBK0....",
    "....0KKKKKK0....",
    "....0K0.0K0.....",
    "...0DD0.0DD0....",
    "....00...00.....",
};

static const char *SPR_ART_LAWYER_SIDE_1[16] = {
    "................",
    "......0000......",
    "....00hhhh00....",
    "...0hhhhhhhh0...",
    "...0hssssshh0...",
    "...0hs0ssshh0...",
    "...0hssssshh0...",
    "....0sssSs0.....",
    ".....0ssss0.....",
    "...0K1KKKK0.....",
    "...0K0KKKKKK0...",
    "...0s0KBKBK0....",
    "...0KKK0KKKK0...",
    "..0KKK0..0KKK0..",
    ".0DD0....0DD0...",
    "..00......00....",
};

static const char *SPR_ART_LAWYER_SIDE_2[16] = {
    "................",
    "......0000......",
    "....00hhhh00....",
    "...0hhhhhhhh0...",
    "...0hssssshh0...",
    "...0hs0ssshh0...",
    "...0hssssshh0...",
    "....0sssSs0.....",
    ".....0ssss0.....",
    "...0K1KKKK0.....",
    "..0sK0KKKKKK0...",
    "...0s0KBKBK0....",
    "....0KKKKKK0....",
    ".....0KKK0......",
    "....0K0DD0......",
    ".....00.........",
};

/* THE PRIEST. Black cassock, white collar, and a sermon that stops
 * mid-sentence. Frame 1 opens the arms -- "...and no evil shall..." */
static const char *SPR_ART_PRIEST[16] = {
    "................",
    ".....00000......",
    "....0kkkkk0.....",
    "...0kkkkkkk0....",
    "...0ksssssk0....",
    "...0s0ss0s0.....",
    "....0sssSs0.....",
    ".....0ss0.......",
    "....0K1K0.......",
    "...0KK1KK0......",
    "..0sKK1KKs0.....",
    "...0KKKKKK0.....",
    "...0KKKKKK0.....",
    "...0KKKKKK0.....",
    "..0KKKKKKKK0....",
    "...00000000.....",
};

static const char *SPR_ART_PRIEST_1[16] = {
    "................",
    ".....00000......",
    "....0kkkkk0.....",
    "...0kkkkkkk0....",
    "...0ksssssk0....",
    "...0s0ss0s0.....",
    "....0sssSs0.....",
    ".....0ss0.......",
    "....0K1K0.......",
    "..00KK1KK00.....",
    ".0s0KK1KK0s0....",
    "...0KKKKKK0.....",
    "...0KKKKKK0.....",
    "...0KKKKKK0.....",
    "..0KKKKKKKK0....",
    "...00000000.....",
};

/* THE COP. Holding a line nobody drew, on a street he doesn't believe. */
static const char *SPR_ART_COP[16] = {
    "................",
    ".....00000......",
    "....0bbbbb0.....",
    "...0bbbbbbb0....",
    "...00000000.....",
    "...0ssssss0.....",
    "...0s0ss0s0.....",
    "....0sssSs0.....",
    ".....0ss0.......",
    "...0bbbbbb0.....",
    "..0sbbybbbs0....",
    "...0bBbbBb0.....",
    "...0BBBBBB0.....",
    "...0B0..0B0.....",
    "..0000..0000....",
    "................",
};

static const char *SPR_ART_COP_1[16] = {
    "................",
    ".....00000......",
    "....0bbbbb0.....",
    "...0bbbbbbb0....",
    "...00000000.....",
    "...0ssssss0.....",
    "...0s0ss0s0.....",
    "....0sssSs0.....",
    ".....0ss0.......",
    "...0bbbbbb0.....",
    "..0sbbybbbs0....",
    "...0bBbbBb0.....",
    "...0BBBBBB0.....",
    "....0B0.0B0.....",
    "...0000.0000....",
    "................",
};

/* SOMEBODY'S SECRETARY, three blocks from home when it started. */
static const char *SPR_ART_CITYWOMAN[16] = {
    "................",
    ".....00000......",
    "....0hhhhh0.....",
    "...0hhhhhhh0....",
    "...0hsssssh0....",
    "...0h0ss0sh0....",
    "....0sssSs0.....",
    "....0h0ss0h0....",
    ".....0ss0.......",
    "...0mmmmmm0.....",
    "..0smmmmmms0....",
    "...0mMmmMm0.....",
    "...0KKKKKK0.....",
    "....0s0.0s0.....",
    "...000..000.....",
    "................",
};

static const char *SPR_ART_CITYWOMAN_1[16] = {
    "................",
    ".....00000......",
    "....0hhhhh0.....",
    "...0hhhhhhh0....",
    "...0hsssssh0....",
    "...0h0ss0sh0....",
    "....0sssSs0.....",
    "....0h0ss0h0....",
    ".....0ss0.......",
    "...0mmmmmm0.....",
    ".0s0mmmmmm0s0...",
    "...0mMmmMm0.....",
    "...0KKKKKK0.....",
    "....0s0.0s0.....",
    "...000..000.....",
    "................",
};

/* THE CRUCIFIX -- the Part 1 name-screen cursor. The saucer picked the
 * farmer; the lawyer gets picked by something older. */
static const char *SPR_ART_CRUCIFIX[16] = {
    "......00........",
    ".....0yy0.......",
    ".....0yY0.......",
    "..0000yy0000....",
    ".0yyyyyyyyyy0...",
    "..0000yY0000....",
    ".....0yy0.......",
    ".....0yY0.......",
    ".....0yy0.......",
    ".....0yY0.......",
    ".....0yy0.......",
    ".....0yY0.......",
    "....00yy00......",
    "....0yyyy0......",
    ".....0000.......",
    "................",
};

/* THE PISTOL, lying where Mr. Kowalski left it. */
static const char *SPR_ART_ITEM_HANDGUN[16] = {
    "................",
    "................",
    "................",
    "................",
    "....00000000....",
    "...0kkkkkkkk0...",
    "...0kKKKKKKk0...",
    "....000KK000....",
    "......0KK0......",
    "......0KD0......",
    "......0DD0......",
    ".......00.......",
    "................",
    "................",
    "................",
    "................",
};

/* THE BULLETS: an open carton of pistol rounds, brass ends up. Shells are
 * for shotguns; the revolver gets these. */
static const char *SPR_ART_ITEM_BULLETS[16] = {
    "................",
    "................",
    "................",
    "....00000000....",
    "...0yYyYyYyY0...",
    "...0YyYyYyYy0...",
    "....00000000....",
    "...0bbbbbbbb0...",
    "...0b111111b0...",
    "...0bbbbbbbb0...",
    "....00000000....",
    "................",
    "................",
    "................",
    "................",
    "................",
};

/* HOLY WATER: a stoppered glass vial, cold to the touch. There are three
 * of these in the entire game. */
static const char *SPR_ART_ITEM_HOLYWATER[16] = {
    "................",
    "................",
    "......000.......",
    "......0D0.......",
    ".....00000......",
    "....0wWWww0.....",
    "....0wWyWw0.....",
    "....0wyyyw0.....",
    "....0wWyWw0.....",
    "....0wwyww0.....",
    "....0wwwww0.....",
    ".....00000......",
    "................",
    "................",
    "................",
    "................",
};

/* THE ROSARY: a loop of garnet beads and a small gold cross. It lies where
 * she dropped it. */
static const char *SPR_ART_ITEM_ROSARY[16] = {
    "................",
    "................",
    "....R.R.R.R.....",
    "...R.......R....",
    "...R.......R....",
    "...R.......R....",
    "....R.....R.....",
    ".....R...R......",
    "......R.R.......",
    ".......R........",
    "......0y0.......",
    ".....0yyy0......",
    "......0y0.......",
    "......0y0.......",
    "......000.......",
    "................",
};

/* DEACON CHARLIE. Almost ordained, still carrying the sacristy's last
 * blessed vial. Frame 1: he lifts the little book. */
static const char *SPR_ART_DEACON[16] = {
    "................",
    ".....00000......",
    "....0hhhhh0.....",
    "...0hhhhhhh0....",
    "...0hsssssh0....",
    "...0s0ss0s0.....",
    "....0sssSs0.....",
    ".....0ss0.......",
    "....0K1K0.......",
    "...0KK1KK0......",
    "..0sKKdKKs0.....",
    "...0KKKKKK0.....",
    "...0KKKKKK0.....",
    "...0KKKKKK0.....",
    "..0KKKKKKKK0....",
    "...00000000.....",
};

static const char *SPR_ART_DEACON_1[16] = {
    "................",
    ".....00000......",
    "....0hhhhh0.....",
    "...0hhhhhhh0....",
    "...0hsssssh0....",
    "...0s0ss0s0.....",
    "....0sssSs0.....",
    ".....0ss0.......",
    "....0KdK0.......",
    "...0KKdKK0......",
    "..0sKK1KKs0.....",
    "...0KKKKKK0.....",
    "...0KKKKKK0.....",
    "...0KKKKKK0.....",
    "..0KKKKKKKK0....",
    "...00000000.....",
};

/* MRS. ABERNATHY, THE LIBRARIAN. What the pond gave back. She is lying
 * where it left her, glasses still on, her book beside her, and the blood
 * has stopped moving. Both idle frames are the same frame: nothing about
 * her is going to move again. */
static const char *SPR_ART_DEADLADY[16] = {
    "................",
    "................",
    "................",
    "................",
    "..0000..........",
    ".0kkkk000000.dd.",
    "0kksss0pppppp0..",
    "0kks0s0pPppPp0..",
    ".0kkss0pppppp00.",
    "..rrRrrrRrrrr...",
    "...rrrRrrrr.....",
    "................",
    "................",
    "................",
    "................",
    "................",
};

/* THE COP CAR, from above, parked along the kerb. 48x32 like nothing else
 * on the street -- see copcar[] in assets.c and the special case in the
 * overworld renderer, same arrangement as the van. */
static const char *SPR_ART_COPCAR[32] = {
    "................................................",
    "................................................",
    ".......000000........................000000.....",
    ".......000000........................000000.....",
    "..00000000000000000000000000000000000000000000..",
    "..00KKKKKKKKK11111111111111111111111KKKKKKKK00..",
    "..00KKKKKKKKK11111111111111111111111KKKKKKKK00..",
    "..00KKKKKKKKK1wwwwwwkkkkkkkkkkwwwww1KKKKKKKK00..",
    "..00yKKKKKKKK1wWWWWwkkkkkkkkkkwWWWw1KKKKKKKr00..",
    "..00yKKKKKKKK1wWWWWwkkkkkkkkkkwWWWw1KKKKKKKr00..",
    "..00yKKKKKKKK1wWWWWwk0000000kkwWWWw1KKKKKKKr00..",
    "..00yKKKKKKKK1wWWWWwk0rrrbbbkkwWWWw1KKKKKKKr00..",
    "..00KKKKKKKKK1wWWWWwk0rrrbbbkkwWWWw1KKKKKKKK00..",
    "..00KKKKKKKKK1wWWWWwk0rrrbbbkkwWWWw1KKKKKKKK00..",
    "..00KKKKKKKKK1wWWWWwk0rrrbbbkkwWWWw1KKKKKKKK00..",
    "..00KKKKKKKKK1wWWWWwk0rrrbbbkkwWWWw1KKKKKKKK00..",
    "..00KKKKKKKKK1wWWWWwk0rrrbbbkkwWWWw1KKKKKKKK00..",
    "..00KKKKKKKKK1wWWWWwk0rrrbbbkkwWWWw1KKKKKKKK00..",
    "..00KKKKKKKKK1wWWWWwk0rrrbbbkkwWWWw1KKKKKKKK00..",
    "..00KKKKKKKKK1wWWWWwk0rrrbbbkkwWWWw1KKKKKKKK00..",
    "..00yKKKKKKKK1wWWWWwk0rrrbbbkkwWWWw1KKKKKKKr00..",
    "..00yKKKKKKKK1wWWWWwk0000000kkwWWWw1KKKKKKKr00..",
    "..00yKKKKKKKK1wWWWWwkkkkkkkkkkwWWWw1KKKKKKKr00..",
    "..00yKKKKKKKK1wWWWWwkkkkkkkkkkwWWWw1KKKKKKKr00..",
    "..00KKKKKKKKK1wwwwwwkkkkkkkkkkwwwww1KKKKKKKK00..",
    "..00KKKKKKKKK11111111111111111111111KKKKKKKK00..",
    "..00KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK00..",
    "..00000000000000000000000000000000000000000000..",
    ".......000000........................000000.....",
    ".......000000........................000000.....",
    "................................................",
    "................................................",
};

#endif /* SPRITES_H */
