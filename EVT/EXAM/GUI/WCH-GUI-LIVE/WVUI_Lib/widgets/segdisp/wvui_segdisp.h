#ifndef WVUI_SEGDISP_H
#define WVUI_SEGDISP_H

/**
 * @file wvui_segdisp.h
 * @brief Seven-segment display: glyphs are an 8-bit segment mask table, zero font dependency.
 * @note  Decorative widget, gestures are not intercepted; digits are laid out right to left,
 *        the digit gap defaults to the segment thickness.
 */

#include "wvui_widget.h"

typedef struct
{
    char text[12];     /**< Display buffer, read-only during drawing; up to 11 characters */
    uint32_t value;    /**< Numeric value, the input of set_value() */
    wvui_color_t on;    /**< Lit segment color */
    wvui_color_t off;   /**< Unlit segment color; unlit segments are not drawn when equal to on */
    uint8_t digits;    /**< Number of digits used by set_value(); default 4, range 1..10 */
    uint8_t decimals;  /**< Decimal places 0..digits; default 0, i.e. no decimal point */
    uint8_t seg_w;     /**< Per-digit width in pixels; default 16, minimum 4 */
    uint8_t seg_h;     /**< Per-digit height in pixels; 0 = widget height */
    uint8_t seg_t;     /**< Segment thickness in pixels; default 3, minimum 1 (drawing starts at 2) */
    uint8_t gap;       /**< Digit gap in pixels; 0 = auto (= max(3, seg_t)) */
    uint8_t opa;       /**< Widget opacity 0..255; 0 = not drawn */
} wvui_segdisp_rt_t;

extern const wvui_class_t wvui_segdisp_class;

/**
 * @brief Set the string directly, drawn digit by digit from the segment mask table.
 * @param text  Characters to display; NULL is treated as an empty string
 * @note  Recognizes 0-9, A-F (lowercase a-f as well), -, : and .; characters outside the table are drawn blank.
 * @note  Buffer is 12 bytes, hold at most 11 characters, longer input is truncated; the decimal point takes no
 *        slot, the colon takes one.
 */
void wvui_segdisp_set_text(wvui_widget_t *w, const char *text);

/**
 * @brief Set a number: right-aligned into digits digits, decimal point inserted per decimals.
 * @param value  Value 0..4294967295
 * @note  When the value exceeds the digits, every saturated digit shows 9, no wrap-around.
 * @note  The result goes into the buffer shared with set_text(), read-only during drawing.
 */
void wvui_segdisp_set_value(wvui_widget_t *w, uint32_t value);

/**
 * @brief Set the number of digits used by set_value().
 * @param digits  Digit count; below 1 is clamped to 1, above 10 is clamped to 10
 * @note  After a digit change the current value is re-formatted to the new width and the widget is dirtied.
 */
void wvui_segdisp_set_digits(wvui_widget_t *w, uint8_t digits);

/**
 * @brief Set the number of decimal places.
 * @param decimals  Decimal places; clamped to the current digits when exceeded
 * @note  Only affects set_value() formatting, unrelated to set_text().
 */
void wvui_segdisp_set_decimals(wvui_widget_t *w, uint8_t decimals);

/**
 * @brief Set the per-digit width, height and segment thickness.
 * @param seg_w  Per-digit width in pixels; below 4 is clamped to 4
 * @param seg_h  Per-digit height in pixels; 0 = widget height
 * @param seg_t  Segment thickness in pixels; below 1 is clamped to 1
 * @note  The effective drawn thickness is at least 2 (a 1-pixel round-cap line is smeared by anti-aliasing).
 */
void wvui_segdisp_set_seg_size(wvui_widget_t *w, uint8_t seg_w, uint8_t seg_h, uint8_t seg_t);

/**
 * @brief Set the digit gap; the decimal point is drawn inside the gap and takes no digit width.
 * @param gap  Digit gap in pixels; 0 = auto, equal to max(3, the drawn segment thickness)
 * @note  When the gap is smaller than the segment thickness the decimal point is clamped to the gap and
 *        never overlaps the neighbouring digit.
 */
void wvui_segdisp_set_gap(wvui_widget_t *w, uint8_t gap);

/**
 * @brief Set the lit and unlit segment colors.
 * @param on   Lit segment color
 * @param off  Unlit segment color; unlit segments are not drawn when equal to on
 */
void wvui_segdisp_set_colors(wvui_widget_t *w, wvui_color_t on, wvui_color_t off);

/**
 * @brief Set the opacity.
 * @param opa  Widget opacity 0..255; 0 = not drawn
 */
void wvui_segdisp_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Read back the value.
 * @return The most recent set_value() input; returns 0 on a type mismatch
 */
uint32_t wvui_segdisp_value(const wvui_widget_t *w);

/**
 * @brief Read back the display buffer.
 * @return NUL-terminated display string, owned by the widget and read-only for the caller;
 *         returns an empty string on a type mismatch
 */
const char *wvui_segdisp_text(const wvui_widget_t *w);

#endif /* WVUI_SEGDISP_H */
