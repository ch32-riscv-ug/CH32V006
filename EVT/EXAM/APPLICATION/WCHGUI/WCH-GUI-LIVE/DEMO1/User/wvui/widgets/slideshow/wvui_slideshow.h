#ifndef WVUI_SLIDESHOW_H
#define WVUI_SLIDESHOW_H

/**
 * @file wvui_slideshow.h
 * @brief Auto slideshow: frame-counted timing; holds each image for a few frames, then slides horizontally to the next.
 * @note  Both images are drawn once per frame in transition, no framebuffer needed; slide = 0 means a hard cut.
 */

#include "wvui_anim.h"
#include "wvui_asset.h"
#include "wvui_widget.h"

typedef struct
{
    wvui_anim_t an;             /**< Slideshow timing; joins the active list while auto-playing */
    const wvui_asset_id_t *ids; /**< Asset ID table, owned by the caller, not copied */
    uint16_t count;            /**< Image count; no slideshow below 2 */
    uint16_t index;            /**< Current index, the image shown while idle */
    int16_t offset;            /**< Transition offset in pixels; 0 = idle */
    wvui_color_t bg;            /**< Background color; 0 = no background fill */
    uint8_t hold_cfg;          /**< Hold frames per image; default WVUI_CFG_SLIDESHOW_HOLD, overridden by set_timing */
    uint8_t slide_cfg;         /**< Transition frames; 0 = hard cut */
    uint8_t hold_left;         /**< Remaining hold frames */
    uint8_t slide_left;        /**< Remaining transition frames */
    uint16_t frac;             /**< Bresenham remainder; one pixel steps in once accrued */
    uint8_t phase;             /**< 0 = holding, 1 = transitioning */
    uint8_t autoplay;          /**< 1 = auto play on */
    uint8_t opa;               /**< Widget opacity 0..255; 0 = not drawn */
} wvui_slideshow_rt_t;

extern const wvui_class_t wvui_slideshow_class;

/**
 * @brief Bind the asset ID table, no copy.
 * @param ids    Array of count asset IDs
 * @param count  Image count; treated as 0, i.e. unbind, when ids is NULL
 * @note  Binding resets the index to 0 and restarts auto-play timing with the new count.
 * @note  ids must stay readable for as long as the widget lives; assets are opened on demand while drawing.
 */
void wvui_slideshow_bind(wvui_widget_t *w, const wvui_asset_id_t *ids, uint16_t count);

/**
 * @brief Switch to the next image immediately, wrapping around.
 * @note  Offset resets to 0, phase returns to holding, and the hold countdown restarts.
 * @note  Returns right away with fewer than two images.
 */
void wvui_slideshow_next(wvui_widget_t *w);

/**
 * @brief Jump to the given index.
 * @param index  Target index 0..count-1
 * @note  Out-of-range indexes return without clamping; an index equal to the current one does nothing.
 * @note  Hard cut, no slide transition; the hold countdown restarts.
 */
void wvui_slideshow_set_index(wvui_widget_t *w, uint16_t index);

/**
 * @brief Set the hold and transition frame counts.
 * @param hold_frames   Hold frames per image 1..255; 0 clamps to 1
 * @param slide_frames  Transition frames 0..255; 0 = hard cut
 * @note  A hold of 0 would switch every frame, so it clamps to 1.
 * @note  The unit is frames, not milliseconds; changes take effect at once without restarting the timing node.
 */
void wvui_slideshow_set_timing(wvui_widget_t *w, uint8_t hold_frames, uint8_t slide_frames);

/**
 * @brief Turn auto play on or off.
 * @param on  1 = on, anything else = off
 * @note  With fewer than two images only the switch state changes, nothing starts.
 * @note  An unchanged state does not restart the timing node.
 */
void wvui_slideshow_set_autoplay(wvui_widget_t *w, uint8_t on);

/**
 * @brief Set the background color.
 * @param bg  Background color; 0 = no background fill, uncovered areas are left as is
 * @note  When the image is smaller than the widget only the image is drawn, and the background shows around it.
 */
void wvui_slideshow_set_colors(wvui_widget_t *w, wvui_color_t bg);

/**
 * @brief Set the opacity.
 * @param opa  Widget opacity 0..255; 0 = not drawn
 */
void wvui_slideshow_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Read back the image count.
 * @return Image count; 0 when unbound or on wrong type
 */
uint16_t wvui_slideshow_count(const wvui_widget_t *w);

/**
 * @brief Read back the current index.
 * @return Current index 0..count-1; 0 when unbound or on wrong type
 */
uint16_t wvui_slideshow_index(const wvui_widget_t *w);

/**
 * @brief Read back the auto play switch.
 * @return 1 = auto play on; 0 = off or wrong type
 */
uint8_t wvui_slideshow_is_playing(const wvui_widget_t *w);

/**
 * @brief Read back the transition offset.
 * @return Transition offset in pixels; 0 = idle, and one slide completes when it equals the widget width
 */
int16_t wvui_slideshow_offset(const wvui_widget_t *w);

#endif /* WVUI_SLIDESHOW_H */
