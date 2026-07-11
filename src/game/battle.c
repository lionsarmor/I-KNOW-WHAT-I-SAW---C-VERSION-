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

void battle_start(int ent_index)
{
    const species_t *sp = &species[G.ents[ent_index].kind];
    G.battle.ent       = ent_index;
    G.battle.kind      = G.ents[ent_index].kind;
    G.battle.enemy_max = sp->base_hp + rng_range(0, 4)
                       + (G.player.level - 1) * 2;   /* they scale with you */
    G.battle.enemy_hp  = G.battle.enemy_max;
    G.battle.menu      = 0;
    msgb("A ", sp->name, -1, " BLOCKS YOUR PATH!");
    set_phase(PH_APPEAR);
    audio_sfx(SFX_STING);
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

static void enemy_attacks(void)
{
    int dmg = foe()->atk + rng_range(0, 2);
    G.player.hp -= dmg;
    if (G.player.hp < 0) G.player.hp = 0;
    audio_sfx(SFX_HURT);
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

static void use_item(int kind)
{
    if (G.player.items[kind] <= 0) {
        audio_sfx(SFX_BLIP);
        return;                       /* pocket's empty -- stay in menu */
    }
    G.player.items[kind]--;
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
    G.ents[G.battle.ent].type = ENT_NONE;   /* gone from the map */
    set_phase(PH_WIN);
}

static void return_to_overworld(void)
{
    G.state = ST_OVERWORLD;
    G.t = 90;                 /* skip the map-name banner */
    G.battle_grace = 90;      /* breathing room so it can't instantly re-grab */
}

/* a message phase advances on A, or by itself after 1.5s */
static int msg_done(void)
{
    return PRESSED(BTN_A) || G.battle.timer > 90;
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
        if (PRESSED(BTN_UP) || PRESSED(BTN_DOWN)) {
            G.battle.item_sel ^= 1;
            audio_sfx(SFX_BLIP);
        }
        if (PRESSED(BTN_B))
            set_phase(PH_MENU);
        else if (PRESSED(BTN_A))
            use_item(G.battle.item_sel ? ITEM_MEDKIT : ITEM_HERB);
        break;

    case PH_ITEM_USED:
        if (msg_done()) enemy_attacks();
        break;

    case PH_PLAYER_HIT:
        if (msg_done()) {
            if (G.battle.enemy_hp <= 0) win_battle();
            else                        enemy_attacks();
        }
        break;

    case PH_ENEMY_HIT:
        if (msg_done()) {
            if (G.player.hp <= 0) {     /* lights out */
                audio_music(MUSIC_NONE);   /* dead silence */
                G.state = ST_GAMEOVER;
                G.t = 0;
            } else {
                set_phase(PH_MENU);
            }
        }
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

/* The menu cursor: a right-pointing triangle that breathes.
 *
 * NOTE: do NOT try to draw this with gfx_text(">") -- the 8x8 font only
 * has A-Z 0-9 and a little punctuation (see assets/font.h), so '>' comes
 * out BLANK. Hence the hand-built triangle. */
static void cursor(int x, int y)
{
    /* triangle wave over one second: 0..30..0 -- a slow, steady pulse
     * (a hard blink would fight the typewriter and the HP bars) */
    int t    = (int)(G.frame % 60);
    int glow = (t < 30) ? t : 60 - t;                  /* 0..30       */
    uint16_t col = RGB565(255, 170 + 85 * glow / 30, 90);  /* amber -> gold */

    /* solid right-pointing triangle: rows 1,2,3,4,3,2,1 px wide, so the
     * flat edge is on the left and the point lands at (x+3, y+3) */
    for (int i = 0; i < 7; i++) {
        int w = (i < 4) ? i + 1 : 7 - i;
        gfx_fill_rect(x, y + i, w, 1, col);
    }
    gfx_fill_rect(x + 3, y + 3, 1, 1, RGB565(255, 255, 220)); /* glint */
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
    int espr  = ((G.frame / 15) % 2) ? foe()->spr1 : foe()->spr0;
    gfx_blit_ex(sprites[espr].px, TILE, TILE,
                150 + shake, 22 + bob, 3, foe()->bright, 0);

    /* you, seen from behind, lower left -- flinch when hit, and rock
     * back from the recoil when the gun goes off */
    int pshake = (G.battle.phase == PH_ENEMY_HIT && G.battle.timer < 12)
               ? ((G.battle.timer / 2) % 2 ? 2 : -2) : 0;
    if (shot_landed && G.battle.timer < SHOT_FIRE_TICK + 5)
        pshake = -2;
    gfx_blit_ex(sprites[SPR_FARMER_UP_0].px, TILE, TILE,
                40 + pshake, 78, 3, 256, 0);

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

    /* message / menu box along the bottom */
    gfx_fill_rect(4, SCREEN_H - 34, SCREEN_W - 8, 30, RGB565(16, 16, 24));
    gfx_rect     (4, SCREEN_H - 34, SCREEN_W - 8, 30, RGB565(255, 255, 255));

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
        cursor(19 + (G.battle.menu & 1) * 80,
               SCREEN_H - 30 + (G.battle.menu >> 1) * 12);
    } else if (G.battle.phase == PH_ITEMS) {
        /* the pockets: herb and medkit with counts, B backs out */
        for (int i = 0; i < 2; i++) {
            int kind = i ? ITEM_MEDKIT : ITEM_HERB;
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
            gfx_text(30, SCREEN_H - 30 + i * 12, row,
                     n ? RGB565(255, 255, 255) : RGB565(110, 110, 110));
        }
        gfx_text(SCREEN_W - 64, SCREEN_H - 18, "B:BACK",
                 RGB565(110, 110, 110));
        cursor(19, SCREEN_H - 30 + G.battle.item_sel * 12);
    } else {
        gfx_text(12, SCREEN_H - 24, G.battle.msg, RGB565(255, 255, 255));
    }
}

/* ============================ GAME OVER =====================================*/

void gameover_update(void)
{
    if (G.t > 30 && (PRESSED(BTN_START) || PRESSED(BTN_A))) {
        /* you wake up at home. hurt, but alive. keep your levels. */
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
    if (G.t > 30)
        gfx_text((SCREEN_W - gfx_text_width(l1, 1)) / 2, 48, l1, gray);
    if (G.t > 90)
        gfx_text((SCREEN_W - gfx_text_width(l2, 1)) / 2, 64, l2, gray);
    if (G.t > 150)
        gfx_text((SCREEN_W - gfx_text_width(l3, 1)) / 2, 80, l3, red);
    if (G.t > 180 && G.t % 60 < 40) {
        const char *p = "PRESS START";
        gfx_text((SCREEN_W - gfx_text_width(p, 1)) / 2, 120, p,
                 RGB565(255, 255, 255));
    }
}
