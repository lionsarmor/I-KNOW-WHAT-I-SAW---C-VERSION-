/* ============================================================================
 * battle.c -- turn-based combat, Game-Boy-RPG style, plus the game-over
 * screen.
 *
 * Flow (G.battle.phase):
 *
 *   APPEAR -> MENU -> PLAYER_HIT -> (enemy dead? WIN : ENEMY_HIT) -> MENU
 *                \-> RUN_OK  -> back to the overworld
 *                 \-> RUN_NO -> ENEMY_HIT -> MENU
 *   WIN    -> (leveled? LEVELUP) -> back to the overworld
 *   ENEMY_HIT with 0 hp -> ST_GAMEOVER
 *
 * Messages advance on A (or on their own after a beat).
 *
 * IDEAS TO BUILD OUT: more menu entries (ITEMS, TALK?), multiple enemy
 * species with their own sprites/stats (add fields to spawn_t), enemy
 * attack variety, battle music, capture mechanic...
 * ==========================================================================*/
#include "game_internal.h"

enum {
    PH_APPEAR, PH_MENU, PH_PLAYER_HIT, PH_ENEMY_HIT,
    PH_RUN_OK, PH_RUN_NO, PH_WIN, PH_LEVELUP,
};

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
        if (PRESSED(BTN_UP) || PRESSED(BTN_DOWN)) {
            G.battle.menu ^= 1;
            audio_sfx(SFX_BLIP);
        }
        if (PRESSED(BTN_A))
            (G.battle.menu == 0) ? player_attacks() : try_run();
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

    /* the enemy, big, upper right -- it floats, and shudders when hit */
    int shake = (G.battle.phase == PH_PLAYER_HIT && G.battle.timer < 12)
              ? ((G.battle.timer / 2) % 2 ? 2 : -2) : 0;
    int bob   = (int)((G.frame / 12) % 2) * 2;
    int espr  = ((G.frame / 15) % 2) ? foe()->spr1 : foe()->spr0;
    gfx_blit_ex(sprites[espr].px, TILE, TILE,
                150 + shake, 22 + bob, 3, foe()->bright, 0);

    /* you, seen from behind, lower left -- flinch when hit */
    int pshake = (G.battle.phase == PH_ENEMY_HIT && G.battle.timer < 12)
               ? ((G.battle.timer / 2) % 2 ? 2 : -2) : 0;
    gfx_blit_ex(sprites[SPR_FARMER_UP_0].px, TILE, TILE,
                40 + pshake, 78, 3, 256, 0);

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
        gfx_text(30, SCREEN_H - 24, "FIGHT", RGB565(255, 255, 255));
        gfx_text(110, SCREEN_H - 24, "RUN", RGB565(255, 255, 255));
        int cx = (G.battle.menu == 0) ? 18 : 98;
        if (G.frame % 30 < 20)   /* blinking cursor */
            gfx_text(cx, SCREEN_H - 24, ">", RGB565(255, 220, 80));
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
