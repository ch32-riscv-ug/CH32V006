#ifndef WVUI_TOGGLE_H
#define WVUI_TOGGLE_H

/**
 * @file wvui_toggle.h
 * @brief Toggle: a track with a round knob, flipping reacts to CLICKED only.
 * @note  Events return 0 when disabled and gestures are not intercepted, so an outer scrolling container can still start a drag.
 */

#include "wvui_anim.h"
#include "wvui_widget.h"

/**
 * @brief State change callback.
 * @param on    New state 0/1 after the flip
 * @param user  User pointer given at registration
 */
typedef void (*wvui_toggle_change_cb_t)(wvui_widget_t *w, uint8_t on, void *user);

typedef struct
{
    wvui_anim_t an;            /**< Knob transition (embedded): approaches per frame, snaps when the gap <= 2 */
    wvui_color_t track_off;    /**< Track color when off */
    wvui_color_t track_on;     /**< Track color when on */
    wvui_color_t knob_color;   /**< Knob color */
    uint8_t on;               /**< Target state 0/1 */
    uint8_t pos;              /**< Knob position 0..255, Q8 ratio of the travel */
    uint8_t radius;           /**< Track corner radius in pixels; 0 = capsule (height / 2) */
    uint8_t opa;              /**< Widget opacity 0..255; 0 = not drawn */
    uint8_t enabled;          /**< 1 = interactive; 0 = disabled */
    uint8_t pressed;          /**< Pressed flag (maintained by the input layer) */
    wvui_toggle_change_cb_t on_change; /**< State change callback; 0 = none */
    void *user;               /**< User pointer carried by the callback */
} wvui_toggle_rt_t;

extern const wvui_class_t wvui_toggle_class;

/**
 * @brief Set the toggle state.
 * @param on  1 = on, anything else = off
 * @note  No knob transition and no callback when the state is unchanged.
 */
void wvui_toggle_set_on(wvui_widget_t *w, uint8_t on);

/**
 * @brief Flip the toggle state.
 * @note  Equivalent to calling set_on() with the inverted state; animation and callback rules follow it.
 */
void wvui_toggle_flip(wvui_widget_t *w);

/**
 * @brief Set the off track color, the on track color, and the knob color.
 * @param track_off  Track color when off
 * @param track_on   Track color when on
 * @param knob       Knob color
 */
void wvui_toggle_set_colors(wvui_widget_t *w, wvui_color_t track_off, wvui_color_t track_on,
                           wvui_color_t knob);

/**
 * @brief Set the track corner radius.
 * @param radius  Corner radius in pixels; 0 = capsule (height / 2)
 * @note  A radius above height / 2 is drawn as height / 2.
 */
void wvui_toggle_set_radius(wvui_widget_t *w, uint8_t radius);

/**
 * @brief Set the opacity.
 * @param opa  Widget opacity 0..255; 0 = not drawn
 */
void wvui_toggle_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Enable or disable interaction.
 * @param enabled  1 = interactive, anything else = disabled
 * @note  Events return 0 when disabled, no gestures intercepted.
 */
void wvui_toggle_set_enabled(wvui_widget_t *w, uint8_t enabled);

/**
 * @brief Register the state change callback.
 * @param cb    Callback; 0 = unregister
 * @param user  User pointer carried by the callback
 * @note  on is the new state 0/1; no call when the state is unchanged.
 */
void wvui_toggle_set_change_cb(wvui_widget_t *w, wvui_toggle_change_cb_t cb, void *user);

/**
 * @brief Read back the toggle state.
 * @return 1 = on, 0 = off; 0 on wrong type
 */
uint8_t wvui_toggle_is_on(const wvui_widget_t *w);

/**
 * @brief Read back the pressed state.
 * @return 1 = being held; 0 = not pressed or wrong type
 */
uint8_t wvui_toggle_is_pressed(const wvui_widget_t *w);

/**
 * @brief Read back the knob display position.
 * @return Q8 ratio of the travel 0..255: 0 = off position, 255 = on position, intermediate values mid-transition
 * @note   0 on wrong type.
 */
uint8_t wvui_toggle_knob_pos(const wvui_widget_t *w);

#endif /* WVUI_TOGGLE_H */
