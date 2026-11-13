#ifndef WVUI_BARCHART_H
#define WVUI_BARCHART_H

/**
 * @file wvui_barchart.h
 * @brief Bar chart and strip chart: values 0..255 mapped to bar length.
 * @note  Decorative widget; does not intercept gestures (event is NULL and returns 0).
 */

#include "wvui_widget.h"

typedef struct
{
    const uint8_t *samples;  /**< Held by the caller, which also guarantees its lifetime */
    uint16_t count;          /**< Number of bars */
    wvui_color_t bar_color;   /**< Bar color */
    wvui_color_t track_color; /**< Track color; equal to bar_color = not drawn */
    wvui_color_t base_color;  /**< Zero-value line color; equal to track_color = not drawn */
    uint8_t bar_w;           /**< Bar width, px; 0 = auto even split */
    uint8_t gap;             /**< Bar spacing, px */
    uint8_t radius;          /**< Bar corner radius, px; 0 = square corners */
    uint8_t vertical;        /**< 1 = vertical bars (default), 0 = horizontal strips */
    uint8_t opa;             /**< Widget opacity 0..255; 0 = not drawn */
} wvui_barchart_rt_t;

extern const wvui_class_t wvui_barchart_class;

/**
 * @brief Bind the data array without copying.
 * @param samples  Points to count values, each 0..255
 * @param count    Number of bars, must be greater than 0
 * @return         1 = success; 0 = invalid argument (type mismatch, NULL pointer, count of 0)
 * @note           samples must stay readable for the widget's lifetime; it is read directly when drawing.
 */
uint8_t wvui_barchart_bind(wvui_widget_t *w, const uint8_t *samples, uint16_t count);

/**
 * @brief Set one bar's value and redraw.
 * @param index  Bar index, 0..count-1
 * @param value  New value, 0..255
 * @note   Returns right away without redraw when index is out of range or the value is unchanged.
 * @note   Changing the array directly bypassing this function does not redraw the widget.
 */
void wvui_barchart_set(wvui_widget_t *w, uint16_t index, uint8_t value);

/**
 * @brief Set the bar and track colors.
 * @param bar    Bar color
 * @param track  Track color; equal to bar = track not drawn
 */
void wvui_barchart_set_colors(wvui_widget_t *w, wvui_color_t bar, wvui_color_t track);

/**
 * @brief Set the zero-value line color.
 * @param base  Zero-value line color; equal to the track color = not drawn
 */
void wvui_barchart_set_base_color(wvui_widget_t *w, wvui_color_t base);

/**
 * @brief Set the bar width and spacing.
 * @param bar_w  Bar width, px; 0 = auto even split by count (one division per draw)
 * @param gap    Bar spacing, px
 * @note   With bar_w = 0 the bar width = widget width / count − gap.
 * @note   With bar_w > 0 the slot width = bar_w + gap; bars that do not fit are truncated.
 */
void wvui_barchart_set_bar(wvui_widget_t *w, uint8_t bar_w, uint8_t gap);

/**
 * @brief Set the bar corner radius.
 * @param radius  Corner radius, px; 0 = square corners
 * @note   Clamped to min(bar width, bar length) / 2 when drawing.
 */
void wvui_barchart_set_radius(wvui_widget_t *w, uint8_t radius);

/**
 * @brief Set the bar direction.
 * @param vertical  1 = vertical bars (default), 0 = horizontal strips
 * @note   Horizontal strips grow from the left and stack top to bottom.
 */
void wvui_barchart_set_vertical(wvui_widget_t *w, uint8_t vertical);

/**
 * @brief Set the opacity.
 * @param opa  Widget opacity 0..255; 0 = not drawn
 */
void wvui_barchart_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Get one bar's value.
 * @param index  Bar index
 * @return       Value of that bar, 0..255; 0 when out of range or not bound
 */
uint8_t wvui_barchart_value(const wvui_widget_t *w, uint16_t index);

/**
 * @brief Get the number of bars.
 * @return       Number of bars; 0 when not bound or on type mismatch
 */
uint16_t wvui_barchart_count(const wvui_widget_t *w);

#endif /* WVUI_BARCHART_H */
