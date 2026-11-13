#ifndef WVUI_CHECKBOX_H
#define WVUI_CHECKBOX_H

/**
 * @file wvui_checkbox.h
 * @brief Checkbox: box with a stroke-drawn check mark, animated toggle with callback.
 * @note  Only accepts CLICKED; events return 0 when disabled, does not intercept gestures.
 */

#include "wvui_anim.h"
#include "wvui_widget.h"

/**
 * @brief Callback for the checked-state change.
 * @param on    New state 0/1 after the toggle
 * @param user  User pointer given at registration
 */
typedef void (*wvui_checkbox_change_cb_t)(wvui_widget_t *w, uint8_t on, void *user);

typedef struct
{
    wvui_anim_t an;                        /**< Check mark animation node, embedded in the widget runtime */
    wvui_color_t box_off;                  /**< Box fill when unchecked */
    wvui_color_t box_on;                   /**< Box fill when checked */
    wvui_color_t mark_color;               /**< Check mark color */
    wvui_color_t border_color;             /**< Border color */
    uint8_t on;                           /**< Target state 0/1 */
    uint8_t progress;                     /**< Check mark stroke progress: 0..255 (displayed value) */
    uint8_t radius;                       /**< Box corner radius in pixels; 0 = square */
    uint8_t opa;                          /**< Widget opacity 0..255; 0 = not drawn */
    uint8_t enabled;                      /**< 0 = grayed out and unresponsive */
    uint8_t pressed;                      /**< Pressed state (visual) */
    wvui_checkbox_change_cb_t on_change;   /**< State change callback; NULL = none */
    void *user;                           /**< User pointer for the callback, passed back as-is */
} wvui_checkbox_rt_t;

extern const wvui_class_t wvui_checkbox_class;

/**
 * @brief Set the target state and start the check mark animation.
 * @param on  Target state: 0 = unchecked, 1 = checked (any non-zero counts as 1)
 * @note  No animation and no callback when the state is unchanged; on_change is called only on change.
 */
void wvui_checkbox_set_on(wvui_widget_t *w, uint8_t on);

/**
 * @brief Toggle the target state.
 * @note  Equivalent to calling set_on() with the inverted state; the animation and callback rules are the same.
 */
void wvui_checkbox_flip(wvui_widget_t *w);

/**
 * @brief Set all four colors at once.
 * @param box_off  Box fill when unchecked
 * @param box_on   Box fill when checked
 * @param mark     Check mark color
 * @param border   Border color
 */
void wvui_checkbox_set_colors(wvui_widget_t *w, wvui_color_t box_off, wvui_color_t box_on,
                             wvui_color_t mark, wvui_color_t border);

/**
 * @brief Set the box corner radius.
 * @param radius  Corner radius in pixels; 0 = square
 */
void wvui_checkbox_set_radius(wvui_widget_t *w, uint8_t radius);

/**
 * @brief Set the opacity.
 * @param opa  Widget opacity 0..255; 0 = not drawn
 */
void wvui_checkbox_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Set the disabled state.
 * @param enabled  1 = normal (default), 0 = grayed out and unresponsive
 * @note  Events return 0 when disabled, does not intercept gestures.
 */
void wvui_checkbox_set_enabled(wvui_widget_t *w, uint8_t enabled);

/**
 * @brief Set the state change callback.
 * @param cb    Callback; NULL = remove it
 * @param user  User pointer passed back to the callback as-is
 */
void wvui_checkbox_set_change_cb(wvui_widget_t *w, wvui_checkbox_change_cb_t cb, void *user);

/**
 * @brief Read back the target state.
 * @return 0 = unchecked, 1 = checked
 */
uint8_t wvui_checkbox_is_on(const wvui_widget_t *w);

/**
 * @brief Read back the pressed state.
 * @return 1 = being pressed, 0 = not pressed
 */
uint8_t wvui_checkbox_is_pressed(const wvui_widget_t *w);

/**
 * @brief Read back the check mark stroke progress.
 * @return Progress 0..255; up to 128 draws the short edge, beyond 128 draws the long edge
 * @note   Animation intermediate value, read-only.
 */
uint8_t wvui_checkbox_progress(const wvui_widget_t *w);

#endif /* WVUI_CHECKBOX_H */
