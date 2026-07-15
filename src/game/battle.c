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
    PH_LOB,         /* TNT or holy water in the air: arc, impact,
                       explosion or sizzling mist (see the renderer) */
};

/* which tick of PH_SHOOT the trigger gets pulled on */
#define SHOT_FIRE_TICK 10

/* which tick of PH_LOB the thrown thing lands on */
#define LOB_IMPACT_TICK 16

/* ---- THE ROSARY, IN A FIGHT ------------------------------------------------
 * Worn (not merely carried), Mrs. Abernathy's beads do two things:
 * incoming attacks MISS often, and you swing ROSARY_LEVELS above your
 * real level. Neither works from the bottom of the pack. */
static int rosary_worn(void)
{
    return G.player.items[ITEM_ROSARY] && G.player.rosary;
}

static int rosary_deflect(void)
{
    return rosary_worn() && rng_range(1, 100) <= ROSARY_MISS_PCT;
}

/* the level your DAMAGE is computed at -- never fed to enemy scaling,
 * or wearing the beads would fatten every creature you meet */
static int eff_level(void)
{
    return G.player.level + (rosary_worn() ? ROSARY_LEVELS : 0);
}

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
    journal_saw(e->kind);        /* it's filling the screen. You SAW it. */
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
    int dmg = PLAYER_BASE_ATK + eff_level() + rng_range(0, 2);
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
    rumble(90 + dmg * 8);     /* the harder it hits, the harder the pad shakes */

    /* ...and so does the screen. Scaled by the damage, capped so a big move
     * doesn't turn the fight into an earthquake. */
    int mag = SHAKE_HIT_BASE + dmg / 6;
    if (mag > SHAKE_HIT_MAX)
        mag = SHAKE_HIT_MAX;
    shake(mag, SHAKE_HIT_TICKS);
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

    /* every move that TARGETS you can be turned aside by the beads --
     * a heal isn't aimed at you, so it always goes through */
    if (m->kind != MV_HEAL && rosary_deflect()) {
        audio_sfx(SFX_BLIP);
        msgb("THE BEADS FLARE. IT MISSES.", "", -1, "");
        set_phase(PH_ENEMY_HIT);
        return;
    }

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

    if (rosary_deflect()) {
        audio_sfx(SFX_BLIP);
        msgb("THE ROSARY TURNS IT ASIDE!", "", -1, "");
        set_phase(PH_ENEMY_HIT);
        return;
    }

    int dmg = foe()->atk + rng_range(0, 2);
    hurt_player(dmg);
    msgb("ITS EYES FLASH RED! ", "", dmg, " DMG!");
    set_phase(PH_ENEMY_HIT);
}

static void try_shoot(void)
{
    if (!PLAYER_HAS_GUN()) {
        msgb("YOU MIME A GUN. IT ISN'T IMPRESSED.", "", -1, "");
        set_phase(PH_NOPE);
    } else if (G.player.items[GUN_AMMO()] <= 0) {
        /* each gun starves on its own ammo: shells for the shotgun,
         * bullets for the pistol */
        msgb(GUN_AMMO() == ITEM_SHELLS ? "CLICK. OUT OF SHELLS."
                                       : "CLICK. OUT OF BULLETS.",
             "", -1, "");
        set_phase(PH_NOPE);
    } else {
        G.player.items[GUN_AMMO()]--;
        /* the shotgun gets both barrels; the pistol gets both hands;
         * the laser just gets held down */
        msgb(G.player.items[ITEM_LASER]
                 ? "YOU HOLD THE TRIGGER DOWN..."
                 : G.player.items[ITEM_SHOTGUN]
                 ? "YOU LEVEL BOTH BARRELS..."
                 : "YOU RAISE THE PISTOL, TWO HANDS...", "", -1, "");
        set_phase(PH_SHOOT);   /* the bang lands at SHOT_FIRE_TICK */
    }
}

/* Every pocket you might reach mid-fight, across both halves of the game.
 * The menu only lists what you're actually CARRYING (see avail_items), so
 * a farm fight shows HERB/MEDKIT/TNT/H.WATER and a ship fight shows
 * HERB/MEDKIT/GEL/A.NUKE -- the same list, filtered by what's in hand. */
#define N_BATTLE_ITEMS 6
static const int battle_items[N_BATTLE_ITEMS] = {
    ITEM_HERB, ITEM_MEDKIT, ITEM_TNT, ITEM_HOLYWATER, ITEM_GEL, ITEM_NUKE
};

/* the panel shows this many rows at once; a longer list SCROLLS */
#define ITEM_MENU_ROWS 4

/* The menu only lists what you're actually carrying -- an empty pocket is
 * not a menu entry, it's nothing (it used to sit there greyed out, four
 * rows of things you don't have). Rebuilt every frame it's needed, because
 * using the last herb has to pull HERB off the list on the spot.
 * Fills `out` with item kinds, returns how many. */
static int avail_items(int *out)
{
    int n = 0;
    for (int i = 0; i < N_BATTLE_ITEMS; i++)
        if (G.player.items[battle_items[i]] > 0)
            out[n++] = battle_items[i];
    return n;
}

static void use_item(int kind)
{
    if (G.player.items[kind] <= 0) {
        audio_sfx(SFX_BLIP);
        return;                       /* pocket's empty -- stay in menu */
    }
    G.player.items[kind]--;

    if (kind == ITEM_TNT || kind == ITEM_HOLYWATER || kind == ITEM_NUKE) {
        /* THROWN. The damage doesn't land here -- it lands when the arc
         * comes down, LOB_IMPACT_TICK into PH_LOB (see battle_update and
         * the animation in battle_render). */
        G.battle.lob_kind = kind;
        audio_sfx(SFX_BLIP);          /* the grunt of the throw */
        msgb(kind == ITEM_TNT  ? "YOU LIGHT THE FUSE AND THROW!"
           : kind == ITEM_NUKE ? "YOU ARM IT AND THROW -- GET SMALL!"
                               : "YOU HURL THE VIAL!", "", -1, "");
        set_phase(PH_LOB);
        return;
    }

    audio_sfx(SFX_HEAL);
    if (kind == ITEM_HERB) {
        G.player.hp += HERB_HEAL;
        if (G.player.hp > G.player.max_hp)
            G.player.hp = G.player.max_hp;
        msgb("THE HERB STINGS. GOOD. +", "", HERB_HEAL, " HP");
    } else if (kind == ITEM_GEL) {
        G.player.hp += GEL_HEAL;
        if (G.player.hp > G.player.max_hp)
            G.player.hp = G.player.max_hp;
        msgb("THE GEL CLOSES IT. COLD. +", "", GEL_HEAL, " HP");
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
    journal_kill(kind);          /* it goes in the book */
    /* It vanishes in light -- but the ground where it stood keeps the
     * stain. A boss leaves a big one. (The pool lands on the CURRENT map,
     * which is the map the fight started on: battles never change maps.) */
    add_blood(G.ents[G.battle.ent].x + 8, G.ents[G.battle.ent].y + 8,
              foe()->boss);
    G.ents[G.battle.ent].type = ENT_NONE;   /* gone from the map */
    mark_dead(G.battle.ent);
    if (kind == SPECIES_GOBLIN)
        G.flags |= FLAG_GOBLIN_DEAD;        /* the road east of town opens */
    if (kind == SPECIES_CHUPA_BOSS)
        G.flags |= FLAG_CHUPA_DEAD;         /* the park path stands empty */
    if (kind == SPECIES_TAN)
        G.flags |= FLAG_TAN_DEAD;           /* the pod will open now */
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
            case 2: {
                /* nothing to list? say so and bounce -- an empty menu is
                 * worse than no menu */
                int list[N_BATTLE_ITEMS];
                if (avail_items(list) == 0) {
                    msgb("YOUR POCKETS ARE EMPTY.", "", -1, "");
                    set_phase(PH_NOPE);
                } else {
                    G.battle.item_sel = 0;
                    G.battle.item_top = 0;
                    set_phase(PH_ITEMS);
                }
                break;
            }
            case 3: try_run();        break;
            }
        }
        break;

    case PH_SHOOT:
        if (G.battle.timer == SHOT_FIRE_TICK) {          /* BOOM */
            int laser   = G.player.items[ITEM_LASER];
            int shotgun = G.player.items[ITEM_SHOTGUN];
            int base = laser   ? LASER_BASE_DMG
                     : shotgun ? SHOTGUN_BASE_DMG : HANDGUN_BASE_DMG;
            int dmg  = base + eff_level() + rng_range(0, 4);
            G.battle.enemy_hp -= dmg;
            if (G.battle.enemy_hp < 0) G.battle.enemy_hp = 0;
            audio_sfx(laser ? SFX_STING : SFX_SHOTGUN);
            msgb(laser   ? "THE BEAM BURNS THROUGH IT! "
               : shotgun ? "THE FAMILY GUN ROARS! "
                         : "THE PISTOL CRACKS! ", "", dmg, " DMG!");
        }
        if (G.battle.timer > 40 && msg_done()) {
            if (G.battle.enemy_hp <= 0) win_battle();
            else                        enemy_attacks();
        }
        break;

    case PH_NOPE:
        if (msg_done()) set_phase(PH_MENU);
        break;

    case PH_ITEMS: {
        /* the list is what you HAVE, re-taken every frame: using the last
         * of something shortens it under the cursor */
        int list[N_BATTLE_ITEMS];
        int n = avail_items(list);
        if (n == 0) {                  /* threw the last thing you had */
            set_phase(PH_MENU);
            break;
        }
        if (G.battle.item_sel >= n)
            G.battle.item_sel = n - 1;
        if (PRESSED(BTN_UP)) {
            G.battle.item_sel = (G.battle.item_sel + n - 1) % n;
            audio_sfx(SFX_BLIP);
        }
        if (PRESSED(BTN_DOWN)) {
            G.battle.item_sel = (G.battle.item_sel + 1) % n;
            audio_sfx(SFX_BLIP);
        }
        /* keep the cursor on a visible row -- the window slides, the
         * cursor never leaves it (wrap included: top snaps to the end) */
        if (G.battle.item_sel < G.battle.item_top)
            G.battle.item_top = G.battle.item_sel;
        if (G.battle.item_sel >= G.battle.item_top + ITEM_MENU_ROWS)
            G.battle.item_top = G.battle.item_sel - (ITEM_MENU_ROWS - 1);
        if (PRESSED(BTN_B))
            set_phase(PH_MENU);
        else if (PRESSED(BTN_A))
            use_item(list[G.battle.item_sel]);
        break;
    }

    case PH_LOB:
        /* the throw is in the air; the impact is a single, exact tick */
        if (G.battle.timer == LOB_IMPACT_TICK) {
            if (G.battle.lob_kind == ITEM_TNT ||
                G.battle.lob_kind == ITEM_NUKE) {
                int nuke = (G.battle.lob_kind == ITEM_NUKE);
                G.battle.enemy_hp -= nuke ? NUKE_DMG : TNT_DMG;
                if (G.battle.enemy_hp < 0)
                    G.battle.enemy_hp = 0;
                audio_sfx(SFX_BOOM);
                rumble(255);
                shake(SHAKE_TNT_MAG - (nuke ? 0 : 2), nuke ? 24 : 18);
                msgb(nuke ? "GREEN FIRE SWALLOWS IT! "
                          : "THE BLAST TEARS AT IT! ",
                     "", nuke ? NUKE_DMG : TNT_DMG, " DMG!");
            } else {
                /* HOLY WATER. No roll, no resistance, no boss clause:
                 * whatever it is, it is OVER. That is what one of three
                 * vials in the whole game buys. */
                G.battle.enemy_hp = 0;
                audio_sfx(SFX_SIZZLE);       /* shatter, shine, the hiss */
                msgb("IT SIZZLES. IT SCREAMS. IT COMES APART!", "", -1, "");
            }
        }
        /* let the explosion / mist play out before the verdict */
        if (G.battle.timer > 64 && msg_done()) {
            if (G.battle.enemy_hp <= 0) win_battle();
            else                        enemy_attacks();
        }
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
                /* YOU leave a big one, right where you were standing when
                 * it happened. You wake up somewhere safe (gameover_update)
                 * -- and if you walk back before the next restock, there
                 * it is. Proof. You told them you were out here. */
                add_blood(G.player.x + 8, G.player.y + 8, 1);
                audio_music(MUSIC_NONE);   /* dead silence */
                G.state = ST_GAMEOVER;
                G.t = 0;
            } else if (G.battle.hits_left > 0) {
                /* a MULTI move still has blows to land -- and the beads
                 * get a say on every single one of them */
                G.battle.hits_left--;
                if (rosary_deflect()) {
                    msgb("AND AGAIN -- WIDE!", "", -1, "");
                } else {
                    int dmg = G.battle.hit_power + rng_range(0, 1);
                    hurt_player(dmg);
                    msgb("AND AGAIN! ", "", dmg, "!");
                }
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

static void hp_bar(int x, int y, int hp, int max, const char *label,
                   uint16_t edge, uint16_t text)
{
    /* The NAME decides how wide the box is: "HOPKINSVILLE GOBLIN" is more
     * than twice the old fixed 88, and it used to hang naked past the
     * border. The box never shrinks below 88 -- the HP bar needs the room. */
    int bw = gfx_text_width(label, 1) + 8;
    if (bw < 88) bw = 88;
    gfx_fill_rect(x, y, bw, 26, RGB565(16, 16, 24));
    gfx_rect     (x, y, bw, 26, edge);
    gfx_text(x + 4, y + 3, label, text);
    /* the bar itself stays 80 wide no matter what the box does: a health
     * bar has to mean the same amount of life on every creature */
    int w = (max > 0) ? 80 * hp / max : 0;
    uint16_t col = (hp * 4 > max) ? RGB565(80, 200, 80)
                                  : RGB565(220, 60, 50);
    gfx_fill_rect(x + 4, y + 15, 80, 6, RGB565(50, 50, 58));
    if (w > 0)
        gfx_fill_rect(x + 4, y + 15, w, 6, col);
}

/* HOW BAD IS THIS? The box wears it so you don't have to learn it the hard
 * way: the border and name are colored by the creature's class, judged by
 * the xp it pays -- the one number that already ranks every entry in the
 * bestiary. White you can hit with a spade. Yellow will hurt you. Orange
 * is why you carry the gun. Red is a BOSS, and red means bring everything.
 * (Public: the JOURNAL paints its pages with the same tiers.) */
void species_colors(int kind, uint16_t *edge, uint16_t *text)
{
    const species_t *f = &species[kind];
    if (f->boss)         { *edge = RGB565(210,  50,  40);
                           *text = RGB565(255, 100,  90); }
    else if (f->xp > 45) { *edge = RGB565(220, 130,  40);
                           *text = RGB565(255, 175,  95); }
    else if (f->xp > 15) { *edge = RGB565(200, 185,  60);
                           *text = RGB565(255, 240, 130); }
    else                 { *edge = RGB565(200, 200, 200);
                           *text = RGB565(255, 255, 255); }
}

void battle_render(void)
{
    /* the whole fight jolts when something lands on you */
    int sx, sy;
    shake_offset(&sx, &sy);
    gfx_origin(sx, sy);

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

    /* ITS BOX GOES DOWN FIRST. A long name (the goblin's reaches most of
     * the way across the screen) widens the box far enough to run under
     * the sprite -- so the creature stands ON the box, and the attack
     * effects below land on the creature. Box, monster, muzzle fire:
     * in that order. */
    {
        uint16_t edge, text;
        species_colors(G.battle.kind, &edge, &text);
        hp_bar(6, 6, G.battle.enemy_hp, G.battle.enemy_max, foe()->name,
               edge, text);
    }

    gfx_blit_ex(sprites[espr].px, TILE, TILE,
                ex + shake, ey + bob, escale, foe()->bright, 0);

    /* you, seen from behind, lower left -- flinch when hit, and rock
     * back from the recoil when the gun goes off */
    int pshake = (G.battle.phase == PH_ENEMY_HIT && G.battle.timer < 12)
               ? ((G.battle.timer / 2) % 2 ? 2 : -2) : 0;
    if (shot_landed && G.battle.timer < SHOT_FIRE_TICK + 5)
        pshake = -2;
    gfx_blit_ex(sprites[(G.flags & FLAG_PART1) ? SPR_LAWYER_UP_0
                                               : SPR_FARMER_UP_0].px,
                TILE, TILE, 40 + pshake, 78, 3, 256, 0);

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

    /* ---- THE LOB: the thrown thing arcs at it, and lands ---------------*/
    if (G.battle.phase == PH_LOB) {
        int t   = G.battle.timer;
        int exc = ex + 8 * escale;             /* the enemy's chest */
        int eyc = ey + 8 * escale;
        if (t < LOB_IMPACT_TICK) {
            /* the arc, hand to target, with a tumble */
            int fx = 66 + (exc - 66) * t / LOB_IMPACT_TICK;
            int fy = 84 + (eyc - 84) * t / LOB_IMPACT_TICK
                   - 4 * 26 * t * (LOB_IMPACT_TICK - t)
                     / (LOB_IMPACT_TICK * LOB_IMPACT_TICK);
            int spr = (G.battle.lob_kind == ITEM_TNT)  ? SPR_ITEM_TNT
                    : (G.battle.lob_kind == ITEM_NUKE) ? SPR_ITEM_NUKE
                                                       : SPR_ITEM_HOLYWATER;
            gfx_blit_ex(sprites[spr].px, TILE, TILE, fx - 8, fy - 8,
                        1, 256, (t / 3) % 2);
        } else if ((G.battle.lob_kind == ITEM_TNT ||
                    G.battle.lob_kind == ITEM_NUKE) &&
                   t < LOB_IMPACT_TICK + 26) {
            /* THE EXPLOSION: swells, whites out, eats itself into smoke.
             * TNT burns orange; the alien nuke burns GREEN, and bigger. */
            int nuke = (G.battle.lob_kind == ITEM_NUKE);
            int age = t - LOB_IMPACT_TICK;
            int r   = (nuke ? 12 : 8) + age * (nuke ? 4 : 3);
            int rmax = nuke ? 56 : 40;
            if (r > rmax) r = rmax;
            int holes = age * 8 / 26;
            for (int y = -r; y <= r; y++)
                for (int x = -r; x <= r; x++) {
                    int d2 = x * x + y * y;
                    if (d2 > r * r)
                        continue;
                    if (((x * 7 + y * 13 + (int)G.frame) & 7) < holes)
                        continue;
                    int d = d2 * 256 / (r * r + 1);
                    uint16_t col;
                    if (nuke)
                        col = (d <  90) ? RGB565(220, 255, 220)
                            : (d < 170) ? RGB565( 90, 240,  70)
                                        : RGB565( 24, 150,  40);
                    else
                        col = (d <  90) ? RGB565(255, 250, 220)
                            : (d < 170) ? RGB565(255, 176,  48)
                                        : RGB565(190,  70,  24);
                    gfx_pixel(exc + x, eyc + y, col);
                }
        } else if (G.battle.lob_kind == ITEM_HOLYWATER
                   && t < LOB_IMPACT_TICK + 34) {
            /* THE SIZZLING MIST: a pale cloud rises through it, popping
             * with white sparks, and the thing inside it stops existing */
            int age = t - LOB_IMPACT_TICK;
            int r   = 10 + age * 2;
            if (r > 38) r = 38;
            int fade = 256 - age * 256 / 34;
            for (int y = -r; y <= r; y++)
                for (int x = -r; x <= r; x++) {
                    int d2 = x * x + y * y;
                    if (d2 > r * r)
                        continue;
                    if (((x * 5 + y * 9 + (int)(G.frame * 3)) & 7) < 3)
                        continue;
                    int edge = 256 - d2 * 256 / (r * r + 1);
                    int a = 130 * edge / 256 * fade / 256;
                    if (a < 8)
                        continue;
                    gfx_blend_pixel(exc + x, eyc + y - age / 2,
                                    RGB565(205, 232, 255), a);
                }
            for (int i = 0; i < 10; i++) {         /* the sizzle */
                uint32_t h = (uint32_t)i * 2654435761u + G.frame * 131u;
                int sxp = (int)(h % (uint32_t)(r * 2 + 1)) - r;
                int syp = (int)((h >> 9) % (uint32_t)(r * 2 + 1)) - r;
                if (sxp * sxp + syp * syp > r * r)
                    continue;
                gfx_add_pixel(exc + sxp, eyc + syp - age / 2,
                              RGB565(255, 255, 255), 150 * fade / 256);
            }
        }
    }

    /* status boxes. Worn beads show the borrowed level -- same as the HUD. */
    char plabel[16] = "YOU LV ";
    {
        int lv = G.player.level
               + ((G.player.items[ITEM_ROSARY] && G.player.rosary)
                  ? ROSARY_LEVELS : 0);
        int p = 7;
        if (lv > 9) plabel[p++] = (char)('0' + (lv / 10) % 10);
        plabel[p++] = (char)('0' + lv % 10);
        plabel[p] = '\0';
    }
    /* (the enemy's box was drawn back at the top, UNDER the creature --
     * yours has no monster standing on it, so it draws here as ever) */
    hp_bar(SCREEN_W - 94, 78, G.player.hp, G.player.max_hp, plabel,
           RGB565(200, 200, 200), RGB565(255, 255, 255));

    /* Message / menu box along the bottom. The ITEM list is FOUR rows deep
     * now, which does NOT fit the 30px box the 2x2 menu uses -- so that one
     * panel grows upward to hold it. */
    int panel_h = (G.battle.phase == PH_ITEMS) ? 54 : 30;
    int panel_y = SCREEN_H - 4 - panel_h;
    gfx_fill_rect(4, panel_y, SCREEN_W - 8, panel_h, RGB565(16, 16, 24));
    gfx_rect     (4, panel_y, SCREEN_W - 8, panel_h, RGB565(255, 255, 255));

    if (G.battle.phase == PH_MENU) {
        /* 2x2: FIGHT SHOOT / ITEM RUN. SHOOT greys out until you have
         * the gun (and something to feed it). */
        int can_shoot = PLAYER_HAS_GUN() &&
                        G.player.items[GUN_AMMO()] > 0;
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
        /* the pockets: ONLY what you're carrying, with counts. No greyed
         * ghosts of things you don't have -- an empty pocket isn't a row.
         * The panel shows ITEM_MENU_ROWS at a time and the list slides
         * under it (item_top); arrows at the edge say there's more. */
        const int top = panel_y + 4;
        int list[N_BATTLE_ITEMS];
        int n = avail_items(list);
        int sel = (G.battle.item_sel < n) ? G.battle.item_sel
                                          : (n ? n - 1 : 0);
        for (int r = 0; r < ITEM_MENU_ROWS; r++) {
            int i = G.battle.item_top + r;
            if (i >= n)
                break;
            int kind  = list[i];
            int count = G.player.items[kind];
            char row[16];
            int p = 0;
            for (const char *nm = item_info[kind].name; *nm; nm++)
                row[p++] = *nm;
            row[p++] = ' ';
            row[p++] = 'X';
            if (count > 9) row[p++] = (char)('0' + (count / 10) % 10);
            row[p++] = (char)('0' + count % 10);
            row[p] = '\0';
            /* TNT reads hot -- it's the panic button. Holy water reads
             * COLD, because it's the other one. */
            uint16_t col = (kind == ITEM_TNT)       ? RGB565(255, 140, 80)
                         : (kind == ITEM_HOLYWATER) ? RGB565(160, 215, 255)
                                                    : RGB565(255, 255, 255);
            gfx_text(30, top + r * 12, row, col);
        }
        /* more list than panel: a nub of an arrow at whichever edge
         * still has rows hiding past it */
        if (G.battle.item_top > 0)
            gfx_text(SCREEN_W - 78, top, "^", RGB565(200, 200, 200));
        if (G.battle.item_top + ITEM_MENU_ROWS < n)
            gfx_text(SCREEN_W - 78, top + (ITEM_MENU_ROWS - 1) * 12, "V",
                     RGB565(200, 200, 200));
        gfx_text(SCREEN_W - 64, top + 36, "B:BACK", RGB565(110, 110, 110));
        gfx_cursor(19, top + (sel - G.battle.item_top) * 12, G.frame);
    } else {
        draw_msg(G.battle.msg, 12, SCREEN_H - 28);
    }
}

/* ============================ GAME OVER =====================================*/

void gameover_update(void)
{
    /* You read it. All of it. See GAMEOVER_READ_TICKS. */
    if (G.t > GAMEOVER_READ_TICKS && (PRESSED(BTN_START) || PRESSED(BTN_A))) {
        /* You wake up somewhere you started. Hurt, but alive.
         *
         * THE PRICE: half the experience you'd banked toward your next
         * level is simply gone. Whatever they did to you out there, some
         * of it didn't come back with you. You keep your LEVEL -- losing
         * that would be brutal -- but the climb starts again. */
        G.player.xp /= 2;

        G.player.hp = G.player.max_hp;
        /* the farmer wakes in his field; the lawyer wherever the night
         * dropped him -- the church steps up north, or the mouth of the
         * South Side if that's where it happened. And the man in the
         * silver suit doesn't get to die on their ship: he wakes up right
         * back on the table, because that is the whole horror of it.
         * (Check PART2 first -- MAP_UFO sorts after MAP_SOUTH, so the
         * ">= MAP_SOUTH" branch would otherwise swallow it.) */
        if (G.flags & FLAG_PART2)
            overworld_enter_map(MAP_UFO, 19, 6);
        else if (!(G.flags & FLAG_PART1))
            overworld_enter_map(MAP_FARM, 5, 6);
        else if (G.map_id >= MAP_SOUTH)
            overworld_enter_map(MAP_SOUTH, 11, 1);
        else
            overworld_enter_map(MAP_CITY, 3, 5);
    }
}

/* Text with a black halo, so it reads over ANYTHING without needing a panel
 * behind it. The death screen needs this: a solid plate behind the words
 * would box a hole out of the shape standing behind them, which is the one
 * thing on that screen worth looking at. */
static void ghost_text(int x, int y, const char *s, uint16_t col)
{
    static const int off[8][2] = {
        {-1,-1},{0,-1},{1,-1},{-1,0},{1,0},{-1,1},{0,1},{1,1}
    };
    for (int i = 0; i < 8; i++)
        gfx_text(x + off[i][0], y + off[i][1], s, RGB565(4, 4, 6));
    gfx_text(x, y, s, col);
}

/* the dead channel behind the death screen -- its own noise, so it can't
 * disturb the game's rng (damage rolls must stay reproducible) */
static uint32_t dead_seed = 0x7F4A7C15u;
static uint32_t dead_noise(void)
{
    dead_seed ^= dead_seed << 13;
    dead_seed ^= dead_seed >> 17;
    dead_seed ^= dead_seed << 5;
    return dead_seed;
}

void gameover_render(void)
{
    gfx_clear(0);

    /* ---- IT IS STANDING BEHIND THE WORDS ---------------------------------
     * THE TALL ONE, drawn seven times life size in the dark. It fades up so
     * slowly you are not sure it is there until it is -- and it never gets
     * anywhere near lit, so you never get a good look at it. Every so often
     * it simply isn't there for a frame.
     *
     * You didn't see it in the fight. You are seeing it now.
     */
    {
        const int scale = 7;                       /* 16x16 art -> 112x112 */
        int fx = (SCREEN_W - TILE * scale) / 2;
        int fy = (SCREEN_H - TILE * scale) / 2 - 4;

        /* creeps in, and stops well short of visible */
        int b = (int)G.t / 5;
        if (b > 40) b = 40;
        b += (int)((G.t / 9) % 5);                 /* it breathes */

        /* ...and it flickers out. Did it move? */
        if (b > 0 && (dead_noise() & 63) != 0)
            gfx_blit_ex(sprites[SPR_TALL_0].px, TILE, TILE, fx, fy,
                        scale, b, 0);

        /* THE EYES. They come up last, and they do not blink. The art puts
         * them at columns 6 and 9 of row 2 (see SPR_ART_TALL_0). */
        if (G.t > 140) {
            int pulse = 45 + (int)((G.t / 4) % 45);
            uint16_t glow = gfx_dim(RGB565(255, 40, 30), pulse);
            gfx_fill_rect(fx + 6 * scale, fy + 2 * scale, scale, scale, glow);
            gfx_fill_rect(fx + 9 * scale, fy + 2 * scale, scale, scale, glow);
        }
    }

    /* the signal is bad and getting worse */
    for (int i = 0; i < 26; i++) {
        uint32_t r = dead_noise();
        uint8_t v = (uint8_t)(r >> 20);
        gfx_pixel((int)(r % SCREEN_W), (int)((r >> 9) % SCREEN_H),
                  gfx_dim(RGB565(v, v, v), 90));
    }

    uint16_t red  = RGB565(216, 40, 32);
    uint16_t gray = RGB565(120, 120, 120);

    int part1 = (G.flags & FLAG_PART1) != 0;
    const char *l1 = "EVERYTHING WENT WHITE.";
    const char *l2 = part1 ? "YOU WOKE ON THE CHURCH STEPS."
                           : "YOU WOKE IN YOUR OWN FIELD.";
    const char *l3 = "SIX HOURS ARE MISSING.";
    const char *l4 = "SO IS HALF OF WHAT YOU KNEW.";

    /* haloed, NOT plated -- the shape behind stays whole */
    if (G.t > 30)
        ghost_text((SCREEN_W - gfx_text_width(l1, 1)) / 2, 48, l1, gray);
    if (G.t > 90)
        ghost_text((SCREEN_W - gfx_text_width(l2, 1)) / 2, 64, l2, gray);
    if (G.t > 150)
        ghost_text((SCREEN_W - gfx_text_width(l3, 1)) / 2, 80, l3, red);
    if (G.t > 200)
        ghost_text((SCREEN_W - gfx_text_width(l4, 1)) / 2, 96, l4, red);

    /* The prompt appears EXACTLY when the button starts working -- not one
     * tick before. The screen never invites you to leave while it is still
     * talking to you. */
    if (G.t > GAMEOVER_READ_TICKS && (G.t % 60) < 40) {
        const char *p = "PRESS START";
        ghost_text((SCREEN_W - gfx_text_width(p, 1)) / 2, 120, p,
                   RGB565(255, 255, 255));
    }
}
