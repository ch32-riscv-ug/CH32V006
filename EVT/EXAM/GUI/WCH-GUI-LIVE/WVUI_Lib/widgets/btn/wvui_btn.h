#ifndef WVUI_BTN_H
#define WVUI_BTN_H

/**
 * @file wvui_btn.h
 * @brief Button widget: press visuals, click callback and disabled state.
 * @note  Only CLICKED triggers the callback; when disabled events return 0 and are left to the outer container.
 */

#include "wvui_widget.h"
#include "wvui_font.h"

/** Click callback: fired on release over the widget after a press. */
typedef void (*wvui_btn_click_cb_t)(wvui_widget_t *w, void *user);

typedef struct
{
    const char *text;             /**< Text; NULL = not drawn, only the pointer is stored */
    const wvui_font_t *font;       /**< Font; NULL = no text drawn */
    wvui_color_t bg;               /**< Normal-state base color */
    wvui_color_t bg_pressed;       /**< Pressed-state base color */
    wvui_color_t fg;               /**< Text color */
    wvui_color_t border;           /**< Border color; same as bg = no border */
    uint8_t opa;                  /**< Widget opacity 0..255; 0 = not drawn */
    uint8_t pressed;              /**< Pressed state (visual) */
    uint8_t enabled;              /**< 0 = grayed out and unresponsive */
    uint8_t pad;                  /**< Text padding, px; default 2 */
    uint8_t radius;               /**< Corner radius, px; 0 = square corners (default) */
    wvui_btn_click_cb_t on_click;  /**< Click callback; NULL = none */
    void *user;                   /**< Callback user pointer, passed back unchanged */
} wvui_btn_rt_t;

extern const wvui_class_t wvui_btn_class;

/**
 * @brief Set the button text.
 * @param text  String; NULL = no text drawn
 * @note  Only the pointer is stored without copying; the string's lifetime is guaranteed by the caller.
 * @note  An equal pointer does not mean equal content, so this function marks the widget dirty unconditionally.
 */
void wvui_btn_set_text(wvui_widget_t *w, const char *text);

/**
 * @brief Set the text font.
 * @param font  Font; the framework has no default font, it must be given explicitly
 * @note  NULL = no text drawn.
 */
void wvui_btn_set_font(wvui_widget_t *w, const wvui_font_t *font);

/**
 * @brief Set the three colors at once.
 * @param bg          Normal-state base color
 * @param bg_pressed  Pressed-state base color
 * @param fg          Text color
 */
void wvui_btn_set_colors(wvui_widget_t *w, wvui_color_t bg, wvui_color_t bg_pressed, wvui_color_t fg);

/**
 * @brief Set the border color.
 * @param border  Border color; same as the base color = no border
 */
void wvui_btn_set_border(wvui_widget_t *w, wvui_color_t border);

/**
 * @brief Set the corner radius.
 * @param radius  Corner radius, px; 0 = square corners (default)
 * @note  Clamped to min(w,h)/2 when drawing; the property stores the original value, reads return the set value.
 * @note  The radius changes only the shape; text stays centered on the whole rect and padding is unaffected.
 */
void wvui_btn_set_radius(wvui_widget_t *w, uint8_t radius);

/**
 * @brief Get the corner radius.
 * @return The set value (not clamped to the rect); 0 on type mismatch
 */
uint8_t wvui_btn_radius(const wvui_widget_t *w);

/**
 * @brief Set the enabled state.
 * @param enabled  1 = normal (default), 0 = grayed out and unresponsive
 * @note  While disabled PRESSED returns 0 and events are left to the outer container.
 */
void wvui_btn_set_enabled(wvui_widget_t *w, uint8_t enabled);

/**
 * @brief Set the click callback.
 * @param cb    Callback; NULL = detach
 * @param user  User pointer, passed back unchanged in the callback
 * @note  The callback must not delete itself or ancestors; deletion must be deferred to a later frame.
 * @note  When a container takes over the gesture at DRAG_BEGIN the button does not receive CLICKED.
 */
void wvui_btn_set_click_cb(wvui_widget_t *w, wvui_btn_click_cb_t cb, void *user);

/**
 * @brief Get the pressed state.
 * @return 1 = currently pressed, 0 = not pressed
 */
uint8_t wvui_btn_is_pressed(const wvui_widget_t *w);

#endif /* WVUI_BTN_H */
