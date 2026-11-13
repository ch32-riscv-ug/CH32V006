#ifndef WVUI_LABEL_H
#define WVUI_LABEL_H

/**
 * @file wvui_label.h
 * @brief Text label: aligns a piece of text to the widget rectangle and draws it.
 * @note  Decorative widget: no event callback, no gesture interception, events pass to the layer below.
 */

#include "wvui_widget.h"
#include "wvui_font.h"

/** Horizontal alignment, values for wvui_label_set_align(). */
#define WVUI_LABEL_ALIGN_LEFT (0u)
#define WVUI_LABEL_ALIGN_CENTER (1u)
#define WVUI_LABEL_ALIGN_RIGHT (2u)

typedef struct
{
    const char *text;          /**< Text pointer, not copied, owned by the caller */
    const wvui_font_t *font;    /**< Font; NULL = drew nothing, default NULL */
    wvui_color_t color;         /**< Text color, default white */
    uint8_t opa;               /**< Opacity 0..255, default 255 */
    uint8_t align;             /**< WVUI_LABEL_ALIGN_*, default left */
    int8_t line_gap;           /**< Reserved field, unused by the current drawing */
} wvui_label_rt_t;

extern const wvui_class_t wvui_label_class;

/**
 * @brief Set the text.
 * @param text  NUL-terminated string; NULL is equivalent to drew nothing
 * @note  Stores the pointer without copying; the caller keeps the buffer alive.
 * @note  An unchanged pointer also marks the whole block dirty: rewriting the same buffer is the common case.
 */
void wvui_label_set_text(wvui_widget_t *w, const char *text);

/**
 * @brief Set the font.
 * @param font  Font; NULL = drew nothing, default NULL
 */
void wvui_label_set_font(wvui_widget_t *w, const wvui_font_t *font);

/**
 * @brief Set the text color.
 * @param color  RGB565 color, default white
 */
void wvui_label_set_color(wvui_widget_t *w, wvui_color_t color);

/**
 * @brief Set the horizontal alignment.
 * @param align  WVUI_LABEL_ALIGN_LEFT/CENTER/RIGHT, default LEFT
 * @note  Alignment is relative to the widget rectangle; text wider than the widget is clipped.
 */
void wvui_label_set_align(wvui_widget_t *w, uint8_t align);

/**
 * @brief Set the overall opacity.
 * @param opa  0..255, default 255; 0 = nothing is drawn
 */
void wvui_label_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Read back the current text pointer.
 * @return The stored pointer, not a copy; NULL if never set or on a type mismatch
 */
const char *wvui_label_text(const wvui_widget_t *w);

#endif /* WVUI_LABEL_H */
