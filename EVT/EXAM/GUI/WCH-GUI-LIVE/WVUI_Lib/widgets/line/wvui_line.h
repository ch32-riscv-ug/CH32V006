#ifndef WVUI_LINE_H
#define WVUI_LINE_H

/**
 * @file wvui_line.h
 * @brief  Line widget: an anti-aliased line segment, also usable as a separator.
 * @note   Decorative widget: does not intercept gestures, events go to the widget below.
 */

#include "wvui_draw.h"
#include "wvui_widget.h"

/** End cap styles, forwarded from the render layer. */
#define WVUI_LINE_CAP_BUTT (WVUI_CAP_BUTT)
#define WVUI_LINE_CAP_ROUND (WVUI_CAP_ROUND)

typedef struct
{
    int16_t x0;            /**< Start point x, screen coordinates */
    int16_t y0;            /**< Start point y, screen coordinates */
    int16_t x1;            /**< End point x, screen coordinates */
    int16_t y1;            /**< End point y, screen coordinates */
    wvui_color_t color;     /**< Line color, default neutral gray */
    uint8_t width;         /**< Line width in pixels, default 1; 0 = do not draw */
    uint8_t cap;           /**< WVUI_LINE_CAP_*, default butt */
    uint8_t opa;           /**< Opacity 0..255, default 255 */
} wvui_line_rt_t;

extern const wvui_class_t wvui_line_class;

/**
 * @brief Set both endpoints, in absolute screen coordinates.
 * @param x0,y0  Start point
 * @param x1,y1  End point
 * @note  Endpoints may sit outside the widget rectangle; dirtying marks the
 *        real clipped bounding box, once for the old and once for the new line.
 * @note  Drawing is clipped to the widget rectangle, parts of the line
 *        outside are not drawn.
 */
void wvui_line_set_points(wvui_widget_t *w, int16_t x0, int16_t y0, int16_t x1, int16_t y1);

/**
 * @brief Recompute the default endpoints from the widget rectangle.
 * @note  w >= h picks the horizontal midline, otherwise the vertical one; a
 *        separator only needs w = width and h = 1.
 * @note  init and the command channel changing the rectangle call it
 *        automatically; call manually once after changing only w->x / w->y.
 */
void wvui_line_reset_points(wvui_widget_t *w);

/**
 * @brief Set the line width.
 * @param width  Line width in pixels, default 1; 0 = do not draw
 * @note  The width changes the bounding box, which dirties the old and the
 *        new rectangle once each.
 */
void wvui_line_set_width(wvui_widget_t *w, uint8_t width);

/**
 * @brief Set the end cap style.
 * @param cap  WVUI_LINE_CAP_BUTT or WVUI_LINE_CAP_ROUND, default BUTT
 * @note  Out-of-range values are ignored; a round cap takes half the line
 *        width more than a butt cap.
 */
void wvui_line_set_cap(wvui_widget_t *w, uint8_t cap);

/**
 * @brief Set the line color.
 * @param color  RGB565 color, default neutral gray
 */
void wvui_line_set_color(wvui_widget_t *w, wvui_color_t color);

/**
 * @brief Set the opacity.
 * @param opa  0..255, default 255; 0 = do not draw
 */
void wvui_line_set_opa(wvui_widget_t *w, uint8_t opa);

#endif /* WVUI_LINE_H */
