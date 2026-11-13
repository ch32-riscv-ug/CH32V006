#ifndef WVUI_ROLLER_H
#define WVUI_ROLLER_H

/**
 * @file wvui_roller.h
 * @brief Roller selector: snaps to a slot after dragging, the selected item lands in the middle band.
 * @note  Release only snaps, no inertia; inside a scrollable container the drag is taken over by the container.
 */

#include "wvui_anim.h"
#include "wvui_font.h"
#include "wvui_scroll.h"
#include "wvui_widget.h"

/**
 * @brief Selection-changed callback.
 * @param index  Selected index after the change
 * @param user   User pointer given at registration
 */
typedef void (*wvui_roller_select_cb_t)(wvui_widget_t *w, uint16_t index, void *user);

typedef struct
{
    wvui_anim_t an;            /**< Per-frame snap driver, embedded */
    wvui_scroll_t sc;          /**< Press / follow-the-touch / overscroll damping */
    const char *const *items; /**< Item table, owned by the caller (not copied) */
    const wvui_font_t *font;   /**< Font for drawing; 0 = draw no text */
    wvui_color_t band;         /**< Middle band highlight background */
    wvui_color_t fg;           /**< Text color of unselected rows */
    wvui_color_t sel_fg;       /**< Text color of the selected row */
    uint16_t count;           /**< Item count */
    uint16_t selected;        /**< Selected index, 0..count-1 */
    int16_t row_h;            /**< Row height in pixels; default 20, minimum 1 */
    uint8_t opa;              /**< Widget opacity 0..255; 0 = not drawn */
    uint8_t enabled;          /**< 1 = interactive; 0 = disabled */
    uint8_t pressed;          /**< Press flag (maintained by the input layer) */
    uint8_t moved;            /**< Press turned into a drag, not a tap-select */
    wvui_roller_select_cb_t on_select; /**< Selection-changed callback; 0 = none */
    void *user;               /**< User pointer carried into the callback */
} wvui_roller_rt_t;

extern const wvui_class_t wvui_roller_class;

/**
 * @brief Bind the item table, no copy.
 * @param items  Array of count strings
 * @param count  Item count; items == NULL is treated as 0, i.e. unbind
 * @note  A selected index beyond the new range is clamped to count-1 and snapped to immediately.
 * @note  items and every string in it must stay readable while the widget is alive.
 */
void wvui_roller_bind(wvui_widget_t *w, const char *const *items, uint16_t count);

/**
 * @brief Set the font for drawing.
 * @param font  Font; 0 = draw no text, keep only the middle band
 */
void wvui_roller_set_font(wvui_widget_t *w, const wvui_font_t *font);

/**
 * @brief Set the row height.
 * @param row_h  Row height in pixels; below 1 clamped to 1
 * @note  Slot pitch = row_h: item i corresponds to scroll position i·row_h.
 * @note  Changing the row height recomputes the scroll range and snaps to the current selection immediately.
 */
void wvui_roller_set_row_height(wvui_widget_t *w, int16_t row_h);

/**
 * @brief Select item index and snap to it.
 * @param index  Target index, clamped to 0..count-1
 * @note  Lands immediately, no glide; an unchanged index neither snaps nor calls back.
 * @note  on_select is called only if the index changed.
 */
void wvui_roller_set_selected(wvui_widget_t *w, uint16_t index);

/**
 * @brief Set the middle band background and the two text colors.
 * @param band    Middle band highlight background
 * @param fg      Text color of unselected rows
 * @param sel_fg  Text color of the selected row
 */
void wvui_roller_set_colors(wvui_widget_t *w, wvui_color_t band, wvui_color_t fg,
                           wvui_color_t sel_fg);

/**
 * @brief Set the opacity.
 * @param opa  Widget opacity 0..255; 0 = not drawn
 */
void wvui_roller_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Enable or disable interaction.
 * @param enabled  1 = interactive, 0 = disabled
 * @note  When disabled events return 0 and no gesture is intercepted; an outer scroll container can still start a drag.
 */
void wvui_roller_set_enabled(wvui_widget_t *w, uint8_t enabled);

/**
 * @brief Set the selection-changed callback.
 * @param cb    Callback; NULL = no callback
 * @param user  Pointer passed through unchanged to the callback
 */
void wvui_roller_set_select_cb(wvui_widget_t *w, wvui_roller_select_cb_t cb, void *user);

/**
 * @brief Recompute the scroll range and redraw.
 * @note  Range = [0, (count-1)·row_h], negatives treated as 0.
 * @note  bind() and set_row_height() already recompute internally, no need to call again.
 */
void wvui_roller_refresh(wvui_widget_t *w);

/**
 * @brief Read back the item count.
 * @return  Item count; 0 if unbound or the type does not match
 */
uint16_t wvui_roller_count(const wvui_widget_t *w);

/**
 * @brief Read back the selected index.
 * @return  Selected index 0..count-1; 0 if unbound or the type does not match
 */
uint16_t wvui_roller_selected(const wvui_widget_t *w);

/**
 * @brief Read back the scroll position.
 * @return  Scroll position in pixels; item i centered equals i·row_h, 0 if the type does not match
 */
int32_t wvui_roller_scroll(const wvui_widget_t *w);

/**
 * @brief Read back the string of item index.
 * @param index  Item index, 0..count-1
 * @return  Pointer to that string, not copied by the widget; NULL on out of range, unbound, a NULL table entry or type mismatch
 */
const char *wvui_roller_item(const wvui_widget_t *w, uint16_t index);

#endif /* WVUI_ROLLER_H */
