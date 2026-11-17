#ifndef WVUI_LIST_H
#define WVUI_LIST_H

/**
 * @file wvui_list.h
 * @brief  Text list: rows draw themselves, long lists still cost the RAM of a single widget.
 * @note   Dragging beyond WVUI_CFG_DRAG_THRESHOLD scrolls; below it counts as a selection click.
 */

#include "wvui_anim.h"
#include "wvui_font.h"
#include "wvui_scroll.h"
#include "wvui_widget.h"

/** Selection change callback: index is the newly selected item index. */
typedef void (*wvui_list_select_cb_t)(wvui_widget_t *w, uint16_t index, void *user);

typedef struct
{
    wvui_anim_t an;                   /**< Per-frame driver for inertia and bounce-back */
    wvui_scroll_t sc;                 /**< Scroll physics, embedded */
    const char *const *items;        /**< String table, owned by the caller */
    const wvui_font_t *font;          /**< Font; NULL = no text drawn */
    wvui_color_t bg;                  /**< Selected row background, default blue */
    wvui_color_t fg;                  /**< Unselected row text color */
    wvui_color_t sel_fg;              /**< Selected row text color */
    uint16_t count;                  /**< Item count, default 0 */
    uint16_t selected;               /**< Selected item index, default 0 */
    int16_t row_h;                   /**< Row height in pixels, default 20 */
    int16_t press_y;                 /**< Press point y, used for the selection hit test */
    uint8_t pressed;                 /**< Pressed state 0/1 */
    uint8_t moved;                   /**< Drag threshold exceeded */
    uint8_t opa;                     /**< Opacity 0..255, default 255 */
    uint8_t enabled;                 /**< 1 = interactive, default 1 */
    wvui_list_select_cb_t on_select;  /**< Selection change callback */
    void *user;                      /**< User pointer passed to the callback */
} wvui_list_rt_t;

extern const wvui_class_t wvui_list_class;

/**
 * @brief Bind a string table, no copy.
 * @param items  Array of NUL-terminated strings; owned by the caller
 * @param count  Item count; 0, or items = NULL, unbinds
 * @note  After binding the selection is clamped into range and the scroll
 *        position returns to the top.
 */
void wvui_list_bind(wvui_widget_t *w, const char *const *items, uint16_t count);

/**
 * @brief Set the row text font.
 * @param font  Font; NULL = draws only the selection background, no text
 * @note  NULL by default; the font must be set explicitly.
 */
void wvui_list_set_font(wvui_widget_t *w, const wvui_font_t *font);

/**
 * @brief Set the row height.
 * @param row_h  Row height in pixels, default 20; < 1 clamps to 1
 * @note  Changing it recomputes the scroll range and marks the widget dirty.
 */
void wvui_list_set_row_height(wvui_widget_t *w, int16_t row_h);

/**
 * @brief Select item index.
 * @param index  Item index; beyond the last item it clamps to count-1
 *               (no clamping when count is 0)
 * @note  Returns early when the selection is unchanged: no scroll, no callback.
 * @note  Scrolls the selected row into view first, then fires on_select.
 */
void wvui_list_set_selected(wvui_widget_t *w, uint16_t index);

/**
 * @brief Set the selection background and both text colors.
 * @param bg      Selected row background color, default blue
 * @param fg      Unselected row text color, default light gray
 * @param sel_fg  Selected row text color, default white
 */
void wvui_list_set_colors(wvui_widget_t *w, wvui_color_t bg, wvui_color_t fg, wvui_color_t sel_fg);

/**
 * @brief Set the overall opacity.
 * @param opa  0..255, default 255; 0 = do not draw
 */
void wvui_list_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Set the interactivity.
 * @param enabled  1 = clickable and draggable (default), 0 = read-only
 * @note  Disabled, events return 0, gestures are not intercepted and keep
 *        going to the widgets below.
 */
void wvui_list_set_enabled(wvui_widget_t *w, uint8_t enabled);

/**
 * @brief Set the selection change callback.
 * @param cb    Callback; NULL = no callback
 * @param user  Pointer passed to the callback as is
 */
void wvui_list_set_select_cb(wvui_widget_t *w, wvui_list_select_cb_t cb, void *user);

/**
 * @brief Recompute the scroll range and mark the widget dirty.
 * @note  bind and set_row_height call it; call manually once only after
 *        changing w->h or w->w.
 */
void wvui_list_refresh(wvui_widget_t *w);

/**
 * @brief Read back the item count.
 * @return  The currently bound item count; 0 when unbound or on type mismatch
 */
uint16_t wvui_list_count(const wvui_widget_t *w);

/**
 * @brief Read back the selected item index.
 * @return  Index, starting from 0; 0 when unbound
 */
uint16_t wvui_list_selected(const wvui_widget_t *w);

/**
 * @brief Read back the scroll position.
 * @return  Pixels scrolled so far, 0 = top
 */
int32_t wvui_list_scroll(const wvui_widget_t *w);

/**
 * @brief Get the string of item index.
 * @param index  Item index, starting from 0
 * @return  The pointer from the caller's string table; NULL if out of range
 *          or unbound
 */
const char *wvui_list_item(const wvui_widget_t *w, uint16_t index);

#endif /* WVUI_LIST_H */
