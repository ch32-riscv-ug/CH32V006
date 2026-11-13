#ifndef WVUI_LABEL_EX_H
#define WVUI_LABEL_EX_H

/**
 * @file wvui_label_ex.h
 * @brief  Word-wrapping label: breaks text into lines to fit the widget width, then draws.
 * @note   Decorative widget: no event callback, does not intercept gestures, events go to the widget below.
 */

#include "wvui_font.h"
#include "wvui_widget.h"

/** Max number of lines; excess text is truncated, no font scaling, no ellipsis. */
#ifndef WVUI_CFG_LABEL_EX_LINES
#define WVUI_CFG_LABEL_EX_LINES 6
#endif

typedef struct
{
    const char *text;                            /**< Text pointer, not copied */
    const wvui_font_t *font;                      /**< Font; NULL = do not draw */
    wvui_color_t color;                           /**< Text color, default light gray */
    uint16_t line_off[WVUI_CFG_LABEL_EX_LINES];   /**< Byte offset of each line start */
    uint16_t line_len[WVUI_CFG_LABEL_EX_LINES];   /**< Byte length of each line */
    uint8_t lines;                               /**< Lines actually used, <= limit */
    uint8_t align;                               /**< WVUI_LABEL_ALIGN_* */
    uint8_t opa;                                 /**< Opacity 0..255 */
    uint8_t truncated;                           /**< 1 = truncated by the line limit */
} wvui_label_ex_rt_t;

extern const wvui_class_t wvui_label_ex_class;

/**
 * @brief Set the text and re-wrap.
 * @param text  NUL-terminated string; NULL means no drawing
 * @note  Stores the pointer only, no copy; the buffer lifetime is the caller's duty.
 * @note  An unchanged pointer skips the re-wrap; call wvui_label_ex_refresh()
 *        manually after rewriting the same buffer.
 */
void wvui_label_ex_set_text(wvui_widget_t *w, const char *text);

/**
 * @brief Set the font and re-wrap.
 * @param font  Font; NULL = do not draw
 * @note  NULL by default; a font must be set explicitly, otherwise no line can be broken.
 */
void wvui_label_ex_set_font(wvui_widget_t *w, const wvui_font_t *font);

/**
 * @brief Set the text color.
 * @param color  RGB565 color, default light gray
 * @note  Does not touch the wrapping, marks the widget dirty only.
 */
void wvui_label_ex_set_color(wvui_widget_t *w, wvui_color_t color);

/**
 * @brief Set the horizontal alignment.
 * @param align  WVUI_LABEL_ALIGN_LEFT/CENTER/RIGHT, default LEFT
 * @note  Each line aligns by its own width; multi-line centering is per line.
 */
void wvui_label_ex_set_align(wvui_widget_t *w, uint8_t align);

/**
 * @brief Set the overall opacity.
 * @param opa  0..255, default 255; 0 = do not draw
 */
void wvui_label_ex_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Re-wrap the text to the current widget width.
 * @note  set_text / set_font already call it; call manually only after changing w->w.
 * @note  The line width limit is w->w - 2, leaving 1 px on each side.
 */
void wvui_label_ex_refresh(wvui_widget_t *w);

/**
 * @brief Read back the current text pointer.
 * @return The stored pointer, not a copy; an empty string on type mismatch
 */
const char *wvui_label_ex_text(const wvui_widget_t *w);

/**
 * @brief Read back the actual line count.
 * @return Line count 0..WVUI_CFG_LABEL_EX_LINES; 0 without font or text
 */
uint8_t wvui_label_ex_line_count(const wvui_widget_t *w);

/**
 * @brief Read back the truncation flag.
 * @return 1 = remaining text dropped by hitting the line limit
 */
uint8_t wvui_label_ex_truncated(const wvui_widget_t *w);

/**
 * @brief Get the pixel width of one line.
 * @param index  Line index, starting from 0
 * @return Width in pixels; 0 if out of range or not wrapped
 */
int16_t wvui_label_ex_line_width(const wvui_widget_t *w, uint8_t index);

#endif /* WVUI_LABEL_EX_H */
