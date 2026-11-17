#ifndef WVUI_SLIDER_H
#define WVUI_SLIDER_H

/**
 * @file wvui_slider.h
 * @brief Slider: a continuous 0..255 value; pressed at once jumps to the press point.
 * @note  Does not take over DRAG_BEGIN; when nested in a scrollable panel the drag is taken by the panel.
 */

#include "wvui_widget.h"

/**
 * @brief Value change callback.
 * @param value  New value 0..255
 * @param user   User pointer given at registration
 */
typedef void (*wvui_slider_change_cb_t)(wvui_widget_t *w, uint8_t value, void *user);

typedef struct
{
    wvui_color_t track_color; /**< Track color */
    wvui_color_t fill_color;  /**< Fill color: from the left end to the knob */
    wvui_color_t knob_color;  /**< Knob color */
    uint8_t value;           /**< Current value 0..255; 0 hugs the left end, 255 the right end */
    uint8_t radius;          /**< Track corner radius in pixels; 0 = capsule (height / 2) */
    uint8_t opa;             /**< Widget opacity 0..255; 0 = not drawn */
    uint8_t enabled;         /**< 1 = interactive; 0 = disabled */
    uint8_t pressed;         /**< Pressed flag (maintained by the input layer) */
    wvui_slider_change_cb_t on_change; /**< Value change callback; 0 = none */
    void *user;              /**< User pointer carried to the callback */
} wvui_slider_rt_t;

extern const wvui_class_t wvui_slider_class;

/**
 * @brief Set the value.
 * @param value  Target value 0..255
 * @note  No repaint and no callback when the value is unchanged.
 */
void wvui_slider_set_value(wvui_widget_t *w, uint8_t value);

/**
 * @brief Set the track, fill and knob colors.
 * @param track  Track color
 * @param fill   Fill color, drawn up to the knob
 * @param knob   Knob color
 */
void wvui_slider_set_colors(wvui_widget_t *w, wvui_color_t track, wvui_color_t fill,
                           wvui_color_t knob);

/**
 * @brief Set the track corner radius.
 * @param radius  Corner radius in pixels; 0 = capsule (height / 2)
 * @note  Clamped to the knob radius (inner height / 2) when drawn; anything larger is drawn as a capsule.
 * @note  The knob diameter equals the widget height, the range = widget width − diameter.
 */
void wvui_slider_set_radius(wvui_widget_t *w, uint8_t radius);

/**
 * @brief Set the opacity.
 * @param opa  Widget opacity 0..255; 0 = not drawn
 */
void wvui_slider_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Enable or disable interaction.
 * @param enabled  1 = interactive, anything else = disabled
 * @note  While disabled events return 0 and gestures are not intercepted.
 */
void wvui_slider_set_enabled(wvui_widget_t *w, uint8_t enabled);

/**
 * @brief Register the value change callback.
 * @param cb    Callback; 0 = unregister
 * @param user  User pointer carried to the callback
 * @note  Called on every change while dragging; programmatic set_value() calls it too.
 */
void wvui_slider_set_change_cb(wvui_widget_t *w, wvui_slider_change_cb_t cb, void *user);

/**
 * @brief Read back the current value.
 * @return Current value 0..255; returns 0 on a type mismatch
 */
uint8_t wvui_slider_value(const wvui_widget_t *w);

/**
 * @brief Read back the pressed state.
 * @return 1 = currently held down; 0 = not held or type mismatch
 */
uint8_t wvui_slider_is_pressed(const wvui_widget_t *w);

#endif /* WVUI_SLIDER_H */
