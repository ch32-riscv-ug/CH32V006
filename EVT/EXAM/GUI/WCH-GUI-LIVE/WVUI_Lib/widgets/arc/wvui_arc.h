#ifndef WVUI_ARC_H
#define WVUI_ARC_H

/**
 * @file wvui_arc.h
 * @brief Arc widget: tick ring, ring progress and pie sectors.
 * @note  Angles use a 512-step scale: 0 = +x, growing clockwise on screen; center = rect center, decorative widget takes no touch.
 */

#include "wvui_draw.h"
#include "wvui_widget.h"

typedef struct
{
    wvui_color_t color;  /**< Arc color */
    uint16_t a0;        /**< Start angle; 512-step scale */
    uint16_t a1;        /**< End angle; may be less than a0 (wraps past 0), a0 == a1 = full turn */
    uint8_t width;      /**< Ring width (outer radius − inner radius), px; 0 = solid sector */
    uint8_t radius;     /**< Outer radius, px; 0 = min(w,h)/2 */
    uint8_t opa;        /**< Widget opacity 0..255; 0 = not drawn */
} wvui_arc_rt_t;

extern const wvui_class_t wvui_arc_class;

/**
 * @brief Set the start and end angles.
 * @param a0  Start angle, 512-step scale (90° = 128, 180° = 256); masked with 0x1FF first
 * @param a1  End angle, 512-step scale; masked with 0x1FF first
 * @note  a1 < a0 wraps past 0; a1 == a0 means a full turn.
 */
void wvui_arc_set_angles(wvui_widget_t *w, uint16_t a0, uint16_t a1);

/**
 * @brief Set the arc color.
 * @param color  Arc color
 */
void wvui_arc_set_color(wvui_widget_t *w, wvui_color_t color);

/**
 * @brief Set the ring width.
 * @param width  Ring width (outer radius − inner radius), px; 0 = solid sector
 * @note  When width exceeds the outer radius the inner radius is treated as 0, i.e. a solid sector.
 */
void wvui_arc_set_width(wvui_widget_t *w, uint8_t width);

/**
 * @brief Set the outer radius.
 * @param radius  Outer radius, px; 0 = min(w,h)/2
 * @note  The drawn arc is still clipped to the widget rect; an enlarged radius paints nothing outside.
 */
void wvui_arc_set_radius(wvui_widget_t *w, uint8_t radius);

/**
 * @brief Set the opacity.
 * @param opa  Widget opacity 0..255; 0 = not drawn
 */
void wvui_arc_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Get the start angle.
 * @return Start angle, 512-step scale; 0 on widget type mismatch
 */
uint16_t wvui_arc_start(const wvui_widget_t *w);

/**
 * @brief Get the end angle.
 * @return End angle, 512-step scale; 0 on widget type mismatch
 */
uint16_t wvui_arc_end(const wvui_widget_t *w);

#endif /* WVUI_ARC_H */
