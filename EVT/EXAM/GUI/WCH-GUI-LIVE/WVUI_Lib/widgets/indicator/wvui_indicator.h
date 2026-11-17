#ifndef WVUI_INDICATOR_H
#define WVUI_INDICATOR_H

/**
 * @file wvui_indicator.h
 * @brief Status indicator dot: a single dot that can light up and go out.
 * @note  Read-only by default and does not intercept gestures; only after set_clickable(1) does it accept CLICKED.
 */

#include "wvui_anim.h"
#include "wvui_widget.h"

/** State change callback: on is the new state after the change (0/1). */
typedef void (*wvui_indicator_change_cb_t)(wvui_widget_t *w, uint8_t on, void *user);

typedef struct
{
    wvui_anim_t an;                        /**< Embedded blink node */
    wvui_color_t on_color;                 /**< On color, default green */
    wvui_color_t off_color;                /**< Off color, default dark gray */
    uint8_t on;                           /**< Logical state 0/1, default 0 */
    uint8_t blink_on;                     /**< 1 = blinking, default 0 */
    uint8_t blink_period;                 /**< Blink period in frames, default 30 */
    uint8_t blink_count;                  /**< Frames elapsed within the current period */
    uint8_t blink_visible;                /**< Whether the current phase is lit, default 1 */
    uint8_t radius;                       /**< Corner radius; 0 = a full round dot */
    uint8_t opa;                          /**< Opacity 0..255, default 255 */
    uint8_t clickable;                    /**< 1 = intercept gestures, default 0 */
    uint8_t pressed;                      /**< Press state 0/1 */
    wvui_indicator_change_cb_t on_change;  /**< State change callback */
    void *user;                           /**< User pointer passed to the callback */
} wvui_indicator_rt_t;

extern const wvui_class_t wvui_indicator_class;

/**
 * @brief Set the logical state.
 * @param on  0/1, any non-zero counts as 1; default 0
 * @note  Returns immediately when the state is unchanged: no dirty marking, no callback.
 * @note  on_change fires only when the state actually changes, and receives the new value.
 */
void wvui_indicator_set_on(wvui_widget_t *w, uint8_t on);

/**
 * @brief Flip the logical state.
 * @note  Equivalent to negating the current value and calling wvui_indicator_set_on().
 */
void wvui_indicator_flip(wvui_widget_t *w);

/**
 * @brief Set the blink switch and period.
 * @param blink         1 = start blinking, 0 = stop; default 0
 * @param period_frames Phase flip period in frames; < 2 is clamped to 2, default 30
 * @note  The blink node is permanent on the anim chain and returns 1 every frame; dirty is set only on phase flips.
 * @note  Turning blink off detaches the node; re-enabling restarts from frame 0 and resets the phase to lit.
 */
void wvui_indicator_set_blink(wvui_widget_t *w, uint8_t blink, uint8_t period_frames);

/**
 * @brief Set the on and off colors.
 * @param off  Off color, default dark gray
 * @param on   On color, default green
 * @note  Changes colors only, never the current on/off state.
 */
void wvui_indicator_set_colors(wvui_widget_t *w, wvui_color_t off, wvui_color_t on);

/**
 * @brief Set the corner radius.
 * @param radius  Radius in pixels; 0 = draw a round dot at min(w,h)/2, default 0
 */
void wvui_indicator_set_radius(wvui_widget_t *w, uint8_t radius);

/**
 * @brief Set the opacity.
 * @param opa  0..255, default 255; 0 = nothing is drawn
 */
void wvui_indicator_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Set whether gestures are intercepted.
 * @param clickable  1 = tappable, 0 = read-only, default 0
 * @note  When tappable it works with toggle discipline: only CLICKED is accepted, and gestures are not intercepted while disabled.
 * @note  One click flips the logical state and triggers on_change.
 */
void wvui_indicator_set_clickable(wvui_widget_t *w, uint8_t clickable);

/**
 * @brief Set the state change callback.
 * @param cb    Callback; NULL = no callback
 * @param user  Pointer passed through verbatim on each call
 * @note  Called only after the state truly flips; other widgets can be changed safely inside the callback.
 */
void wvui_indicator_set_change_cb(wvui_widget_t *w, wvui_indicator_change_cb_t cb, void *user);

/**
 * @brief Read back the logical state.
 * @return 1 = lit, 0 = out; returns 0 on a type mismatch
 */
uint8_t wvui_indicator_is_on(const wvui_widget_t *w);

/**
 * @brief Read back the press state.
 * @return 1 = currently held down; always 0 in read-only mode
 */
uint8_t wvui_indicator_is_pressed(const wvui_widget_t *w);

/**
 * @brief Check whether the current phase is lit.
 * @return 1 = the on color is drawn now; always equals on when not blinking, phase-dependent when blinking
 */
uint8_t wvui_indicator_visible(const wvui_widget_t *w);

#endif /* WVUI_INDICATOR_H */
