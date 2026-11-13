#ifndef WVUI_BOX_H
#define WVUI_BOX_H

/**
 * @file wvui_box.h
 * @brief Box widget: boxes and panels with four independent corners; round, chamfered and square can be mixed.
 * @note  When all four corners share one radius use wvui_draw_round_rect(); this widget takes no touch.
 */

#include "wvui_draw.h"
#include "wvui_widget.h"

/** Aliases for the corner styles, forwarding the render-layer enums. */
#define WVUI_BOX_STYLE_SQUARE (WVUI_BOX_SQUARE)
#define WVUI_BOX_STYLE_ROUND (WVUI_BOX_ROUND)
#define WVUI_BOX_STYLE_CHAMFER (WVUI_BOX_CHAMFER)

typedef struct
{
    wvui_color_t fill;      /**< Fill color */
    wvui_color_t border;    /**< Border color */
    uint8_t corners[4];    /**< [LT, RT, LB, RB], same order as wvui_draw_box */
    uint8_t radius;        /**< Corner radius or chamfer size, px */
    uint8_t bw;            /**< Border width, px; 0 = no border */
    uint8_t opa;           /**< Widget opacity 0..255; 0 = not drawn */
} wvui_box_rt_t;

extern const wvui_class_t wvui_box_class;

/**
 * @brief Set the fill and border colors.
 * @param fill    Fill color
 * @param border  Border color
 */
void wvui_box_set_colors(wvui_widget_t *w, wvui_color_t fill, wvui_color_t border);

/**
 * @brief Set all four corner styles at once.
 * @param lt  Top-left corner style: WVUI_BOX_SQUARE / ROUND / CHAMFER
 * @param rt  Top-right corner style
 * @param lb  Bottom-left corner style
 * @param rb  Bottom-right corner style
 * @note  Styles are masked with & 0x03; all four corners SQUARE give a plain filled rectangle.
 */
void wvui_box_set_corners(wvui_widget_t *w, uint8_t lt, uint8_t rt, uint8_t lb, uint8_t rb);

/**
 * @brief Set a single corner style.
 * @param corner  Corner index: WVUI_BOX_LT / RT / LB / RB
 * @param style   Corner style: WVUI_BOX_SQUARE / ROUND / CHAMFER; masked with & 0x03
 * @note  Returns right away when corner is greater than WVUI_BOX_RB.
 */
void wvui_box_set_corner(wvui_widget_t *w, uint8_t corner, uint8_t style);

/**
 * @brief Set the corner radius or chamfer size.
 * @param radius  Radius, px; 0 = square corners
 * @note  Clamped to min(w,h)/2 when drawing.
 */
void wvui_box_set_radius(wvui_widget_t *w, uint8_t radius);

/**
 * @brief Set the border width.
 * @param bw  Border width, px; 0 = no border
 */
void wvui_box_set_border_width(wvui_widget_t *w, uint8_t bw);

/**
 * @brief Set the opacity.
 * @param opa  Widget opacity 0..255; 0 = not drawn
 */
void wvui_box_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Pack the four corner styles into one byte.
 * @param lt  Top-left corner style
 * @param rt  Top-right corner style
 * @param lb  Bottom-left corner style
 * @param rb  Bottom-right corner style
 * @return Packed result: 2 bits per corner, LT/RT/LB/RB from the low bits up
 */
uint8_t wvui_box_pack_corners(uint8_t lt, uint8_t rt, uint8_t lb, uint8_t rb);

/**
 * @brief Extract one corner from a packed byte.
 * @param packed  Packed corner styles, see wvui_box_pack_corners()
 * @param corner  Corner index: WVUI_BOX_LT / RT / LB / RB
 * @return Style of that corner, 0..3
 */
uint8_t wvui_box_corner_of(uint8_t packed, uint8_t corner);

#endif /* WVUI_BOX_H */
