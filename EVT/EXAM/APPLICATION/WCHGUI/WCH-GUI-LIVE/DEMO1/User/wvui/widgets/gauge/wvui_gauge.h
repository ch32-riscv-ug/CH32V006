#ifndef WVUI_GAUGE_H
#define WVUI_GAUGE_H

/**
 * @file wvui_gauge.h
 * @brief Gauge: value arc, track arc and needle, value 0..255 mapped to [a0, a1].
 * @note  Angles use the 512-step scale, 0 = +x, increasing clockwise on screen; decorative widget, does not intercept gestures.
 */

#include "wvui_draw.h"
#include "wvui_widget.h"

typedef struct
{
    wvui_color_t arc_color;     /**< Value arc color */
    wvui_color_t track_color;   /**< Track arc color (full sweep minus the value arc) */
    wvui_color_t needle_color;  /**< Needle color */
    uint16_t a0;               /**< Start angle, 512-step scale */
    uint16_t a1;               /**< End angle, 512-step scale */
    uint8_t value;             /**< Value 0..255 */
    uint8_t width;             /**< Ring width in pixels; 0 = filled sector */
    uint8_t needle_w;          /**< Needle line width in pixels; 0 = no needle */
    uint8_t radius;            /**< Outer radius in pixels; 0 = min(w,h)/2 */
    uint8_t opa;               /**< Widget opacity 0..255; 0 = not drawn */
} wvui_gauge_rt_t;

extern const wvui_class_t wvui_gauge_class;

/**
 * @brief Set the value.
 * @param value  Value 0..255; the corresponding angle is taken on [a0, a1] in 256 equal parts
 * @note   No redraw when the value is unchanged.
 */
void wvui_gauge_set_value(wvui_widget_t *w, uint8_t value);

/**
 * @brief Set the start and end angles, inputs are masked with 0x1FF first.
 * @param a0  Start angle, 512-step scale (90° = 128, 180° = 256)
 * @param a1  End angle, 512-step scale
 * @note   a1 below a0 means crossing the 0 point, a1 equal to a0 means a full circle.
 */
void wvui_gauge_set_angles(wvui_widget_t *w, uint16_t a0, uint16_t a1);

/**
 * @brief Set all three colors at once.
 * @param track   Track arc color
 * @param arc     Value arc color
 * @param needle  Needle color
 * @note   Value arc and track arc are complementary; with translucency each pixel is painted once.
 */
void wvui_gauge_set_colors(wvui_widget_t *w, wvui_color_t track, wvui_color_t arc,
                          wvui_color_t needle);

/**
 * @brief Set the ring width.
 * @param width  Ring width in pixels; 0 = filled sector
 * @note   With a filled sector the needle is still drawn from the center.
 */
void wvui_gauge_set_width(wvui_widget_t *w, uint8_t width);

/**
 * @brief Set the needle line width.
 * @param width  Needle line width in pixels; 0 = no needle
 * @note   The needle is a round-capped line segment from the center along the current angle.
 */
void wvui_gauge_set_needle(wvui_widget_t *w, uint8_t width);

/**
 * @brief Set the outer radius.
 * @param radius  Outer radius in pixels; 0 = min(w,h)/2
 * @note   Drawn arcs are still clipped to the widget rect.
 */
void wvui_gauge_set_radius(wvui_widget_t *w, uint8_t radius);

/**
 * @brief Set the opacity.
 * @param opa  Widget opacity 0..255; 0 = not drawn
 */
void wvui_gauge_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Read back the value.
 * @return        Value 0..255; returns 0 on wrong type
 */
uint8_t wvui_gauge_value(const wvui_widget_t *w);

/**
 * @brief Read back the angle of the current value.
 * @return        Angle, 512-step scale
 */
uint16_t wvui_gauge_angle(const wvui_widget_t *w);

#endif /* WVUI_GAUGE_H */
