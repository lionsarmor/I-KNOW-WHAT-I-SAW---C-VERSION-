/* ============================================================================
 * overworld.c -- walking around the world: movement + collision, the
 * scrolling camera, wandering aliens, talking to NPCs, warps between
 * maps, the HUD, and the dialog textbox.
 * ==========================================================================*/
#include "game_internal.h"

/* The player's collision box is just their FEET (bottom half of the
 * sprite), so the hat can overlap a fence behind them -- standard
 * top-down RPG trick. Box is relative to the sprite's top-left. */
#define FEET_X0  3
#define FEET_X1 12
#define FEET_Y0  9
#define FEET_Y1 15

static const map_t *cur_map(void) { return &maps[G.map_id]; }

/* is the feet-box at pixel position (x,y) standing in a wall? */
static int blocked(int x, int y)
{
    const map_t *m = cur_map();
    int corners[4][2] = {
        { x + FEET_X0, y + FEET_Y0 }, { x + FEET_X1, y + FEET_Y0 },
        { x + FEET_X0, y + FEET_Y1 }, { x + FEET_X1, y + FEET_Y1 },
    };
    for (int i = 0; i < 4; i++)
        if (tile_solid[map_tile(m, corners[i][0] / TILE,
                                   corners[i][1] / TILE)])
            return 1;
    return 0;
}

/* do two 16x16 characters at these pixel positions overlap?
 * (boxes shrunk a little so brushing past doesn't count) */
static int touching(int ax, int ay, int bx, int by)
{
    return ax + 3 < bx + 13 && bx + 3 < ax + 13 &&
           ay + 3 < by + 13 && by + 3 < ay + 13;
}

/* would a character standing at (x,y) overlap a living entity?
 * returns its index, or -1. `self` skips one entity (pass -1 for the
 * player). Characters are solid to each other -- this is what stops
 * everyone walking through everyone. */
static int hits_entity(int x, int y, int self)
{
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (i == self || G.ents[i].type == ENT_NONE)
            continue;
        if (touching(x, y, G.ents[i].x, G.ents[i].y))
            return i;
    }
    return -1;
}

/* ---- entering maps / starting the game ------------------------------------*/

void overworld_enter_map(int map, int tx, int ty)
{
    G.map_id     = map;
    G.player.x   = tx * TILE;
    G.player.y   = ty * TILE;
    G.battle_grace = 60;

    /* (re)populate the map from its spawn list */
    for (int i = 0; i < MAX_ENTITIES; i++)
        G.ents[i].type = ENT_NONE;
    const map_t *m = cur_map();
    for (int i = 0; i < m->nspawns && i < MAX_ENTITIES; i++) {
        if (G.spawns_gone[map] & (1u << i))
            continue;          /* pocketed / already gifted / boss killed */
        entity_t *e = &G.ents[i];
        e->type   = m->spawns[i].type;
        e->kind   = m->spawns[i].kind;
        e->x      = m->spawns[i].tx * TILE;
        e->y      = m->spawns[i].ty * TILE;
        e->dir    = DIR_DOWN;
        e->timer  = rng_range(30, 90);
        e->dialog = m->spawns[i].dialog;
        e->gift   = m->spawns[i].gift;
        e->after  = m->spawns[i].after;
    }

    G.state = ST_OVERWORLD;
    G.t     = 0;            /* also drives the "map name" fade */

    /* each map has its own ambience -- add a case when you add a map */
    audio_music(map == MAP_FARM ? MUSIC_FARM : MUSIC_NONE);
}

void overworld_start_game(void)
{
    G.player.hp     = PLAYER_MAX_HP;
    G.player.max_hp = PLAYER_MAX_HP;
    G.player.level  = 1;
    G.player.xp     = 0;
    G.player.dir    = DIR_DOWN;
    for (int i = 0; i < NUM_ITEMS; i++)
        G.player.items[i] = 0;             /* empty pockets */
    G.player.lamp = 0;
    G.items_seen  = 0;                     /* an empty pack, nothing spoiled */
    G.pack_sel = G.pack_top = 0;
    for (int m = 0; m < NUM_MAPS; m++)
        G.spawns_gone[m] = 0;              /* the world is restocked */
    G.daytime = 0;                         /* it starts at first light */
    G.last_restock = 0;
    G.toast_ticks = 0;
    overworld_enter_map(MAP_FARM, 5, 6);   /* on the path by the door */
}

/* CONTINUE from the title: game_save_load() has already restored the
 * player, the map and what's been taken -- we just have to repopulate the
 * map around them WITHOUT moving them off their saved pixel. */
void overworld_resume(void)
{
    int px = G.player.x, py = G.player.y;   /* enter_map would stomp these */
    overworld_enter_map(G.map_id, 0, 0);
    G.player.x = px;
    G.player.y = py;
    G.pack_sel = G.pack_top = 0;
    G.t = 90;                               /* skip the map-name banner */
}

/* ---- the world restocks ----------------------------------------------------
 * Every ITEM_RESPAWN_TICKS, everything you PICKED UP comes back where it
 * lay: the herbs regrow, somebody leaves another box of shells out. Without
 * this the shotgun runs dry for good and the goblin is simply unbeatable.
 *
 * Only ENT_ITEM spawns are restocked. A gift an NPC handed over, and a boss
 * you put down, stay done -- their spawn bits are left alone.
 */
static void restock_items(void)
{
    for (int m = 0; m < NUM_MAPS; m++)
        for (int i = 0; i < maps[m].nspawns && i < MAX_ENTITIES; i++)
            if (maps[m].spawns[i].type == ENT_ITEM)
                G.spawns_gone[m] &= (uint16_t)~(1u << i);

    /* Anything on the map we're standing on has to appear NOW -- the map is
     * already populated, so re-entering it later isn't good enough. */
    const map_t *m = cur_map();
    for (int i = 0; i < m->nspawns && i < MAX_ENTITIES; i++) {
        if (m->spawns[i].type != ENT_ITEM || G.ents[i].type != ENT_NONE)
            continue;
        entity_t *e = &G.ents[i];
        e->type   = ENT_ITEM;
        e->kind   = m->spawns[i].kind;
        e->x      = m->spawns[i].tx * TILE;
        e->y      = m->spawns[i].ty * TILE;
        e->dir    = DIR_DOWN;
        e->dialog = 0;
        e->gift   = 0;
        e->after  = 0;
        /* don't drop an item on the player's head -- they'd pocket it
         * without ever seeing it. It'll still be there when they move. */
    }

    G.toast       = "THE FIELDS HAVE GROWN BACK";
    G.toast_good  = 1;
    G.toast_ticks = 150;
}

/* ---- day & night -----------------------------------------------------------
 * One clock, one curve: 256 in daylight, NIGHT_BRIGHTNESS at night,
 * with a short linear dusk/dawn ramp between. The lengths live in
 * config.h (set to 1 minute each right now, for testing).
 */
static int day_brightness(void)
{
    uint32_t cycle = DAY_LEN_TICKS + NIGHT_LEN_TICKS;
    uint32_t t     = G.daytime % cycle;
    int span = 256 - NIGHT_BRIGHTNESS;
    if (t < DAY_LEN_TICKS - DUSK_LEN_TICKS)
        return 256;                                       /* broad day */
    if (t < DAY_LEN_TICKS)                                /* dusk      */
        return 256 - span * (int)(t - (DAY_LEN_TICKS - DUSK_LEN_TICKS))
                          / DUSK_LEN_TICKS;
    if (t < cycle - DUSK_LEN_TICKS)
        return NIGHT_BRIGHTNESS;                          /* night     */
    return NIGHT_BRIGHTNESS + span * (int)(t - (cycle - DUSK_LEN_TICKS))
                                   / DUSK_LEN_TICKS;      /* dawn      */
}

/* ---- update ----------------------------------------------------------------*/

/* the player pushed into entity `i`: NPCs are simply solid, but pushing
 * into a visitor starts the fight (unless we just fled one) */
static void bump_entity(int i)
{
    if (G.ents[i].type == ENT_ALIEN && !G.battle_grace)
        battle_start(i);
}

/* walking into an item pockets it: chime, message, and it stays gone
 * for the rest of this run (spawns_gone remembers across map changes) */
static void pick_up(int i)
{
    int kind = G.ents[i].kind;
    G.player.items[kind] += (kind == ITEM_SHELLS) ? SHELLS_PER_BOX : 1;
    pack_discover(kind);
    if (kind == ITEM_SHOTGUN) {
        G.player.items[ITEM_SHELLS] += 2;   /* pa kept it loaded */
        pack_discover(ITEM_SHELLS);
    }
    G.ents[i].type = ENT_NONE;
    G.spawns_gone[G.map_id] |= (uint16_t)(1u << i);
    audio_sfx(SFX_PICKUP);
    dialog_start(item_info[kind].pickup_msg);
}

/* one axis of player movement: walls block, characters block (and may
 * react to the shove) */
static int try_step(int nx, int ny)
{
    if (blocked(nx, ny))
        return 0;
    int hit = hits_entity(nx, ny, -1);
    if (hit >= 0) {
        if (G.ents[hit].type == ENT_ITEM)
            pick_up(hit);
        else
            bump_entity(hit);
        return 0;
    }
    G.player.x = nx;
    G.player.y = ny;
    return 1;
}

/* Walking INTO a door: if that door tile has a warp, step through it.
 * If it doesn't, it's locked -- say so once (the cooldown stops the
 * message machine-gunning while you keep pushing). */
static int locked_cd;   /* ticks left before "LOCKED" can show again */

static void check_door_bump(void)
{
    /* the tile just beyond the player's feet, in the facing direction */
    int px = G.player.x + 8, py = G.player.y + (FEET_Y0 + FEET_Y1) / 2;
    switch (G.player.dir) {
    case DIR_UP:    py -= 10; break;
    case DIR_DOWN:  py += 10; break;
    case DIR_LEFT:  px -= 10; break;
    case DIR_RIGHT: px += 10; break;
    }
    const map_t *m = cur_map();
    int tx = px / TILE, ty = py / TILE;
    if (map_tile(m, tx, ty) != TILE_DOOR)
        return;

    for (int i = 0; i < m->nwarps; i++)
        if (m->warps[i].tx == tx && m->warps[i].ty == ty) {
            audio_sfx(SFX_CONFIRM);
            overworld_enter_map(m->warps[i].dest_map,
                                m->warps[i].dest_tx, m->warps[i].dest_ty);
            return;
        }
    /* a door with no warp: locked */
    if (locked_cd == 0)
        dialog_start("LOCKED. NOBODY'S ANSWERING.");
    locked_cd = 30;   /* refreshed every tick you keep pushing */
}

/* The tile you're facing, in tile coords. */
static void facing_tile(int *tx, int *ty)
{
    int px = G.player.x + 8, py = G.player.y + (FEET_Y0 + FEET_Y1) / 2;
    switch (G.player.dir) {
    case DIR_UP:    py -= 10; break;
    case DIR_DOWN:  py += 10; break;
    case DIR_LEFT:  px -= 10; break;
    case DIR_RIGHT: px += 10; break;
    }
    *tx = px / TILE;
    *ty = py / TILE;
}

/* Push into your own bed and you give up on the day. */
static void check_bed_bump(void)
{
    if (G.map_id != MAP_HOME)
        return;                       /* only YOUR bed. */
    int tx, ty;
    facing_tile(&tx, &ty);
    if (map_tile(cur_map(), tx, ty) != TILE_BED)
        return;

    G.state   = ST_SLEEP;
    G.sleep_t = 0;
    G.t = 0;
    audio_music(MUSIC_NONE);
}

static void move_player(void)
{
    int dx = 0, dy = 0;
    if (G.held & BTN_UP)    { dy = -PLAYER_SPEED; G.player.dir = DIR_UP; }
    if (G.held & BTN_DOWN)  { dy =  PLAYER_SPEED; G.player.dir = DIR_DOWN; }
    if (G.held & BTN_LEFT)  { dx = -PLAYER_SPEED; G.player.dir = DIR_LEFT; }
    if (G.held & BTN_RIGHT) { dx =  PLAYER_SPEED; G.player.dir = DIR_RIGHT; }

    /* move one axis at a time so you slide along walls */
    int moved = 0;
    if (dx) moved |= try_step(G.player.x + dx, G.player.y);
    if (dy) moved |= try_step(G.player.x, G.player.y + dy);
    if (G.state != ST_OVERWORLD)
        return;                     /* a bump started a battle */

    if ((dx || dy) && !moved) {
        check_bed_bump();           /* ...your bed? then sleep. */
        if (G.state != ST_OVERWORLD)
            return;
        check_door_bump();          /* pushing against something... a door? */
    }

    G.player.walking = (dx || dy);
    if (G.player.walking)
        G.player.anim++;
}

static void check_warps(void)
{
    const map_t *m = cur_map();
    int tx = (G.player.x + TILE / 2) / TILE;   /* tile under player center */
    int ty = (G.player.y + FEET_Y1) / TILE;    /* ...at their feet */
    for (int i = 0; i < m->nwarps; i++)
        if (m->warps[i].tx == tx && m->warps[i].ty == ty) {
            overworld_enter_map(m->warps[i].dest_map,
                                m->warps[i].dest_tx, m->warps[i].dest_ty);
            return;
        }
}

static void update_aliens(void)
{
    static const int step[4][2] = {   /* DIR_DOWN,UP,LEFT,RIGHT */
        { 0, 1 }, { 0, -1 }, { -1, 0 }, { 1, 0 }
    };
    for (int i = 0; i < MAX_ENTITIES; i++) {
        entity_t *e = &G.ents[i];
        if (e->type != ENT_ALIEN)
            continue;
        if (species[e->kind].boss)
            continue;      /* a boss doesn't wander. it waits for you. */

        /* wander: walk a while, then pick a new direction (or pause) */
        if (--e->timer <= 0) {
            e->dir   = rng_range(0, 3);
            e->timer = rng_range(30, 120);
        }
        int nx = e->x + step[e->dir][0] * ALIEN_WANDER_SPEED;
        int ny = e->y + step[e->dir][1] * ALIEN_WANDER_SPEED;

        if (touching(nx, ny, G.player.x, G.player.y)) {
            /* it walked into YOU -- that's an ambush */
            if (!G.battle_grace) {
                battle_start(i);
                return;
            }
            e->timer = 0;               /* grace period: it hesitates */
        } else if (!blocked(nx, ny) && hits_entity(nx, ny, i) < 0) {
            e->x = nx;                  /* clear ground, no one in the way */
            e->y = ny;
        } else {
            e->timer = 0;               /* bumped something: rethink */
        }
    }
}

/* if there's an NPC on the tile the player is facing, talk to them */
static void try_talk(void)
{
    /* a probe point 12px ahead of the player's center */
    int px = G.player.x + 8, py = G.player.y + 8;
    switch (G.player.dir) {
    case DIR_UP:    py -= 14; break;
    case DIR_DOWN:  py += 14; break;
    case DIR_LEFT:  px -= 14; break;
    case DIR_RIGHT: px += 14; break;
    }
    for (int i = 0; i < MAX_ENTITIES; i++) {
        entity_t *e = &G.ents[i];
        if (e->type != ENT_NPC || !e->dialog)
            continue;
        if (px >= e->x && px < e->x + TILE &&
            py >= e->y && py < e->y + TILE) {
            /* Some NPCs are holding something for you. The first time you
             * talk they hand it over (their `dialog` is the line that does
             * it); after that they fall back to `after`. We reuse the
             * spawns_gone bitmask -- an NPC's spawn slot gets its own bit,
             * so the handover survives leaving and re-entering the map. */
            if (e->gift && !(G.spawns_gone[G.map_id] & (1u << i))) {
                int kind = e->gift - 1;
                G.player.items[kind] +=
                    (kind == ITEM_SHELLS) ? SHELLS_PER_BOX : 1;
                pack_discover(kind);
                G.spawns_gone[G.map_id] |= (uint16_t)(1u << i);
                audio_sfx(SFX_PICKUP);
                dialog_start(e->dialog);
                return;
            }
            audio_sfx(SFX_CONFIRM);
            dialog_start((e->gift && e->after) ? e->after : e->dialog);
            return;
        }
    }
}

void overworld_update(void)
{
    G.daytime++;                   /* the sun only moves while you walk */

    if (G.daytime - G.last_restock >= (uint32_t)ITEM_RESPAWN_TICKS) {
        G.last_restock = G.daytime;
        restock_items();
    }

    if (G.battle_grace > 0)
        G.battle_grace--;
    if (locked_cd > 0)
        locked_cd--;

    move_player();
    check_warps();
    if (G.state != ST_OVERWORLD)   /* warp changed scene state/timer */
        return;
    update_aliens();

    if (PRESSED(BTN_A))
        try_talk();

    /* START opens the pack. (On the C3 handheld there's no START button --
     * hold A+B together; the platform layer reports that as START.) */
    if (PRESSED(BTN_START)) {
        G.pack_sel = 0;
        audio_sfx(SFX_CONFIRM);
        G.state = ST_PACK;
        G.t = 0;
    }
}

/* ---- render ----------------------------------------------------------------*/

/* camera top-left in map pixels, clamped to the map edges */
static void camera(int *cx, int *cy)
{
    const map_t *m = cur_map();
    *cx = G.player.x + TILE / 2 - SCREEN_W / 2;
    *cy = G.player.y + TILE / 2 - SCREEN_H / 2;
    if (*cx < 0) *cx = 0;
    if (*cy < 0) *cy = 0;
    if (*cx > m->w * TILE - SCREEN_W) *cx = m->w * TILE - SCREEN_W;
    if (*cy > m->h * TILE - SCREEN_H) *cy = m->h * TILE - SCREEN_H;
}

/* which sprite frame is the player right now?
 * classic 4-phase walk cycle over 3 frames: stand, stepL, stand, stepR */
static void player_sprite(int *spr, int *flip)
{
    static const int cycle[4] = { 0, 1, 0, 2 };
    int frame = G.player.walking ? cycle[(G.player.anim / 5) % 4] : 0;
    *flip = 0;
    switch (G.player.dir) {
    case DIR_DOWN:  *spr = SPR_FARMER_DOWN_0 + frame; break;
    case DIR_UP:    *spr = SPR_FARMER_UP_0   + frame; break;
    case DIR_LEFT:  *spr = SPR_FARMER_SIDE_0 + frame; break;
    default:        *spr = SPR_FARMER_SIDE_0 + frame; *flip = 1; break;
    }
}

/* ---- the HUD ---------------------------------------------------------------
 * One panel, top-left:
 *      NAME
 *      HP  [==========]
 *      LV 3 [====     ]      <- the XP bar, filling toward the next level
 * ...then a row of icons for the things you're carrying that matter at a
 * glance (the key, the flashlight -- lit when it's on).
 */
static void bar(int x, int y, int w, int h, int val, int max,
                uint16_t fill, uint16_t back)
{
    gfx_fill_rect(x, y, w, h, back);
    if (max > 0 && val > 0) {
        int fw = w * val / max;
        if (fw > w) fw = w;
        if (fw < 1)  fw = 1;
        gfx_fill_rect(x, y, fw, h, fill);
    }
}

static void draw_hud(void)
{
    const int PW = 100, PH = 42;
    gfx_fill_rect(4, 4, PW, PH, RGB565(16, 16, 24));
    gfx_rect     (4, 4, PW, PH, RGB565(90, 90, 98));

    /* who you are */
    gfx_text(8, 7, G.player.name[0] ? G.player.name : NAME_DEFAULT,
             RGB565(255, 255, 255));

    /* health */
    gfx_text(8, 18, "HP", RGB565(255, 255, 255));
    bar(26, 19, 74, 6, G.player.hp, G.player.max_hp,
        G.player.hp * 4 > G.player.max_hp ? RGB565(80, 200, 80)
                                          : RGB565(220, 60, 50),
        RGB565(50, 50, 58));

    /* level + the climb to the next one */
    char lv[8] = "LV ";
    int  l = G.player.level;
    int  p = 3;
    if (l > 9) lv[p++] = (char)('0' + (l / 10) % 10);
    lv[p++] = (char)('0' + l % 10);
    lv[p] = '\0';
    gfx_text(8, 29, lv, RGB565(255, 236, 150));
    /* you level at xp >= level * XP_PER_LEVEL -- so that's the bar's top */
    bar(44, 30, 56, 5, G.player.xp, G.player.level * XP_PER_LEVEL,
        RGB565(120, 190, 255), RGB565(40, 44, 60));

    /* ---- the icon strip: what you're carrying that matters ---------------*/
    int ix = 6;
    if (G.player.items[ITEM_KEY]) {
        gfx_blit(sprites[SPR_ITEM_KEY].px, TILE, TILE, ix, PH - 2);
        ix += 15;
    }
    if (G.player.items[ITEM_FLASHLIGHT]) {
        /* lit when it's switched on, dull when it isn't */
        gfx_blit_ex(sprites[SPR_ITEM_FLASHLIGHT].px, TILE, TILE, ix, PH - 2,
                    1, G.player.lamp ? 256 : 110, 0);
        if (G.player.lamp)      /* a little glow off the lens */
            gfx_fill_rect(ix + 5, PH + 2, 6, 2, RGB565(255, 240, 170));
        ix += 15;
    }

    /* shells, top-right, once you carry the gun */
    if (G.player.items[ITEM_SHOTGUN]) {
        char am[12] = "SHELLS ";
        int n = G.player.items[ITEM_SHELLS], i = 7;
        if (n > 99) n = 99;
        if (n >= 10) am[i++] = (char)('0' + n / 10);
        am[i++] = (char)('0' + n % 10);
        am[i] = '\0';
        int w = gfx_text_width(am, 1) + 8;
        gfx_fill_rect(SCREEN_W - w - 4, 4, w, 14, RGB565(16, 16, 24));
        gfx_text(SCREEN_W - w, 7, am, RGB565(255, 220, 80));
    }

    /* SAVED. / LOADED. / NO SAVE FOUND / THE FIELDS HAVE GROWN BACK */
    if (G.toast_ticks > 0 && G.toast) {
        G.toast_ticks--;
        int tw = gfx_text_width(G.toast, 1);
        gfx_fill_rect((SCREEN_W - tw) / 2 - 6, 60, tw + 12, 14,
                      RGB565(16, 16, 24));
        gfx_text((SCREEN_W - tw) / 2, 63, G.toast,
                 G.toast_good ? RGB565(150, 240, 150)
                              : RGB565(240, 110, 100));
    }
}

void overworld_render(void)
{
    int cx, cy;
    camera(&cx, &cy);
    const map_t *m = cur_map();

    /* tiles (only the ones on screen) */
    for (int ty = cy / TILE; ty <= (cy + SCREEN_H) / TILE; ty++)
        for (int tx = cx / TILE; tx <= (cx + SCREEN_W) / TILE; tx++) {
            int id = map_tile(m, tx, ty);
            if (id == TILE_WATER && (G.frame / 28) % 2)
                id = TILE_WATER2;             /* lapping water */
            gfx_blit(tiles[id].px, TILE, TILE,
                     tx * TILE - cx, ty * TILE - cy);
        }

    /* entities: sprites + animation come from the cast tables in
     * assets.c, so new species/looks show up here automatically */
    for (int i = 0; i < MAX_ENTITIES; i++) {
        entity_t *e = &G.ents[i];
        if (e->type == ENT_NONE)
            continue;
        if (e->type == ENT_ITEM) {
            /* items lie still and catch the light now and then */
            int glint = (int)(((G.frame + (uint32_t)i * 13) / 40) % 2);
            gfx_blit_ex(sprites[item_info[e->kind].spr].px, TILE, TILE,
                        e->x - cx, e->y - cy, 1, glint ? 256 : 208, 0);
            continue;
        }
        int spr, bright = 256, bob = 0;
        if (e->type == ENT_ALIEN) {
            const species_t *sp = &species[e->kind];
            spr    = ((G.frame / 20) % 2) ? sp->spr1 : sp->spr0;
            bright = sp->bright;
            /* they don't walk. they float, slightly out of sync
             * with each other (the i*7 offset). */
            bob = (int)(((G.frame + (uint32_t)i * 7) / 16) % 2);
        } else {
            const npc_look_t *lk = &npc_looks[e->kind];
            spr = ((G.frame / 45) % 2) ? lk->spr1 : lk->spr0;
        }
        gfx_blit_ex(sprites[spr].px, TILE, TILE,
                    e->x - cx, e->y - cy - bob, 1, bright, 0);
    }

    /* the player */
    int spr, flip;
    player_sprite(&spr, &flip);
    gfx_blit_ex(sprites[spr].px, TILE, TILE,
                G.player.x - cx, G.player.y - cy, 1, 256, flip);

    /* Night falls on the outdoors (indoors the lamps stay lit).
     * If the flashlight is on, it carves a pool of daylight around you.
     * Applied before the HUD so the HUD stays readable in the dark. */
    if (m->outdoor) {
        int lit = (G.player.items[ITEM_FLASHLIGHT] && G.player.lamp)
                ? FLASHLIGHT_RADIUS : 0;
        gfx_night(day_brightness(),
                  G.player.x - cx + TILE / 2,
                  G.player.y - cy + TILE / 2, lit);
    }

    draw_hud();

    /* map name banner for the first ~1.5s after arriving */
    if (G.t < 90) {
        const char *name = m->name;
        int w = gfx_text_width(name, 1) + 12;
        gfx_fill_rect((SCREEN_W - w) / 2, 24, w, 14, RGB565(16, 16, 24));
        gfx_text((SCREEN_W - gfx_text_width(name, 1)) / 2, 27, name,
                 RGB565(255, 255, 255));
    }
}

/* ============================ DIALOG ========================================
 * A classic bottom textbox with a typewriter effect.
 *   A while typing  -> reveal everything
 *   A when done     -> close
 */
#define DIALOG_COLS 26                 /* chars per line in the box */
#define DIALOG_ROWS 3                  /* lines per PAGE                */

/* Expand an NPC line into dialog_buf, replacing every '~' with the
 * player's name. Done ONCE here, so the typewriter and the word-wrap
 * below never have to know the name exists. */
void dialog_start(const char *text)
{
    int o = 0;
    for (const char *s = text; *s && o < (int)sizeof G.dialog_buf - 1; s++) {
        if (*s != '~') {
            G.dialog_buf[o++] = *s;
            continue;
        }
        for (const char *n = G.player.name;
             *n && o < (int)sizeof G.dialog_buf - 1; n++)
            G.dialog_buf[o++] = *n;
    }
    G.dialog_buf[o] = '\0';

    G.dialog_text  = G.dialog_buf;
    G.dialog_page  = 0;
    G.dialog_shown = 0;
    G.state = ST_DIALOG;
    G.t = 0;
}

/* ---- ONE page of the textbox ----------------------------------------------
 * Greedy word-wrap starting at `from`, filling up to DIALOG_ROWS lines.
 * Fills `out` with the lines, sets *rows, and RETURNS the index just past
 * the last character used -- which is where the NEXT page starts.
 *
 * Both the renderer and the "advance to the next page" logic call this, so
 * they can never disagree about where a page ends. Long speeches simply run
 * onto as many pages as they need instead of being cut off.
 */
static int dialog_page_layout(int from, char out[DIALOG_ROWS][DIALOG_COLS + 1],
                              int *rows)
{
    const char *s = G.dialog_text;
    int i = from, r = 0;

    while (r < DIALOG_ROWS) {
        while (s[i] == ' ')            /* no line starts with a space */
            i++;
        if (!s[i])
            break;

        int begin = i, last_space = -1, n = 0;
        while (s[i] && n < DIALOG_COLS) {
            if (s[i] == ' ')
                last_space = i;
            i++;
            n++;
        }
        /* if we stopped in the middle of a word, back up to the last space */
        int stop = i;
        if (s[i] && s[i] != ' ' && last_space > begin)
            stop = last_space;

        int k = 0;
        for (int j = begin; j < stop && k < DIALOG_COLS; j++)
            out[r][k++] = s[j];
        out[r][k] = '\0';

        i = stop;
        r++;
    }

    *rows = r;
    while (s[i] == ' ')                /* don't leave a space leading the
                                          next page */
        i++;
    return i;
}

/* how many characters are actually printed on this page */
static int page_visible(char out[DIALOG_ROWS][DIALOG_COLS + 1], int rows)
{
    int n = 0;
    for (int r = 0; r < rows; r++)
        for (int k = 0; out[r][k]; k++)
            n++;
    return n;
}

void dialog_update(void)
{
    char lines[DIALOG_ROWS][DIALOG_COLS + 1];
    int  rows;
    int  next  = dialog_page_layout(G.dialog_page, lines, &rows);
    int  total = page_visible(lines, rows);
    int  more  = (G.dialog_text[next] != '\0');   /* another page after this */

    /* typewriter, one char every other tick */
    if (G.t % 2 == 0 && G.dialog_shown < total)
        G.dialog_shown++;

    if (!PRESSED(BTN_A) && !PRESSED(BTN_B))
        return;

    if (G.dialog_shown < total) {
        G.dialog_shown = total;        /* impatient: show the whole page */
    } else if (more) {
        G.dialog_page  = next;         /* turn the page */
        G.dialog_shown = 0;
        audio_sfx(SFX_BLIP);
    } else {
        G.state = ST_OVERWORLD;        /* that was the last page */
        G.t = 0;
        G.battle_grace = 30;
    }
}

void dialog_render(void)
{
    overworld_render();   /* the world stays visible behind the box */

    int bh = 46;
    gfx_fill_rect(4, SCREEN_H - bh - 4, SCREEN_W - 8, bh, RGB565(16, 16, 24));
    gfx_rect     (4, SCREEN_H - bh - 4, SCREEN_W - 8, bh, RGB565(255, 255, 255));

    char lines[DIALOG_ROWS][DIALOG_COLS + 1];
    int  rows;
    int  next = dialog_page_layout(G.dialog_page, lines, &rows);
    int  more = (G.dialog_text[next] != '\0');
    int  total = page_visible(lines, rows);

    /* draw the page, but only as much of it as the typewriter has reached */
    int left = G.dialog_shown;
    int y0 = SCREEN_H - bh + 2;
    for (int r = 0; r < rows && left > 0; r++) {
        char clipped[DIALOG_COLS + 1];
        int k = 0;
        for (; lines[r][k] && k < left; k++)
            clipped[k] = lines[r][k];
        clipped[k] = '\0';
        gfx_text(12, y0 + r * 11, clipped, RGB565(255, 255, 255));
        left -= k;
    }

    /* the prompt in the corner, once the page is fully out:
     *   a little DOWN triangle = there's more to read
     *   an 'A'                 = this is the end, press to close        */
    if (G.dialog_shown >= total && (G.t % 40) < 25) {
        int px = SCREEN_W - 20, py = SCREEN_H - 16;
        if (more) {
            for (int i = 0; i < 4; i++)     /* v */
                gfx_fill_rect(px + i, py + i, 8 - i * 2, 1,
                              RGB565(255, 220, 80));
        } else {
            gfx_text(px, py, "A", RGB565(255, 220, 80));
        }
    }
}

/* ============================ THE PACK ======================================
 * The overworld item menu. START opens it, START or B closes it, A uses
 * the highlighted row. The world stays drawn (and frozen) behind it.
 *
 * IT STARTS EMPTY. The pack lists only what you have ACTUALLY FOUND
 * (G.items_seen, set the moment a thing first enters your hands), so it
 * never spoils an item you haven't met yet -- however many we add later.
 * Something you found and used up still shows, at x0: you already know it
 * exists, and hiding it again would just be confusing.
 *
 * It SCROLLS. Only PACK_ROWS rows are on screen at a time, with little
 * arrows when there is more above or below, so the list can grow without
 * bound.
 *
 * The last row is always SAVE GAME -- it's the one entry that isn't an
 * item, and it's always available.
 *
 *   HERB / MEDKIT  heal you, and are consumed
 *   LIGHT          toggles the flashlight -- never consumed
 *   SHELLS/SHOTGUN are carried, not "used" here: listed so you can see
 *                  what you have, but they do nothing from the pack
 */
#define PACK_ROWS 4               /* visible rows at once */

/* can you DO anything with this from the pack? */
static int pack_usable(int kind)
{
    return kind == ITEM_HERB || kind == ITEM_MEDKIT || kind == ITEM_FLASHLIGHT;
}

/* An item enters the pack the first time it touches your hands. Call this
 * from anywhere something is given to the player. */
void pack_discover(int kind)
{
    G.items_seen |= (uint16_t)(1u << kind);
}

/* Build the list of rows currently in the pack: every item you've found,
 * then SAVE and LOAD. Returns how many rows there are; `out` gets the
 * ITEM_* id of each, or ROW_SAVE / ROW_LOAD. */
#define ROW_SAVE (-1)
#define ROW_LOAD (-2)

static int pack_rows(int *out)
{
    int n = 0;
    for (int i = 0; i < NUM_ITEMS; i++)
        if (G.items_seen & (1u << i))
            out[n++] = i;
    out[n++] = ROW_SAVE;      /* always there */
    out[n++] = ROW_LOAD;      /* ...but only usable once a save exists */
    return n;
}

void pack_update(void)
{
    int rows[NUM_ITEMS + 1];
    int n = pack_rows(rows);

    if (PRESSED(BTN_START) || PRESSED(BTN_B)) {
        audio_sfx(SFX_BLIP);
        G.state = ST_OVERWORLD;
        G.t = 90;                  /* skip the map-name banner */
        G.battle_grace = 20;       /* no ambush the instant you close it */
        return;
    }

    if (PRESSED(BTN_UP)) {
        G.pack_sel = (G.pack_sel + n - 1) % n;
        audio_sfx(SFX_BLIP);
    }
    if (PRESSED(BTN_DOWN)) {
        G.pack_sel = (G.pack_sel + 1) % n;
        audio_sfx(SFX_BLIP);
    }

    /* keep the highlighted row inside the visible window */
    if (G.pack_sel < G.pack_top)
        G.pack_top = G.pack_sel;
    if (G.pack_sel >= G.pack_top + PACK_ROWS)
        G.pack_top = G.pack_sel - PACK_ROWS + 1;
    if (G.pack_top > n - PACK_ROWS) G.pack_top = n - PACK_ROWS;
    if (G.pack_top < 0)             G.pack_top = 0;

    if (!PRESSED(BTN_A))
        return;

    int kind = rows[G.pack_sel];

    if (kind == ROW_SAVE) {
        /* We can't write it ourselves -- the core has no idea what a file
         * is. Raise the flag; the platform layer picks it up after
         * game_update() and reports back via game_save_done(). */
        G.save_pending = 1;
        audio_sfx(SFX_CONFIRM);
        return;
    }

    if (kind == ROW_LOAD) {
        if (!G.has_save) {
            audio_sfx(SFX_BLIP);      /* nothing to load */
            return;
        }
        G.load_pending = 1;           /* the platform reads it and calls
                                         game_save_load(), which drops us
                                         straight into the loaded game */
        audio_sfx(SFX_CONFIRM);
        return;
    }

    if (G.player.items[kind] <= 0 || !pack_usable(kind)) {
        audio_sfx(SFX_BLIP);       /* empty, or nothing to do with it */
        return;
    }

    switch (kind) {
    case ITEM_FLASHLIGHT:          /* a switch, not a consumable */
        G.player.lamp = !G.player.lamp;
        audio_sfx(SFX_CONFIRM);
        break;

    case ITEM_HERB:
    case ITEM_MEDKIT:
        if (G.player.hp >= G.player.max_hp) {
            audio_sfx(SFX_BLIP);   /* already whole -- don't waste it */
            return;
        }
        G.player.items[kind]--;
        G.player.hp = (kind == ITEM_HERB)
                    ? G.player.hp + HERB_HEAL
                    : G.player.max_hp;
        if (G.player.hp > G.player.max_hp)
            G.player.hp = G.player.max_hp;
        audio_sfx(SFX_HEAL);
        break;
    }
}

void pack_render(void)
{
    overworld_render();            /* the world stays visible behind it */

    int rows[NUM_ITEMS + 1];
    int n = pack_rows(rows);
    int shown = (n < PACK_ROWS) ? n : PACK_ROWS;

    int w = 154;
    int h = 22 + shown * 12 + 14;
    int x = (SCREEN_W - w) / 2;
    int y = (SCREEN_H - h) / 2;

    gfx_fill_rect(x, y, w, h, RGB565(16, 16, 24));
    gfx_rect     (x, y, w, h, RGB565(255, 255, 255));
    gfx_text(x + 10, y + 6, "PACK", RGB565(255, 255, 255));

    for (int r = 0; r < shown; r++) {
        int  idx  = G.pack_top + r;
        int  kind = rows[idx];
        int  ry   = y + 22 + r * 12;
        char row[20];
        int  p    = 0;

        if (kind == ROW_SAVE) {
            gfx_text(x + 20, ry, "SAVE GAME", RGB565(255, 236, 150));
            continue;
        }
        if (kind == ROW_LOAD) {
            gfx_text(x + 20, ry, "LOAD GAME",
                     G.has_save ? RGB565(255, 236, 150)   /* there is one  */
                                : RGB565(100, 100, 108)); /* nothing saved */
            continue;
        }

        int n_held = G.player.items[kind];
        for (const char *s = item_info[kind].name; *s && p < 8; s++)
            row[p++] = *s;
        while (p < 9)
            row[p++] = ' ';
        row[p++] = 'X';
        if (n_held > 9)
            row[p++] = (char)('0' + (n_held / 10) % 10);
        row[p++] = (char)('0' + n_held % 10);
        row[p]   = '\0';

        uint16_t col = !n_held             ? RGB565(100, 100, 108)  /* none   */
                     : pack_usable(kind)   ? RGB565(255, 255, 255)  /* usable */
                                           : RGB565(170, 170, 178); /* carried*/
        gfx_text(x + 20, ry, row, col);

        /* the flashlight shows its switch position */
        if (kind == ITEM_FLASHLIGHT && n_held)
            gfx_text(x + w - 40, ry, G.player.lamp ? "ON" : "OFF",
                     G.player.lamp ? RGB565(255, 236, 150)
                                   : RGB565(100, 100, 108));
    }

    /* "there's more" arrows -- little triangles, since the font has no
     * up/down glyphs (same reason the cursor is hand-drawn) */
    uint16_t hint = RGB565(150, 150, 158);
    if (G.pack_top > 0)
        for (int i = 0; i < 3; i++)
            gfx_fill_rect(x + w - 16 - i, y + 17 + i, 1 + i * 2, 1, hint);
    if (G.pack_top + shown < n)
        for (int i = 0; i < 3; i++)
            gfx_fill_rect(x + w - 16 - (2 - i), y + h - 15 + i,
                          1 + (2 - i) * 2, 1, hint);

    gfx_cursor(x + 10, y + 22 + (G.pack_sel - G.pack_top) * 12, G.frame);
    gfx_text(x + 10, y + h - 11, "A USE  B CLOSE", RGB565(110, 110, 118));
}

/* ============================ SLEEP =========================================
 * Push into your bed in Ma's house and the day is over. The screen sinks to
 * black, the night passes in a few lines, and you wake at first light: full
 * health, and the fields have grown back.
 *
 * Timeline (ticks):  0..FADE   the room dims out
 *                    FADE..    black, with the night written on it
 *                    WAKE..    dawn fades back in
 */
#define SLEEP_FADE 90
#define SLEEP_DARK 300
#define SLEEP_WAKE 390

void sleep_update(void)
{
    G.sleep_t++;

    /* at the darkest point, the night actually happens */
    if (G.sleep_t == SLEEP_DARK) {
        G.player.hp = G.player.max_hp;

        /* skip the clock forward to the next dawn */
        uint32_t cycle = DAY_LEN_TICKS + NIGHT_LEN_TICKS;
        G.daytime = ((G.daytime / cycle) + 1) * cycle;

        restock_items();               /* the fields grow back */
        G.last_restock = G.daytime;    /* ...so don't immediately do it again */
        G.toast_ticks = 0;             /* the sleep text already says so */
        audio_sfx(SFX_HEAL);
    }

    if (G.sleep_t >= SLEEP_WAKE) {
        G.state = ST_OVERWORLD;
        G.t = 90;                      /* no map-name banner */
        G.battle_grace = 30;
        audio_music(G.map_id == MAP_FARM ? MUSIC_FARM : MUSIC_NONE);
    }
}

void sleep_render(void)
{
    overworld_render();                /* the room, still there behind it */

    /* the lights go down, and later come back up */
    int dim;
    if (G.sleep_t < SLEEP_FADE)
        dim = 256 - 256 * G.sleep_t / SLEEP_FADE;          /* fading out  */
    else if (G.sleep_t < SLEEP_WAKE - SLEEP_FADE)
        dim = 0;                                            /* dead black  */
    else
        dim = 256 * (G.sleep_t - (SLEEP_WAKE - SLEEP_FADE)) / SLEEP_FADE;

    if (dim < 256)
        gfx_night(dim, 0, 0, 0);       /* no lantern: this is the whole room */

    /* what the night was like */
    static const struct { int at; const char *line; } NIGHT[] = {
        { SLEEP_FADE + 20,  "YOU PULL THE BLANKET UP." },
        { SLEEP_FADE + 90,  "THE HOUSE TICKS AND SETTLES." },
        { SLEEP_FADE + 160, "SOMETHING PASSES OVERHEAD." },
        { SLEEP_DARK + 20,  "...MORNING." },
    };
    for (unsigned i = 0; i < sizeof NIGHT / sizeof NIGHT[0]; i++) {
        int a = NIGHT[i].at;
        if (G.sleep_t >= a && G.sleep_t < a + 70) {
            const char *l = NIGHT[i].line;
            /* fade each line in and back out so it reads like a dream */
            int k = G.sleep_t - a;
            int b = (k < 20) ? 255 * k / 20
                  : (k > 50) ? 255 * (70 - k) / 20 : 255;
            if (b > 255) b = 255;
            if (b < 0)   b = 0;
            gfx_text((SCREEN_W - gfx_text_width(l, 1)) / 2, 76, l,
                     gfx_dim(RGB565(230, 230, 240), b));
        }
    }

    /* Two lines: at 8px a character, anything over 30 chars runs off the
     * side of a 240px screen. */
    if (G.sleep_t > SLEEP_DARK + 30 && G.sleep_t < SLEEP_WAKE) {
        const char *w1 = "YOU FEEL RESTED.";
        const char *w2 = "THE FIELDS HAVE GROWN BACK.";
        uint16_t c = RGB565(150, 240, 150);
        gfx_text((SCREEN_W - gfx_text_width(w1, 1)) / 2,  96, w1, c);
        gfx_text((SCREEN_W - gfx_text_width(w2, 1)) / 2, 108, w2, c);
    }
}
