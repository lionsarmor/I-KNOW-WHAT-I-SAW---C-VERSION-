/* On-screen controls for Android. See touch.c.
 * The platform's whole job here: fingers -> BTN_* bits. */
#ifndef TOUCH_H
#define TOUCH_H

#include <SDL.h>
#include <stdint.h>

/* Call whenever the window size changes (and once at startup), AFTER the
 * renderer's logical size is set -- it asks SDL where the picture landed so
 * it can put the controls in the bars beside it. */
void touch_layout(SDL_Renderer *ren, int w, int h);

/* Feed it every SDL event; it only cares about SDL_FINGER*. */
void touch_event(const SDL_Event *e);

/* The buttons currently held, as BTN_* bits -- OR this into the keyboard
 * and gamepad reads, exactly like another controller. */
uint16_t touch_buttons(void);

/* Draw the controls. Call AFTER the game framebuffer is blitted and AFTER
 * the renderer's logical size has been reset, so it can paint into the
 * letterbox bars in real window pixels. */
void touch_draw(SDL_Renderer *ren);

#endif /* TOUCH_H */
