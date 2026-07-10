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
        entity_t *e = &G.ents[i];
        e->type   = m->spawns[i].type;
        e->kind   = m->spawns[i].kind;
        e->x      = m->spawns[i].tx * TILE;
        e->y      = m->spawns[i].ty * TILE;
        e->dir    = DIR_DOWN;
        e->timer  = rng_range(30, 90);
        e->dialog = m->spawns[i].dialog;
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
    overworld_enter_map(MAP_FARM, 5, 6);   /* on the path by the door */
}

/* ---- update ----------------------------------------------------------------*/

/* the player pushed into entity `i`: NPCs are simply solid, but pushing
 * into a visitor starts the fight (unless we just fled one) */
static void bump_entity(int i)
{
    if (G.ents[i].type == ENT_ALIEN && !G.battle_grace)
        battle_start(i);
}

/* one axis of player movement: walls block, characters block (and may
 * react to the shove) */
static int try_step(int nx, int ny)
{
    if (blocked(nx, ny))
        return 0;
    int hit = hits_entity(nx, ny, -1);
    if (hit >= 0) {
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

    if ((dx || dy) && !moved)
        check_door_bump();          /* pushing against something... a door? */

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
            audio_sfx(SFX_CONFIRM);
            dialog_start(e->dialog);
            return;
        }
    }
}

void overworld_update(void)
{
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

static void draw_hud(void)
{
    /* HP bar, top-left */
    gfx_fill_rect(4, 4, 64, 14, RGB565(16, 16, 24));
    gfx_rect     (4, 4, 64, 14, RGB565(90, 90, 98));
    gfx_text(8, 7, "HP", RGB565(255, 255, 255));
    int w = 40 * G.player.hp / G.player.max_hp;
    uint16_t col = G.player.hp * 4 > G.player.max_hp
                 ? RGB565(80, 200, 80) : RGB565(220, 60, 50);
    gfx_fill_rect(24, 8, 40, 6, RGB565(50, 50, 58));
    if (w > 0)
        gfx_fill_rect(24, 8, w, 6, col);

    /* level, top-right */
    char lv[8] = "LV ";
    lv[3] = (char)('0' + G.player.level % 10);
    lv[4] = '\0';
    gfx_fill_rect(SCREEN_W - 36, 4, 32, 14, RGB565(16, 16, 24));
    gfx_text(SCREEN_W - 32, 7, lv, RGB565(255, 255, 255));
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

void dialog_start(const char *text)
{
    G.dialog_text  = text;
    G.dialog_shown = 0;
    G.state = ST_DIALOG;
    G.t = 0;
}

static int dialog_len(void)
{
    int n = 0;
    while (G.dialog_text[n]) n++;
    return n;
}

void dialog_update(void)
{
    /* keep the world rendering behind the box, but frozen */
    if (G.t % 2 == 0 && G.dialog_shown < dialog_len())
        G.dialog_shown++;

    if (PRESSED(BTN_A) || PRESSED(BTN_B)) {
        if (G.dialog_shown < dialog_len()) {
            G.dialog_shown = dialog_len();   /* impatient: show it all */
        } else {
            G.state = ST_OVERWORLD;          /* done: close the box */
            G.t = 0;
            G.battle_grace = 30;
        }
    }
}

void dialog_render(void)
{
    overworld_render();   /* the world stays visible behind the box */

    int bh = 46;
    gfx_fill_rect(4, SCREEN_H - bh - 4, SCREEN_W - 8, bh, RGB565(16, 16, 24));
    gfx_rect     (4, SCREEN_H - bh - 4, SCREEN_W - 8, bh, RGB565(255, 255, 255));

    /* greedy word-wrap of the revealed portion into 3 lines */
    char line[DIALOG_COLS + 1];
    int li = 0, row = 0;
    int lastspace = -1;
    int y0 = SCREEN_H - bh + 2;
    for (int i = 0; i < G.dialog_shown && row < 3; i++) {
        char c = G.dialog_text[i];
        if (li == 0 && c == ' ')
            continue;               /* no stray spaces after a wrap */
        if (c == ' ') lastspace = li;
        line[li++] = c;
        if (li >= DIALOG_COLS) {
            int keep = 0;
            if (c != ' ' && lastspace > 0) {        /* break at last space */
                keep = li - lastspace - 1;
                li = lastspace;
            }
            line[li] = '\0';
            gfx_text(12, y0 + row * 11, line, RGB565(255, 255, 255));
            row++;
            /* carry the split word onto the next line */
            for (int k = 0; k < keep; k++)
                line[k] = G.dialog_text[i - keep + 1 + k];
            li = keep;
            lastspace = -1;
        }
    }
    if (row < 3) {
        line[li] = '\0';
        gfx_text(12, y0 + row * 11, line, RGB565(255, 255, 255));
    }

    /* "more" arrow when fully revealed */
    if (G.dialog_shown >= dialog_len() && G.t % 40 < 25)
        gfx_text(SCREEN_W - 20, SCREEN_H - 16, "A", RGB565(255, 220, 80));
}
