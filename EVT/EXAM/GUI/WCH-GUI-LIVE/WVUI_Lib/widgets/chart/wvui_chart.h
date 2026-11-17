#ifndef WVUI_CHART_H
#define WVUI_CHART_H

/**
 * @file wvui_chart.h
 * @brief Line chart: one sample per pixel, values 0..255 mapped to the widget height.
 * @note  Decorative widget, does not intercept gestures (returns 0 when event is NULL).
 */

#include "wvui_widget.h"

typedef struct
{
    const uint8_t *samples;  /**< Owned by the caller, which guarantees the lifetime */
    uint16_t count;          /**< Sample count; must be a power of two and should equal the inner width */
    uint16_t head;           /**< Ring write pointer, the next position to overwrite */
    wvui_color_t line_color;  /**< Line color */
    wvui_color_t base_color;  /**< Zero-line color; same as line_color means not drawn */
    uint8_t ring;            /**< 1 = ring order (oldest on the left, default) */
    uint8_t opa;             /**< Widget opacity 0..255; 0 = not drawn */
} wvui_chart_rt_t;

extern const wvui_class_t wvui_chart_class;

/**
 * @brief Bind a sample array without copying it.
 * @param samples  Points to count values in the range 0..255
 * @param count    Sample count; must be a power of two and should equal the inner width
 * @return          1 = success; 0 = invalid argument (wrong type, NULL pointer, not a power of two)
 * @note   head is reset to 0 after binding; ring indexing relies on masks, so both modes require a power-of-two count.
 * @note   The widget does not resample: samples of other densities must be resampled beforehand.
 */
uint8_t wvui_chart_bind(wvui_widget_t *w, const uint8_t *samples, uint16_t count);

/**
 * @brief Write one sample and redraw.
 * @param index  Sample index, 0..count-1
 * @param value  New value 0..255
 * @note   index is an absolute array index; in ring mode it does not rotate with head.
 * @note   Changing the array directly without this function does not redraw the widget.
 */
void wvui_chart_set(wvui_widget_t *w, uint16_t index, uint8_t value);

/**
 * @brief Append one sample and redraw.
 * @param value  New value 0..255
 * @note   In ring mode writes at head and advances it; in linear mode shifts all samples left by one and appends the new value.
 */
void wvui_chart_push(wvui_widget_t *w, uint8_t value);

/**
 * @brief Clear the samples.
 * @note   Writes 0 to all samples and leaves head untouched.
 */
void wvui_chart_clear(wvui_widget_t *w);

/**
 * @brief Set the line color and the zero-line color.
 * @param line  Line color
 * @param base  Zero-line color; same as line means not drawn
 */
void wvui_chart_set_colors(wvui_widget_t *w, wvui_color_t line, wvui_color_t base);

/**
 * @brief Set the ring mode.
 * @param ring  1 = ring order (oldest on the left, default), 0 = linear
 */
void wvui_chart_set_ring(wvui_widget_t *w, uint8_t ring);

/**
 * @brief Set the opacity.
 * @param opa  Widget opacity 0..255; 0 = not drawn
 */
void wvui_chart_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Read back the sample count.
 * @return        Sample count; returns 0 when unbound or on wrong type
 */
uint16_t wvui_chart_count(const wvui_widget_t *w);

/**
 * @brief Read back the ring write pointer.
 * @return        head, the next index to overwrite; returns 0 on wrong type
 */
uint16_t wvui_chart_head(const wvui_widget_t *w);

#endif /* WVUI_CHART_H */
