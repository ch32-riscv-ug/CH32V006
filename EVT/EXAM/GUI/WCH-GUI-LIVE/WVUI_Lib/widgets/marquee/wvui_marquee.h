#ifndef WVUI_MARQUEE_H
#define WVUI_MARQUEE_H

/**
 * @file wvui_marquee.h
 * @brief  Marquee: scrolls while the text is wider than the widget, otherwise
 *         rests at the text start.
 * @note   Decorative widget: event is NULL and always returns 0, events go to
 *         the widget below.
 */

#include "wvui_anim.h"
#include "wvui_font.h"
#include "wvui_widget.h"

typedef struct
{
    wvui_anim_t an;           /**< Scroll node; attached to the active list only when the text does not fit */
    const char *text;        /**< Text pointer, not copied */
    const wvui_font_t *font;  /**< Font; NULL = do not draw */
    wvui_color_t fg;          /**< Text color, default light gray */
    int16_t text_w;          /**< Text width in pixels, cached */
    int16_t text_h;          /**< Text height in pixels, cached */
    int16_t offset;          /**< Current leftward shift in pixels */
    uint8_t speed;           /**< Pixels per frame, default 1; 0 = at rest */
    uint8_t gap;             /**< Loop gap in pixels, default 16 */
    uint8_t opa;             /**< Opacity 0..255, default 255 */
    uint8_t running;         /**< 1 = node attached to the active list */
} wvui_marquee_rt_t;

extern const wvui_class_t wvui_marquee_class;

/**
 * @brief Set the text and re-check the scroll condition.
 * @param text  NUL-terminated string; NULL means no drawing
 * @note  Stores the pointer only, no copy.
 * @note  An unchanged pointer skips re-measurement; call
 *        wvui_marquee_refresh() after rewriting the same buffer.
 */
void wvui_marquee_set_text(wvui_widget_t *w, const char *text);

/**
 * @brief Set the font and re-check the scroll condition.
 * @param font  Font; NULL = do not draw
 * @note  NULL by default; without a font the text width cannot be measured
 *        and nothing scrolls.
 */
void wvui_marquee_set_font(wvui_widget_t *w, const wvui_font_t *font);

/**
 * @brief Set the text color.
 * @param fg  RGB565 color, default light gray
 */
void wvui_marquee_set_color(wvui_widget_t *w, wvui_color_t fg);

/**
 * @brief Set the scroll speed.
 * @param px_per_frame  Pixels shifted left per frame, default 1; 0 = no scrolling
 * @note  A speed change re-checks the scroll condition, re-attaching the anim
 *        node to the active list or detaching it.
 * @note  The offset wraps to 0 only after covering text_w + gap; the second
 *        text copy fills the seam.
 */
void wvui_marquee_set_speed(wvui_widget_t *w, uint8_t px_per_frame);

/**
 * @brief Set the loop gap.
 * @param gap  Gap between the two text copies in pixels, default 16
 * @note  Changes the gap and marks the widget dirty only; the scroll
 *        condition is not re-checked.
 */
void wvui_marquee_set_gap(wvui_widget_t *w, uint8_t gap);

/**
 * @brief Set the opacity.
 * @param opa  Widget opacity 0..255, default 255; 0 = do not draw
 */
void wvui_marquee_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Re-measure the text size and re-check the scroll condition.
 * @note  Attaches no anim node when the text width <= widget width; rests at
 *        the text start.
 * @note  set_* functions call it internally; call manually once after
 *        changing w->w.
 */
void wvui_marquee_refresh(wvui_widget_t *w);

/**
 * @brief Read back the current text pointer.
 * @return The stored pointer, not a copy; an empty string on type mismatch
 */
const char *wvui_marquee_text(const wvui_widget_t *w);

/**
 * @brief Read back the cached text width.
 * @return Width in pixels; 0 without font or text
 */
int16_t wvui_marquee_text_width(const wvui_widget_t *w);

/**
 * @brief Read back the current leftward shift.
 * @return Offset in pixels, less than text_w + gap; always 0 at rest
 */
int16_t wvui_marquee_offset(const wvui_widget_t *w);

/**
 * @brief Read back whether it is scrolling.
 * @return 1 = anim node attached to the active list; 0 = at rest
 */
uint8_t wvui_marquee_is_running(const wvui_widget_t *w);

#endif /* WVUI_MARQUEE_H */
