/* ============================================================================
 * audio.h -- 2-channel chiptune synth (square wave + noise).
 *
 *   channel 0 = music (looping sequence of notes)
 *   channel 1 = sound effects (one-shot, interrupts itself)
 *
 * A "sound" is just an array of steps: {frequency, duration, volume}.
 * Define new ones in audio.c (see the SFX TABLE) and play them from
 * anywhere in the game with audio_sfx(SFX_WHATEVER).
 * ==========================================================================*/
#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

/* One step of a sound. freq==0 means silence (a rest).
 * ticks are 1/60s, same clock as the game. */
typedef struct {
    uint16_t freq;   /* Hz; 0 = rest. Set the NOISE_BIT for white noise. */
    uint8_t  ticks;  /* duration, 1/60s units */
    uint8_t  vol;    /* 0..255 */
} sfx_step_t;

/* OR this into .freq to make the step play noise (explosions, static)
 * instead of a square wave. The low bits then set the noise pitch. */
#define NOISE_BIT 0x8000

/* ---- Sound effect ids. Add yours here AND in the table in audio.c. ------*/
enum {
    SFX_BLIP,       /* menu cursor                      */
    SFX_CONFIRM,    /* menu accept / talk               */
    SFX_HIT,        /* you strike the alien             */
    SFX_HURT,       /* the alien strikes you            */
    SFX_STING,      /* the intro "BAM" title sting      */
    SFX_VICTORY,    /* battle won jingle                */
    SFX_RUN,        /* fled from battle                 */
    SFX_LEVELUP,
    NUM_SFX
};

/* ---- Music ids. Add yours here AND in the table in audio.c. -------------*/
enum {
    MUSIC_NONE,     /* silence */
    MUSIC_INTRO,    /* slow heartbeat under the intro cinematic */
    MUSIC_TITLE,    /* eerie loop for the title screen */
    MUSIC_FARM,     /* low, wrong-feeling drone over the farm */
    NUM_MUSIC
};

void audio_init(void);
void audio_sfx(int id);
void audio_music(int id);   /* MUSIC_NONE to stop */

#endif /* AUDIO_H */
