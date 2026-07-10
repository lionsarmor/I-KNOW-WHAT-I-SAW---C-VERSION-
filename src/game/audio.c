/* ============================================================================
 * audio.c -- the synth, and the table of every sound in the game.
 *
 * HOW TO ADD A SOUND EFFECT:
 *   1. Add an id to the enum in audio.h        (e.g. SFX_DOOR)
 *   2. Add a step array below                  (e.g. steps_door[])
 *   3. Add it to the sfx_table[] in the same position as the enum
 *   4. Play it anywhere with audio_sfx(SFX_DOOR)
 *
 * The synth is intentionally dumb: square waves and an LFSR noise
 * generator, exactly like a 1980s sound chip. game_audio_fill() may be
 * called from an audio callback/ISR on another thread; it only reads
 * the channel structs, and the game only swaps pointers, so a torn
 * update at worst clicks for one buffer -- fine for a boilerplate.
 * ==========================================================================*/
#include "audio.h"
#include "game.h"   /* for AUDIO_RATE via config.h */

/* ---------------------------------------------------------------------------
 * SFX TABLE -- the actual sounds. Tweak freely; this is meant to be edited.
 * frequencies are Hz. A little cheat sheet of note frequencies:
 *   C2=65  C3=131  G3=196  C4=262  E4=330  G4=392  C5=523  E5=659
 * -------------------------------------------------------------------------*/
static const sfx_step_t steps_blip[]    = { {880, 2, 60} };
static const sfx_step_t steps_confirm[] = { {523, 3, 70}, {784, 4, 70} };
static const sfx_step_t steps_hit[]     = { {NOISE_BIT | 300, 5, 90},
                                            {NOISE_BIT | 120, 4, 60} };
static const sfx_step_t steps_hurt[]    = { {200, 4, 90}, {150, 4, 80},
                                            {100, 6, 70} };
/* The BAM: a huge noise crash falling into a low drone. */
static const sfx_step_t steps_sting[]   = { {NOISE_BIT | 900, 8, 120},
                                            {110, 10, 110}, {82, 14, 100},
                                            {55, 30, 80} };
static const sfx_step_t steps_victory[] = { {523, 5, 80}, {659, 5, 80},
                                            {784, 5, 80}, {1046, 12, 90} };
static const sfx_step_t steps_run[]     = { {392, 3, 70}, {330, 3, 60},
                                            {262, 5, 50} };
static const sfx_step_t steps_levelup[] = { {392, 4, 80}, {523, 4, 80},
                                            {659, 4, 80}, {784, 4, 80},
                                            {1046, 10, 90} };

static const struct { const sfx_step_t *steps; int n; } sfx_table[NUM_SFX] = {
    [SFX_BLIP]    = { steps_blip,    sizeof steps_blip    / sizeof(sfx_step_t) },
    [SFX_CONFIRM] = { steps_confirm, sizeof steps_confirm / sizeof(sfx_step_t) },
    [SFX_HIT]     = { steps_hit,     sizeof steps_hit     / sizeof(sfx_step_t) },
    [SFX_HURT]    = { steps_hurt,    sizeof steps_hurt    / sizeof(sfx_step_t) },
    [SFX_STING]   = { steps_sting,   sizeof steps_sting   / sizeof(sfx_step_t) },
    [SFX_VICTORY] = { steps_victory, sizeof steps_victory / sizeof(sfx_step_t) },
    [SFX_RUN]     = { steps_run,     sizeof steps_run     / sizeof(sfx_step_t) },
    [SFX_LEVELUP] = { steps_levelup, sizeof steps_levelup / sizeof(sfx_step_t) },
};

/* ---------------------------------------------------------------------------
 * MUSIC TABLE -- looping note sequences.
 * -------------------------------------------------------------------------*/

/* a slow human heartbeat in the dark: thump-thump ... silence ... */
static const sfx_step_t music_intro[] = {
    {52, 5, 95}, {0, 6, 0}, {46, 5, 75}, {0, 50, 0},
};

/* the title theme: slow, hollow, minor. lots of rests. */
static const sfx_step_t music_title[] = {
    {110, 30, 45}, {0, 15, 0}, {131, 30, 45}, {0, 15, 0},
    {110, 30, 45}, {0, 15, 0}, {104, 45, 45}, {0, 45, 0},
    {110, 30, 45}, {0, 15, 0}, {165, 30, 40}, {0, 15, 0},
    {156, 45, 45}, {0, 60, 0},
};

/* farm ambience: very low, very sparse, built on a tritone (A against
 * Eb -- the interval medieval monks called "the devil in music").
 * It never resolves. The farm never feels safe again. */
static const sfx_step_t music_farm[] = {
    { 55, 60, 30}, {0,  70, 0},           /* A1 ... long dark nothing   */
    { 78, 45, 26}, {0,  60, 0},           /* Eb2. the wrong note.       */
    { 55, 50, 30}, {0,  40, 0},
    { 82, 30, 22}, { 78, 40, 26},         /* it bends, settles wrong    */
    {0, 110, 0},                          /* wind. crickets. nothing.   */
    { 52, 55, 28}, {0,  90, 0},           /* a half-step LOWER than A.  */
};

static const struct { const sfx_step_t *steps; int n; } music_table[NUM_MUSIC] = {
    [MUSIC_NONE]  = { 0, 0 },
    [MUSIC_INTRO] = { music_intro, sizeof music_intro / sizeof(sfx_step_t) },
    [MUSIC_TITLE] = { music_title, sizeof music_title / sizeof(sfx_step_t) },
    [MUSIC_FARM]  = { music_farm,  sizeof music_farm  / sizeof(sfx_step_t) },
};

/* ---------------------------------------------------------------------------
 * The synth engine.
 * -------------------------------------------------------------------------*/
typedef struct {
    const sfx_step_t *steps;    /* 0 = channel silent            */
    int      nsteps;
    int      step;              /* current step index            */
    int      samples_left;      /* samples left in current step  */
    int      loop;              /* wrap around at the end?       */
    uint32_t phase, phase_inc;  /* 16.16 fixed-point oscillator  */
    int      vol;
    int      noise;             /* square wave or LFSR noise     */
    uint32_t lfsr;              /* noise generator state         */
} chan_t;

static chan_t chans[2];   /* [0] = music, [1] = sfx */

static void chan_load_step(chan_t *c)
{
    const sfx_step_t *s = &c->steps[c->step];
    int freq = s->freq & ~NOISE_BIT;
    c->noise        = (s->freq & NOISE_BIT) != 0;
    c->vol          = (freq == 0) ? 0 : s->vol;
    c->samples_left = s->ticks * (AUDIO_RATE / TICKS_PER_SEC);
    /* For noise, "freq" sets how fast the LFSR is stepped instead. */
    c->phase_inc    = (uint32_t)((uint64_t)(freq ? freq : 1) * 65536u
                                 / AUDIO_RATE);
}

static void chan_start(chan_t *c, const sfx_step_t *steps, int n, int loop)
{
    c->steps  = steps;
    c->nsteps = n;
    c->step   = 0;
    c->loop   = loop;
    c->phase  = 0;
    if (c->lfsr == 0) c->lfsr = 0xACE1u;
    if (steps && n > 0)
        chan_load_step(c);
    else
        c->steps = 0;
}

void audio_init(void)
{
    chans[0].steps = chans[1].steps = 0;
    chans[0].lfsr  = chans[1].lfsr  = 0xACE1u;
}

void audio_sfx(int id)
{
    if (id < 0 || id >= NUM_SFX) return;
    chan_start(&chans[1], sfx_table[id].steps, sfx_table[id].n, 0);
}

void audio_music(int id)
{
    if (id < 0 || id >= NUM_MUSIC) return;
    chan_start(&chans[0], music_table[id].steps, music_table[id].n, 1);
}

/* one sample from one channel, in the range roughly -vol..+vol * 64 */
static int chan_sample(chan_t *c)
{
    if (!c->steps)
        return 0;

    /* advance through the step sequence */
    if (c->samples_left <= 0) {
        c->step++;
        if (c->step >= c->nsteps) {
            if (c->loop) {
                c->step = 0;
            } else {
                c->steps = 0;
                return 0;
            }
        }
        chan_load_step(c);
    }
    c->samples_left--;

    c->phase += c->phase_inc;
    int on;
    if (c->noise) {
        /* step the LFSR each time the oscillator wraps */
        if (c->phase >= 65536u) {
            c->phase &= 0xFFFFu;
            uint32_t bit = ((c->lfsr >> 0) ^ (c->lfsr >> 2)
                          ^ (c->lfsr >> 3) ^ (c->lfsr >> 5)) & 1u;
            c->lfsr = (c->lfsr >> 1) | (bit << 15);
        }
        on = c->lfsr & 1u;
    } else {
        on = (c->phase >> 15) & 1u;   /* 50% duty square wave */
    }
    return (on ? 1 : -1) * c->vol * 64;
}

void game_audio_fill(int16_t *out, int nsamples)
{
    for (int i = 0; i < nsamples; i++) {
        int s = chan_sample(&chans[0]) + chan_sample(&chans[1]);
        if (s >  32767) s =  32767;
        if (s < -32768) s = -32768;
        out[i] = (int16_t)s;
    }
}
