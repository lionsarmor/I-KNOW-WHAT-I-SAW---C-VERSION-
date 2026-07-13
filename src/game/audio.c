/* ============================================================================
 * audio.c -- the tracker engine, the instruments, and every song in the game.
 *
 * TO WRITE A SONG:
 *   1. add an id to the MUSIC_* enum in audio.h
 *   2. write its tracks below (melody first, then bass, then drums --
 *      the machine with the fewest voices drops the LAST tracks)
 *   3. add it to song_table[]
 *   4. play it: audio_music(MUSIC_YOURS)
 *
 * TO ADD A SOUND EFFECT: an id in audio.h, a step list below, a row in
 * sfx_table[], then audio_sfx(SFX_YOURS).
 *
 * game_audio_fill() runs on the platform's AUDIO thread. The game only ever
 * swaps pointers into the channel structs, so the worst a torn update can do
 * is click for one buffer.
 * ==========================================================================*/
#include "audio.h"
#include "game.h"   /* AUDIO_RATE, AUDIO_CHANS via config.h */

/* ===========================================================================
 * THE INSTRUMENTS
 *   wave, attack, decay, sustain, release, vol, vib_rate, vib_depth, slide
 * ========================================================================= */
static const instr_t instruments[NUM_INSTR] = {
    /* Round and soft, so it holds the floor without muddying the tune. */
    [INS_BASS]  = { W_TRI,     1,  10, 200,  14, 210,  0,  0,    0 },

    /* The melody. A slow waver on held notes -- a straight tone reads as a
     * machine; a wavering one reads as somebody playing it. */
    [INS_LEAD]  = { W_PULSE25, 3,  22, 165,  20, 150, 22, 10,    0 },

    /* Swells in, sits under everything, never quite in focus. */
    [INS_PAD]   = { W_PULSE50, 40, 30, 130,  50,  70, 12,  5,    0 },

    /* Plucked. Gone before you're sure you heard it. */
    [INS_ARP]   = { W_PULSE12, 0,   9,   0,   6, 110,  0,  0,    0 },

    /* Saw. Brass. This is the sound of something noticing you. */
    [INS_STAB]  = { W_SAW,     1,  16, 150,  14, 130,  8,  6,    0 },

    /* The long low one that never resolves. */
    [INS_DRONE] = { W_TRI,     50, 60, 190, 100, 130,  6,  4,    0 },

    /* THE BEAM. A sawtooth is the right ugly, buzzing, irradiated shape --
     * and then the vibrato is turned up until the note simply will not hold
     * still. This is not a siren (a siren is a warning, and warnings are for
     * people who can still do something). It just WAVERS at you. */
    [INS_WARBLE]= { W_SAW,    30,   0, 255,  40, 165, 12, 60,   0 },

    /* Broadband hiss under it: the sound of the air being wrong. */
    [INS_WASH]  = { W_NOISE,  60,   0, 140,  50,  55,  0,  0,   0 },

    /* DRUMS. The pitch falling off a cliff is what your ear hears as a HIT
     * rather than a tone -- that's what the big negative slide is doing. */
    [INS_KICK]  = { W_TRI,     0,  16,   0,   2, 255,  0,  0, -900 },
    [INS_SNARE] = { W_NOISE,   0,  18,   0,   4, 175,  0,  0, -140 },
    [INS_HAT]   = { W_NOISE,   0,   4,   0,   2,  80,  0,  0,    0 },
};

/* ===========================================================================
 * THE SONGS
 *
 * Everything here is in A MINOR and leans on the same two intervals: the
 * flat second (A against Bb) and the tritone (A against Eb). They are the
 * two most uncomfortable sounds in western music and they never resolve.
 * That is the whole score. It is one idea, played differently depending on
 * how frightened you are supposed to be.
 * ========================================================================= */

/* ---- THE FARM. The overworld. -----------------------------------------------
 * It's a TUNE now, not a drone -- something you could hum, in a minor key,
 * over a bass that walks somewhere it shouldn't. But it's slow, and there is
 * a lot of space in it, and the space is the point: you are alone out here.
 */
static const ev_t farm_lead[] = {
    { NOTE(N_A,4), INS_LEAD, 6 }, { NOTE(N_C,5), INS_LEAD, 2 },
    { NOTE(N_B,4), INS_LEAD, 4 }, { NOTE(N_A,4), INS_LEAD, 4 },
    { NOTE(N_E,4), INS_LEAD, 8 }, { REST,        INS_LEAD, 8 },

    { NOTE(N_A,4), INS_LEAD, 6 }, { NOTE(N_C,5), INS_LEAD, 2 },
    { NOTE(N_E,5), INS_LEAD, 4 }, { NOTE(N_D,5), INS_LEAD, 4 },
    { NOTE(N_C,5), INS_LEAD, 8 }, { REST,        INS_LEAD, 8 },

    { NOTE(N_F,4), INS_LEAD, 6 }, { NOTE(N_A,4), INS_LEAD, 2 },
    { NOTE(N_G,4), INS_LEAD, 4 }, { NOTE(N_F,4), INS_LEAD, 4 },
    { NOTE(N_E,4), INS_LEAD, 8 }, { REST,        INS_LEAD, 8 },

    /* and it lands a semitone WRONG, and then just stops */
    { NOTE(N_AS,4),INS_LEAD, 8 }, { NOTE(N_A,4), INS_LEAD, 8 },
    { REST,        INS_LEAD,16 },
};
static const ev_t farm_bass[] = {
    { NOTE(N_A,2), INS_BASS, 8 }, { NOTE(N_A,2), INS_BASS, 8 },
    { NOTE(N_E,2), INS_BASS, 8 }, { NOTE(N_E,2), INS_BASS, 8 },

    { NOTE(N_A,2), INS_BASS, 8 }, { NOTE(N_A,2), INS_BASS, 8 },
    { NOTE(N_G,2), INS_BASS, 8 }, { NOTE(N_G,2), INS_BASS, 8 },

    { NOTE(N_F,2), INS_BASS, 8 }, { NOTE(N_F,2), INS_BASS, 8 },
    { NOTE(N_C,2), INS_BASS, 8 }, { NOTE(N_C,2), INS_BASS, 8 },

    { NOTE(N_DS,2),INS_BASS, 8 },   /* the tritone. the wrong note. */
    { NOTE(N_E,2), INS_BASS, 8 },
    { NOTE(N_A,2), INS_BASS,16 },
};
static const ev_t farm_pad[] = {
    { NOTE(N_A,3), INS_PAD, 16 }, { NOTE(N_E,3), INS_PAD, 16 },
    { NOTE(N_A,3), INS_PAD, 16 }, { NOTE(N_G,3), INS_PAD, 16 },
    { NOTE(N_F,3), INS_PAD, 16 }, { NOTE(N_C,3), INS_PAD, 16 },
    { NOTE(N_DS,3),INS_PAD, 16 }, { NOTE(N_E,3), INS_PAD, 16 },
};
static const track_t farm_tracks[] = {
    { farm_lead, sizeof farm_lead / sizeof(ev_t) },
    { farm_bass, sizeof farm_bass / sizeof(ev_t) },
    { farm_pad,  sizeof farm_pad  / sizeof(ev_t) },
};

/* ---- THE TOWN. Same world, a few more people in it. -------------------------
 * Slightly warmer, slightly faster, and it has a pulse. It is still in the
 * same wrong key, because nothing is actually better.
 */
static const ev_t town_lead[] = {
    { NOTE(N_E,4), INS_LEAD, 4 }, { NOTE(N_A,4), INS_LEAD, 4 },
    { NOTE(N_B,4), INS_LEAD, 4 }, { NOTE(N_C,5), INS_LEAD, 4 },
    { NOTE(N_B,4), INS_LEAD, 4 }, { NOTE(N_A,4), INS_LEAD, 4 },
    { NOTE(N_E,4), INS_LEAD, 8 },

    { NOTE(N_G,4), INS_LEAD, 4 }, { NOTE(N_A,4), INS_LEAD, 4 },
    { NOTE(N_C,5), INS_LEAD, 4 }, { NOTE(N_B,4), INS_LEAD, 4 },
    { NOTE(N_A,4), INS_LEAD, 8 }, { REST,        INS_LEAD, 8 },
};
static const ev_t town_bass[] = {
    { NOTE(N_A,2), INS_BASS, 4 }, { NOTE(N_A,2), INS_BASS, 4 },
    { NOTE(N_E,2), INS_BASS, 4 }, { NOTE(N_E,2), INS_BASS, 4 },
    { NOTE(N_F,2), INS_BASS, 4 }, { NOTE(N_F,2), INS_BASS, 4 },
    { NOTE(N_E,2), INS_BASS, 8 },

    { NOTE(N_C,2), INS_BASS, 4 }, { NOTE(N_C,2), INS_BASS, 4 },
    { NOTE(N_G,2), INS_BASS, 4 }, { NOTE(N_G,2), INS_BASS, 4 },
    { NOTE(N_A,2), INS_BASS, 8 }, { NOTE(N_E,2), INS_BASS, 8 },
};
static const ev_t town_arp[] = {
    { NOTE(N_A,4), INS_ARP, 2 }, { NOTE(N_C,5), INS_ARP, 2 },
    { NOTE(N_E,5), INS_ARP, 2 }, { NOTE(N_C,5), INS_ARP, 2 },
    { NOTE(N_A,4), INS_ARP, 2 }, { NOTE(N_C,5), INS_ARP, 2 },
    { NOTE(N_E,5), INS_ARP, 2 }, { NOTE(N_C,5), INS_ARP, 2 },
    { NOTE(N_F,4), INS_ARP, 2 }, { NOTE(N_A,4), INS_ARP, 2 },
    { NOTE(N_C,5), INS_ARP, 2 }, { NOTE(N_A,4), INS_ARP, 2 },
    { NOTE(N_E,4), INS_ARP, 2 }, { NOTE(N_GS,4),INS_ARP, 2 },
    { NOTE(N_B,4), INS_ARP, 2 }, { NOTE(N_GS,4),INS_ARP, 2 },
};
static const track_t town_tracks[] = {
    { town_lead, sizeof town_lead / sizeof(ev_t) },
    { town_bass, sizeof town_bass / sizeof(ev_t) },
    { town_arp,  sizeof town_arp  / sizeof(ev_t) },
};

/* ---- BATTLE. ---------------------------------------------------------------
 * Fast, driving, and it does not let up. A sixteenth-note bass that hammers
 * one note, a lead that keeps clawing UP and getting slapped back down, and
 * a drum kit that never misses a beat because it isn't tired and you are.
 */
static const ev_t battle_lead[] = {
    { NOTE(N_A,4), INS_STAB, 2 }, { NOTE(N_A,4), INS_STAB, 2 },
    { NOTE(N_C,5), INS_STAB, 2 }, { NOTE(N_A,4), INS_STAB, 2 },
    { NOTE(N_DS,5),INS_STAB, 4 },              /* the tritone, screamed */
    { NOTE(N_E,5), INS_STAB, 4 },

    { NOTE(N_A,4), INS_STAB, 2 }, { NOTE(N_A,4), INS_STAB, 2 },
    { NOTE(N_C,5), INS_STAB, 2 }, { NOTE(N_A,4), INS_STAB, 2 },
    { NOTE(N_G,5), INS_STAB, 4 }, { NOTE(N_E,5), INS_STAB, 4 },

    { NOTE(N_F,5), INS_STAB, 2 }, { NOTE(N_E,5), INS_STAB, 2 },
    { NOTE(N_DS,5),INS_STAB, 2 }, { NOTE(N_E,5), INS_STAB, 2 },
    { NOTE(N_F,5), INS_STAB, 2 }, { NOTE(N_E,5), INS_STAB, 2 },
    { NOTE(N_C,5), INS_STAB, 2 }, { NOTE(N_A,4), INS_STAB, 2 },

    { NOTE(N_AS,4),INS_STAB, 4 }, { NOTE(N_A,4), INS_STAB, 4 },
    { NOTE(N_E,4), INS_STAB, 8 },
};
static const ev_t battle_bass[] = {
    { NOTE(N_A,1), INS_BASS, 1 }, { NOTE(N_A,1), INS_BASS, 1 },
    { NOTE(N_A,1), INS_BASS, 1 }, { NOTE(N_A,1), INS_BASS, 1 },
    { NOTE(N_A,1), INS_BASS, 1 }, { NOTE(N_A,1), INS_BASS, 1 },
    { NOTE(N_A,1), INS_BASS, 1 }, { NOTE(N_A,1), INS_BASS, 1 },
    { NOTE(N_A,1), INS_BASS, 1 }, { NOTE(N_A,1), INS_BASS, 1 },
    { NOTE(N_A,1), INS_BASS, 1 }, { NOTE(N_A,1), INS_BASS, 1 },
    { NOTE(N_G,1), INS_BASS, 2 }, { NOTE(N_F,1), INS_BASS, 2 },

    { NOTE(N_A,1), INS_BASS, 1 }, { NOTE(N_A,1), INS_BASS, 1 },
    { NOTE(N_A,1), INS_BASS, 1 }, { NOTE(N_A,1), INS_BASS, 1 },
    { NOTE(N_A,1), INS_BASS, 1 }, { NOTE(N_A,1), INS_BASS, 1 },
    { NOTE(N_A,1), INS_BASS, 1 }, { NOTE(N_A,1), INS_BASS, 1 },
    { NOTE(N_F,1), INS_BASS, 2 }, { NOTE(N_F,1), INS_BASS, 2 },
    { NOTE(N_E,1), INS_BASS, 2 }, { NOTE(N_DS,1),INS_BASS, 2 },

    { NOTE(N_F,1), INS_BASS, 2 }, { NOTE(N_F,1), INS_BASS, 2 },
    { NOTE(N_E,1), INS_BASS, 2 }, { NOTE(N_E,1), INS_BASS, 2 },
    { NOTE(N_A,1), INS_BASS, 2 }, { NOTE(N_A,1), INS_BASS, 2 },
    { NOTE(N_AS,1),INS_BASS, 2 }, { NOTE(N_A,1), INS_BASS, 2 },
};
static const ev_t battle_drums[] = {
    { NOTE(N_C,3), INS_KICK,  2 }, { NOTE(N_C,5), INS_HAT,   1 },
    { NOTE(N_C,5), INS_HAT,   1 }, { NOTE(N_D,4), INS_SNARE, 2 },
    { NOTE(N_C,5), INS_HAT,   1 }, { NOTE(N_C,5), INS_HAT,   1 },

    { NOTE(N_C,3), INS_KICK,  1 }, { NOTE(N_C,3), INS_KICK,  1 },
    { NOTE(N_C,5), INS_HAT,   1 }, { NOTE(N_C,5), INS_HAT,   1 },
    { NOTE(N_D,4), INS_SNARE, 2 },
    { NOTE(N_C,5), INS_HAT,   1 }, { NOTE(N_D,4), INS_SNARE, 1 },
};
static const track_t battle_tracks[] = {
    { battle_lead,  sizeof battle_lead  / sizeof(ev_t) },
    { battle_bass,  sizeof battle_bass  / sizeof(ev_t) },
    { battle_drums, sizeof battle_drums / sizeof(ev_t) },
};

/* ---- BOSS. -----------------------------------------------------------------
 * Slower than the battle theme, and much heavier. Things you cannot outrun
 * do not need to hurry. The lead is the same three notes over and over
 * because it isn't a melody, it's a warning.
 */
static const ev_t boss_lead[] = {
    { NOTE(N_A,4), INS_STAB, 4 }, { NOTE(N_AS,4),INS_STAB, 4 },
    { NOTE(N_A,4), INS_STAB, 8 },
    { NOTE(N_DS,4),INS_STAB, 8 },              /* tritone. held. */
    { REST,        INS_STAB, 4 },

    { NOTE(N_A,4), INS_STAB, 4 }, { NOTE(N_AS,4),INS_STAB, 4 },
    { NOTE(N_C,5), INS_STAB, 8 },
    { NOTE(N_B,4), INS_STAB, 8 }, { NOTE(N_AS,4),INS_STAB, 8 },
};
static const ev_t boss_bass[] = {
    { NOTE(N_A,1), INS_BASS, 2 }, { REST,        INS_BASS, 2 },
    { NOTE(N_A,1), INS_BASS, 2 }, { NOTE(N_A,1), INS_BASS, 2 },
    { NOTE(N_AS,1),INS_BASS, 4 }, { NOTE(N_A,1), INS_BASS, 4 },
    { NOTE(N_DS,1),INS_BASS, 8 },

    { NOTE(N_A,1), INS_BASS, 2 }, { REST,        INS_BASS, 2 },
    { NOTE(N_A,1), INS_BASS, 2 }, { NOTE(N_A,1), INS_BASS, 2 },
    { NOTE(N_F,1), INS_BASS, 4 }, { NOTE(N_E,1), INS_BASS, 4 },
    { NOTE(N_A,1), INS_BASS, 8 },
};
static const ev_t boss_drums[] = {
    { NOTE(N_C,3), INS_KICK,  4 }, { NOTE(N_D,4), INS_SNARE, 4 },
    { NOTE(N_C,3), INS_KICK,  2 }, { NOTE(N_C,3), INS_KICK,  2 },
    { NOTE(N_D,4), INS_SNARE, 4 },
    { NOTE(N_C,3), INS_KICK,  4 }, { NOTE(N_D,4), INS_SNARE, 2 },
    { NOTE(N_D,4), INS_SNARE, 2 }, { NOTE(N_D,4), INS_SNARE, 4 },
};
static const ev_t boss_drone[] = {
    { NOTE(N_A,1), INS_DRONE, 32 }, { NOTE(N_AS,1), INS_DRONE, 32 },
};
static const track_t boss_tracks[] = {
    { boss_lead,  sizeof boss_lead  / sizeof(ev_t) },
    { boss_bass,  sizeof boss_bass  / sizeof(ev_t) },
    { boss_drums, sizeof boss_drums / sizeof(ev_t) },
    { boss_drone, sizeof boss_drone / sizeof(ev_t) },
};

/* ---- INTRO: a heartbeat in the dark. -------------------------------------
 * thump-THUMP ... rest ... thump-THUMP. The kick has to be pitched around
 * C3: it is a drum, and a drum is a pitch FALLING (see INS_KICK's slide).
 * Start it too low and there is nowhere left to fall -- the pitch bottoms
 * out below hearing and all you get is a DC pop. That was the bug.
 *
 * The drone under it sits at A2, not A1. A1 is 55Hz, which a laptop speaker
 * simply cannot reproduce -- it was there all along and nobody could hear it.
 */
static const ev_t intro_beat[] = {
    { NOTE(N_C,3), INS_KICK, 2 },       /* thump...          */
    { REST,        INS_KICK, 1 },
    { NOTE(N_C,3), INS_KICK, 2 },       /* ...THUMP          */
    { REST,        INS_KICK, 13 },      /* ...and the wait   */
};
static const ev_t intro_drone[] = {
    { NOTE(N_A,2), INS_DRONE, 18 },
};
static const ev_t intro_air[] = {
    { NOTE(N_E,3), INS_PAD, 18 },       /* a fifth above. hollow. */
};
static const track_t intro_tracks[] = {
    { intro_beat,  sizeof intro_beat  / sizeof(ev_t) },
    { intro_drone, sizeof intro_drone / sizeof(ev_t) },
    { intro_air,   sizeof intro_air   / sizeof(ev_t) },
};

/* ---- TITLE: hollow, slow, and it will not resolve. ----------------------- */
static const ev_t title_lead[] = {
    { NOTE(N_A,3), INS_PAD, 12 }, { NOTE(N_C,4), INS_PAD, 12 },
    { NOTE(N_A,3), INS_PAD, 12 }, { NOTE(N_GS,3),INS_PAD, 16 },
    { NOTE(N_A,3), INS_PAD, 12 }, { NOTE(N_E,4), INS_PAD, 12 },
    { NOTE(N_DS,4),INS_PAD, 20 }, { REST,        INS_PAD, 12 },
};
static const ev_t title_bass[] = {
    { NOTE(N_A,1), INS_DRONE, 24 }, { NOTE(N_GS,1),INS_DRONE, 24 },
    { NOTE(N_A,1), INS_DRONE, 24 }, { NOTE(N_DS,1),INS_DRONE, 24 },
};
static const track_t title_tracks[] = {
    { title_lead, sizeof title_lead / sizeof(ev_t) },
    { title_bass, sizeof title_bass / sizeof(ev_t) },
};

/* ---- THE HIGHWAY: tyre hum, and something keeping pace. ------------------ */
static const ev_t drive_pulse[] = {
    { NOTE(N_E,1), INS_BASS, 2 }, { NOTE(N_E,1), INS_BASS, 2 },
    { NOTE(N_E,1), INS_BASS, 2 }, { NOTE(N_E,1), INS_BASS, 2 },
    { NOTE(N_E,1), INS_BASS, 2 }, { NOTE(N_E,1), INS_BASS, 2 },
    { NOTE(N_F,1), INS_BASS, 2 }, { NOTE(N_E,1), INS_BASS, 2 },
};
static const ev_t drive_air[] = {
    { NOTE(N_B,3), INS_PAD, 16 }, { NOTE(N_AS,3), INS_PAD, 16 },
    { NOTE(N_B,3), INS_PAD, 16 }, { NOTE(N_F,3),  INS_PAD, 16 },
};
static const ev_t drive_hat[] = {
    { NOTE(N_C,5), INS_HAT, 2 }, { NOTE(N_C,5), INS_HAT, 2 },
    { NOTE(N_C,5), INS_HAT, 2 }, { NOTE(N_C,5), INS_HAT, 2 },
};
static const track_t drive_tracks[] = {
    { drive_pulse, sizeof drive_pulse / sizeof(ev_t) },
    { drive_air,   sizeof drive_air   / sizeof(ev_t) },
    { drive_hat,   sizeof drive_hat   / sizeof(ev_t) },
};

/* ---- THE BEAM. -------------------------------------------------------------
 * The van is off the ground. This is what that sounds like.
 *
 * A warbling tone that climbs -- and it climbs by SEMITONES, not by anything
 * you could sing, so it never arrives anywhere. Under it a drone that rises
 * with it, and a wash of noise that is just the air being wrong. It loops,
 * and every loop is a little higher, and it does not stop until the white.
 */
static const ev_t beam_warble[] = {
    { NOTE(N_E,5),  INS_WARBLE, 4 }, { NOTE(N_F,5),  INS_WARBLE, 4 },
    { NOTE(N_FS,5), INS_WARBLE, 4 }, { NOTE(N_G,5),  INS_WARBLE, 4 },
    { NOTE(N_GS,5), INS_WARBLE, 6 }, { NOTE(N_A,5),  INS_WARBLE, 6 },
    { NOTE(N_AS,5), INS_WARBLE, 8 },
};
static const ev_t beam_drone[] = {
    { NOTE(N_A,1),  INS_DRONE, 12 }, { NOTE(N_AS,1), INS_DRONE, 12 },
    { NOTE(N_B,1),  INS_DRONE, 12 },
};
static const ev_t beam_hiss[] = {
    { NOTE(N_C,6),  INS_WASH, 36 },
};
static const track_t beam_tracks[] = {
    { beam_warble, sizeof beam_warble / sizeof(ev_t) },
    { beam_drone,  sizeof beam_drone  / sizeof(ev_t) },
    { beam_hiss,   sizeof beam_hiss   / sizeof(ev_t) },
};

/* ---- END OF PROLOGUE: low, slow, and it lands on the wrong note. --------- */
static const ev_t prologue_lead[] = {
    { NOTE(N_A,3), INS_PAD, 16 }, { NOTE(N_E,3), INS_PAD, 16 },
    { NOTE(N_F,3), INS_PAD, 16 }, { NOTE(N_E,3), INS_PAD, 16 },
    { NOTE(N_DS,3),INS_PAD, 24 }, { NOTE(N_E,3), INS_PAD, 24 },
    { NOTE(N_AS,3),INS_PAD, 24 }, { NOTE(N_A,3), INS_PAD, 32 },
};
static const ev_t prologue_bass[] = {
    { NOTE(N_A,1), INS_DRONE, 32 }, { NOTE(N_F,1), INS_DRONE, 32 },
    { NOTE(N_DS,1),INS_DRONE, 48 }, { NOTE(N_A,1), INS_DRONE, 56 },
};
static const track_t prologue_tracks[] = {
    { prologue_lead, sizeof prologue_lead / sizeof(ev_t) },
    { prologue_bass, sizeof prologue_bass / sizeof(ev_t) },
};

/* rowticks = ENGINE ticks per row. 240 = one second, so 30 is a lively
 * eighth-note and 60 is a slow, sad one. */
static const song_t song_table[NUM_MUSIC] = {
    [MUSIC_NONE]     = { 0, 0, 0, 0 },
    [MUSIC_INTRO]    = { intro_tracks,    3, 60, 1 },
    [MUSIC_TITLE]    = { title_tracks,    2, 50, 1 },
    [MUSIC_FARM]     = { farm_tracks,     3, 46, 1 },
    [MUSIC_TOWN]     = { town_tracks,     3, 34, 1 },
    [MUSIC_BATTLE]   = { battle_tracks,   3, 20, 1 },   /* fast. urgent. */
    [MUSIC_BOSS]     = { boss_tracks,     4, 26, 1 },   /* heavy. slower. */
    [MUSIC_DRIVE]    = { drive_tracks,    3, 24, 1 },
    [MUSIC_BEAM]     = { beam_tracks,     3, 26, 1 },
    [MUSIC_PROLOGUE] = { prologue_tracks, 2, 55, 1 },
};

/* ===========================================================================
 * THE SOUND EFFECTS.  { freq Hz, engine ticks, volume, waveform }
 * ========================================================================= */
static const sfx_step_t sx_blip[]    = { {1200, 4, 90, W_PULSE25} };
static const sfx_step_t sx_confirm[] = { { 660, 6,110, W_PULSE25},
                                         { 990, 8,110, W_PULSE25} };
static const sfx_step_t sx_hit[]     = { {1800, 4,170, W_NOISE},
                                         { 700, 8,140, W_NOISE},
                                         { 220,10, 90, W_NOISE} };
static const sfx_step_t sx_hurt[]    = { { 300, 6,170, W_SAW},
                                         { 220, 8,150, W_SAW},
                                         { 150,12,110, W_SAW} };
static const sfx_step_t sx_sting[]   = { {2200, 6,200, W_NOISE},
                                         { 900,10,180, W_NOISE},
                                         { 110,20,170, W_TRI},
                                         {  82,30,140, W_TRI},
                                         {  55,60,100, W_TRI} };
static const sfx_step_t sx_victory[] = { { 523, 8,150, W_PULSE25},
                                         { 659, 8,150, W_PULSE25},
                                         { 784, 8,150, W_PULSE25},
                                         {1046,24,160, W_PULSE25} };
static const sfx_step_t sx_run[]     = { { 784, 6,120, W_PULSE12},
                                         { 587, 6,100, W_PULSE12},
                                         { 392,12, 80, W_PULSE12} };
static const sfx_step_t sx_levelup[] = { { 523, 6,150, W_PULSE25},
                                         { 659, 6,150, W_PULSE25},
                                         { 784, 6,150, W_PULSE25},
                                         {1046, 6,160, W_PULSE25},
                                         {1318,24,170, W_PULSE25} };
/* BOTH BARRELS: a crack, then a rolling echo off the barn. */
static const sfx_step_t sx_shotgun[] = { {3000, 3,230, W_NOISE},
                                         {1400, 8,210, W_NOISE},
                                         { 500,12,150, W_NOISE},
                                         { 160,24, 80, W_NOISE},
                                         {  90,20, 40, W_NOISE} };
static const sfx_step_t sx_pickup[]  = { { 880, 5,130, W_PULSE25},
                                         {1175, 5,130, W_PULSE25},
                                         {1568,12,140, W_PULSE25} };
static const sfx_step_t sx_heal[]    = { { 523, 8,110, W_TRI},
                                         { 659, 8,120, W_TRI},
                                         { 784,20,130, W_TRI} };
/* PA'S TNT. The crack arrives at FULL volume -- no ramp, the first sample
 * is the loudest thing the speaker has done all day -- then the THUMP: two
 * long sub-bass triangles you feel more than hear (65Hz is still over the
 * 30Hz pitch floor), and the rubble hisses down after. Compare sx_sting,
 * which this used to borrow: sting is a wasp. This is a demolition. */
static const sfx_step_t sx_boom[]    = { {3400, 4,255, W_NOISE},
                                         {1500, 8,245, W_NOISE},
                                         { 400,12,230, W_NOISE},
                                         {  65,26,220, W_TRI},
                                         {  48,34,180, W_TRI},
                                         { 220,22, 90, W_NOISE},
                                         { 140,30, 50, W_NOISE} };
/* HOLY WATER. Three acts: the vial SHATTERS (one hard tick of noise),
 * something bright climbs OUT of it -- four thin pulses walking up over
 * two octaves -- and then the long sizzle: fine, high hiss that flutters
 * between two brightnesses as it dies, steam off a griddle. It used to
 * borrow sx_heal, which is a man drinking medicine. This is the reason
 * there are only three vials. */
static const sfx_step_t sx_sizzle[]  = { {2800, 3,220, W_NOISE},
                                         {1046, 3,150, W_PULSE12},
                                         {1568, 4,165, W_PULSE12},
                                         {2093, 5,180, W_PULSE12},
                                         {3136, 7,190, W_PULSE12},
                                         {4500, 8,170, W_NOISE},
                                         {3700,10,150, W_NOISE},
                                         {4800,12,120, W_NOISE},
                                         {3900,14, 90, W_NOISE},
                                         {5000,18, 60, W_NOISE},
                                         {4200,22, 35, W_NOISE} };
/* THE BABBLE: one soft syllable, spoken-voice register, a quick step down
 * (or up -- talk2 rises, like a question). The dialog box fires one of
 * these per WORD as the typewriter lays it down, hopping between the three
 * pitches so a sentence burbles like speech instead of ticking like a
 * clock. Quiet on purpose: it plays under every line in the game. */
static const sfx_step_t sx_talk0[]   = { { 392, 3, 85, W_PULSE25},
                                         { 330, 3, 60, W_PULSE25} };
static const sfx_step_t sx_talk1[]   = { { 494, 3, 85, W_PULSE25},
                                         { 415, 3, 60, W_PULSE25} };
static const sfx_step_t sx_talk2[]   = { { 440, 3, 85, W_PULSE25},
                                         { 523, 3, 60, W_PULSE25} };

static const struct { const sfx_step_t *s; int n; } sfx_table[NUM_SFX] = {
#define SFXROW(id, arr) [id] = { arr, sizeof arr / sizeof(sfx_step_t) }
    SFXROW(SFX_BLIP,    sx_blip),
    SFXROW(SFX_CONFIRM, sx_confirm),
    SFXROW(SFX_HIT,     sx_hit),
    SFXROW(SFX_HURT,    sx_hurt),
    SFXROW(SFX_STING,   sx_sting),
    SFXROW(SFX_VICTORY, sx_victory),
    SFXROW(SFX_RUN,     sx_run),
    SFXROW(SFX_LEVELUP, sx_levelup),
    SFXROW(SFX_SHOTGUN, sx_shotgun),
    SFXROW(SFX_PICKUP,  sx_pickup),
    SFXROW(SFX_HEAL,    sx_heal),
    SFXROW(SFX_BOOM,    sx_boom),
    SFXROW(SFX_SIZZLE,  sx_sizzle),
    SFXROW(SFX_TALK0,   sx_talk0),
    SFXROW(SFX_TALK1,   sx_talk1),
    SFXROW(SFX_TALK2,   sx_talk2),
#undef SFXROW
};

/* ===========================================================================
 * THE ENGINE
 * ========================================================================= */

/* Frequency of each semitone in octave 0, as Hz * 256. Shift left by the
 * octave to go up: every octave is exactly a doubling, which is the one
 * piece of luck in all of music. */
static const uint32_t semitone_q8[12] = {
    4186, 4435, 4699, 4978, 5274, 5588,
    5920, 6272, 6645, 7040, 7459, 7902,
};

static uint32_t note_freq_q8(int note)      /* note: 1 = C0 */
{
    int n = note - 1;
    if (n < 0)  n = 0;
    if (n > 95) n = 95;                     /* eight octaves is plenty */
    return semitone_q8[n % 12] << (n / 12);
}

enum { CH_IDLE, CH_SONG, CH_SFX };
enum { ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE, ENV_OFF };

typedef struct {
    int mode;

    /* song feed */
    const ev_t *ev;
    int nev, evi, rows_left, rowticks, loop;

    /* sfx feed */
    const sfx_step_t *steps;
    int nsteps, stepi, step_ticks;
    int sfx_vol, sfx_wave;

    /* the voice */
    const instr_t *ins;
    uint32_t freq_q8, base_q8;
    uint32_t phase, phase_inc;
    uint32_t lfsr;
    int env, env_t, level, age;
} chan_t;

#define MASTER_GAIN 60
#define FREQ_FLOOR_Q8 (30 * 256)   /* 30Hz. see chan_tick(). */

static chan_t chans[AUDIO_CHANS];
static int    tick_acc;

/* master volumes, 0..255. Full by default so a platform that never calls
 * audio_volume() still makes a noise. */
static int vol_music = 255;
static int vol_sfx   = 255;

void audio_volume(int music, int sfx)
{
    if (music < 0)   music = 0;
    if (music > 255) music = 255;
    if (sfx   < 0)   sfx   = 0;
    if (sfx   > 255) sfx   = 255;
    vol_music = music;
    vol_sfx   = sfx;
}

/* a triangle LFO, -128..127, for vibrato */
static int lfo(int t, int rate)
{
    int p = (t * rate / 8) & 63;
    int v = (p < 32) ? p * 8 - 128 : 127 - (p - 32) * 8;
    return v;
}

static void chan_note_on(chan_t *c, int note, int instr)
{
    c->ins     = &instruments[instr];
    c->base_q8 = note_freq_q8(note);
    c->freq_q8 = c->base_q8;
    c->env     = ENV_ATTACK;
    c->env_t   = 0;
    c->level   = 0;
    c->age     = 0;
    c->phase   = 0;
    if (!c->lfsr) c->lfsr = 0xACE1u;
}

static void chan_note_off(chan_t *c)
{
    if (c->env != ENV_OFF)
        c->env = ENV_RELEASE, c->env_t = 0;
}

/* one ENGINE tick (1/240s): advance songs, envelopes, vibrato, slides */
static void chan_tick(chan_t *c)
{
    /* --- feed: has the current note/step run out? --- */
    if (c->mode == CH_SONG) {
        if (--c->rows_left <= 0) {
            c->evi++;
            if (c->evi >= c->nev) {
                if (!c->loop) { c->mode = CH_IDLE; chan_note_off(c); }
                else            c->evi = 0;
            }
            if (c->mode == CH_SONG) {
                const ev_t *e = &c->ev[c->evi];
                c->rows_left = e->dur * c->rowticks;
                if (e->note == REST) chan_note_off(c);
                else                 chan_note_on(c, e->note, e->instr);
            }
        }
    } else if (c->mode == CH_SFX) {
        if (--c->step_ticks <= 0) {
            c->stepi++;
            if (c->stepi >= c->nsteps) {
                c->mode = CH_IDLE;
                chan_note_off(c);
            } else {
                const sfx_step_t *s = &c->steps[c->stepi];
                c->step_ticks = s->ticks;
                c->sfx_vol    = s->vol;
                c->sfx_wave   = s->wave;
                c->base_q8    = (uint32_t)s->freq << 8;
                c->freq_q8    = c->base_q8;
                c->level      = s->freq ? 255 : 0;
                c->age        = 0;
            }
        }
    }

    if (!c->ins && c->mode != CH_SFX)
        return;

    c->age++;

    /* --- envelope (song voices only; sfx steps carry their own volume) --- */
    if (c->mode != CH_SFX && c->ins) {
        const instr_t *in = c->ins;
        c->env_t++;
        switch (c->env) {
        case ENV_ATTACK:
            c->level = in->attack ? (255 * c->env_t / in->attack) : 255;
            if (c->level >= 255) {
                c->level = 255;
                c->env = ENV_DECAY;
                c->env_t = 0;
            }
            break;
        case ENV_DECAY:
            if (!in->decay || c->env_t >= in->decay) {
                c->level = in->sustain;
                c->env = ENV_SUSTAIN;
            } else {
                c->level = 255 - (255 - in->sustain) * c->env_t / in->decay;
            }
            break;
        case ENV_SUSTAIN:
            c->level = in->sustain;
            break;
        case ENV_RELEASE:
            if (!in->release || c->env_t >= in->release) {
                c->level = 0;
                c->env = ENV_OFF;
            } else {
                c->level = in->sustain
                         - in->sustain * c->env_t / in->release;
            }
            break;
        default:
            c->level = 0;
            break;
        }
    }

    /* --- pitch: vibrato and slide -------------------------------------- */
    int32_t f = (int32_t)c->base_q8;
    if (c->ins && c->mode != CH_SFX) {
        if (c->ins->vib_rate)
            f += (int32_t)c->base_q8 / 256 * lfo(c->age, c->ins->vib_rate)
                 * c->ins->vib_depth / 128;
        if (c->ins->slide)
            f += (int32_t)c->ins->slide * c->age;
    }
    /* THE PITCH FLOOR. A slide (or a badly-pitched drum) must never drag a
     * voice down towards 0Hz: a triangle at 1Hz isn't a note, it's a slow
     * ramp between the rails, and it comes out of the speaker as a POP. Stop
     * well above that -- 30Hz is already below what anything can reproduce,
     * but it still behaves like a wave. */
    if (f < FREQ_FLOOR_Q8) f = FREQ_FLOOR_Q8;
    c->freq_q8 = (uint32_t)f;

    c->phase_inc = (uint32_t)(((uint64_t)c->freq_q8 * 256u) / AUDIO_RATE);
}

/* one SAMPLE from one channel, roughly -8000..8000 before mixing */
static int chan_sample(chan_t *c)
{
    if (c->mode == CH_IDLE && c->env == ENV_OFF)
        return 0;
    if (!c->level)
        return 0;

    int wave = (c->mode == CH_SFX) ? c->sfx_wave
             : (c->ins ? c->ins->wave : W_PULSE50);

    c->phase += c->phase_inc;

    int s;   /* -128..127 */
    if (wave == W_NOISE) {
        if (c->phase >= 65536u) {
            c->phase &= 0xFFFFu;
            uint32_t bit = ((c->lfsr >> 0) ^ (c->lfsr >> 2)
                          ^ (c->lfsr >> 3) ^ (c->lfsr >> 5)) & 1u;
            c->lfsr = (c->lfsr >> 1) | (bit << 15);
        }
        s = (c->lfsr & 1u) ? 110 : -110;
    } else if (wave == W_TRI) {
        uint32_t p = (c->phase >> 8) & 0xFFu;      /* 0..255 */
        s = (p < 128) ? (int)p * 2 - 128 : 127 - ((int)p - 128) * 2;
    } else if (wave == W_SAW) {
        s = (int)((c->phase >> 8) & 0xFFu) - 128;
    } else {
        /* the pulse family: duty decides how thin it sounds */
        uint32_t duty = (wave == W_PULSE12) ? 8192u
                      : (wave == W_PULSE25) ? 16384u : 32768u;
        s = ((c->phase & 0xFFFFu) < duty) ? 110 : -110;
    }

    int vol = (c->mode == CH_SFX)
            ? c->sfx_vol
            : (c->ins ? c->ins->vol : 0);

    /* the player's master, applied last */
    vol = vol * ((c->mode == CH_SFX) ? vol_sfx : vol_music) / 255;

    /* sample * envelope * instrument volume, then MASTER_GAIN.
     *
     * The gain is set so the loudest song (the boss theme: four voices, all
     * busy) peaks around 20000 of 32767, which leaves headroom for a shotgun
     * blast on top of it without the mix clipping. Raise it and the loud
     * moments distort; there is a hard limiter below, but it sounds like
     * one. */
    return s * c->level / 255 * vol / 255 * MASTER_GAIN;
}

/* ---- public --------------------------------------------------------------*/

void audio_init(void)
{
    for (int i = 0; i < AUDIO_CHANS; i++) {
        chan_t *c = &chans[i];
        c->mode = CH_IDLE;
        c->env  = ENV_OFF;
        c->ins  = 0;
        c->level = 0;
        c->lfsr = 0xACE1u + (uint32_t)i * 7u;
    }
    tick_acc = 0;
}

void audio_sfx(int id)
{
    if (id < 0 || id >= NUM_SFX || !sfx_table[id].n)
        return;
    /* SFX live on the top channels, so a gunshot never eats the bassline */
    chan_t *c = &chans[MUSIC_CHANS];
    const sfx_step_t *s = sfx_table[id].s;

    c->mode       = CH_SFX;
    c->steps      = s;
    c->nsteps     = sfx_table[id].n;
    c->stepi      = 0;
    c->step_ticks = s[0].ticks;
    c->sfx_vol    = s[0].vol;
    c->sfx_wave   = s[0].wave;
    c->base_q8    = (uint32_t)s[0].freq << 8;
    c->freq_q8    = c->base_q8;
    c->phase      = 0;
    c->level      = s[0].freq ? 255 : 0;
    c->env        = ENV_SUSTAIN;
    c->age        = 0;
    c->ins        = 0;
    c->phase_inc  = (uint32_t)(((uint64_t)c->freq_q8 * 256u) / AUDIO_RATE);
}

void audio_music(int id)
{
    if (id < 0 || id >= NUM_MUSIC)
        return;

    const song_t *sg = &song_table[id];

    for (int i = 0; i < MUSIC_CHANS; i++) {
        chan_t *c = &chans[i];

        /* A song with more tracks than this machine has voices simply loses
         * the last ones -- which is why the tracks are ordered by how much
         * the song needs them. The ESP32 keeps the tune, the bass and the
         * drums, and drops the sweetening. */
        if (!sg->tracks || i >= sg->ntracks) {
            c->mode = CH_IDLE;
            chan_note_off(c);
            continue;
        }

        const track_t *t = &sg->tracks[i];
        c->mode      = CH_SONG;
        c->ev        = t->ev;
        c->nev       = t->n;
        c->evi       = 0;
        c->rowticks  = sg->rowticks;
        c->loop      = sg->loop;
        c->rows_left = t->ev[0].dur * sg->rowticks;
        c->phase     = 0;
        if (t->ev[0].note == REST) {
            c->ins   = &instruments[t->ev[0].instr];
            c->env   = ENV_OFF;
            c->level = 0;
        } else {
            chan_note_on(c, t->ev[0].note, t->ev[0].instr);
        }
    }
}

void game_audio_fill(int16_t *out, int nsamples)
{
    for (int i = 0; i < nsamples; i++) {
        /* run the engine's own 240Hz clock off the sample clock, exactly */
        tick_acc += ENGINE_HZ;
        while (tick_acc >= AUDIO_RATE) {
            tick_acc -= AUDIO_RATE;
            for (int c = 0; c < AUDIO_CHANS; c++)
                chan_tick(&chans[c]);
        }

        int s = 0;
        for (int c = 0; c < AUDIO_CHANS; c++)
            s += chan_sample(&chans[c]);

        if (s >  32767) s =  32767;
        if (s < -32768) s = -32768;
        out[i] = (int16_t)s;
    }
}
