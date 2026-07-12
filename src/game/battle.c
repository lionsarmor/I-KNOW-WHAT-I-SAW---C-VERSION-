/* ============================================================================
 * battle.c -- turn-based combat, Game-Boy-RPG style, plus the game-over
 * screen.
 *
 * Flow (G.battle.phase):
 *
 *   APPEAR -> MENU -> FIGHT: PLAYER_HIT --\
 *                 \-> SHOOT: SHOOT --------+-> (enemy dead? WIN
 *                 |    (or NOPE -> MENU)   |    : ENEMY_HIT -> MENU)
 *                 \-> ITEM:  ITEMS submenu-/   (ITEM_USED counts as
 *                 |    (B backs out)            your turn too)
 *                 \-> RUN_OK  -> back to the overworld
 *                  \-> RUN_NO -> ENEMY_HIT -> MENU
 *   WIN    -> (leveled? LEVELUP) -> back to the overworld
 *   ENEMY_HIT with 0 hp -> ST_GAMEOVER
 *
 * Messages advance on A (or on their own after a beat).
 *
 * SHOOT needs the family shotgun + shells (items[] on the player --
 * see "THE ITEMS" in assets.h). The blast animation lives at the
 * bottom of battle_render().
 *
 * IDEAS TO BUILD OUT: multiple enemy species with their own
 * sprites/stats, enemy attack variety, battle music, capture mechanic...
 * ==========================================================================*/
#include "game_internal.h"

enum {
    PH_APPEAR, PH_MENU, PH_PLAYER_HIT, PH_ENEMY_HIT,
    PH_RUN_OK, PH_RUN_NO, PH_WIN, PH_LEVELUP,
    PH_SPECIAL,     /* it announces the move, THEN it lands           */
    PH_STUNNED,     /* you lost your turn -- it gets a free swing     */
    PH_SHOOT,       /* the gun comes up, roars, the pellets fly     */
    PH_NOPE,        /* tried to SHOOT without gun/shells -> MENU    */
    PH_ITEMS,       /* the ITEM submenu (not a message phase)       */
    PH_ITEM_USED,   /* healed up -- costs your turn                 */
};

/* which tick of PH_SHOOT the trigger gets pulled on */
#define SHOT_FIRE_TICK 10

/* tiny sprintf-free message builder: text a, text b, a number (if >= 0),
 * text c. e.g.  msgb("A ", sp->name, -1, " BLOCKS YOUR PATH!") */
static void msgb(const char *a, const char *b, int num, const char *c)
{
    char *m = G.battle.msg;
    int i = 0;
    while (*a && i < 60) m[i++] = *a++;
    while (*b && i < 60) m[i++] = *b++;
    if (num >= 0) {
        if (num >= 10) m[i++] = (char)('0' + (num / 10) % 10);
        m[i++] = (char)('0' + num % 10);
    }
    while (*c && i < 78) m[i++] = *c++;
    m[i] = '\0';
}

/* the species we're fighting (stats, name, sprites -- see assets.c) */
static const species_t *foe(void)
{
    return &species[G.battle.kind];
}

static void set_phase(int ph)
{
    G.battle.phase = ph;
    G.battle.timer = 0;
}

/* ---- THE TWO HEALTH BARS ARE THE SAME WOUND -------------------------------
 * Out in the field a creature's health is counted in HITS (entity.hp, out of
 * the species' ow_hits -- see config.h). In a battle it's counted in HP
 * (battle.enemy_hp, out of a level-scaled enemy_max). They are two views of
 * one creature, so damage has to survive the trip in BOTH directions:
 *
 *   field -> battle : you put four shells into the goblin and it caught you
 *                     anyway. It does NOT get to start the fight healthy.
 *   battle -> field : you hurt it in a fight and then ran. It does NOT get
 *                     to be back at full strength when you shoot at it.
 *
 * Rounding is deliberately in the creature's favour on the way back out
 * (round UP), so something that survived a fight on a sliver still costs you
 * one more shell -- never zero.
 */
static int hits_max_of(const species_t *sp)
{
    return (sp->ow_hits > 0) ? sp->ow_hits : 1;
}

void battle_start(int ent_index)
{
    entity_t *e = &G.ents[ent_index];
    const species_t *sp = &species[e->kind];
    G.battle.ent       = ent_index;
    G.battle.kind      = e->kind;
    G.battle.enemy_max = sp->base_hp + rng_range(0, 4)
                       + (G.player.level - 1) * 2;   /* they scale with you */

    /* it arrives as hurt as you left it */
    int hmax = hits_max_of(sp);
    int hits = e->hp;
    if (hits > hmax) hits = hmax;
    if (hits < 1)    hits = 1;            /* it's standing, so it has SOME */
    G.battle.enemy_hp = G.battle.enemy_max * hits / hmax;
    if (G.battle.enemy_hp < 1)
        G.battle.enemy_hp = 1;

    G.battle.menu = 0;
    /* "A GIANT ANT", but "THE TALL ONE" -- a couple of them carry their own
     * article, and "A THE GREY BLOCKS YOUR PATH" is not a sentence.
     *
     * The enemy_max bar stays FULL, so something you already shot walks in
     * with a visibly short bar -- and the line says why. */
    const char *name = sp->name;
    const char *art  = (name[0] == 'T' && name[1] == 'H' && name[2] == 'E'
                        && name[3] == ' ') ? "" : "A ";
    msgb(art, name, -1, (hits < hmax) ? ", BLEEDING, BLOCKS YOUR PATH!"
                                      : " BLOCKS YOUR PATH!");
    set_phase(PH_APPEAR);
    audio_sfx(SFX_STING);

    /* A boss gets its own theme -- slower, heavier, and it does not sound
     * like something you are going to win. */
    audio_music(sp->boss ? MUSIC_BOSS : MUSIC_BATTLE);

    G.state = ST_BATTLE;
    G.t = 0;
}

/* --- turn resolution -------------------------------------------------------*/

static void player_attacks(void)
{
    int dmg = PLAYER_BASE_ATK + G.player.level + rng_range(0, 2);
    G.battle.enemy_hp -= dmg;
    if (G.battle.enemy_hp < 0) G.battle.enemy_hp = 0;
    audio_sfx(SFX_HIT);
    msgb("YOU SWING YOUR SPADE! ", "", dmg, " DMG!");
    set_phase(PH_PLAYER_HIT);
}

/* land `dmg` on the player */
static void hurt_player(int dmg)
{
    G.player.hp -= dmg;
    if (G.player.hp < 0)
        G.player.hp = 0;
    audio_sfx(SFX_HURT);
    rumble(90 + dmg * 8);     /* the harder it hits, the harder it shakes */
}

/* heal the thing you're fighting */
static void heal_foe(int amount)
{
    G.battle.enemy_hp += amount;
    if (G.battle.enemy_hp > G.battle.enemy_max)
        G.battle.enemy_hp = G.battle.enemy_max;
}

/* ---- SPECIAL MOVES ---------------------------------------------------------
 * Each turn the creature rolls its moves in order. The first one that lands
 * is announced, and PH_SPECIAL resolves it a beat later -- so you get to
 * read "PROPHECY OF DISASTER!" before it hits you with it.
 */
static const move_t *pick_move(void)
{
    for (int i = 0; i < 2; i++) {
        const move_t *m = &foe()->moves[i];
        if (m->kind == MV_NONE || !m->name)
            continue;
        /* don't waste a heal at full health */
        if (m->kind == MV_HEAL && G.battle.enemy_hp >= G.battle.enemy_max)
            continue;
        if (rng_range(1, 100) <= m->chance) {
            G.battle.move_idx = i;      /* PH_SPECIAL needs to know which */
            return m;
        }
    }
    return 0;
}

/* the move was announced; now it actually happens */
static void resolve_move(const move_t *m)
{
    int dmg;
    switch (m->kind) {
    case MV_HEAVY:
        dmg = m->power + rng_range(0, 3);
        hurt_player(dmg);
        msgb("IT HITS FOR ", "", dmg, "!");
        break;

    case MV_MULTI:
        /* 2-3 quick blows -- resolved one per beat so you feel each one */
        G.battle.hits_left = rng_range(2, 3);
        G.battle.hit_power = m->power;
        dmg = m->power + rng_range(0, 1);
        hurt_player(dmg);
        G.battle.hits_left--;
        msgb("IT LANDS ", "", dmg, "!");
        break;

    case MV_DRAIN:
        dmg = m->power + rng_range(0, 2);
        hurt_player(dmg);
        heal_foe(dmg);
        msgb("IT DRAINS ", "", dmg, " AND SWELLS!");
        break;

    case MV_HEAL:
        heal_foe(m->power);
        audio_sfx(SFX_HEAL);
        msgb("IT KNITS ITSELF BACK TOGETHER! +", "", m->power, "");
        break;

    case MV_STUN:
        G.battle.stunned = 1;
        audio_sfx(SFX_STING);
        msgb("YOU CANNOT MOVE.", "", -1, "");
        break;

    default:
        break;
    }
    set_phase(PH_ENEMY_HIT);
}

static void enemy_attacks(void)
{
    const move_t *m = pick_move();
    if (m) {
        /* announce it. It lands next beat, in PH_SPECIAL. */
        msgb("", foe()->name, -1, " USES ");
        /* append the move name by hand -- msgb only takes three pieces */
        char *p = G.battle.msg;
        int i = 0;
        while (p[i]) i++;
        for (const char *c = m->name; *c && i < 70; c++)
            p[i++] = *c;
        if (i < 78) p[i++] = '!';
        p[i] = '\0';
        audio_sfx(SFX_STING);
        set_phase(PH_SPECIAL);
        return;
    }

    int dmg = foe()->atk + rng_range(0, 2);
    hurt_player(dmg);
    msgb("ITS EYES FLASH RED! ", "", dmg, " DMG!");
    set_phase(PH_ENEMY_HIT);
}

static void try_shoot(void)
{
    if (!G.player.items[ITEM_SHOTGUN]) {
        msgb("YOU MIME A GUN. IT ISN'T IMPRESSED.", "", -1, "");
        set_phase(PH_NOPE);
    } else if (G.player.items[ITEM_SHELLS] <= 0) {
        msgb("CLICK. OUT OF SHELLS.", "", -1, "");
        set_phase(PH_NOPE);
    } else {
        G.player.items[ITEM_SHELLS]--;
        msgb("YOU LEVEL BOTH BARRELS...", "", -1, "");
        set_phase(PH_SHOOT);   /* the bang lands at SHOT_FIRE_TICK */
    }
}

/* The pockets you can reach mid-fight, in menu order. */
static const int battle_items[3] = { ITEM_HERB, ITEM_MEDKIT, ITEM_TNT };

static void use_item(int kind)
{
    if (G.player.items[kind] <= 0) {
        audio_sfx(SFX_BLIP);
        return;                       /* pocket's empty -- stay in menu */
    }
    G.player.items[kind]--;

    if (kind == ITEM_TNT) {
        /* PA'S TNT. A flat, enormous hit that doesn't roll and doesn't
         * care what it's hitting. You still lose the turn -- you threw it
         * and got down. */
        G.battle.enemy_hp -= TNT_DMG;
        if (G.battle.enemy_hp < 0)
            G.battle.enemy_hp = 0;
        audio_sfx(SFX_STING);
        msgb("YOU LIGHT THE FUSE AND RUN! ", "", TNT_DMG, " DMG!");
        set_phase(PH_ITEM_USED);
        return;
    }

    audio_sfx(SFX_HEAL);
    if (kind == ITEM_HERB) {
        G.player.hp += HERB_HEAL;
        if (G.player.hp > G.player.max_hp)
            G.player.hp = G.player.max_hp;
        msgb("THE HERB STINGS. GOOD. +", "", HERB_HEAL, " HP");
    } else {
        G.player.hp = G.player.max_hp;
        msgb("GAUZE AND IODINE. FULL HP!", "", -1, "");
    }
    set_phase(PH_ITEM_USED);          /* patching up costs the turn */
}

static void try_run(void)
{
    if (rng_range(0, 1)) {
        audio_sfx(SFX_RUN);
        msgb("YOU GOT AWAY!", "", -1, "");
        set_phase(PH_RUN_OK);
    } else {
        msgb("IT WON'T LET YOU LEAVE.", "", -1, "");
        set_phase(PH_RUN_NO);
    }
}

static void win_battle(void)
{
    audio_sfx(SFX_VICTORY);
    msgb("IT VANISHES IN LIGHT! +", "", foe()->xp, " XP");
    G.player.xp += foe()->xp;
    /* Dead is dead: mark the spawn slot so it isn't standing there again
     * the moment you re-enter the map. Ordinary creatures are put back by
     * the next day's restock; a boss never is. (mark_dead lives in
     * overworld.c, next to the field kill, so both routes agree.) */
    int kind = G.battle.kind;
    G.ents[G.battle.ent].type = ENT_NONE;   /* gone from the map */
    mark_dead(G.battle.ent);
    if (kind == SPECIES_GOBLIN)
        G.flags |= FLAG_GOBLIN_DEAD;        /* the road east of town opens */
    set_phase(PH_WIN);
}

static void return_to_overworld(void)
{
    /* Carry the fight back out into the field. (A creature you KILLED is
     * already ENT_NONE -- win_battle() cleared it -- so this only touches
     * something you ran from.) */
    entity_t *e = &G.ents[G.battle.ent];
    if (e->type == ENT_ALIEN && G.battle.enemy_max > 0) {
        int hmax = hits_max_of(&species[e->kind]);
        int hits = (G.battle.enemy_hp * hmax + G.battle.enemy_max - 1)
                 / G.battle.enemy_max;            /* round UP, in its favour */
        if (hits < 1)    hits = 1;
        if (hits > hmax) hits = hmax;
        e->hp = hits;
        if (hits < hmax)
            e->hurt = 8;      /* it's still flinching as you back away */
    }

    G.state = ST_OVERWORLD;
    G.t = 0;                  /* G.banner is untouched: no banner on the
                                 way back from a fight */
    G.battle_grace = 90;      /* breathing room so it can't instantly re-grab */
    audio_music(map_music(G.map_id));    /* the world's song comes back */
}

/* A message phase advances on A -- but not INSTANTLY. It has to have been
 * readable for MSG_MIN_TICKS first, or a player mashing the button skips
 * every line the game says to them. It still auto-advances after 1.5s. */
static int msg_done(void)
{
    return (PRESSED(BTN_A) && G.battle.timer >= MSG_MIN_TICKS)
        || G.battle.timer > 90;
}

void battle_update(void)
{
    G.battle.timer++;

    switch (G.battle.phase) {

    case PH_APPEAR:
        if (msg_done()) set_phase(PH_MENU);
        break;

    case PH_MENU:
        /* 2x2 grid: FIGHT SHOOT / ITEM RUN. bit0 = column, bit1 = row */
        if (PRESSED(BTN_LEFT) || PRESSED(BTN_RIGHT)) {
            G.battle.menu ^= 1;
            audio_sfx(SFX_BLIP);
        }
        if (PRESSED(BTN_UP) || PRESSED(BTN_DOWN)) {
            G.battle.menu ^= 2;
            audio_sfx(SFX_BLIP);
        }
        if (PRESSED(BTN_A)) {
            switch (G.battle.menu) {
            case 0: player_attacks(); break;
            case 1: try_shoot();      break;
            case 2: G.battle.item_sel = 0; set_phase(PH_ITEMS); break;
            case 3: try_run();        break;
            }
        }
        break;

    case PH_SHOOT:
        if (G.battle.timer == SHOT_FIRE_TICK) {          /* BOOM */
            int dmg = SHOTGUN_BASE_DMG + G.player.level + rng_range(0, 4);
            G.battle.enemy_hp -= dmg;
            if (G.battle.enemy_hp < 0) G.battle.enemy_hp = 0;
            audio_sfx(SFX_SHOTGUN);
            msgb("THE FAMILY GUN ROARS! ", "", dmg, " DMG!");
        }
        if (G.battle.timer > 40 && msg_done()) {
            if (G.battle.enemy_hp <= 0) win_battle();
            else                        enemy_attacks();
        }
        break;

    case PH_NOPE:
        if (msg_done()) set_phase(PH_MENU);
        break;

    case PH_ITEMS:
        if (PRESSED(BTN_UP)) {
            G.battle.item_sel = (G.battle.item_sel + 2) % 3;
            audio_sfx(SFX_BLIP);
        }
        if (PRESSED(BTN_DOWN)) {
            G.battle.item_sel = (G.battle.item_sel + 1) % 3;
            audio_sfx(SFX_BLIP);
        }
        if (PRESSED(BTN_B))
            set_phase(PH_MENU);
        else if (PRESSED(BTN_A))
            use_item(battle_items[G.battle.item_sel]);
        break;

    case PH_ITEM_USED:
        /* the TNT can end the fight outright -- everything else just
         * hands the turn over */
        if (msg_done()) {
            if (G.battle.enemy_hp <= 0) win_battle();
            else                        enemy_attacks();
        }
        break;

    case PH_PLAYER_HIT:
        if (msg_done()) {
            if (G.battle.enemy_hp <= 0) win_battle();
            else                        enemy_attacks();
        }
        break;

    case PH_SPECIAL:
        /* it was announced last beat -- now it lands */
        if (msg_done())
            resolve_move(&foe()->moves[G.battle.move_idx]);
        break;

    case PH_ENEMY_HIT:
        if (msg_done()) {
            if (G.player.hp <= 0) {     /* lights out */
                audio_music(MUSIC_NONE);   /* dead silence */
                G.state = ST_GAMEOVER;
                G.t = 0;
            } else if (G.battle.hits_left > 0) {
                /* a MULTI move still has blows to land */
                int dmg = G.battle.hit_power + rng_range(0, 1);
                hurt_player(dmg);
                G.battle.hits_left--;
                msgb("AND AGAIN! ", "", dmg, "!");
                set_phase(PH_ENEMY_HIT);
            } else if (G.battle.stunned) {
                G.battle.stunned = 0;
                msgb("IT GETS A FREE SWING.", "", -1, "");
                set_phase(PH_STUNNED);
            } else {
                set_phase(PH_MENU);
            }
        }
        break;

    case PH_STUNNED:
        if (msg_done())
            enemy_attacks();     /* your turn is simply gone */
        break;

    case PH_RUN_OK:
        if (msg_done()) return_to_overworld();
        break;

    case PH_RUN_NO:
        if (msg_done()) enemy_attacks();
        break;

    case PH_WIN:
        if (msg_done()) {
            /* level up when you have level * XP_PER_LEVEL total xp */
            if (G.player.xp >= G.player.level * XP_PER_LEVEL) {
                G.player.xp -= G.player.level * XP_PER_LEVEL;
                G.player.level++;
                G.player.max_hp += 5;
                G.player.hp = G.player.max_hp;   /* full heal on level */
                audio_sfx(SFX_LEVELUP);
                msgb("YOU FEEL TOUGHER! LEVEL ", "", G.player.level, "!");
                set_phase(PH_LEVELUP);
            } else {
                return_to_overworld();
            }
        }
        break;

    case PH_LEVELUP:
        if (msg_done()) return_to_overworld();
        break;
    }
}

/* --- drawing ----------------------------------------------------------------*/

/* Draw the battle message, wrapped onto two lines.
 *
 * The box is only 28 characters wide at this font size, and lines like
 * "A HOPKINSVILLE GOBLIN BLOCKS YOUR PATH!" are 39 -- without this they
 * run straight off the right edge of the screen. Greedy wrap at spaces. */
#define MSG_COLS 27

static void draw_msg(const char *s, int x, int y)
{
    char line[MSG_COLS + 1];
    int  n = 0, row = 0;

    while (*s && row < 2) {
        /* how much of the rest fits on this line? */
        int take = 0, last_space = -1;
        while (s[take] && take < MSG_COLS) {
            if (s[take] == ' ')
                last_space = take;
            take++;
        }
        /* if we stopped mid-word, back up to the last space */
        if (s[take] && last_space > 0)
            take = last_space;

        for (n = 0; n < take; n++)
            line[n] = s[n];
        line[n] = '\0';
        gfx_text(x, y + row * 11, line, RGB565(255, 255, 255));

        s += take;
        while (*s == ' ')
            s++;                 /* swallow the break */
        row++;
    }
}

static void hp_bar(int x, int y, int hp, int max, const char *label)
{
    gfx_fill_rect(x, y, 88, 26, RGB565(16, 16, 24));
    gfx_rect     (x, y, 88, 26, RGB565(200, 200, 200));
    gfx_text(x + 4, y + 3, label, RGB565(255, 255, 255));
    int w = (max > 0) ? 80 * hp / max : 0;
    uint16_t col = (hp * 4 > max) ? RGB565(80, 200, 80)
                                  : RGB565(220, 60, 50);
    gfx_fill_rect(x + 4, y + 15, 80, 6, RGB565(50, 50, 58));
    if (w > 0)
        gfx_fill_rect(x + 4, y + 15, w, 6, col);
}

void battle_render(void)
{
    /* night field backdrop */
    gfx_clear(RGB565(10, 10, 28));
    gfx_fill_rect(0, 110, SCREEN_W, 50, RGB565(24, 44, 24)); /* dark grass */

    /* the enemy, big, upper right -- it floats, and shudders when hit
     * (a shotgun blast rattles it harder than the spade) */
    int shot_landed = (G.battle.phase == PH_SHOOT &&
                       G.battle.timer >= SHOT_FIRE_TICK &&
                       G.battle.timer <  SHOT_FIRE_TICK + 14);
    int shake = 0;
    if (G.battle.phase == PH_PLAYER_HIT && G.battle.timer < 12)
        shake = (G.battle.timer / 2) % 2 ? 2 : -2;
    else if (shot_landed)
        shake = (G.battle.timer / 2) % 2 ? 3 : -3;
    int bob   = (int)((G.frame / 12) % 2) * 2;
    /* In battle it's looking straight at you, so a directional creature
     * uses its DOWN-facing pair (spr0, spr0+1). */
    int eframe = (int)((G.frame / 15) % 2);
    int espr   = foe()->dirs ? foe()->spr0 + eframe
                             : (eframe ? foe()->spr1 : foe()->spr0);
    /* a boss fills the sky: drawn one scale-step bigger, and lower/left
     * so the bigger sprite still fits on the panel */
    int escale = foe()->boss ? 4 : 3;
    int ex     = foe()->boss ? 138 : 150;
    int ey     = foe()->boss ? 10  : 22;
    gfx_blit_ex(sprites[espr].px, TILE, TILE,
                ex + shake, ey + bob, escale, foe()->bright, 0);

    /* you, seen from behind, lower left -- flinch when hit, and rock
     * back from the recoil when the gun goes off */
    int pshake = (G.battle.phase == PH_ENEMY_HIT && G.battle.timer < 12)
               ? ((G.battle.timer / 2) % 2 ? 2 : -2) : 0;
    if (shot_landed && G.battle.timer < SHOT_FIRE_TICK + 5)
        pshake = -2;
    gfx_blit_ex(sprites[SPR_FARMER_UP_0].px, TILE, TILE,
                40 + pshake, 78, 3, 256, 0);

    /* THE GUN. Once you own the shotgun you carry it into every fight --
     * shouldered and ready between turns, and only swapped for the
     * leveled SPR_GUN_AIM while you're actually firing (below). */
    if (G.player.items[ITEM_SHOTGUN] && G.battle.phase != PH_SHOOT)
        gfx_blit_ex(sprites[SPR_GUN_READY].px, TILE, TILE,
                    58 + pshake, 62, 3, 256, 0);

    /* ---- the SHOOT animation: gun up, flash, pellets up the diagonal */
    if (G.battle.phase == PH_SHOOT) {
        int t = G.battle.timer;
        gfx_blit_ex(sprites[SPR_GUN_AIM].px, TILE, TILE,
                    58 + pshake, 52, 3, 256, 0);
        if (t >= SHOT_FIRE_TICK && t < SHOT_FIRE_TICK + 4) {
            /* muzzle flash: a ragged yellow-white star at the tip */
            gfx_fill_rect(92, 48, 10, 10, RGB565(255, 244, 180));
            gfx_fill_rect(87, 51, 20,  4, RGB565(255, 200,  60));
            gfx_fill_rect(95, 43,  4, 20, RGB565(255, 200,  60));
        }
        if (t >= SHOT_FIRE_TICK && t < SHOT_FIRE_TICK + 8) {
            /* the pellet cloud races at the thing in the air */
            int f = t - SHOT_FIRE_TICK;                  /* 0..7 */
            int x = 98 + (170 - 98) * f / 8;
            int y = 52 + (40  - 52) * f / 8;
            gfx_fill_rect(x,     y,     2, 2, RGB565(255, 255, 255));
            gfx_fill_rect(x - 5, y + 4, 2, 2, RGB565(255, 255, 200));
            gfx_fill_rect(x - 9, y - 3, 2, 2, RGB565(255, 255, 200));
        }
    }

    /* status boxes */
    char plabel[16] = "YOU LV ";
    plabel[7] = (char)('0' + G.player.level % 10);
    plabel[8] = '\0';
    hp_bar(6, 6, G.battle.enemy_hp, G.battle.enemy_max, foe()->name);
    hp_bar(SCREEN_W - 94, 78, G.player.hp, G.player.max_hp, plabel);

    /* Message / menu box along the bottom. The ITEM list is three rows deep
     * now, which does NOT fit the 30px box the 2x2 menu uses -- so that one
     * panel grows upward to hold it. */
    int panel_h = (G.battle.phase == PH_ITEMS) ? 42 : 30;
    int panel_y = SCREEN_H - 4 - panel_h;
    gfx_fill_rect(4, panel_y, SCREEN_W - 8, panel_h, RGB565(16, 16, 24));
    gfx_rect     (4, panel_y, SCREEN_W - 8, panel_h, RGB565(255, 255, 255));

    if (G.battle.phase == PH_MENU) {
        /* 2x2: FIGHT SHOOT / ITEM RUN. SHOOT greys out until you have
         * the gun (and something to feed it). */
        int can_shoot = G.player.items[ITEM_SHOTGUN] &&
                        G.player.items[ITEM_SHELLS] > 0;
        static const char *entry[4] = { "FIGHT", "SHOOT", "ITEM", "RUN" };
        for (int i = 0; i < 4; i++) {
            uint16_t col = (i == 1 && !can_shoot)
                         ? RGB565(110, 110, 110) : RGB565(255, 255, 255);
            gfx_text(30 + (i & 1) * 80,
                     SCREEN_H - 30 + (i >> 1) * 12, entry[i], col);
        }
        gfx_cursor(19 + (G.battle.menu & 1) * 80,
                   SCREEN_H - 30 + (G.battle.menu >> 1) * 12, G.frame);
    } else if (G.battle.phase == PH_ITEMS) {
        /* the pockets: herb, medkit and pa's TNT, with counts. B backs out.
         * Rows hang off the top of the (taller) panel, so they stay inside
         * it however the panel is sized. */
        const int top = panel_y + 4;
        for (int i = 0; i < 3; i++) {
            int kind = battle_items[i];
            int n = G.player.items[kind];
            char row[16];
            int p = 0;
            for (const char *nm = item_info[kind].name; *nm; nm++)
                row[p++] = *nm;
            row[p++] = ' ';
            row[p++] = 'X';
            if (n > 9) row[p++] = (char)('0' + (n / 10) % 10);
            row[p++] = (char)('0' + n % 10);
            row[p] = '\0';
            /* TNT reads hot when you have it -- it's the panic button */
            uint16_t col = !n ? RGB565(110, 110, 110)
                         : (kind == ITEM_TNT) ? RGB565(255, 140, 80)
                                              : RGB565(255, 255, 255);
            gfx_text(30, top + i * 12, row, col);
        }
        gfx_text(SCREEN_W - 64, top + 24, "B:BACK", RGB565(110, 110, 110));
        gfx_cursor(19, top + G.battle.item_sel * 12, G.frame);
    } else {
        draw_msg(G.battle.msg, 12, SCREEN_H - 28);
    }
}

/* ============================ GAME OVER =====================================*/

void gameover_update(void)
{
    if (G.t > 30 && (PRESSED(BTN_START) || PRESSED(BTN_A))) {
        /* You wake up in your own field. Hurt, but alive.
         *
         * THE PRICE: half the experience you'd banked toward your next
         * level is simply gone. Whatever they did to you out there, some
         * of it didn't come back with you. You keep your LEVEL -- losing
         * that would be brutal -- but the climb starts again. */
        G.player.xp /= 2;

        G.player.hp = G.player.max_hp;
        overworld_enter_map(MAP_FARM, 5, 6);
    }
}

void gameover_render(void)
{
    gfx_clear(0);
    uint16_t red  = RGB565(216, 40, 32);
    uint16_t gray = RGB565(120, 120, 120);

    const char *l1 = "EVERYTHING WENT WHITE.";
    const char *l2 = "YOU WOKE IN YOUR OWN FIELD.";
    const char *l3 = "SIX HOURS ARE MISSING.";
    const char *l4 = "SO IS HALF OF WHAT YOU KNEW.";
    if (G.t > 30)
        gfx_text((SCREEN_W - gfx_text_width(l1, 1)) / 2, 48, l1, gray);
    if (G.t > 90)
        gfx_text((SCREEN_W - gfx_text_width(l2, 1)) / 2, 64, l2, gray);
    if (G.t > 150)
        gfx_text((SCREEN_W - gfx_text_width(l3, 1)) / 2, 80, l3, red);
    if (G.t > 200)
        gfx_text((SCREEN_W - gfx_text_width(l4, 1)) / 2, 96, l4, red);
    if (G.t > 180 && G.t % 60 < 40) {
        const char *p = "PRESS START";
        gfx_text((SCREEN_W - gfx_text_width(p, 1)) / 2, 120, p,
                 RGB565(255, 255, 255));
    }
}
