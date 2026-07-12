/* ============================================================================
 * audio.h -- a small TRACKER. Still no floats, no libc, no OS: it runs the
 * same on the desktop and on the microcontroller.
 *
 * The old synth was two channels of bare square wave, which is why it went
 * beep. This one has the things that actually make a chiptune sound like
 * MUSIC rather than a doorbell:
 *
 *   - several voices at once, so a song can have a bassline UNDER a melody
 *   - ENVELOPES: a note swells and decays instead of switching on and off
 *   - real waveforms: pulse (three widths), triangle, sawtooth, noise
 *   - VIBRATO, so a held note wavers instead of sitting there
 *   - PITCH SLIDE, which is what turns noise into a drum kit
 *
 * A SONG is a set of TRACKS (melody, bass, drums...), one per voice, each a
 * list of notes. See the songs at the top of audio.c.
 *
 * HOW MANY VOICES depends on the machine (AUDIO_CHANS in config.h). The
 * desktop gets 8, the ESP32 gets 4. A song written for 6 tracks simply
 * loses its last tracks on the small machine -- so put the tracks that
 * matter FIRST (lead, bass, drums), and the sweetening after.
 * ==========================================================================*/
#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include "config.h"

/* ---- how the voices are divided up ---------------------------------------*/
#define SFX_CHANS   1                              /* one voice kept for sfx */
#define MUSIC_CHANS (AUDIO_CHANS - SFX_CHANS)

/* The synth's own clock. Envelopes and song rows are counted in these, NOT
 * in game frames -- 240Hz gives drums a sharp enough edge to sound hit
 * rather than faded in. */
#define ENGINE_HZ 240

/* ---- waveforms ------------------------------------------------------------*/
enum {
    W_PULSE12,   /* 12.5% duty: thin, reedy, nasal    */
    W_PULSE25,   /* 25%: the classic chiptune lead    */
    W_PULSE50,   /* 50% square: hollow, hard          */
    W_TRI,       /* triangle: soft. this is your bass */
    W_SAW,       /* sawtooth: buzzy, brassy, angry    */
    W_NOISE,     /* LFSR noise: drums, wind, screams  */
};

/* ---- an instrument --------------------------------------------------------
 * The shape of one voice. Times are in ENGINE_HZ ticks (240 = one second).
 */
typedef struct {
    uint8_t wave;
    uint8_t attack;     /* ticks from silence to full                     */
    uint8_t decay;      /* ticks from full down to `sustain`              */
    uint8_t sustain;    /* 0..255: the level a held note settles at       */
    uint8_t release;    /* ticks to fade out once the note ends           */
    uint8_t vol;        /* this instrument's own loudness, 0..255         */
    uint8_t vib_rate;   /* vibrato speed (0 = dead straight)              */
    uint8_t vib_depth;  /* vibrato depth                                  */
    int16_t slide;      /* pitch drift per tick. Large negative = a DRUM: *
                         * the pitch falls off a cliff and you hear a hit  */
} instr_t;

/* Add an instrument: an id here, a row in instruments[] in audio.c. */
enum {
    INS_BASS,      /* round triangle bass                        */
    INS_LEAD,      /* pulse lead with a slow waver               */
    INS_PAD,       /* soft, swelling, sits underneath everything */
    INS_ARP,       /* short plucks                               */
    INS_STAB,      /* saw. brass. it accuses you.                */
    INS_DRONE,     /* very long, very low, does not resolve      */
    INS_KICK,      /* pitch falls off a cliff = a thump          */
    INS_SNARE,     /* noise with a body                          */
    INS_HAT,       /* noise, gone almost before it arrives       */
    NUM_INSTR
};

/* ---- notes ---------------------------------------------------------------
 * REST = silence. Everything else is NOTE(name, octave): NOTE(N_A, 3) is
 * A below middle C. Octave 1-2 = bass, 4-5 = melody.
 */
enum { N_C, N_CS, N_D, N_DS, N_E, N_F, N_FS, N_G, N_GS, N_A, N_AS, N_B };
#define REST 0
#define NOTE(name, oct) (uint8_t)(1 + (oct) * 12 + (name))

/* One event on one track: play `note` with `instr` for `dur` ROWS. */
typedef struct {
    uint8_t note;
    uint8_t instr;
    uint8_t dur;
} ev_t;

typedef struct { const ev_t *ev; uint16_t n; } track_t;

typedef struct {
    const track_t *tracks;
    uint8_t ntracks;
    uint8_t rowticks;   /* engine ticks per row -- this is the TEMPO */
    uint8_t loop;
} song_t;

/* ---- sound effects --------------------------------------------------------
 * SFX are pitch SWEEPS, not tunes, so they stay a plain list of steps with
 * frequencies in Hz -- much easier to design a shotgun blast that way.
 */
typedef struct {
    uint16_t freq;   /* Hz. 0 = a rest.        */
    uint8_t  ticks;  /* engine ticks           */
    uint8_t  vol;    /* 0..255                 */
    uint8_t  wave;   /* W_*                    */
} sfx_step_t;

enum {
    SFX_BLIP, SFX_CONFIRM, SFX_HIT, SFX_HURT, SFX_STING, SFX_VICTORY,
    SFX_RUN, SFX_LEVELUP, SFX_SHOTGUN, SFX_PICKUP, SFX_HEAL,
    NUM_SFX
};

enum {
    MUSIC_NONE,
    MUSIC_INTRO,      /* the heartbeat under the cinematic          */
    MUSIC_TITLE,      /* the title: slow, hollow, wrong             */
    MUSIC_FARM,       /* the overworld. lonely, and it has a TUNE.  */
    MUSIC_TOWN,       /* town: the same world, a little less alone  */
    MUSIC_BATTLE,     /* a fight. driving, urgent, nasty.           */
    MUSIC_BOSS,       /* a fight you are probably going to lose.    */
    MUSIC_DRIVE,      /* the highway at night                       */
    MUSIC_PROLOGUE,   /* END OF PROLOGUE                            */
    NUM_MUSIC
};

void audio_init(void);
void audio_sfx(int id);
void audio_music(int id);   /* MUSIC_NONE to stop */

/* Master volumes, 0..255 each. The music and the effects are separate
 * because people want them separate: the score can be too loud while the
 * shotgun is exactly right. Set from the OPTIONS menu (see game.c). */
void audio_volume(int music, int sfx);

#endif /* AUDIO_H */
