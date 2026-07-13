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

/* Do two boxes overlap? The 3px inset is the same forgiveness `touching`
 * gives: sprites have transparent margins, and a hitbox that matched the
 * sprite's full square would feel sticky. */
static int boxes_hit(int ax, int ay, int aw, int ah,
                     int bx, int by, int bw, int bh)
{
    return ax + 3 < bx + bw - 3 && bx + 3 < ax + aw - 3 &&
           ay + 3 < by + bh - 3 && by + 3 < ay + ah - 3;
}

/* would a character standing at (x,y) overlap a living entity?
 * returns its index, or -1. `self` skips one entity (pass -1 for the
 * player). Characters are solid to each other -- this is what stops
 * everyone walking through everyone.
 *
 * Entities carry their OWN box size (cw/ch) because the van is four tiles
 * wide and you should not be able to walk through it. */
static int hits_entity(int x, int y, int self)
{
    for (int i = 0; i < MAX_ENTITIES; i++) {
        const entity_t *e = &G.ents[i];
        if (i == self || e->type == ENT_NONE)
            continue;
        if (boxes_hit(x, y, TILE, TILE, e->x, e->y, e->cw, e->ch))
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
static char boon_buf[DIALOG_BUF_MAX];

static void boon_cat(int *p, const char *s)
{
    while (*s && *p < (int)sizeof boon_buf - 1)
        boon_buf[(*p)++] = *s++;
}

/* THE NPC STILL GETS TO SPEAK.
 *
 * This used to return only the gift line, which meant that whenever somebody
 * happened to be the map's generous one, the thing they were actually WRITTEN
 * to say was silently thrown away -- a random item was quietly costing the
 * player a piece of the story. Now their own line runs first, and the gift is
 * a page after it. */
static const char *boon_line(const char *npc_says, int kind)
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
    if (npc_says && npc_says[0]) {
        boon_cat(&p, npc_says);      /* what they were always going to say */
        boon_cat(&p, "\n");
    }
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
    case MAP_CITY:   return MUSIC_TOWN;    /* more people, same pulse      */
    case MAP_SOUTH:  return MUSIC_TOWN;    /* same city, deeper in         */
    default:         return MUSIC_NONE;    /* indoors: just the room --
                                              and the OFFICE stays silent
                                              on purpose. So does the
                                              DINER. Listen.               */
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

        /* one tile each, except the van -- which is a VAN (see the render) */
        e->cw = e->ch = TILE;
        if (e->type == ENT_NPC && e->kind == LOOK_VAN) {
            e->cw = VANTOP_W;      /* 48 -- three tiles wide */
            e->ch = VANTOP_H;      /* 64 -- four tiles long  */
        }
        if (e->type == ENT_NPC && e->kind == LOOK_COPCAR) {
            e->cw = COPCAR_W;      /* 48 -- parked along the kerb */
            e->ch = COPCAR_H;      /* 32 -- and solid, like a car */
        }

        /* a hill is already working when you walk in on it */
        if (e->type == ENT_ALIEN && species[e->kind].brood > 0)
            e->timer = rng_range(30, species[e->kind].brood);
    }
    for (int i = 0; i < MAX_SHOTS; i++) G.shots[i].active = 0;
    for (int i = 0; i < MAX_GORE;  i++) G.gore[i].active  = 0;
    G.shoot_cd   = 0;
    G.boom_t     = 0;
    G.mist_t     = 0;       /* effects don't follow you through a door */
    G.lob_active = 0;       /* ...and neither does a thing mid-air     */

    /* SOMEBODY DROPPED THEIR NERVE. Outdoors, sometimes, a stick of TNT is
     * just lying in the open -- dropped by whoever ran through here before
     * you. Rolled fresh on every visit, placed in a runtime slot above the
     * scripted spawns (same slots the ant hill uses), so it owes nothing
     * to spawns_gone and nothing to the restock clock. */
    if (m->outdoor && rng_range(1, 100) <= 12) {
        int slot = -1;
        for (int i = m->nspawns; i < MAX_ENTITIES; i++)
            if (G.ents[i].type == ENT_NONE) { slot = i; break; }
        for (int tries = 0; slot >= 0 && tries < 24; tries++) {
            int rx = rng_range(1, m->w - 2), ry = rng_range(1, m->h - 2);
            int px = rx * TILE, py = ry * TILE;
            if (tile_solid[map_tile(m, rx, ry)])
                continue;
            if (hits_entity(px, py, -1) >= 0)
                continue;
            if (touching(px, py, G.player.x, G.player.y))
                continue;                 /* not straight into your pocket */
            entity_t *e = &G.ents[slot];
            e->type = ENT_ITEM;
            e->kind = ITEM_TNT;
            e->x = px;  e->y = py;
            e->cw = e->ch = TILE;
            e->dialog = 0; e->gift = 0; e->after = 0;
            e->stun = e->hurt = e->aggro = e->hp = 0;
            break;
        }
    }

    roll_boon(map);         /* who here is holding something for you? */

    G.state  = ST_OVERWORLD;
    G.t      = 0;
    G.banner = 90;          /* name the place -- ONCE, on the way in */
    G.warp_cd = 10;         /* no walk-on warp fires the instant you arrive:
                               a held direction key must not bounce you
                               straight back through the door you used */

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
    G.player.lamp   = 0;
    G.player.rosary = 0;                   /* beads back in the pocket */
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
    G.species_seen = 0;                    /* an empty journal... */
    for (int i = 0; i < NUM_SPECIES; i++)
        G.species_kills[i] = 0;            /* ...and nothing put down yet */
    blood_clear();                         /* the ground starts clean */
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
    blood_clear();   /* the same clock washes the streets */

    for (int m = 0; m < NUM_MAPS; m++)
        for (int i = 0; i < maps[m].nspawns && i < MAX_ENTITIES; i++) {
            const spawn_t *s = &maps[m].spawns[i];
            /* HOLY WATER NEVER COMES BACK. Three vials exist in the whole
             * game; a herb-style respawn would quietly turn "three" into
             * "three per day" -- a different item entirely. */
            int back = (s->type == ENT_ITEM && s->kind != ITEM_HOLYWATER) ||
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
        e->cw = e->ch = TILE;
        e->hp     = (s->type == ENT_ALIEN) ? species[s->kind].ow_hits : 0;
    }

    /* A day has passed. People have had time to find something else they
     * can spare -- every town is willing to be kind once more. */
    G.boons_done = 0;
    roll_boon(G.map_id);        /* including the one you're standing in */

    /* Say it in the language of WHERE YOU'RE STANDING: fields grow back on
     * a farm; a city just goes quiet for a while, and quiet isn't safe. */
    G.toast = (G.map_id == MAP_CITY || G.map_id == MAP_OFFICE)
        ? "THE CITY IS CALMER AFTER THE NIGHT. IT WON'T LAST."
        : "THE FIELDS GREW BACK. SO DID THEY.";
    G.toast_good  = 1;
    G.toast_ticks = 120;   /* every toast lives 120 ticks -- draw_toast's
                              slide-in is keyed to that number */
}

/* ---- day & night -----------------------------------------------------------
 * One clock, one curve: 256 in daylight, NIGHT_BRIGHTNESS at night,
 * with a short linear dusk/dawn ramp between. The lengths live in
 * config.h (set to 1 minute each right now, for testing).
 */
/* ============================ THE WEATHER ==================================
 * The sky does its own thing, on its own clock, whether or not you're
 * looking. It only ever SHOWS outdoors -- indoors you just hear about it
 * from the light going out of the windows.
 *
 * Rolled fresh each time the current spell runs out. Rain is common enough
 * to matter; a tornado is rare, and when one comes it walks across the map
 * from one side to the other and then it's gone.
 * ==========================================================================*/
static void weather_roll(void)
{
    int r = rng_range(1, 100);

    /* NO TORNADOES DOWNTOWN. The city gets rain and searchlights; a funnel
     * walking between the towers would be a different game. A tornado roll
     * here just falls through and becomes rain. */
    if (r <= WX_TORNADO_PCT &&
        (G.map_id == MAP_CITY || G.map_id == MAP_SOUTH))
        r = WX_TORNADO_PCT + 1;

    if (r <= WX_TORNADO_PCT) {
        G.wx   = WX_TORNADO;
        G.wx_t = WX_TORNADO_LEN;
        /* it enters from one side and walks to the other */
        G.wx_x = rng_range(0, 1) ? -60 : cur_map()->w * TILE + 60;
    } else if (r <= WX_TORNADO_PCT + WX_RAIN_PCT) {
        G.wx   = WX_RAIN;
        G.wx_t = rng_range(WX_RAIN_MIN, WX_RAIN_MAX);
    } else {
        G.wx   = WX_CLEAR;
        G.wx_t = rng_range(WX_CLEAR_MIN, WX_CLEAR_MAX);
    }
    G.wx_flash = 0;
}

static void weather_update(void)
{
    if (--G.wx_t <= 0)
        weather_roll();

    if (G.wx_flash > 0)
        G.wx_flash--;

    if (G.wx == WX_RAIN) {
        /* lightning: rare, and it comes with the sky splitting open */
        if (rng_range(1, 260) == 1) {
            G.wx_flash = 5;
            audio_sfx(SFX_STING);
            shake(2, 20);
        }
    } else if (G.wx == WX_TORNADO) {
        /* it walks. it does not hurry. */
        int target = cur_map()->w * TILE / 2;
        G.wx_x += (G.wx_x < target) ? 1 : -1;
        /* the ground shakes the whole time it's on the map */
        shake(2, 4);
    }
}

/* how much the weather is eating the daylight */
static int weather_dim(void)
{
    if (G.wx == WX_RAIN)    return RAIN_DIM;
    if (G.wx == WX_TORNADO) return RAIN_DIM - 40;
    return 256;
}

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

/* the light actually reaching the ground: the sun, minus the sky */
static int world_brightness(void)
{
    return day_brightness() * weather_dim() / 256;
}

/* ---- what the place SOUNDS like --------------------------------------------
 * The bed under the music (see AMBIENCE in audio.h). Decided from where
 * you're standing and what the sky is doing, every tick -- audio_ambient()
 * no-ops until the answer changes, so dusk fading in brings the crickets
 * up with it, mid-map, without anyone keeping track.
 *
 *   THE RIDGE      wind, day and night. It's high ground; it never stops.
 *   THE CITY       the hum. Brad says it's the power lines. The power has
 *                  been out since the office tower went dark. It hums.
 *   anywhere else  crickets outdoors once the light drops past dusk's
 *                  midpoint -- and indoors, nothing: walls work.
 */
static void ambient_refresh(void)
{
    int amb = AMB_NONE;
    if (cur_map()->outdoor) {
        if (G.map_id == MAP_RIDGE)
            amb = AMB_WIND;
        else if (G.map_id == MAP_CITY || G.map_id == MAP_SOUTH)
            amb = AMB_HUM;
        else if (day_brightness() < 200)
            amb = AMB_CRICKETS;
    }
    audio_ambient(amb);
}

/* ============================ ZELDA MODE ====================================
 * Shooting things in the field. Press B with the shotgun and a shell in it:
 * a blast flies out the way you're facing. It staggers what it hits, and
 * when the thing runs out of hit points it comes apart.
 *
 * You still get a proper turn-based battle if you WALK INTO one instead --
 * both modes live side by side, and the battle pays better.
 * ==========================================================================*/

/* a triangle wave in [-128, 127] with the given period. No floats, no rng:
 * the searchlights must wander the same way on every machine. */
static int tri_wave(uint32_t t, uint32_t period)
{
    uint32_t p = t % period;
    int v = (int)(p * 256 / period);        /* 0..255 */
    return (v < 128) ? (v * 2 - 128) : (383 - v * 2);
}

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

/* A KILL SOAKS THE GROUND. The spray (above) flies and fades; this is the
 * part that stays. One pool per body, on the map you're standing on, until
 * the restock clock washes it (blood_clear, called from restock_items).
 * When every slot is wet the oldest pool dries over and its slot is
 * taken -- blood_head walks the array like a ring. */
void add_blood(int x, int y, int big)
{
    int slot = -1;
    for (int i = 0; i < MAX_BLOOD; i++)
        if (G.blood[i].map < 0) { slot = i; break; }
    if (slot < 0) {
        slot = G.blood_head;
        G.blood_head = (G.blood_head + 1) % MAX_BLOOD;
    }
    blood_t *b = &G.blood[slot];
    b->map  = G.map_id;
    b->x    = x;
    b->y    = y;
    b->big  = big;
    b->seed = rng_next();
}

/* ...and the cycle washes it. Also the ONLY thing that makes the array
 * sane: a zeroed G would mean a pool at (0,0) of map 0, so anything that
 * starts a game -- fresh or loaded -- has to call this first. */
void blood_clear(void)
{
    for (int i = 0; i < MAX_BLOOD; i++)
        G.blood[i].map = -1;
    G.blood_head = 0;
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

/* THE FIELD LEVELS YOU TOO. Battle has its own version of this check (see
 * PH_WIN in battle.c -- same math, same +5, or the two routes would grow
 * two different farmers), but field kills used to bank the xp and never
 * look at it: the bar sat there full and nothing happened. Now it happens
 * here, and instead of battle's full-screen message you get the corner
 * toast, in GOLD -- the only news that earns that color.
 *
 * A while, not an if: field xp arrives in dribs, but a boss field kill
 * lands a lump that can clear two bars at once. */
static void field_level_up(void)
{
    int rose = 0;
    while (G.player.xp >= G.player.level * XP_PER_LEVEL) {
        G.player.xp -= G.player.level * XP_PER_LEVEL;
        G.player.level++;
        G.player.max_hp += 5;
        G.player.hp = G.player.max_hp;   /* full heal on level */
        rose = 1;
    }
    if (!rose)
        return;

    audio_sfx(SFX_LEVELUP);

    /* the toast wants a string, and the core has no sprintf -- build it
     * the way msgb does (battle.c). Static: G.toast keeps pointing here. */
    static char msg[32];
    int p = 0;
    for (const char *s = "YOU FEEL TOUGHER! LEVEL "; *s; s++)
        msg[p++] = *s;
    if (G.player.level >= 10)
        msg[p++] = (char)('0' + (G.player.level / 10) % 10);
    msg[p++] = (char)('0' + G.player.level % 10);
    msg[p++] = '!';
    msg[p]   = '\0';

    G.toast       = msg;
    G.toast_good  = 2;      /* gold */
    G.toast_ticks = 120;
}

static void kill_entity(int i)
{
    entity_t *e = &G.ents[i];
    const species_t *sp = &species[e->kind];

    spray_gore(e->x, e->y, 14);
    add_blood(e->x + 8, e->y + 8, sp->boss);   /* where the body dropped */
    journal_saw(e->kind);        /* you shot it. You definitely saw it. */
    journal_kill(e->kind);
    audio_sfx(SFX_HIT);

    /* A field kill pays less than a proper fight -- see OVERWORLD_XP_PCT.
     * You took the quick way out, and the quick way pays worse. */
    G.player.xp += sp->xp * OVERWORLD_XP_PCT / 100;
    field_level_up();                  /* ...but it still counts */

    mark_dead(i);
    /* Each boss's death sets ITS flag -- a field kill has to count for
     * exactly as much as a battle win (win_battle sets the same ones). */
    if (e->kind == SPECIES_GOBLIN)
        G.flags |= FLAG_GOBLIN_DEAD;
    if (e->kind == SPECIES_CHUPA_BOSS)
        G.flags |= FLAG_CHUPA_DEAD;

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

/* PA'S TNT going off at (bx,by): everything inside TNT_RADIUS loses
 * TNT_OW_HITS, which drops most things outright, and anything still
 * standing reels for DOUBLE the usual stagger. That last part is the whole
 * point -- it's the only way to buy distance from something that barely
 * staggers at all. */
static void tnt_boom(int bx, int by)
{
    G.boom_x = bx;
    G.boom_y = by;
    G.boom_t = TNT_BOOM_TICKS;
    audio_sfx(SFX_BOOM);             /* the crack, and then the THUMP */
    rumble(255);                     /* it is DYNAMITE */
    shake(SHAKE_TNT_MAG, SHAKE_TNT_TICKS);

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

/* HOLY WATER landing at (bx,by): the mist rises, and everything the mist
 * touches simply ENDS -- ants, dogmen, the bosses, all of it. That is what
 * three-in-the-whole-game buys. */
static void holy_mist(int bx, int by)
{
    G.mist_x = bx;
    G.mist_y = by;
    G.mist_t = MIST_TICKS;
    audio_sfx(SFX_SIZZLE);           /* the vial breaks, the light climbs
                                        out, and it hisses like a griddle */
    shake(1, 12);

    for (int i = 0; i < MAX_ENTITIES; i++) {
        entity_t *e = &G.ents[i];
        if (e->type != ENT_ALIEN)
            continue;
        if (dist2(e->x + 8, e->y + 8, bx, by) > HOLY_RADIUS * HOLY_RADIUS)
            continue;
        kill_entity(i);              /* no roll. no survivors. */
    }
}

/* ---- THE LOB ---------------------------------------------------------------
 * TNT and holy water are THROWN, and the throw is visible: the item leaves
 * your hand, arcs LOB_ARC pixels into the air over LOB_TICKS, and only when
 * it lands does anything explode or sizzle (see the renderer for the arc).
 */
static void start_lob(int kind)
{
    static const int step[4][2] = { {0,1}, {0,-1}, {-1,0}, {1,0} };
    if (G.lob_active)
        return;                      /* one thing in the air at a time */

    G.player.items[kind]--;
    G.lob_active = 1;
    G.lob_kind   = kind;
    G.lob_t      = 0;
    G.lob_x0     = G.player.x + 8;
    G.lob_y0     = G.player.y + 4;
    G.lob_x1     = G.player.x + 8 + step[G.player.dir][0] * TNT_THROW;
    G.lob_y1     = G.player.y + 8 + step[G.player.dir][1] * TNT_THROW;
    audio_sfx(SFX_BLIP);             /* the grunt of the throw */
}

static void update_lob(void)
{
    if (!G.lob_active)
        return;
    if (++G.lob_t < LOB_TICKS)
        return;

    G.lob_active = 0;
    if (G.lob_kind == ITEM_TNT)
        tnt_boom(G.lob_x1, G.lob_y1);
    else
        holy_mist(G.lob_x1, G.lob_y1);
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
    G.player.items[GUN_AMMO()]--;   /* shells or bullets, whichever gun */
    G.shoot_cd = SHOOT_COOLDOWN;
    G.recoil   = 6;
    audio_sfx(SFX_SHOTGUN);
    rumble(150);              /* the gun kicks */
    shake(SHAKE_SHOT_MAG, SHAKE_SHOT_TICKS);
}

static void try_shoot(void)
{
    if (!PLAYER_HAS_GUN() || G.shoot_cd > 0)
        return;
    if (G.player.items[GUN_AMMO()] <= 0) {
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

/* how many of a thing one pickup is worth: ammo comes by the box */
static int item_count(int kind)
{
    if (kind == ITEM_SHELLS)  return SHELLS_PER_BOX;
    if (kind == ITEM_BULLETS) return BULLETS_PER_BOX;
    return 1;
}

/* walking into an item pockets it: chime, message, and it stays gone
 * for the rest of this run (spawns_gone remembers across map changes) */
static void pick_up(int i)
{
    int kind = G.ents[i].kind;
    G.player.items[kind] += item_count(kind);
    pack_discover(kind);
    if (kind == ITEM_SHOTGUN) {
        G.player.items[ITEM_SHELLS] += 2;   /* pa kept it loaded */
        pack_discover(ITEM_SHELLS);
    }
    if (kind == ITEM_HANDGUN) {
        G.player.items[ITEM_BULLETS] += 6;  /* Kowalski kept it loaded too --
                                               with BULLETS. Shells are for
                                               shotguns. */
        pack_discover(ITEM_BULLETS);
    }
    G.ents[i].type = ENT_NONE;
    /* NOT cast to uint16_t: spawns_gone is 32 bits wide and the city has
     * more than 16 spawns -- truncating here quietly resurrects anything
     * in a high slot the next time the map is entered. */
    G.spawns_gone[G.map_id] |= (1u << i);
    audio_sfx(SFX_PICKUP);

    /* In Part 1 the two items the whole level is about arrive in the
     * lawyer's own head, not on a museum card. */
    if ((G.flags & FLAG_PART1) && kind == ITEM_FLASHLIGHT) {
        monologue_start("A FLASHLIGHT. MAINTENANCE LEAVES IT ON THE HOOK "
                        "BY THE DOOR AND I HAVE WALKED PAST IT FOR NINE "
                        "YEARS WITHOUT SEEING IT.\nI SEE IT NOW. TURN IT "
                        "ON FROM THE PACK. KEEP MOVING.");
        return;
    }
    if ((G.flags & FLAG_PART1) && kind == ITEM_HANDGUN) {
        monologue_start("KOWALSKI'S REVOLVER. HE SHOWED IT TO ME AT THE "
                        "CHRISTMAS PARTY LIKE IT WAS A JOKE.\nIT IS NOT A "
                        "JOKE NOW. IT IS THE WHOLE PLAN. LIGHT, GUN, "
                        "HOME.");
        return;
    }
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

/* every tile that behaves as a door: the farm's plank door, the church's
 * arch, the tower's glass lobby */
static int is_door_tile(int t)
{
    return t == TILE_DOOR || t == TILE_CHURCH_DOOR || t == TILE_DOOR_GLASS;
}

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
    if (!is_door_tile(map_tile(m, tx, ty)))
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

/* ---- MA AT THE EAST GATE ---------------------------------------------------
 * The first time you try to walk east out of town -- which can only happen
 * once the thing in the north road is dead, because that's what opens the
 * gate -- SHE IS ALREADY RUNNING. She crosses the whole of Main Street to
 * catch you, hands over grandpa's field journal, tells you to be careful,
 * and runs home. Then the gate lets you through, because you were already
 * mid-step when she caught you.
 *
 * The world holds still for it: overworld_update sees ma_scene and skips
 * input, creatures, everything. Phase 2 rides the ordinary dialog box --
 * when the state comes back to ST_OVERWORLD the speech is over and the
 * journal changes hands.
 */
static int ma_scene_start(void)
{
    const map_t *m = cur_map();
    int slot = -1;
    for (int i = m->nspawns; i < MAX_ENTITIES; i++)
        if (G.ents[i].type == ENT_NONE) { slot = i; break; }
    if (slot < 0)
        return 0;                  /* no room -- fail open, just warp */

    entity_t *e = &G.ents[slot];
    e->type = ENT_NPC;
    e->kind = LOOK_MA;
    e->x    = 1 * TILE;            /* the west end of Main Street */
    e->y    = G.player.y;          /* the road row you're standing on */
    e->dir  = DIR_RIGHT;
    e->cw   = e->ch = TILE;
    e->dialog = 0;                 /* the scene speaks for her */
    e->gift   = 0;
    e->after  = 0;
    e->stun = e->hurt = e->aggro = 0;

    G.ma_ent    = slot;
    G.ma_scene  = 1;
    G.player.dir = DIR_LEFT;       /* you hear her before you see her */
    return 1;
}

static void ma_scene_update(void)
{
    entity_t *e = &G.ents[G.ma_ent];

    if (G.ma_scene == 1) {
        /* she RUNS -- 2px a tick, faster than anything else on the street
         * walks. Straight down the road, then square up beside you. */
        int tx = G.player.x - TILE, ty = G.player.y;
        if      (e->x < tx) { e->x += 2; if (e->x > tx) e->x = tx; }
        else if (e->x > tx) { e->x -= 2; if (e->x < tx) e->x = tx; }
        else if (e->y < ty) { e->y += 2; if (e->y > ty) e->y = ty; }
        else if (e->y > ty) { e->y -= 2; if (e->y < ty) e->y = ty; }
        else {
            G.ma_scene = 2;
            dialog_start(
                "~! WAIT! *HUFF* WAIT RIGHT THERE!\n"
                "I RAN ALL THE WAY FROM THE FARM. THE WHOLE TOWN IS "
                "SAYING WHAT YOU DID TO THAT THING IN THE NORTH ROAD.\n"
                "AND NOW YOU'RE WALKING EAST. OF COURSE YOU ARE.\n"
                "HERE -- GRANDPA'S FIELD JOURNAL. HE WROTE DOWN EVERY "
                "LAST THING HE SAW OUT THERE, AND NOBODY BELIEVED HIM "
                "EITHER.\n"
                "WRITE IT ALL DOWN, ~. AND YOU COME HOME.\n"
                "...BE CAREFUL. IT ISN'T JUST LIGHTS ANYMORE.");
        }
        return;
    }

    if (G.ma_scene == 2) {
        /* the dialog just closed: the journal changes hands */
        G.flags |= FLAG_JOURNAL;
        journal_saw(SPECIES_GOBLIN);   /* she watched you do it; it's the
                                          first thing you write down */
        audio_sfx(SFX_PICKUP);
        G.toast       = "GRANDPA'S JOURNAL IS IN THE PACK.";
        G.toast_good  = 1;
        G.toast_ticks = 120;
        G.ma_scene = 3;
        e->dir = DIR_LEFT;             /* and she heads home */
        return;
    }

    /* phase 3: she runs back the way she came, and the street lets her go */
    e->x -= 2;
    if (e->x <= 2) {
        e->type = ENT_NONE;
        G.ma_scene = 0;                /* the world starts moving again --
                                          you're still standing on the
                                          warp, so east happens NOW */
    }
}

static void check_warps(void)
{
    if (G.warp_cd > 0)      /* just arrived -- see overworld_enter_map */
        return;
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

        /* THE EAST GATE, the first time it's open (the goblin is down --
         * this warp's needs_flag just said so): Ma catches you before
         * you're through it. See ma_scene_start above. */
        if (G.map_id == MAP_TOWN && w->dest_map == MAP_VANLOT &&
            !(G.flags & FLAG_JOURNAL) && !(G.flags & FLAG_PART1) &&
            ma_scene_start())
            return;

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
        e->cw = e->ch = TILE;
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
        if (px >= e->x && px < e->x + e->cw &&
            py >= e->y && py < e->y + e->ch) {
            /* Some NPCs are holding something for you. The first time you
             * talk they hand it over (their `dialog` is the line that does
             * it); after that they fall back to `after`. We reuse the
             * spawns_gone bitmask -- an NPC's spawn slot gets its own bit,
             * so the handover survives leaving and re-entering the map. */
            if (e->gift && !(G.spawns_gone[G.map_id] & (1u << i))) {
                int kind = e->gift - 1;
                G.player.items[kind] += item_count(kind);
                pack_discover(kind);
                G.spawns_gone[G.map_id] |= (1u << i);  /* 32-bit: no cast */
                audio_sfx(SFX_PICKUP);
                /* the dead don't speak. Taking the rosary from the
                 * librarian happens entirely inside his own head -- the
                 * red box, the half-remembered prayer. */
                if (e->kind == LOOK_DEADLADY)
                    monologue_start(e->dialog);
                else
                    dialog_start(e->dialog);
                return;
            }
            /* This map's one generous soul (see roll_boon). They hand it
             * over, they patch you up, and after that they're just another
             * frightened neighbour with a line to say. */
            if (i == G.boon_ent) {
                int kind = G.boon_item;
                /* the boon pool is farm-stocked; downtown, a stranger with
                 * ammunition to spare has PISTOL ammunition */
                if ((G.flags & FLAG_PART1) && kind == ITEM_SHELLS)
                    kind = ITEM_BULLETS;
                G.player.items[kind] += item_count(kind);
                pack_discover(kind);
                G.player.hp += BOON_HEAL;
                if (G.player.hp > G.player.max_hp)
                    G.player.hp = G.player.max_hp;
                G.boons_done |= (uint16_t)(1u << G.map_id);
                G.boon_ent = -1;
                audio_sfx(SFX_PICKUP);
                dialog_start(boon_line(e->dialog, kind));
                return;
            }

            /* the van is not a person. Talk to it and you leave. */
            if (e->kind == LOOK_VAN) {
                audio_sfx(SFX_CONFIRM);
                drive_start();
                return;
            }

            /* MARIE. The first conversation is the reunion, and when it
             * closes, Part 1 closes with it (dialog_update raises the
             * card). After that she's herself again -- the `after` line,
             * for as long as you want to stay. */
            if (e->kind == LOOK_WIFE) {
                audio_sfx(SFX_CONFIRM);
                if (!(G.flags & FLAG_FAMILY)) {
                    G.flags |= FLAG_FAMILY;
                    G.part1end_after = 1;
                    dialog_start(e->dialog);
                } else {
                    dialog_start(e->after ? e->after : e->dialog);
                }
                return;
            }
            audio_sfx(SFX_CONFIRM);
            {
                const char *line = (e->gift && e->after) ? e->after
                                                         : e->dialog;
                if (e->kind == LOOK_DEADLADY)
                    monologue_start(line);   /* still inside his head */
                else
                    dialog_start(line);
            }
            return;
        }
    }
}

void overworld_update(void)
{
    G.daytime++;                   /* the sun only moves while you walk */
    weather_update();              /* ...and so does the sky */
    ambient_refresh();             /* ...and the sound of the place */
    if (G.banner > 0)
        G.banner--;

    /* ---- PART 1'S VOICE IN HIS HEAD ------------------------------------
     * One-shot internal monologues (red box, his own words -- see
     * monologue_start). Each is keyed to a story flag, so it fires once
     * per RUN, not once per visit, and never again after a reload. */
    if (G.flags & FLAG_PART1) {
        if (G.map_id == MAP_CITY && !(G.flags & FLAG_M_CITY) && G.t > 40) {
            G.flags |= FLAG_M_CITY;
            monologue_start(
                "CREATURES. THAT'S WHAT EVERY TV IN THE SHOP WINDOW SAID, "
                "AND THEN THE CHANNELS STOPPED SAYING ANYTHING.\n"
                "MARIE AND DANNY ARE OUT THERE. HOME IS TWENTY BLOCKS "
                "SOUTH AND I'M STANDING HERE IN A SUIT.\n"
                "THINK, COUNSELOR. DARK IS COMING. THE FIRM KEEPS A "
                "FLASHLIGHT IN MAINTENANCE. START THERE.");
            return;
        }
        if (G.map_id == MAP_SOUTH && !(G.flags & FLAG_M_SOUTH) && G.t > 40) {
            G.flags |= FLAG_M_SOUTH;
            monologue_start(
                "THE SOUTH SIDE. EVERY WINDOW DARK, AND NOBODY ON THE "
                "STREET WILL LOOK UP. I LOOKED UP. I WISH I HADN'T.\n"
                "THE HOUSE IS EMPTY -- I KNOW IT'S EMPTY, MARIE ISN'T "
                "STUPID. IF SHE RAN ANYWHERE SHE RAN TO ROSA'S WALK-UP, "
                "OFF THE LOW STREET.\n"
                "FIND THE LOW STREET. FIND THE DOOR. BRING THE GUN.");
            return;
        }
        if (G.map_id == MAP_OFFICE && !(G.flags & FLAG_M_OFFICE) && G.t > 20) {
            G.flags |= FLAG_M_OFFICE;
            monologue_start(
                "THE POWER'S OUT. OF COURSE THE POWER'S OUT.\n"
                "NINE YEARS IN THIS BUILDING AND I HAVE NEVER ONCE HEARD "
                "IT THIS QUIET.\n"
                "...SOMETHING UPSTAIRS JUST STOPPED WALKING.");
            return;
        }
        if (G.map_id == MAP_CITY && !(G.flags & FLAG_M_ARMED) &&
            G.player.items[ITEM_FLASHLIGHT] && G.player.items[ITEM_HANDGUN]) {
            G.flags |= FLAG_M_ARMED;
            monologue_start(
                "OKAY. LIGHT. GUN. TWENTY BLOCKS.\n"
                "SOUTH IS THROUGH THE PARK. THE PARK WITH THE THING IN IT "
                "THAT TOOK A MAN WHO HAD A SHOTGUN.\n"
                "...MARIE, I'M COMING. LEAVE THE DOOR LOCKED UNTIL YOU "
                "HEAR MY VOICE.");
            return;
        }
    }

    if (G.daytime - G.last_restock >= (uint32_t)ITEM_RESPAWN_TICKS) {
        G.last_restock = G.daytime;
        restock_items();
    }

    if (G.battle_grace > 0)
        G.battle_grace--;
    if (locked_cd > 0)
        locked_cd--;
    if (G.warp_cd > 0)
        G.warp_cd--;

    /* Ma has the street: no input, no creatures, no warps -- the whole
     * town holds still until she's said her piece and gone home */
    if (G.ma_scene) {
        ma_scene_update();
        return;
    }

    move_player();
    check_warps();
    if (G.state != ST_OVERWORLD)   /* warp changed scene state/timer */
        return;
    update_aliens();

    if (G.shoot_cd > 0) G.shoot_cd--;
    if (G.recoil   > 0) G.recoil--;
    if (G.boom_t   > 0) G.boom_t--;
    if (G.mist_t   > 0) G.mist_t--;
    update_lob();                  /* anything mid-air comes down */
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
    /* Part 1 swaps the man, not the machinery: the lawyer's nine frames
     * sit in the same order as the farmer's, so one base id does it. */
    int base = (G.flags & FLAG_PART1) ? SPR_LAWYER_DOWN_0 : SPR_FARMER_DOWN_0;
    *flip = 0;
    switch (G.player.dir) {
    case DIR_DOWN:  *spr = base + frame;     break;
    case DIR_UP:    *spr = base + 3 + frame; break;
    case DIR_LEFT:  *spr = base + 6 + frame; break;
    default:        *spr = base + 6 + frame; *flip = 1; break;
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

    /* level, tucked to the right of the name. Worn beads show you the
     * BORROWED level, in a colder color -- strength that isn't yours. */
    int boosted = G.player.items[ITEM_ROSARY] && G.player.rosary;
    char lv[8] = "LV ";
    int  l = G.player.level + (boosted ? ROSARY_LEVELS : 0), p = 3;
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
    if (G.player.items[ITEM_ROSARY]) {
        /* worn = bright, pocketed = dim -- same code the flashlight speaks */
        gfx_blit_ex(sprites[SPR_ITEM_ROSARY].px, TILE, TILE, ix - 4, iy - 4,
                    1, G.player.rosary ? 256 : 110, 0);
        ix += 12;
    }
    if (PLAYER_HAS_GUN()) {
        /* the AMMO THAT FITS THE GUN: shells beside the shotgun, the
         * bullet carton beside the pistol */
        int ammo = GUN_AMMO();
        char am[8];
        int n = G.player.items[ammo], k = 0;
        if (n > 99) n = 99;
        if (n >= 10) am[k++] = (char)('0' + n / 10);
        am[k++] = (char)('0' + n % 10);
        am[k] = '\0';
        gfx_blit(sprites[ammo == ITEM_SHELLS ? SPR_ITEM_SHELLS
                                             : SPR_ITEM_BULLETS].px,
                 TILE, TILE, ix - 4, iy - 4);
        gfx_text_small_outlined(ix + 11, iy + 4, am, RGB565(255, 220, 80));
    }

    draw_toast();   /* SAVED. / LOADED. / the restock line -- corner popup */
}

/* ============================ THE TOAST =====================================
 * SAVED. / LOADED. / NO SAVE FOUND / the overnight-restock line. It used to
 * be a plate across the middle of the screen -- a status note dressed up as
 * an event. Now it's a TOAST: a small popup in the bottom-right corner, in
 * the HUD's small font, that slides up as it arrives and drops away as it
 * leaves. Every scene that can show one calls this same function, so the
 * pause screen, the prologue and the church all agree with the overworld.
 */
void draw_toast(void)
{
    if (G.toast_ticks <= 0 || !G.toast)
        return;
    G.toast_ticks--;

    /* slide in over the first few ticks, sink away over the last few.
     * toast_ticks counts DOWN from 120-150, so "young" means large. */
    int slide = 0;
    if (G.toast_ticks > 112)      slide = (G.toast_ticks - 112) * 2;
    else if (G.toast_ticks < 6)   slide = (6 - G.toast_ticks) * 2;

    int tw = gfx_text_small_width(G.toast);
    int x  = SCREEN_W - tw - 10;
    int y  = SCREEN_H - 14 + slide;

    /* three moods: red for bad news, green for good news, and GOLD for the
     * one piece of news that's about YOU getting stronger (see toast_good
     * in game_internal.h) */
    uint16_t edge, text;
    if (G.toast_good == 2) { edge = RGB565(210, 165, 40);
                             text = RGB565(255, 225, 120); }
    else if (G.toast_good) { edge = RGB565(90, 170, 90);
                             text = RGB565(150, 240, 150); }
    else                   { edge = RGB565(190, 80, 70);
                             text = RGB565(240, 110, 100); }
    gfx_fill_rect(x - 4, y - 3, tw + 8, 11, RGB565(10, 10, 16));
    gfx_rect     (x - 4, y - 3, tw + 8, 11, edge);
    gfx_text_small(x, y, G.toast, text);
}

/* RAIN. Streaks, not dots -- a dot is a snowflake. Each drop's position is
 * derived from its index and the frame, so there is no per-drop state to
 * store and it costs nothing on the little machine. The wind blows them
 * sideways, and they fall FAST: rain you can follow with your eye is not
 * rain, it's confetti. */
static void draw_rain(int hard)
{
    int len   = hard ? 9 : 6;
    int slant = hard ? 3 : 1;
    int n     = hard ? RAIN_DROPS * 3 / 2 : RAIN_DROPS;
    if (n > RAIN_DROPS * 2) n = RAIN_DROPS * 2;

    for (int i = 0; i < n; i++) {
        uint32_t h = (uint32_t)i * 2654435761u;
        int speed = 11 + (int)((h >> 5) & 7);
        int x = (int)(((h % SCREEN_W) + (uint32_t)(G.frame * (uint32_t)slant))
                      % SCREEN_W);
        int y = (int)((((h >> 9) % SCREEN_H)
                      + (uint32_t)(G.frame * (uint32_t)speed)) % SCREEN_H);

        for (int k = 0; k < len; k++)
            gfx_add_pixel(x + k * slant / 3, y + k,
                          RGB565(170, 190, 225), 70);
    }
}

/* THE TORNADO.
 *
 * A funnel: narrow where it is chewing the ground, widening as it climbs off
 * the top of the frame. It is DARK -- a tornado is a hole in the daylight
 * with other people's roofs in it -- so it is blended, never added.
 *
 * The bands that make it look like it's SPINNING rise vertically (y minus
 * time), not diagonally. Diagonal stripes read as hatching; rising ones read
 * as rotation, and that is the entire difference between a tornado and a
 * smudge.
 *
 * It lives in WORLD coordinates: it is a thing standing in the field, and it
 * slides past as you walk.
 */
static void draw_tornado(int cx)
{
    int sx = G.wx_x - cx;                 /* where it is on screen */
    if (sx < -120 || sx > SCREEN_W + 120)
        return;

    const int foot = SCREEN_H - 34;       /* where it touches the ground */

    for (int y = 0; y < foot; y++) {
        int up = foot - y;                        /* height above the ground */

        /* narrow at the ground, wide at the sky */
        int w = 5 + up * up / (foot * 2) + up / 5;

        /* it leans and it sways -- more at the top, like a rope */
        int wob = ((int)((G.frame * 2 + (uint32_t)y * 3) % 48) - 24) * up
                  / (foot * 5);

        int mid = sx + wob;
        for (int x = mid - w; x <= mid + w; x++) {
            int off  = (x > mid) ? x - mid : mid - x;
            int edge = off * 256 / (w ? w : 1);    /* 0 core .. 256 rim */

            /* solid in the middle, ragged at the rim */
            int a = 225 - edge * 205 / 256;
            if (a < 12)
                continue;

            /* THE SPIN. Keyed on the SIGNED offset from the centre, not on
             * the distance from it: distance is symmetric, and symmetric
             * bands come out as chevrons -- a zigzag ribbon, not a column.
             * Signed, they slant one way across the whole funnel and read as
             * a helix, which is what rotation looks like. */
            int band = (((y * 5) - (int)G.frame * 7 + (x - mid) * 2) & 31) < 13;
            uint16_t col = band ? RGB565(126, 118, 106)   /* dust in the light */
                                : RGB565( 38,  36,  42);  /* the dark inside   */
            gfx_blend_pixel(x, y, col, a);
        }
    }

    /* the debris ring where it meets the ground -- dirt, fence, somebody's
     * shed. This is what sells the scale. */
    for (int i = 0; i < 60; i++) {
        uint32_t h = (uint32_t)i * 2246822519u + G.frame * 977u;
        int spread = 14 + (int)((h >> 3) % 34);
        int ang    = (int)(h % 64);
        int dx = (ang < 32 ? ang - 16 : 48 - ang) * spread / 16;
        int dy = (int)((h >> 11) % 22);
        gfx_blend_pixel(sx + dx, foot - dy, RGB565(122, 100, 74), 190);
    }
}

/* THE OBJECTS. Over the South Side they aren't a rumour any more: a loose
 * formation of lights that drifts, holds, and drifts again, too high to
 * matter and too low to be stars. Drawn in SCREEN space -- they're miles
 * up, so they don't parallax with the street -- and every so often all of
 * them blink out together for a few seconds, which is worse than any of
 * the things they do while visible. */
static void draw_sky_objects(void)
{
    /* the blink. all at once. nobody talks about the blink. */
    if (G.frame % 1100 < 45)
        return;

    for (int i = 0; i < 3; i++) {
        int ox = 34 + i * 78
               + tri_wave(G.frame / 5 + (uint32_t)i * 90, 260) / 4;
        int oy = 10 + (i % 2) * 9
               + tri_wave(G.frame / 7 + (uint32_t)i * 40, 200) / 16;
        int pulse = 120 + (int)((G.frame / (7 + i)) % 2) * 70;

        for (int ring = 3; ring >= 1; ring--) {
            int rr = ring * 3;
            int a  = pulse / (ring * ring + 1);
            for (int y = -rr; y <= rr; y++)
                for (int x = -rr * 2; x <= rr * 2; x++)
                    if (x * x / 4 + y * y <= rr * rr)
                        gfx_add_pixel(ox + x, oy + y,
                                      RGB565(200, 190, 255), a);
        }
        gfx_fill_rect(ox - 1, oy, 3, 1, RGB565(255, 244, 220));
    }
}

/* THE SEARCHLIGHTS. Something up there is looking for her.
 *
 * They do NOT sweep in a tidy back-and-forth: two different periods on each
 * axis means the pool wanders, doubles back, and lingers. It reads as
 * hunting rather than as a machine doing a pattern. And they go out for long
 * stretches, which is worse, because then you are just waiting. */
static void draw_searchlights(int cx, int cy, light_t *lights, int *nlights)
{
    const map_t *m = cur_map();
    int mw = m->w * TILE, mh = m->h * TILE;

    for (int i = 0; i < SEARCH_LIGHTS; i++) {
        uint32_t t = G.frame + (uint32_t)i * 900u;

        /* on for a third of the cycle, off for the rest */
        if ((t % SEARCH_CYCLE) > SEARCH_CYCLE / 3)
            continue;

        /* a slow wander, in world pixels */
        int wx = mw / 2 + (int)tri_wave(t / 3 + i * 40, 210) * (mw / 2 - 40) / 128;
        int wy = mh / 2 + (int)tri_wave(t / 5 + i * 90, 130) * (mh / 2 - 40) / 128;

        int px = wx - cx, py = wy - cy;
        if (px < -SEARCH_RADIUS * 2 || px > SCREEN_W + SEARCH_RADIUS * 2)
            continue;

        /* the pool it burns onto the tarmac: an ellipse, brightest in the
         * middle, and it FLICKERS -- whatever is holding it isn't steady */
        int r  = SEARCH_RADIUS - (int)((t / 7) % 4);
        int ry = r * 2 / 3;
        for (int y = -ry; y <= ry; y++)
            for (int x = -r; x <= r; x++) {
                int d = x * x * 100 / (r * r) + y * y * 100 / (ry * ry);
                if (d > 100) continue;
                int a = 165 - d * 150 / 100;
                gfx_add_pixel(px + x, py + y, RGB565(210, 230, 255), a);
            }

        /* and it lights the dark, so hand it to the night pass too */
        if (*nlights < MAX_LIGHTS) {
            lights[*nlights].x = px;
            lights[*nlights].y = py;
            lights[*nlights].r = SEARCH_RADIUS + 10;
            lights[*nlights].warm = 0;          /* cold. clinical. */
            (*nlights)++;
        }
    }
}

void overworld_render(void)
{
    int cx, cy;
    camera(&cx, &cy);
    const map_t *m = cur_map();

    /* SCREEN SHAKE, applied to the CAMERA rather than to the renderer.
     * Nudging the camera means the tile loop simply draws a different slice
     * of the map, so the world still covers the screen -- shifting the
     * renderer instead would leave a bare strip along one edge. It also
     * leaves the HUD alone, which is what you want: the world lurches, the
     * numbers you're reading do not. */
    {
        int sx, sy;
        shake_offset(&sx, &sy);
        cx += sx;
        cy += sy;
    }

    /* tiles (only the ones on screen) */
    for (int ty = cy / TILE; ty <= (cy + SCREEN_H) / TILE; ty++)
        for (int tx = cx / TILE; tx <= (cx + SCREEN_W) / TILE; tx++) {
            int id = map_tile(m, tx, ty);
            if (id == TILE_WATER && (G.frame / 28) % 2)
                id = TILE_WATER2;             /* lapping water */
            gfx_blit(tiles[id].px, TILE, TILE,
                     tx * TILE - cx, ty * TILE - cy);
        }

    /* THE POOLS -- what soaked in. Painted straight over the tiles, under
     * everything that still moves. Each pool re-rolls its shape from its
     * own stored seed every frame -- a private xorshift, NOT the game rng,
     * which must never advance from inside a render -- so it holds
     * perfectly still, and no pixels are stored anywhere. Two draws summed
     * per axis piles the splats up in the middle: a pool, not confetti. */
    for (int i = 0; i < MAX_BLOOD; i++) {
        const blood_t *b = &G.blood[i];
        if (b->map != G.map_id)
            continue;
        int px = b->x - cx, py = b->y - cy;
        int r  = b->big ? 11 : 6;              /* how far it spread */
        if (px < -2 * r || px >= SCREEN_W + 2 * r ||
            py < -2 * r || py >= SCREEN_H + 2 * r)
            continue;
        uint32_t s = b->seed ? b->seed : 0x2545F491u;
        int n = b->big ? 30 : 12;              /* how much there was */
        for (int k = 0; k < n; k++) {
            int dx, dy, sz;
            s ^= s << 13; s ^= s >> 17; s ^= s << 5;
            dx = (int)(s % (uint32_t)(r + 1));
            s ^= s << 13; s ^= s >> 17; s ^= s << 5;
            dx += (int)(s % (uint32_t)(r + 1)) - r;
            s ^= s << 13; s ^= s >> 17; s ^= s << 5;
            dy = (int)(s % (uint32_t)(r + 1));
            s ^= s << 13; s ^= s >> 17; s ^= s << 5;
            dy += (int)(s % (uint32_t)(r + 1)) - r;
            s ^= s << 13; s ^= s >> 17; s ^= s << 5;
            /* fat in the middle, flecks at the rim */
            sz = (dx * dx + dy * dy < r * r / 4) ? 3 : 1 + (int)(s % 2u);
            /* dried past the bright of the spray -- see spray_gore */
            gfx_fill_rect(px + dx, py + dy, sz, sz,
                          (s & 4) ? RGB565(96, 12, 10)
                                  : RGB565(130, 22, 16));
        }
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
        } else if (e->kind == LOOK_COPCAR) {
            /* the cruiser, 1:1 like the van -- and the light bar still
             * TURNS: nobody switched it off. A soft red/blue pulse washes
             * over the roof, trading sides. */
            int ex2 = e->x - cx, ey2 = e->y - cy;
            gfx_blit_ex(copcar, COPCAR_W, COPCAR_H, ex2, ey2, 1, 256, 0);
            {
                int red_on = (int)((G.frame / 16) % 2);
                uint16_t c = red_on ? RGB565(255, 60, 50)
                                    : RGB565(80, 120, 255);
                int bx = ex2 + 21, by = ey2 + (red_on ? 11 : 16);
                for (int gy2 = -3; gy2 <= 8; gy2++)
                    for (int gx2 = -3; gx2 <= 9; gx2++)
                        gfx_add_pixel(bx + gx2, by + gy2, c,
                                      70 - (gx2 * gx2 + gy2 * gy2) * 2);
            }
            continue;
        } else if (e->kind == LOOK_VAN) {
            /* IT IS A VAN, SEEN FROM ABOVE -- van_top, not van_big. van_big is
             * the REAR of the van and belongs to the highway cutscene; putting
             * it on the tarmac meant looking down at the sky and seeing the
             * back doors, which is not a thing that happens.
             *
             * Drawn 1:1, so its pixels are the same size as the tarmac it is
             * parked on. Its collision box (cw/ch) is exactly this rectangle,
             * so it is solid all the way across and you walk up to its side. */
            gfx_blit_ex(van_top, VANTOP_W, VANTOP_H,
                        e->x - cx, e->y - cy, 1, 256, 0);
            continue;
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

    /* SOMETHING THROWN, STILL IN THE AIR. The item's own sprite rides a
     * parabola from your hand to where it will land, spinning as it goes --
     * this is the attack ANIMATION, and the boom or the mist below is its
     * punchline. */
    if (G.lob_active) {
        int t  = G.lob_t;
        int lx = G.lob_x0 + (G.lob_x1 - G.lob_x0) * t / LOB_TICKS;
        int ly = G.lob_y0 + (G.lob_y1 - G.lob_y0) * t / LOB_TICKS;
        /* the arc: 4h*t(T-t)/T^2 peaks at LOB_ARC mid-flight */
        int arc = 4 * LOB_ARC * t * (LOB_TICKS - t) / (LOB_TICKS * LOB_TICKS);
        int spr = (G.lob_kind == ITEM_TNT) ? SPR_ITEM_TNT : SPR_ITEM_HOLYWATER;
        gfx_blit_ex(sprites[spr].px, TILE, TILE,
                    lx - 8 - cx, ly - 8 - arc - cy, 1, 256,
                    (t / 4) % 2);                 /* it tumbles */
        /* a small shadow racing along the ground under it */
        gfx_blend_pixel(lx - cx, ly - cy + 6, RGB565(0, 0, 0), 120);
        gfx_blend_pixel(lx - cx + 1, ly - cy + 6, RGB565(0, 0, 0), 120);
    }

    /* THE SIZZLING MIST. Holy water doesn't explode -- it RISES: a pale
     * blue-white cloud that eats what it touches and hisses while it does.
     * Blended (a fog), then salted with bright sizzle sparks that drift
     * upward like steam off a griddle. */
    if (G.mist_t > 0) {
        int age  = MIST_TICKS - G.mist_t;
        int r    = HOLY_RADIUS * (age + 6) / (MIST_TICKS / 2 + 6);
        if (r > HOLY_RADIUS) r = HOLY_RADIUS;
        int fade = G.mist_t * 256 / MIST_TICKS;
        int mx = G.mist_x - cx, my = G.mist_y - cy;

        for (int y = -r; y <= r; y++)
            for (int x = -r; x <= r; x++) {
                int d2 = x * x + y * y;
                if (d2 > r * r)
                    continue;
                /* holey (ha) dither so it reads as vapour, not paint */
                if (((x * 5 + y * 9 + (int)(G.frame * 3)) & 7) < 3)
                    continue;
                int edge = 256 - d2 * 256 / (r * r + 1);
                int a = 120 * edge / 256 * fade / 256;
                if (a < 8)
                    continue;
                gfx_blend_pixel(x + mx, y + my - age / 4,   /* it rises */
                                RGB565(205, 232, 255), a);
            }

        /* the SIZZLE: bright sparks popping off inside the cloud */
        for (int i = 0; i < 14; i++) {
            uint32_t h = (uint32_t)i * 2654435761u + G.frame * 131u;
            int sxp = (int)(h % (uint32_t)(r * 2 + 1)) - r;
            int syp = (int)((h >> 9) % (uint32_t)(r * 2 + 1)) - r;
            if (sxp * sxp + syp * syp > r * r)
                continue;
            gfx_add_pixel(mx + sxp, my + syp - age / 3,
                          RGB565(255, 255, 255), 150 * fade / 256);
        }
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

        /* SOMETHING IS LOOKING FOR HER. Only in the car park -- this is the
         * lot where the girl went out at nine to bring the trolleys in.
         * Drawn BEFORE the night pass, and handed to it, so the beams both
         * glow on the tarmac and cut real holes in the dark.
         *
         * The CITY gets them too. Downtown, you get to decide whether
         * they're a police helicopter. The cops on the street haven't
         * mentioned a helicopter. */
        if (G.map_id == MAP_VANLOT || G.map_id == MAP_CITY ||
            G.map_id == MAP_SOUTH)
            draw_searchlights(cx, cy, lights, &n);

        /* the TNT, for as long as it burns, lights the whole field */
        if (boom_r > 0) {
            lights[n].x = G.boom_x - cx;
            lights[n].y = G.boom_y - cy;
            lights[n].r = boom_r * 3;
            lights[n].warm = 200;
            n++;
        }

        /* ...and the mist glows COLD in the dark, for as long as it hangs */
        if (G.mist_t > 0 && n < MAX_LIGHTS) {
            lights[n].x = G.mist_x - cx;
            lights[n].y = G.mist_y - cy;
            lights[n].r = HOLY_RADIUS + 10;
            lights[n].warm = 0;
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

        gfx_night(world_brightness(), lights, n);

        /* ---- THE SKY, over the top of everything -------------------------
         * After the night pass, so rain is not dimmed into invisibility and
         * lightning actually blinds you. */
        if (G.wx == WX_TORNADO) {
            draw_tornado(cx);
            draw_rain(1);                       /* it is not gentle out */
        } else if (G.wx == WX_RAIN) {
            draw_rain(0);
        }

        /* LIGHTNING. The whole world, for four frames, in white. */
        if (G.wx_flash > 0) {
            int a = 40 * G.wx_flash;
            for (int y = 0; y < SCREEN_H; y++)
                for (int x = 0; x < SCREEN_W; x++)
                    gfx_add_pixel(x, y, RGB565(200, 210, 255), a);
        }

        /* ...and over the South Side, THEM. After the night pass, so
         * darkness never dims them -- they are not lit BY anything. */
        if (G.map_id == MAP_SOUTH)
            draw_sky_objects();
    } else if (m->dark) {
        /* THE POWER IS OUT. An interior with no sun, no lamps, no weather:
         * flat DARK_BRIGHT black, and the only light in the building is
         * the one in your hand. This is what the flashlight is FOR. */
        light_t lights[1];
        int n = 0;
        if (G.player.items[ITEM_FLASHLIGHT] && G.player.lamp) {
            lights[0].x = G.player.x - cx + TILE / 2;
            lights[0].y = G.player.y - cy + TILE / 2;
            lights[0].r = FLASHLIGHT_RADIUS;
            lights[0].warm = 0;
            n = 1;
        }
        gfx_night(DARK_BRIGHT, lights, n);
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
    G.dialog_style = 0;            /* spoken, unless monologue_start says so */
    G.state = ST_DIALOG;
    G.t = 0;
}

/* THE INTERNAL MONOLOGUE. Same textbox machinery, different voice: black
 * plate, red border, red italic letters -- nobody on the street can hear
 * this one. Part 1's lawyer thinks at you; the farmer never did. */
void monologue_start(const char *text)
{
    dialog_start(text);
    G.dialog_style = 1;
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

/* Is the idx-th printed character of this page the FIRST letter of a word?
 * Walks the wrapped rows the same way the typewriter reveals them. Column
 * zero of any row counts as a word start too -- the wrap already ate the
 * space that used to prove it. */
static int word_starts_at(char out[DIALOG_ROWS][DIALOG_COLS + 1], int rows,
                          int idx)
{
    for (int r = 0; r < rows; r++) {
        int len = 0;
        while (out[r][len])
            len++;
        if (idx < len)
            return out[r][idx] != ' '
                && (idx == 0 || out[r][idx - 1] == ' ');
        idx -= len;
    }
    return 0;
}

void dialog_update(void)
{
    char lines[DIALOG_ROWS][DIALOG_COLS + 1];
    int  rows;
    int  next  = dialog_page_layout(G.dialog_page, lines, &rows);
    int  total = page_visible(lines, rows);
    int  more  = (G.dialog_text[next] != '\0');   /* another page after this */

    /* typewriter, one char every other tick */
    if (G.t % 2 == 0 && G.dialog_shown < total) {
        G.dialog_shown++;
        /* THE BABBLE. One little syllable per WORD as it lands, hopping
         * between three pitches (the hash keeps a sentence from ticking
         * like a metronome, and the same line always burbles the same
         * way). Only for the SPOKEN box: the internal monologue stays
         * silent, because nobody can hear you think. */
        if (G.dialog_style == 0 &&
            word_starts_at(lines, rows, G.dialog_shown - 1)) {
            static const int talk[3] = { SFX_TALK0, SFX_TALK1, SFX_TALK2 };
            audio_sfx(talk[(G.dialog_shown * 13 + G.dialog_page * 7) % 3]);
        }
    }

    /* YOU CANNOT RUSH IT.
     *
     * A used to fast-forward the typewriter, which meant a player mashing the
     * button -- and everyone mashes the button -- blew straight through every
     * word in the game without reading one of them. The button does nothing
     * until the page has finished typing itself out. Then, and only then, it
     * turns the page. */
    if (G.dialog_shown < total)
        return;

    if (!PRESSED(BTN_A) && !PRESSED(BTN_B))
        return;

    if (more) {
        G.dialog_page  = next;         /* turn the page */
        G.dialog_shown = 0;
        audio_sfx(SFX_BLIP);
    } else if (G.part1end_after) {
        /* that was HER last page. Roll the card. */
        G.part1end_after = 0;
        part1end_start();
    } else {
        G.state = ST_OVERWORLD;        /* that was the last page */
        G.t = 0;
        G.battle_grace = 30;
    }
}

void dialog_render(void)
{
    overworld_render();   /* the world stays visible behind the box */

    /* Two voices, two boxes:
     *   spoken     dark blue plate, white border, white upright text
     *   MONOLOGUE  black plate, red border (it breathes), red ITALIC text
     * The border pulse is the tell that this one is inside his head. */
    int mono = (G.dialog_style == 1);
    uint16_t plate  = mono ? RGB565(0, 0, 0)       : RGB565(16, 16, 24);
    uint16_t border = mono
        ? ((G.frame % 90 < 6) ? RGB565(120, 20, 16) : RGB565(210, 40, 32))
        : RGB565(255, 255, 255);
    uint16_t ink    = mono ? RGB565(225, 65, 55)   : RGB565(255, 255, 255);

    int bh = 46;
    gfx_fill_rect(4, SCREEN_H - bh - 4, SCREEN_W - 8, bh, plate);
    gfx_rect     (4, SCREEN_H - bh - 4, SCREEN_W - 8, bh, border);
    if (mono)   /* a second, inner line -- the box is thinking hard */
        gfx_rect(6, SCREEN_H - bh - 2, SCREEN_W - 12, bh - 4,
                 RGB565(70, 12, 10));

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
        if (mono)
            gfx_text_italic(12, y0 + r * 11, clipped, ink);
        else
            gfx_text(12, y0 + r * 11, clipped, ink);
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
           kind == ITEM_FLASHLIGHT || kind == ITEM_TNT ||
           kind == ITEM_HOLYWATER || kind == ITEM_ROSARY;
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
#define ROW_SAVE    (-1)
#define ROW_LOAD    (-2)
#define ROW_JOURNAL (-3)

static int pack_rows(int *out)
{
    int n = 0;
    for (int i = 0; i < NUM_ITEMS; i++)
        if (G.items_seen & (1u << i))
            out[n++] = i;
    if (G.flags & FLAG_JOURNAL)
        out[n++] = ROW_JOURNAL;   /* grandpa's journal, once Ma catches
                                     you at the east gate */
    out[n++] = ROW_SAVE;      /* always there */
    out[n++] = ROW_LOAD;      /* ...but only usable once a save exists */
    return n;
}

void pack_update(void)
{
    int rows[NUM_ITEMS + 3];  /* every item + JOURNAL + SAVE + LOAD */
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

    if (kind == ROW_JOURNAL) {
        journal_start(ST_PACK);      /* B brings you back here */
        audio_sfx(SFX_CONFIRM);
        return;
    }

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

    case ITEM_ROSARY:              /* also a switch: WEAR it, or pocket it */
        G.player.rosary = !G.player.rosary;
        audio_sfx(SFX_CONFIRM);
        break;

    case ITEM_TNT:
    case ITEM_HOLYWATER:
        /* You throw it -- so get out of the menu first. You are going to
         * want to watch this, and to be facing the right way when it
         * lands (the arc itself is drawn by the renderer). */
        G.state = ST_OVERWORLD;
        G.t = 0;
        G.battle_grace = 30;
        G.shoot_cd = 10;           /* and no stray shell on the way out */
        start_lob(kind);
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

    int rows[NUM_ITEMS + 3];  /* every item + JOURNAL + SAVE + LOAD */
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

        if (kind == ROW_JOURNAL) {
            gfx_text(x + 20, ry, "WHAT I SAW", RGB565(180, 200, 255));
            continue;
        }
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

        /* the switches show their position: the flashlight, and now the
         * rosary (equipped = worn, not just carried) */
        if ((kind == ITEM_FLASHLIGHT || kind == ITEM_ROSARY) && n_held) {
            int on = (kind == ITEM_FLASHLIGHT) ? G.player.lamp
                                               : G.player.rosary;
            gfx_text(x + w - 40, ry, on ? "ON" : "OFF",
                     on ? RGB565(255, 236, 150) : RGB565(100, 100, 108));
        }
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
