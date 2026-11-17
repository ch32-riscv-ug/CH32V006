#ifndef WVUI_PROGRESS_H
#define WVUI_PROGRESS_H

/**
 * @file wvui_progress.h
 * @brief Progress bar: a full track plus a rounded fill that advances with the value.
 * @note  The fill is clipped to the track rect, its right end moves with the value while the left end keeps its corners; decorative widget, does not intercept gestures.
 */

#include "wvui_anim.h"
#include "wvui_widget.h"

typedef struct
{
    wvui_anim_t an;           /**< Value transition node (embedded) */
    wvui_color_t track_color; /**< Track color */
    wvui_color_t fill_color;  /**< Fill color */
    uint8_t value;           /**< Target value 0..255 */
    uint8_t display;         /**< Current display value 0..255, intermediate during a transition */
    uint8_t radius;          /**< Corner radius in pixels; 0 = square */
    uint8_t opa;             /**< Widget opacity 0..255; 0 = not drawn */
} wvui_progress_rt_t;

extern const wvui_class_t wvui_progress_class;

/**
 * @brief Set the target value; starts a transition if it differs from the current display value.
 * @param value  Target value 0..255
 * @note   Each animation frame moves the display value toward the target by at most WVUI_CFG_PROGRESS_STEP,
 *         on arrival the callback returns 0 and the engine removes the node automatically.
 * @note   No value change, no move; while the node is on the chain only the callback is refreshed.
 */
void wvui_progress_set_value(wvui_widget_t *w, uint8_t value);

/**
 * @brief Add delta to the current target value, saturating at 255.
 * @param delta  Increment 0..255
 * @note   Past 255 it clamps to 255 and starts a transition per set_value() rules.
 */
void wvui_progress_add(wvui_widget_t *w, uint8_t delta);

/**
 * @brief Subtract delta from the current target value, saturating at 0.
 * @param delta  Decrement 0..255
 * @note   Below 0 it stops at 0.
 */
void wvui_progress_sub(wvui_widget_t *w, uint8_t delta);

/**
 * @brief Set the track and fill colors.
 * @param track  Track color
 * @param fill   Fill color
 */
void wvui_progress_set_colors(wvui_widget_t *w, wvui_color_t track, wvui_color_t fill);

/**
 * @brief Set the corner radius.
 * @param radius  Corner radius in pixels; 0 = square
 * @note   Clamped to min(w, h) / 2 at draw time; beyond that it becomes a capsule.
 */
void wvui_progress_set_radius(wvui_widget_t *w, uint8_t radius);

/**
 * @brief Set the opacity.
 * @param opa  Widget opacity 0..255; 0 = not drawn
 */
void wvui_progress_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Read back the target value.
 * @return        Target value 0..255; 0 if the widget type does not match
 */
uint8_t wvui_progress_value(const wvui_widget_t *w);

/**
 * @brief Read back the current display value.
 * @return        Current display value 0..255, intermediate during a transition; 0 if the type does not match
 */
uint8_t wvui_progress_display(const wvui_widget_t *w);

#endif /* WVUI_PROGRESS_H */
