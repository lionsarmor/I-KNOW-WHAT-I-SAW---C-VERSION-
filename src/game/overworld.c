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

/* ============================ THE KINDNESS OF STRANGERS =====================
 * People help -- but not all of them, and not on cue. On every map, exactly
 * ONE person is holding something for you, drawn at random the moment you
 * walk in, and you don't know who until you talk to them. They give you a
 * random item and patch you up while they're at it.
 *
 * Animals are not eligible. Neither is the van. Neither is anyone already
 * scripted to hand you something -- they have their own line to say.
 *
 * boons_done keeps one bit per map so a town is only kind ONCE; walking
 * back out the door and in again gets you a shrug. It clears when the
 * world restocks, so the towns warm up again after a day and a night.
 * ==========================================================================*/
static void roll_boon(int map)
{
    G.boon_ent = -1;
    if (G.boons_done & (1u << map))
        return;                    /* this place has already been kind today */

    /* Reservoir sample: one pass, no array, and every candidate ends up
     * equally likely to be the one. */
    int seen = 0;
    for (int i = 0; i < MAX_ENTITIES; i++) {
        const entity_t *e = &G.ents[i];
        if (e->type != ENT_NPC || !e->dialog || e->gift)
            continue;
        if (e->kind > LOOK_STOREKEEP)
            continue;              /* a cow is not going to give you a medkit */
        seen++;
        if (rng_range(1, seen) == 1)
            G.boon_ent = i;
    }
    if (G.boon_ent < 0)
        return;                    /* nobody here but the livestock */

    /* whatever they happen to have on them. Herbs are what a farm town
     * has; TNT is what somebody's frightened enough to part with. */
    static const int pool[] = {
        ITEM_HERB, ITEM_HERB, ITEM_HERB,
        ITEM_SHELLS, ITEM_SHELLS,
        ITEM_MEDKIT,
        ITEM_TNT,
    };
    G.boon_item = pool[rng_range(0, (int)(sizeof pool / sizeof pool[0]) - 1)];
}

/* What they say as they hand it over. Assembled here rather than stored in
 * the table, because the item is only decided at run time. */
static char boon_buf[192];

static void boon_cat(int *p, const char *s)
{
    while (*s && *p < (int)sizeof boon_buf - 1)
        boon_buf[(*p)++] = *s++;
}

static const char *boon_line(int kind)
{
    static const char *lead[4] = {
        "WAIT. WAIT -- TAKE THIS WITH YOU. I'M NOT GOING BACK OUT THERE "
        "TO USE IT.",
        "HERE. MY HUSBAND KEPT IT BY THE DOOR. HE HASN'T COME BACK IN "
        "TWO NIGHTS.",
        "TAKE IT. TAKE IT AND GO, AND DON'T TELL ME WHAT YOU SAW OUT "
        "THERE. I DON'T WANT TO KNOW.",
        "SIT DOWN, YOU'RE BLEEDING. ...NO. DON'T EXPLAIN. JUST TAKE THIS "
        "AND KEEP MOVING.",
    };
    int p = 0;
    boon_cat(&p, lead[rng_range(0, 3)]);
    boon_cat(&p, "\n");
    boon_cat(&p, "THEY PRESS ");
    boon_cat(&p, item_info[kind].name);
    boon_cat(&p, " INTO YOUR HANDS, PATCH YOU UP, AND LOOK AT THE SKY.");
    boon_buf[p] = '\0';
    return boon_buf;
}

/* Which song belongs to which place. Lives here (not in a table) because a
 * BATTLE has to be able to put the right one BACK when it's over -- see
 * return_to_overworld() in battle.c. Add a case when you add a map. */
int map_music(int map)
{
    switch (map) {
    case MAP_FARM:   return MUSIC_FARM;    /* home, and it is not safe    */
    case MAP_TOWN:   return MUSIC_TOWN;    /* people. a pulse. still wrong */
    case MAP_VANLOT: return MUSIC_TOWN;
    case MAP_RIDGE:  return MUSIC_BOSS;    /* you should not be up here    */
    default:         return MUSIC_NONE;    /* indoors: just the room       */
    }
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
        e->stun = e->hurt = e->aggro = 0;
        e->hp = (e->type == ENT_ALIEN) ? species[e->kind].ow_hits : 0;
        /* a hill is already working when you walk in on it */
        if (e->type == ENT_ALIEN && species[e->kind].brood > 0)
            e->timer = rng_range(30, species[e->kind].brood);
    }
    for (int i = 0; i < MAX_SHOTS; i++) G.shots[i].active = 0;
    for (int i = 0; i < MAX_GORE;  i++) G.gore[i].active  = 0;
    G.shoot_cd = 0;
    G.boom_t   = 0;

    roll_boon(map);         /* who here is holding something for you? */

    G.state  = ST_OVERWORLD;
    G.t      = 0;
    G.banner = 90;          /* name the place -- ONCE, on the way in */

    audio_music(map_music(map));
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
    G.flags = 0;                           /* nothing has happened yet */
    G.boons_done = 0;                      /* nobody has helped you yet */
    G.boon_ent = -1;
    G.boom_t = 0;
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
}

/* ---- the world restocks ----------------------------------------------------
 * Every ITEM_RESPAWN_TICKS -- one day and one night -- the world resets:
 *
 *   ITEMS you picked up are lying where they lay again. The herbs regrow,
 *   somebody leaves another box of shells on the porch. Without this the
 *   shotgun runs dry for good and the goblin is unbeatable.
 *
 *   CREATURES you killed are back. Everything you put down STAYS down until
 *   this moment -- clear a field of ants and it stays clear, walk out and
 *   back in and it's still clear -- and then the sun comes up and they have
 *   dug their way back out. The hills are open again.
 *
 * What does NOT come back, ever: a gift an NPC handed you, and a BOSS. The
 * goblin and the queen die once.
 */
static void restock_items(void)
{
    for (int m = 0; m < NUM_MAPS; m++)
        for (int i = 0; i < maps[m].nspawns && i < MAX_ENTITIES; i++) {
            const spawn_t *s = &maps[m].spawns[i];
            int back = (s->type == ENT_ITEM) ||
                       (s->type == ENT_ALIEN && !species[s->kind].boss);
            if (back)
                G.spawns_gone[m] &= ~(1u << i);
        }

    /* Anything on the map we're standing on has to appear NOW -- the map is
     * already populated, so re-entering it later isn't good enough. */
    const map_t *m = cur_map();
    for (int i = 0; i < m->nspawns && i < MAX_ENTITIES; i++) {
        const spawn_t *s = &m->spawns[i];
        if (G.ents[i].type != ENT_NONE || (G.spawns_gone[G.map_id] & (1u << i)))
            continue;
        if (s->type != ENT_ITEM && s->type != ENT_ALIEN)
            continue;
        /* Don't put a creature (or an item) down on top of the player: they'd
         * be in a fight before they saw it coming, or pocket a thing they
         * never saw. It'll be there the next time they're not standing on it. */
        if (touching(s->tx * TILE, s->ty * TILE, G.player.x, G.player.y))
            continue;

        entity_t *e = &G.ents[i];
        e->type   = s->type;
        e->kind   = s->kind;
        e->x      = s->tx * TILE;
        e->y      = s->ty * TILE;
        e->dir    = DIR_DOWN;
        e->dialog = 0;
        e->gift   = 0;
        e->after  = 0;
        e->stun = e->hurt = e->aggro = 0;
        e->timer  = rng_range(30, 90);
        e->hp     = (s->type == ENT_ALIEN) ? species[s->kind].ow_hits : 0;
    }

    /* A day has passed. People have had time to find something else they
     * can spare -- every town is willing to be kind once more. */
    G.boons_done = 0;
    roll_boon(G.map_id);        /* including the one you're standing in */

    G.toast       = "THE FIELDS GREW BACK. SO DID THEY.";
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

/* ============================ ZELDA MODE ====================================
 * Shooting things in the field. Press B with the shotgun and a shell in it:
 * a blast flies out the way you're facing. It staggers what it hits, and
 * when the thing runs out of hit points it comes apart.
 *
 * You still get a proper turn-based battle if you WALK INTO one instead --
 * both modes live side by side, and the battle pays better.
 * ==========================================================================*/

/* squared distance -- no sqrt anywhere in this file */
static int dist2(int ax, int ay, int bx, int by)
{
    int dx = ax - bx, dy = ay - by;
    return dx * dx + dy * dy;
}

static void spray_gore(int x, int y, int n)
{
    static const uint16_t blood[3] = { 0, 0, 0 };
    (void)blood;
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < MAX_GORE; i++) {
            if (G.gore[i].active)
                continue;
            gore_t *g = &G.gore[i];
            g->active = 1;
            g->x = x + rng_range(2, 13);
            g->y = y + rng_range(2, 13);
            g->vx = rng_range(-40, 40);
            g->vy = rng_range(-52, 12);
            g->life = rng_range(24, 46);
            g->col = (rng_range(0, 2) == 0) ? RGB565(140, 24, 20)
                                            : RGB565(200, 40, 32);
            break;
        }
    }
}

/* DEAD IS DEAD -- until tomorrow.
 *
 * Mark a creature's spawn slot so it does not come back the moment you step
 * out of the map and back in. Ordinary creatures are un-marked again by the
 * next restock (a day and a night later); BOSSES are never un-marked, so
 * they stay dead for the rest of the run.
 *
 * Slots at or above the map's spawn count are things an ANT HILL made at run
 * time. They have no spawn to come back from, so there is nothing to mark. */
void mark_dead(int i)
{
    const map_t *m = cur_map();
    if (i < m->nspawns)
        G.spawns_gone[G.map_id] |= (1u << i);
}

static void kill_entity(int i)
{
    entity_t *e = &G.ents[i];
    const species_t *sp = &species[e->kind];

    spray_gore(e->x, e->y, 14);
    audio_sfx(SFX_HIT);

    /* A field kill pays less than a proper fight -- see OVERWORLD_XP_PCT.
     * You took the quick way out, and the quick way pays worse. */
    G.player.xp += sp->xp * OVERWORLD_XP_PCT / 100;

    mark_dead(i);
    /* NOT "any boss" -- there are two of them now, and only the goblin's
     * death opens the road east of town. */
    if (e->kind == SPECIES_GOBLIN)
        G.flags |= FLAG_GOBLIN_DEAD;

    e->type = ENT_NONE;
}

static void hit_entity(int i)
{
    entity_t *e = &G.ents[i];
    const species_t *sp = &species[e->kind];
    e->hp--;
    e->hurt = 8;
    spray_gore(e->x, e->y, 4);      /* a spatter, not a burst */

    if (e->hp <= 0) {
        kill_entity(i);
        return;
    }

    /* staggered: it can't move and it can't grab you */
    e->stun  = sp->boss ? BOSS_STUN_TICKS : ENEMY_STUN_TICKS;
    e->aggro = 1;                   /* but it has definitely noticed you */
    audio_sfx(SFX_HURT);

    /* THE BULLETS DO NOTHING. The boss doesn't even rock back -- it takes
     * the blast standing, and then it keeps walking at you. */
    if (sp->boss)
        return;

    /* knocked back, if there's anywhere to go */
    static const int step[4][2] = { {0,1}, {0,-1}, {-1,0}, {1,0} };
    int kx = e->x + step[G.player.dir][0] * KNOCKBACK;
    int ky = e->y + step[G.player.dir][1] * KNOCKBACK;
    if (!blocked(kx, ky) && hits_entity(kx, ky, i) < 0) {
        e->x = kx;
        e->y = ky;
    }
}

/* PA'S TNT, thrown. It lands TNT_THROW ahead of you and takes the field
 * apart: everything inside TNT_RADIUS loses TNT_OW_HITS, which drops most
 * things outright, and anything still standing reels for DOUBLE the usual
 * stagger. That last part is the whole point -- it's the only way to buy
 * distance from something that barely staggers at all. */
static void throw_tnt(void)
{
    static const int step[4][2] = { {0,1}, {0,-1}, {-1,0}, {1,0} };
    int bx = G.player.x + 8 + step[G.player.dir][0] * TNT_THROW;
    int by = G.player.y + 8 + step[G.player.dir][1] * TNT_THROW;

    G.player.items[ITEM_TNT]--;
    G.boom_x = bx;
    G.boom_y = by;
    G.boom_t = TNT_BOOM_TICKS;
    audio_sfx(SFX_STING);            /* the crash, falling into a low drone */
    rumble(255);                     /* it is DYNAMITE */

    for (int i = 0; i < MAX_ENTITIES; i++) {
        entity_t *e = &G.ents[i];
        if (e->type != ENT_ALIEN)
            continue;
        if (dist2(e->x + 8, e->y + 8, bx, by) > TNT_RADIUS * TNT_RADIUS)
            continue;

        e->hp -= TNT_OW_HITS;
        if (e->hp <= 0) {
            kill_entity(i);          /* pays the usual field XP */
            continue;
        }
        /* it lived. It is not going anywhere for a while. */
        e->hurt  = 10;
        e->aggro = 1;
        e->stun  = (species[e->kind].boss ? BOSS_STUN_TICKS : ENEMY_STUN_TICKS)
                 * TNT_STUN_MULT;
        spray_gore(e->x, e->y, 6);
    }
    spray_gore(bx - 8, by - 8, 10);  /* dirt and worse, straight up */
}

static void fire_shot(void)
{
    static const int step[4][2] = { {0,1}, {0,-1}, {-1,0}, {1,0} };
    for (int i = 0; i < MAX_SHOTS; i++) {
        if (G.shots[i].active)
            continue;
        shot_t *sh = &G.shots[i];
        sh->active    = 1;
        sh->x         = G.player.x + 4;
        sh->y         = G.player.y + 4;
        sh->dx        = step[G.player.dir][0] * SHOT_SPEED;
        sh->dy        = step[G.player.dir][1] * SHOT_SPEED;
        sh->travelled = 0;
        break;
    }
    G.player.items[ITEM_SHELLS]--;
    G.shoot_cd = SHOOT_COOLDOWN;
    G.recoil   = 6;
    audio_sfx(SFX_SHOTGUN);
    rumble(150);              /* the gun kicks */
}

static void try_shoot(void)
{
    if (!G.player.items[ITEM_SHOTGUN] || G.shoot_cd > 0)
        return;
    if (G.player.items[ITEM_SHELLS] <= 0) {
        audio_sfx(SFX_BLIP);        /* click. empty. */
        G.shoot_cd = 12;
        return;
    }
    fire_shot();
}

static void update_shots(void)
{
    for (int i = 0; i < MAX_SHOTS; i++) {
        shot_t *sh = &G.shots[i];
        if (!sh->active)
            continue;

        sh->x += sh->dx;
        sh->y += sh->dy;
        sh->travelled += SHOT_SPEED;

        if (sh->travelled > SHOT_RANGE || blocked(sh->x - 4, sh->y - 4)) {
            sh->active = 0;         /* spent, or buried in a fence */
            continue;
        }
        /* did it find something? */
        for (int e = 0; e < MAX_ENTITIES; e++) {
            entity_t *en = &G.ents[e];
            if (en->type != ENT_ALIEN)
                continue;           /* you cannot shoot the livestock */
            if (sh->x < en->x + 2 || sh->x > en->x + 14 ||
                sh->y < en->y + 2 || sh->y > en->y + 14)
                continue;
            sh->active = 0;
            hit_entity(e);
            break;
        }
    }
}

static void update_gore(void)
{
    for (int i = 0; i < MAX_GORE; i++) {
        gore_t *g = &G.gore[i];
        if (!g->active)
            continue;
        g->x  += g->vx / 16;
        g->y  += g->vy / 16;
        g->vy += 4;                 /* it falls */
        if (--g->life <= 0)
            g->active = 0;
    }
}

/* ---- update ----------------------------------------------------------------*/

/* the player pushed into entity `i`: NPCs are simply solid, but pushing
 * into a visitor starts the fight (unless we just fled one) */
static void bump_entity(int i)
{
    /* Walk into a visitor and you get the FULL turn-based fight (and the
     * full XP). But something you've already staggered can't grab you --
     * finish it off with the gun. */
    if (G.ents[i].type == ENT_ALIEN && !G.battle_grace && !G.ents[i].stun)
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

    for (int i = 0; i < m->nwarps; i++) {
        if (m->warps[i].tx != tx || m->warps[i].ty != ty)
            continue;

        /* Some doors want the brass key. Without it they're just another
         * locked door; with it, they open. */
        if (m->warps[i].needs_flag && !(G.flags & m->warps[i].needs_flag)) {
            if (locked_cd == 0 && m->warps[i].shut)
                dialog_start(m->warps[i].shut);
            locked_cd = 40;
            return;
        }
        if (m->warps[i].needs_key && !G.player.items[ITEM_KEY]) {
            if (locked_cd == 0)
                dialog_start("A HEAVY BRASS LOCK. THIS ONE ISN'T "
                             "JUST STUCK -- SOMEBODY LOCKED IT.");
            locked_cd = 30;
            return;
        }

        audio_sfx(SFX_CONFIRM);
        overworld_enter_map(m->warps[i].dest_map,
                            m->warps[i].dest_tx, m->warps[i].dest_ty);
        return;
    }
    /* a door with no warp at all: locked, and nobody's home */
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
    for (int i = 0; i < m->nwarps; i++) {
        const warp_t *w = &m->warps[i];
        if (w->tx != tx || w->ty != ty)
            continue;

        /* Some roads don't open until the story says so. */
        if (w->needs_flag && !(G.flags & w->needs_flag)) {
            if (locked_cd == 0 && w->shut)
                dialog_start(w->shut);
            locked_cd = 40;
            return;
        }
        overworld_enter_map(w->dest_map, w->dest_tx, w->dest_ty);
        return;
    }
}

/* ---- WHAT COMES OUT OF THE HILL -------------------------------------------
 * The ant hill pushes a new ant out onto a free tile beside it. The new ant
 * takes an entity slot ABOVE the map's scripted spawn count -- those slots
 * belong to nobody, so a creature born here has no spawn bit to leave behind
 * when it dies (see mark_dead). Returns 0 if there was no room, in which
 * case the hill just seethes and tries again later.
 */
static int hill_brood_count(int hill)
{
    const map_t *m = cur_map();
    int n = 0;
    for (int i = m->nspawns; i < MAX_ENTITIES; i++)
        if (G.ents[i].type == ENT_ALIEN)
            n++;
    (void)hill;          /* the cap is per-map, which is what matters */
    return n;
}

static int hill_spawn(int hill)
{
    const map_t *m = cur_map();
    entity_t *h = &G.ents[hill];

    int slot = -1;
    for (int i = m->nspawns; i < MAX_ENTITIES; i++)
        if (G.ents[i].type == ENT_NONE) { slot = i; break; }
    if (slot < 0)
        return 0;                              /* the map is full of ants */

    /* somewhere clear, right next to the mound */
    static const int around[8][2] = {
        {-1,0},{1,0},{0,-1},{0,1},{-1,-1},{1,-1},{-1,1},{1,1}
    };
    int first = rng_range(0, 7);
    for (int k = 0; k < 8; k++) {
        const int *o = around[(first + k) % 8];
        int nx = h->x + o[0] * TILE, ny = h->y + o[1] * TILE;
        if (blocked(nx, ny) || hits_entity(nx, ny, -1) >= 0)
            continue;
        if (touching(nx, ny, G.player.x, G.player.y))
            continue;                          /* not straight into your face */

        entity_t *e = &G.ents[slot];
        e->type   = ENT_ALIEN;
        e->kind   = (rng_range(1, 100) <= ANTHILL_SOLDIER_PCT)
                  ? SPECIES_SOLDIER : SPECIES_ANT;
        e->x      = nx;
        e->y      = ny;
        e->dir    = DIR_DOWN;
        e->timer  = rng_range(20, 60);
        e->dialog = 0;
        e->gift   = 0;
        e->after  = 0;
        e->stun   = e->hurt = 0;
        e->aggro  = 0;                 /* it doesn't know where you are --
                                          it crawls out and casts about like
                                          anything else, and comes for you
                                          when it gets close enough */
        e->hp     = species[e->kind].ow_hits;
        return 1;
    }
    return 0;                                  /* hemmed in -- try again later */
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

        const species_t *sp = &species[e->kind];

        if (e->hurt > 0) e->hurt--;
        if (e->stun > 0) {          /* staggered: it isn't going anywhere */
            e->stun--;
            continue;
        }

        /* THE ANT HILL. It is rooted -- it will never take a step in its
         * life. All it does is open up and push another one out. Kill it and
         * the spawning stops; what it already made is still out there. */
        if (sp->rooted) {
            if (sp->brood > 0 && --e->timer <= 0) {
                e->timer = sp->brood;
                if (hill_brood_count(i) < BROOD_MAX)
                    hill_spawn(i);
            }
            continue;
        }

        int boss = sp->boss;

        /* HAS IT SEEN YOU? Once it has, it comes. It does not lose interest.
         *
         * The boss is the exception: it never prowls and it never spots you
         * on its own -- it just stands there, waiting, and if you walk into
         * it you get the turn-based fight. But put a shell into it (hit_entity
         * sets aggro) and it comes off that spot at a dead run. */
        if (!e->aggro && !boss &&
            dist2(e->x, e->y, G.player.x, G.player.y)
                < AGGRO_RADIUS * AGGRO_RADIUS)
            e->aggro = 1;

        if (!e->aggro && boss)
            continue;      /* it waits for you. */

        int nx, ny;
        if (e->aggro) {
            /* charge: close whichever gap is bigger, so it comes at you
             * in a straight-ish line instead of drifting diagonally.
             *
             * A species can override the speed of its class (the QUEEN does:
             * she is faster than you can run). 0 = take the default. */
            int speed = sp->ow_speed;
            if (speed <= 0)
                speed = boss ? BOSS_CHASE_SPEED : CHASE_SPEED;
            int dx = G.player.x - e->x, dy = G.player.y - e->y;
            if ((dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy))
                e->dir = (dx > 0) ? DIR_RIGHT : DIR_LEFT;
            else
                e->dir = (dy > 0) ? DIR_DOWN : DIR_UP;
            nx = e->x + step[e->dir][0] * speed;
            ny = e->y + step[e->dir][1] * speed;
        } else {
            /* wander: walk a while, then pick a new direction (or pause) */
            if (--e->timer <= 0) {
                e->dir   = rng_range(0, 3);
                e->timer = rng_range(30, 120);
            }
            nx = e->x + step[e->dir][0] * ALIEN_WANDER_SPEED;
            ny = e->y + step[e->dir][1] * ALIEN_WANDER_SPEED;
        }

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
            /* This map's one generous soul (see roll_boon). They hand it
             * over, they patch you up, and after that they're just another
             * frightened neighbour with a line to say. */
            if (i == G.boon_ent) {
                int kind = G.boon_item;
                G.player.items[kind] +=
                    (kind == ITEM_SHELLS) ? SHELLS_PER_BOX : 1;
                pack_discover(kind);
                G.player.hp += BOON_HEAL;
                if (G.player.hp > G.player.max_hp)
                    G.player.hp = G.player.max_hp;
                G.boons_done |= (uint16_t)(1u << G.map_id);
                G.boon_ent = -1;
                audio_sfx(SFX_PICKUP);
                dialog_start(boon_line(kind));
                return;
            }

            /* the van is not a person. Talk to it and you leave. */
            if (e->kind == LOOK_VAN) {
                audio_sfx(SFX_CONFIRM);
                drive_start();
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
    if (G.banner > 0)
        G.banner--;

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

    if (G.shoot_cd > 0) G.shoot_cd--;
    if (G.recoil   > 0) G.recoil--;
    if (G.boom_t   > 0) G.boom_t--;
    update_shots();
    update_gore();

    if (PRESSED(BTN_A))
        try_talk();

    /* B = the family shotgun, right here in the field.
     *
     * This is the PRESS, not the hold: B also closes the pack and the
     * dialog box, and if we read the hold we'd fire the instant those
     * closed -- the same button-down that dismissed the menu was still
     * down. One press, one shell. */
    if (PRESSED(BTN_B))
        try_shoot();

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
 * It sits straight on top of the world: NO panel, NO black box. Every
 * glyph is drawn in the 3x5 small font with a 1px dark halo around it
 * (gfx_text_small_outlined), which is what makes it readable over grass,
 * dirt, or a pitch-black field at night without covering any of it up.
 *
 *      NAME            LV 3
 *      [========--]           <- health
 *      [====------]           <- the climb to the next level
 *      (key) (lamp)           <- only what you're actually carrying
 *
 * All of it lives in the top-left corner and is about 60x22 -- a quarter
 * of what the old boxed version ate.
 */
static void bar(int x, int y, int w, int h, int val, int max,
                uint16_t fill, uint16_t back)
{
    /* a 1px dark frame so the bar reads over grass, same trick as the text */
    gfx_rect(x - 1, y - 1, w + 2, h + 2, RGB565(8, 8, 12));
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
    const int X = 5, Y = 4;
    const int BW = 52;              /* bar width */

    /* name */
    gfx_text_small_outlined(X, Y, G.player.name[0] ? G.player.name
                                                   : NAME_DEFAULT,
                            RGB565(255, 255, 255));

    /* level, tucked to the right of the name */
    char lv[8] = "LV ";
    int  l = G.player.level, p = 3;
    if (l > 9) lv[p++] = (char)('0' + (l / 10) % 10);
    lv[p++] = (char)('0' + l % 10);
    lv[p] = '\0';
    gfx_text_small_outlined(X + BW - gfx_text_small_width(lv) + 2, Y, lv,
                            RGB565(255, 236, 150));

    /* health */
    bar(X + 1, Y + 8, BW, 4, G.player.hp, G.player.max_hp,
        G.player.hp * 4 > G.player.max_hp ? RGB565(80, 200, 80)
                                          : RGB565(220, 60, 50),
        RGB565(40, 40, 48));

    /* the climb to the next level (you level at level * XP_PER_LEVEL) */
    bar(X + 1, Y + 15, BW, 3, G.player.xp, G.player.level * XP_PER_LEVEL,
        RGB565(120, 190, 255), RGB565(34, 38, 52));

    /* ---- what you're carrying, only if you have it ----------------------*/
    int ix = X, iy = Y + 21;
    if (G.player.items[ITEM_KEY]) {
        gfx_blit(sprites[SPR_ITEM_KEY].px, TILE, TILE, ix - 4, iy - 4);
        ix += 12;
    }
    if (G.player.items[ITEM_FLASHLIGHT]) {
        gfx_blit_ex(sprites[SPR_ITEM_FLASHLIGHT].px, TILE, TILE, ix - 4, iy - 4,
                    1, G.player.lamp ? 256 : 110, 0);
        if (G.player.lamp)                       /* a glow off the lens */
            gfx_fill_rect(ix + 1, iy + 10, 6, 2, RGB565(255, 240, 170));
        ix += 12;
    }
    if (G.player.items[ITEM_SHOTGUN]) {
        char am[8];
        int n = G.player.items[ITEM_SHELLS], k = 0;
        if (n > 99) n = 99;
        if (n >= 10) am[k++] = (char)('0' + n / 10);
        am[k++] = (char)('0' + n % 10);
        am[k] = '\0';
        gfx_blit(sprites[SPR_ITEM_SHELLS].px, TILE, TILE, ix - 4, iy - 4);
        gfx_text_small_outlined(ix + 11, iy + 4, am, RGB565(255, 220, 80));
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
        int spr, bright = 256, bob = 0, flip = 0;
        if (e->type == ENT_ALIEN) {
            const species_t *sp = &species[e->kind];
            bright = sp->bright;
            if (sp->dirs) {
                /* an ANT: six frames, and it faces the way it's walking.
                 * The legs alternate quickly -- they scuttle, they don't
                 * float. */
                int f = (int)((G.frame / 8 + (uint32_t)i) % 2);
                switch (e->dir) {
                case DIR_UP:    spr = sp->spr0 + 2 + f; break;
                case DIR_LEFT:  spr = sp->spr0 + 4 + f; break;
                case DIR_RIGHT: spr = sp->spr0 + 4 + f; flip = 1; break;
                default:        spr = sp->spr0 + f;     break;  /* DOWN */
                }
            } else {
                /* the goblin: it doesn't walk. it hovers, slightly out of
                 * sync with everything else (the i*7 offset). */
                spr = ((G.frame / 20) % 2) ? sp->spr1 : sp->spr0;
                bob = (int)(((G.frame + (uint32_t)i * 7) / 16) % 2);
            }
        } else {
            const npc_look_t *lk = &npc_looks[e->kind];
            spr = ((G.frame / 45) % 2) ? lk->spr1 : lk->spr0;
        }
        /* a blast makes it flash, and a staggered thing shudders */
        int ex = e->x - cx, ey = e->y - cy - bob;
        if (e->stun > 0)
            ex += ((G.frame / 2) % 2) ? 1 : -1;
        if (e->hurt > 0)
            gfx_blit_flash(sprites[spr].px, TILE, TILE, ex, ey,
                           RGB565(255, 255, 255), flip);   /* BLAM */
        else
            gfx_blit_ex(sprites[spr].px, TILE, TILE, ex, ey, 1, bright, flip);
    }

    /* the player */
    int spr, flip;
    player_sprite(&spr, &flip);
    int pxs = G.player.x - cx, pys = G.player.y - cy;
    gfx_blit_ex(sprites[spr].px, TILE, TILE, pxs, pys, 1, 256, flip);

    /* THE FAMILY SHOTGUN, carried. Once he has it, you can see it -- and
     * it kicks when he fires. */
    if (G.player.items[ITEM_SHOTGUN]) {
        int gspr = SPR_CARRY_SIDE, gflip = 0, gx = pxs, gy = pys;
        switch (G.player.dir) {
        case DIR_UP:    gspr = SPR_CARRY_UP;   gy -= (G.recoil ? 2 : 0); break;
        case DIR_DOWN:  gspr = SPR_CARRY_DOWN; gy += (G.recoil ? 2 : 0); break;
        case DIR_LEFT:  gspr = SPR_CARRY_SIDE; gx -= (G.recoil ? 2 : 0); break;
        default:        gspr = SPR_CARRY_SIDE; gflip = 1;
                        gx += (G.recoil ? 2 : 0); break;
        }
        gfx_blit_ex(sprites[gspr].px, TILE, TILE, gx, gy, 1, 256, gflip);
        /* muzzle flash on the frame the gun goes off */
        if (G.recoil > 3) {
            static const int mz[4][2] = { {6,15}, {6,-3}, {-3,7}, {15,7} };
            gfx_fill_rect(pxs + mz[G.player.dir][0] - 2,
                          pys + mz[G.player.dir][1] - 2, 6, 6,
                          RGB565(255, 240, 170));
        }
    }

    /* the blasts in flight */
    for (int i = 0; i < MAX_SHOTS; i++) {
        if (!G.shots[i].active)
            continue;
        int sx = G.shots[i].x - cx, sy = G.shots[i].y - cy;
        gfx_fill_rect(sx, sy, 3, 3, RGB565(255, 244, 190));
        gfx_fill_rect(sx - G.shots[i].dx / 3, sy - G.shots[i].dy / 3,
                      2, 2, RGB565(210, 170, 90));
    }

    /* ...and what's left over */
    for (int i = 0; i < MAX_GORE; i++) {
        gore_t *g = &G.gore[i];
        if (!g->active)
            continue;
        int gsz = (g->life > 12) ? 2 : 1;
        gfx_fill_rect(g->x - cx, g->y - cy, gsz, gsz, g->col);
    }

    /* PA'S TNT GOING OFF. It swells to full size in half its life, whites
     * out at the core, and then eats itself away into smoke -- the dither
     * gets holier as it dies, so it comes apart instead of just fading. */
    int boom_r = 0;
    if (G.boom_t > 0) {
        int age = TNT_BOOM_TICKS - G.boom_t;
        boom_r = TNT_RADIUS * (age + 4) / (TNT_BOOM_TICKS / 2 + 4);
        if (boom_r > TNT_RADIUS)
            boom_r = TNT_RADIUS;

        int bx = G.boom_x - cx, by = G.boom_y - cy;
        int holes = 8 - G.boom_t * 8 / TNT_BOOM_TICKS;   /* 0 -> 8 as it dies */
        for (int y = -boom_r; y <= boom_r; y++)
            for (int x = -boom_r; x <= boom_r; x++) {
                int d2 = x * x + y * y;
                if (d2 > boom_r * boom_r)
                    continue;
                if (((x * 7 + y * 13 + (int)G.frame) & 7) < holes)
                    continue;                     /* burnt through */
                int d = d2 * 256 / (boom_r * boom_r + 1);
                uint16_t col = (d <  90) ? RGB565(255, 250, 220)   /* core  */
                             : (d < 170) ? RGB565(255, 176,  48)   /* fire  */
                                         : RGB565(190,  70,  24);  /* smoke */
                gfx_pixel(bx + x, by + y, col);
            }
    }

    /* NIGHT falls on the outdoors (indoors the lamps stay lit).
     *
     * We gather every light on screen first: your flashlight, and every
     * street lamp you can see. Applied before the HUD, so the HUD stays
     * readable in the dark. */
    if (m->outdoor) {
        light_t lights[MAX_LIGHTS];
        int n = 0;

        /* the TNT, for as long as it burns, lights the whole field */
        if (boom_r > 0) {
            lights[n].x = G.boom_x - cx;
            lights[n].y = G.boom_y - cy;
            lights[n].r = boom_r * 3;
            lights[n].warm = 200;
            n++;
        }

        /* the street lamps -- find the LAMP tiles the camera can see */
        for (int ty = cy / TILE; ty <= (cy + SCREEN_H) / TILE
                                  && n < MAX_LIGHTS; ty++)
            for (int tx = cx / TILE; tx <= (cx + SCREEN_W) / TILE
                                      && n < MAX_LIGHTS; tx++) {
                if (map_tile(m, tx, ty) != TILE_LAMP)
                    continue;
                lights[n].x = tx * TILE - cx + TILE / 2;
                lights[n].y = ty * TILE - cy + 2;   /* the head, not the post */
                /* a sodium lamp buzzes and it is not steady. */
                lights[n].r = LAMP_RADIUS
                            - (int)(((G.frame + (uint32_t)(tx * 7 + ty)) / 5) % 3);
                lights[n].warm = 210;
                n++;
            }

        /* ...and your torch, which is white and much less friendly */
        if (G.player.items[ITEM_FLASHLIGHT] && G.player.lamp
            && n < MAX_LIGHTS) {
            lights[n].x = G.player.x - cx + TILE / 2;
            lights[n].y = G.player.y - cy + TILE / 2;
            lights[n].r = FLASHLIGHT_RADIUS;
            lights[n].warm = 0;
            n++;
        }

        gfx_night(day_brightness(), lights, n);
    }

    draw_hud();

    /* map name banner for the first ~1.5s after ARRIVING (not after every
     * dialog: see G.banner) */
    if (G.banner > 0) {
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
        if (s[i] == '\n') {           /* a deliberate break: eat it */
            i++;
            continue;
        }
        if (!s[i])
            break;

        int begin = i, last_space = -1, n = 0;
        int hard = 0;                  /* did we stop on a '\n'? */
        while (s[i] && n < DIALOG_COLS) {
            if (s[i] == '\n') {       /* HARD BREAK -- end the line here.
                                          Without this the '\n' would ride
                                          along inside the row and gfx_text
                                          would drop a second line on top of
                                          the one below it. */
                hard = 1;
                break;
            }
            if (s[i] == ' ')
                last_space = i;
            i++;
            n++;
        }
        /* if we stopped in the middle of a word, back up to the last space */
        int stop = i;
        if (!hard && s[i] && s[i] != ' ' && last_space > begin)
            stop = last_space;

        int k = 0;
        for (int j = begin; j < stop && k < DIALOG_COLS; j++)
            out[r][k++] = s[j];
        out[r][k] = '\0';

        i = stop;
        if (hard)
            i++;                       /* step over the newline itself */
        r++;
    }

    *rows = r;
    while (s[i] == ' ' || s[i] == '\n')   /* don't leave a space or a stray
                                              break leading the next page */
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
    return kind == ITEM_HERB || kind == ITEM_MEDKIT ||
           kind == ITEM_FLASHLIGHT || kind == ITEM_TNT;
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
        G.t = 0;
        G.battle_grace = 20;       /* no ambush the instant you close it */
        G.shoot_cd = 10;           /* ...and no accidental shot, either */
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

    case ITEM_TNT:
        /* You light it and throw it -- so get out of the menu first. You
         * are going to want to watch this, and to be facing the right way
         * when it lands. */
        G.state = ST_OVERWORLD;
        G.t = 0;
        G.battle_grace = 30;
        G.shoot_cd = 10;           /* and no stray shell on the way out */
        throw_tnt();
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
        G.t = 0;
        G.banner = 0;                  /* you never left the room */
        G.battle_grace = 30;
        audio_music(map_music(G.map_id));
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
        gfx_night(dim, 0, 0);          /* no lantern: this is the whole room */

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
