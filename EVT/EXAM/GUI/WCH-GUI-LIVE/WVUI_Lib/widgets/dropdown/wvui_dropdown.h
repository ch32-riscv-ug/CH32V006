#ifndef WVUI_DROPDOWN_H
#define WVUI_DROPDOWN_H

/**
 * @file wvui_dropdown.h
 * @brief Dropdown selection: closed state draws the widget itself, open state is a modal full-screen-hit popup on the top layer.
 * @note  The object must be allocated in persistent memory, the popup lives across pages.
 */

#include "wvui_font.h"
#include "wvui_widget.h"

/** Selection change callback: `index` is the new selected item index. */
typedef void (*wvui_dropdown_cb_t)(wvui_widget_t *w, uint16_t index, void *user);

typedef struct
{
    const char *const *items;    /**< String table, owned by the caller (not copied) */
    uint16_t count;              /**< Item count */
    uint16_t selected;           /**< Selected item index */
    const wvui_font_t *font;      /**< Font; NULL = no text drawn */
    wvui_color_t bg;              /**< Closed-state background color */
    wvui_color_t fg;              /**< Closed-state text color */
    wvui_color_t border;          /**< Border color */
    wvui_color_t sel_bg;          /**< Selected row background color */
    wvui_color_t sel_fg;          /**< Selected row text color */
    uint8_t row_h;               /**< Popup row height in pixels; below 4 treated as 4 */
    uint8_t max_visible;         /**< Max rows shown in the popup; 0 treated as 1 */
    uint8_t open;                /**< 1 = expanded */
    uint8_t pressed;             /**< 0 = none, 1 = closed-state press, 2 = popup row press */
    uint8_t pressed_row;         /**< Index of the pressed row */
    uint8_t radius;              /**< Corner radius in pixels; 0 = square */
    uint8_t opa;                 /**< Widget opacity 0..255; 0 = not drawn */
    uint8_t enabled;             /**< 0 = grayed out and unresponsive */
    wvui_dropdown_cb_t on_select; /**< Selection change callback; NULL = none */
    void *user;                  /**< User pointer for the callback, passed back as-is */
} wvui_dropdown_rt_t;

extern const wvui_class_t wvui_dropdown_class;

/**
 * @brief Bind a string table without copying it.
 * @param items  String array; the caller guarantees the lifetime
 * @param count  Item count; 0 or NULL items unbinds
 * @note  When the selected index goes out of range after unbinding or shrinking it is clamped to count-1; count 0 auto-collapses.
 */
void wvui_dropdown_bind(wvui_widget_t *w, const char *const *items, uint16_t count);

/**
 * @brief Expand the popup, idempotent.
 * @note  No action when count is 0 or when disabled; expanding joins the top-layer chain and opens a modal.
 * @note  The popup opens downward first, upward when there is no room below; clicks outside the widget rect are sent back to this widget as CLICKED.
 */
void wvui_dropdown_open(wvui_widget_t *w);

/**
 * @brief Collapse the popup, idempotent.
 * @note  After collapsing the widget rejoins the root chain: while off the chain it can neither be drawn nor hit.
 */
void wvui_dropdown_close(wvui_widget_t *w);

/**
 * @brief Expand or collapse according to the current state.
 */
void wvui_dropdown_toggle(wvui_widget_t *w);

/**
 * @brief Set the selected item.
 * @param index  Selected index; clamped to count-1 when beyond it
 * @note  Marks dirty and calls back only when the index changed; a programmatic change also calls on_select.
 * @note  No clamping when count is 0.
 */
void wvui_dropdown_set_selected(wvui_widget_t *w, uint16_t index);

/**
 * @brief Set the font.
 * @param font  Font; NULL = no text drawn
 * @note  The framework has no default font, one must be set explicitly.
 */
void wvui_dropdown_set_font(wvui_widget_t *w, const wvui_font_t *font);

/**
 * @brief Set the popup row height.
 * @param row_h  Row height in pixels; below 4 treated as 4
 */
void wvui_dropdown_set_row_height(wvui_widget_t *w, uint8_t row_h);

/**
 * @brief Set the max rows shown in the popup.
 * @param rows  Upper bound of rows; 0 treated as 1
 * @note  Visible rows = min(count, rows), the rest is truncated, the popup does not scroll.
 */
void wvui_dropdown_set_max_visible(wvui_widget_t *w, uint8_t rows);

/**
 * @brief Set the three closed-state colors.
 * @param bg      Background color
 * @param fg      Text color
 * @param border  Border color
 */
void wvui_dropdown_set_colors(wvui_widget_t *w, wvui_color_t bg, wvui_color_t fg,
                             wvui_color_t border);

/**
 * @brief Set the two selected-row colors.
 * @param sel_bg  Selected row background color
 * @param sel_fg  Selected row text color
 */
void wvui_dropdown_set_sel_colors(wvui_widget_t *w, wvui_color_t sel_bg, wvui_color_t sel_fg);

/**
 * @brief Set the corner radius.
 * @param radius  Corner radius in pixels; 0 = square
 */
void wvui_dropdown_set_radius(wvui_widget_t *w, uint8_t radius);

/**
 * @brief Set the opacity.
 * @param opa  Widget opacity 0..255; 0 = not drawn
 */
void wvui_dropdown_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Set the disabled state.
 * @param enabled  1 = normal (default), 0 = grayed out and unresponsive
 * @note  An expanded popup auto-collapses when disabled; open() does nothing while disabled.
 */
void wvui_dropdown_set_enabled(wvui_widget_t *w, uint8_t enabled);

/**
 * @brief Set the selection change callback.
 * @param cb    Callback; NULL = no callback
 * @param user  Pointer passed through to the callback as-is
 */
void wvui_dropdown_set_select_cb(wvui_widget_t *w, wvui_dropdown_cb_t cb, void *user);

/**
 * @brief Read back the item count.
 * @return  Item count; returns 0 when unbound or on wrong type
 */
uint16_t wvui_dropdown_count(const wvui_widget_t *w);

/**
 * @brief Read back the selected item index.
 * @return  Selected item index; returns 0 on wrong type
 */
uint16_t wvui_dropdown_selected(const wvui_widget_t *w);

/**
 * @brief Read back the popup expand state.
 * @return  1 = expanded, 0 = collapsed
 */
uint8_t wvui_dropdown_is_open(const wvui_widget_t *w);

/**
 * @brief Read back one item text.
 * @param index  Item index
 * @return  String pointer, not copied; NULL when out of range or unbound
 */
const char *wvui_dropdown_item(const wvui_widget_t *w, uint16_t index);

#endif /* WVUI_DROPDOWN_H */
