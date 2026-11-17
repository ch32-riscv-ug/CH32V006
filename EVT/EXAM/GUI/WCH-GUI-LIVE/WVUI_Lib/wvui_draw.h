#ifndef WVUI_DRAW_H
#define WVUI_DRAW_H

/**
 * @file wvui_draw.h
 * @brief Draw primitives: rounded rectangles, boxes, anti-aliased lines and arcs.
 * @note  All writes go through wvui_render_pixel(); each pixel gets coverage counted once and color blended once.
 */

#include <stdint.h>

#include "wvui_config.h"
#include "wvui_types.h"

/** Line cap style. */
typedef enum
{
    WVUI_CAP_BUTT = 0,  /**< Butt: cap area coverage 0 */
    WVUI_CAP_ROUND = 1  /**< Round: half-circle cap */
} wvui_line_cap_t;

/** Per-corner box style; the `corners[]` index order is fixed as LT/RT/LB/RB. */
typedef enum
{
    WVUI_BOX_SQUARE = 0,  /**< Square corner */
    WVUI_BOX_ROUND = 1,   /**< Rounded corner, radius r */
    WVUI_BOX_CHAMFER = 2  /**< 45° chamfer, cut edge length r */
} wvui_box_corner_t;

/** Indexes into `corners[]`. */
#define WVUI_BOX_LT (0u)
#define WVUI_BOX_RT (1u)
#define WVUI_BOX_LB (2u)
#define WVUI_BOX_RB (3u)

/**
 * @brief Integer square root, rounded down.
 * @param v  Value to take the square root of
 * @return    Square root, rounded down
 * @note   Bit-by-bit iteration, at most 16 rounds; callers filter first by comparing squared distances against thresholds.
 */
uint32_t wvui_isqrt32(uint32_t v);

/**
 * @brief Sine, 512-step scale, Q15 fixed-point.
 * @param a  Angle; 512 = full circle (90° = 128, 180° = 256), 0 = +x, growing angle = clockwise on screen (y-axis down)
 * @return    -32767..32767
 * @note   64-entry table with linear interpolation; table takes 130 B.
 */
int16_t wvui_sin512(uint16_t a);

/**
 * @brief Cosine, 512-step scale, Q15 fixed-point.
 * @param a  Angle, same rules as wvui_sin512()
 * @return    -32767..32767
 */
int16_t wvui_cos512(uint16_t a);

/**
 * @brief Draw an anti-aliased line segment (single-pass capsule coverage).
 * @param x0,y0  Start point, screen coordinates
 * @param x1,y1  End point, screen coordinates
 * @param width  Line width; 0 = skip drawing
 * @param cap    Cap style, see wvui_line_cap_t
 * @param color  Line color
 * @param opa    Widget opacity 0..255; container opacity is multiplied once more by the render layer
 * @note   Coverage = 1-pixel ramp of the distance from the pixel center to the segment: perpendicular distance on the body, end-point distance in the caps, no double-blending at the seams; a butt cap is cut axially at the end point.
 * @note   Clips in parameter space first when an end point lies off-screen; end-point coordinates in the visible region carry up to 1 pixel of rounding error.
 */
void wvui_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t width,
                   wvui_line_cap_t cap, wvui_color_t color, uint8_t opa);

/**
 * @brief Draw a rounded rectangle (uniform radius): fill plus optional border.
 * @param rect        Bounding rectangle, screen coordinates, inclusive
 * @param r           Corner radius; 0 = square, clamped to min(w,h)/2 (past it becomes a capsule/circle)
 * @param bw          Border width; 0 = no border, inner radius = max(r - bw, 0)
 * @param fill        Fill color
 * @param fill_opa    Fill opacity; 0 = no fill
 * @param border      Border color
 * @param border_opa  Border opacity; 0 = no border
 * @note   Shape = the region within distance r of the inset rectangle [x0+r, x1+1-r]×[y0+r, y1+1-r]; pixels fully inside or outside take the 255/0 fast path.
 * @note   Coverage has 256 levels and the inner and outer loops are computed together: border coverage = outer − inner, no double-blending when semi-transparent.
 */
void wvui_draw_round_rect(wvui_rect_t rect, uint8_t r, uint8_t bw,
                         wvui_color_t fill, uint8_t fill_opa,
                         wvui_color_t border, uint8_t border_opa);

/**
 * @brief Draw a box (independent corner styles): fill plus optional border.
 * @param rect        Bounding rectangle, screen coordinates, inclusive
 * @param corners     Corner styles, indexed with WVUI_BOX_LT/RT/LB/RB
 * @param r           Corner radius or chamfer cut length, clamped to min(w,h)/2
 * @param bw          Border width; 0 = no border
 * @param fill        Fill color
 * @param fill_opa    Fill opacity; 0 = no fill
 * @param border      Border color
 * @param border_opa  Border opacity; 0 = no border
 * @note   Corners may mix rounded/chamfered/square; all SQUARE gives a solid rectangle, all ROUND is pixel-identical to
 *         wvui_draw_round_rect(), with the same AA ramp and border rules.
 */
void wvui_draw_box(wvui_rect_t rect, const uint8_t corners[4], uint8_t r, uint8_t bw,
                  wvui_color_t fill, uint8_t fill_opa,
                  wvui_color_t border, uint8_t border_opa);

/**
 * @brief Draw an arc or a ring sector.
 * @param cx,cy  Center, screen coordinates
 * @param r_out  Outer radius
 * @param r_in   Inner radius; 0 = filled sector (pie)
 * @param a0     Start angle, 512-step scale, clockwise
 * @param a1     End angle; may be less than a0 (crosses the 0 point)
 * @param color  Arc color
 * @param opa    Widget opacity 0..255; container opacity is multiplied once more by the render layer
 * @note   The radial test uses the distance from the center (threshold comparison plus one Q16 reciprocal for the 1-pixel ramp), the angular test uses cross products of the two boundary rays; no atan2 needed.
 * @note   Cross products are reliable only within a half circle: with a sweep over 180°, compute the complement arc and invert; a1 == a0 means a full circle.
 */
void wvui_draw_arc(int16_t cx, int16_t cy, uint16_t r_out, uint16_t r_in,
                  uint16_t a0, uint16_t a1, wvui_color_t color, uint8_t opa);

#endif /* WVUI_DRAW_H */
