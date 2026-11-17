#ifndef WVUI_IMG_H_WIDGET
#define WVUI_IMG_H_WIDGET

/**
 * @file wvui_img.h
 * @brief Image widget: fetches an image by asset ID and overlays it on the widget rectangle.
 * @note  Decorative widget: no gesture interception, events pass to the layer below.
 */

#include "wvui_asset.h" /* wvui_asset_id_t: narrow integer asset ID */
#include "wvui_widget.h"

typedef struct
{
    wvui_asset_id_t asset;  /**< Asset ID; 0 may also be a valid asset */
    wvui_color_t color;     /**< Recolor for alpha-only formats, default white */
    uint8_t opa;           /**< Opacity 0..255, default 255 */
} wvui_img_rt_t;

extern const wvui_class_t wvui_img_class;

/**
 * @brief Set the image asset ID.
 * @param id  Asset ID; 0 is equivalent to an empty image
 * @note  Draw opens the asset by ID once; an unchanged value does not mark dirty.
 * @note  Missing asset or unsupported format draws nothing; the cause goes into wvui_img_stats().
 */
void wvui_img_set_asset(wvui_widget_t *w, wvui_asset_id_t id);

/**
 * @brief Set the recolor for alpha-only formats.
 * @param color  RGB565 color, default white
 * @note  Applies only to alpha-only formats such as A8/A4/A2/A1 and QOI_MASK; full pixel formats ignore it.
 */
void wvui_img_set_color(wvui_widget_t *w, wvui_color_t color);

/**
 * @brief Set the image opacity.
 * @param opa  0..255, default 255; 0 = nothing is drawn
 * @note  An unchanged value does not mark dirty.
 */
void wvui_img_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Read back the current asset ID.
 * @return Asset ID; 0 means an empty image, also returns 0 on a type mismatch
 */
wvui_asset_id_t wvui_img_asset(const wvui_widget_t *w);

/**
 * @brief Read back the current opacity.
 * @return 0..255; returns 0 on a type mismatch
 */
uint8_t wvui_img_opa(const wvui_widget_t *w);

#endif /* WVUI_IMG_H_WIDGET */
