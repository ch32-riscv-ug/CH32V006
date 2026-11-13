#ifndef WVUI_STEPPER_H
#define WVUI_STEPPER_H

/**
 * @file wvui_stepper.h
 * @brief Stepper: fixed-point int32 value with long-press repeat, decrement on the left half, increment on the right half.
 * @note  Value = value / 10^decimals, clamped to +/-9,999,999; the widget does not draw the number.
 */

#include "wvui_widget.h"

/**
 * @brief Value change callback.
 * @param value  New value (fixed point)
 * @param user   User pointer given at registration
 */
typedef void (*wvui_stepper_change_cb_t)(wvui_widget_t *w, int32_t value, void *user);

typedef struct
{
    int32_t value;    /**< Current value (fixed point) */
    int32_t min;      /**< Lower limit (fixed point); default -9,999,999 */
    int32_t max;      /**< Upper limit (fixed point); default +9,999,999 */
    int32_t step;     /**< Increment in the smallest unit; default 1, minimum 1 */
    char text[16];    /**< Format buffer, recomputed on value change, read-only while drawing */
    wvui_color_t bg;   /**< Background color */
    wvui_color_t fg;   /**< Foreground color, draws the + and - glyphs */
    wvui_color_t border; /**< Border color */
    uint8_t decimals; /**< Decimal places 0..4; default 0 */
    uint8_t repeat;   /**< Long-press repeat interval in frames; default 6, minimum 1 */
    uint8_t repeat_count; /**< Frames already counted for the current hold */
    uint8_t part;     /**< Pressed part: 0 none, 1 decrement, 2 increment */
    uint8_t opa;      /**< Widget opacity 0..255; 0 = not drawn */
    uint8_t enabled;  /**< 1 = interactive; 0 = disabled */
    wvui_stepper_change_cb_t on_change; /**< Value change callback; 0 = none */
    void *user;       /**< User pointer carried by the callback */
} wvui_stepper_rt_t;

extern const wvui_class_t wvui_stepper_class;

/**
 * @brief Set the range and decimal places.
 * @param min       Lower limit (fixed point)
 * @param max       Upper limit (fixed point)
 * @param decimals  Decimal places 0..4; above 4 clamps to 4
 * @note  Swaps automatically when min > max; the min lower bound and the max upper bound both clamp to +/-9,999,999.
 * @note  The current value is re-clamped to the new range and the formatted text is rebuilt along with it.
 */
void wvui_stepper_set_range(wvui_widget_t *w, int32_t min, int32_t max, uint8_t decimals);

/**
 * @brief Set the step applied on each increment / decrement.
 * @param step  Increment in the smallest unit; negative values use the absolute value, 0 clamps to 1
 * @note  Both the first press and the long-press repeat advance by this amount.
 */
void wvui_stepper_set_step(wvui_widget_t *w, int32_t step);

/**
 * @brief Set the value.
 * @param value  New value (fixed point), clamped to [min, max] first
 * @note  The same value does not rebuild the text, redraw, or call back.
 */
void wvui_stepper_set_value(wvui_widget_t *w, int32_t value);

/**
 * @brief Add delta to the current value.
 * @param delta  Amount to add (fixed point), may be negative
 * @note  Clamps to the range before adding, so int32 cannot overflow.
 */
void wvui_stepper_add(wvui_widget_t *w, int32_t delta);

/**
 * @brief Set the background, foreground, and border colors.
 * @param bg      Background color
 * @param fg      Foreground color, draws the + and - glyphs
 * @param border  Border color
 * @note  While disabled the glyphs turn gray regardless of fg.
 */
void wvui_stepper_set_colors(wvui_widget_t *w, wvui_color_t bg, wvui_color_t fg,
                            wvui_color_t border);

/**
 * @brief Set the long-press repeat interval.
 * @param frames  Interval in frames, >= 1; 0 clamps to 1
 * @note  Advances one step every frames while held; the first step fires right on press.
 * @note  The unit is frames, not milliseconds.
 */
void wvui_stepper_set_repeat(wvui_widget_t *w, uint8_t frames);

/**
 * @brief Set the opacity.
 * @param opa  Widget opacity 0..255; 0 = not drawn
 */
void wvui_stepper_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Enable or disable interaction.
 * @param enabled  1 = interactive, anything else = disabled
 * @note  While disabled the glyphs turn gray, events return 0, and gestures are not intercepted.
 */
void wvui_stepper_set_enabled(wvui_widget_t *w, uint8_t enabled);

/**
 * @brief Register the value change callback.
 * @param cb    Callback; 0 = unregister
 * @param user  User pointer carried by the callback
 * @note  Fires only when the value actually changed; no call when clamping leaves it in place.
 */
void wvui_stepper_set_change_cb(wvui_widget_t *w, wvui_stepper_change_cb_t cb, void *user);

/**
 * @brief Read back the current value.
 * @return Current value (fixed point); 0 on wrong type
 */
int32_t wvui_stepper_value(const wvui_widget_t *w);

/**
 * @brief Read back the decimal places.
 * @return Decimal places 0..4; 0 on wrong type
 */
uint8_t wvui_stepper_decimals(const wvui_widget_t *w);

/**
 * @brief Read back the formatted string.
 * @return Null-terminated string owned by the widget and read-only for the caller; empty string on wrong type
 * @note   Rebuilt once per value change; display is left to a label next to it.
 */
const char *wvui_stepper_text(const wvui_widget_t *w);

#endif /* WVUI_STEPPER_H */
