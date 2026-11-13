#ifndef WVUI_MONOIMG_H
#define WVUI_MONOIMG_H

/**
 * @file wvui_monoimg.h
 * @brief Monochrome image: 1bpp bitmap drawn pixel by pixel in the foreground color.
 * @note  Decorative widget, does not intercept gestures; the caller owns the bitmap, the widget only stores the pointer.
 */

#include "wvui_widget.h"

typedef struct
{
    const uint8_t *bits; /**< Bitmap: MSB first, rows byte-aligned, owned by the caller */
    wvui_color_t color;   /**< Draw color, the foreground color */
    int16_t bw;          /**< Bitmap width in pixels */
    int16_t bh;          /**< Bitmap height in pixels */
    uint8_t opa;         /**< Widget opacity 0..255; 0 = not drawn */
    uint8_t invert;      /**< 1 = draw bit 0, leave bit 1 blank (inverted display) */
} wvui_monoimg_rt_t;

extern const wvui_class_t wvui_monoimg_class;

/**
 * @brief Bind a 1bpp bitmap.
 * @param bits  Bitmap data, MSB first, each row padded to whole bytes (same layout as glyph rasters)
 * @param bw    Bitmap width in pixels, must be > 0
 * @param bh    Bitmap height in pixels, must be > 0
 * @return 1 = bound; 0 = invalid argument (wrong type, NULL pointer, zero width or height)
 * @note  The widget rect is the draw area: pasted 1:1 from the top-left, clipped beyond the edge, the rest stays transparent; bits must stay readable for its lifetime.
 */
uint8_t wvui_monoimg_set_bitmap(wvui_widget_t *w, const uint8_t *bits, int16_t bw, int16_t bh);

/**
 * @brief Set the draw color.
 * @param color  Foreground color
 * @note  Pixels have only two states, drawn and blank; blending is done by opa.
 */
void wvui_monoimg_set_color(wvui_widget_t *w, wvui_color_t color);

/**
 * @brief Set the opacity.
 * @param opa  Widget opacity 0..255; 0 = not drawn
 */
void wvui_monoimg_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Set inverted display.
 * @param invert  0 = draw bit 1 (default), 1 = draw bit 0
 * @note  Only flips the bit polarity, does not copy the bitmap.
 */
void wvui_monoimg_set_invert(wvui_widget_t *w, uint8_t invert);

/**
 * @brief Read back the bitmap pointer.
 * @return Bound bitmap; 0 if unbound or the type does not match
 */
const uint8_t *wvui_monoimg_bits(const wvui_widget_t *w);

/**
 * @brief Read back the bitmap width.
 * @return Width in pixels; 0 if unbound or the type does not match
 */
int16_t wvui_monoimg_width(const wvui_widget_t *w);

/**
 * @brief Read back the bitmap height.
 * @return Height in pixels; 0 if unbound or the type does not match
 */
int16_t wvui_monoimg_height(const wvui_widget_t *w);

#endif /* WVUI_MONOIMG_H */
