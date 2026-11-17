#ifndef WVUI_RENDER_H
#define WVUI_RENDER_H

/**
 * @file wvui_render.h
 * @brief PFB render plane: slice filling, blending, opacity and image decode dispatch.
 * @note  All writes are bounded by the current slice; coordinates outside it are clipped.
 */

#include <stdint.h>
#include "wvui_config.h"
#include "wvui_types.h"
#include "wvui_asset.h" /* images fetch data via wvui_blob_t */

/** Slices */

/**
 * @brief Record the screen size so wvui_draw_slice() can return the full-screen slice outside chunking.
 * @param screen_w  Screen width in pixels
 * @param screen_h  Screen height in pixels
 */
void wvui_render_init(int16_t screen_w, int16_t screen_h);

/**
 * @brief Unbind the render layer; afterwards wvui_draw_slice() still returns the last full-screen slice.
 */
void wvui_render_deinit(void);

/**
 * @brief Start rendering one chunk.
 * @param plane         PFB buffer to write for this chunk, address of its first pixel
 * @param plane_pixels  Capacity of that buffer, in pixels
 * @param chunk         Chunk rectangle, screen coordinates, inclusive
 * @note  PFB row 0 corresponds to chunk.y0 and the first pixel to chunk.x0.
 * @note  Row stride == wvui_rect_w(&chunk); narrower than the screen during horizontal second-chunking;
 *        with double buffering plane is handed over per chunk.
 */
void wvui_render_begin_chunk(wvui_color_t *plane, uint32_t plane_pixels, wvui_rect_t chunk);

/**
 * @brief End rendering of the current chunk.
 */
void wvui_render_end_chunk(void);

/**
 * @brief Query whether a chunk is currently being rendered.
 * @return  1 = inside a chunk; 0 = not
 */
uint8_t wvui_render_in_chunk(void);

/**
 * @brief Set an extra clip rectangle; draw callbacks use it to bound their drawing.
 * @param r  Clip rectangle, screen coordinates, inclusive
 * @note  Stores current clip intersected with r and can only narrow; resets to the whole
 *        slice at the start of every chunk.
 */
void wvui_render_set_clip(wvui_rect_t r);

/**
 * @brief Clear the extra clip rectangle, back to the whole slice.
 */
void wvui_render_clear_clip(void);

/**
 * @brief Get the current effective clip.
 * @return  Current clip rectangle, screen coordinates, inclusive
 * @note  Row-level drawing clips once against it first; per-pixel writes clip a second
 *        time; reading it is only an optimization.
 * @note  Outside a chunk the previous clip is returned.
 */
wvui_rect_t wvui_render_clip(void);

/** Fill methods */

/**
 * @brief Fill the whole current slice with the background color.
 * @param c  Background color
 * @note  Base of stateless redraw: every refreshed slice gets the background first, then content.
 * @note  No-op when not inside a chunk or plane is NULL.
 */
void wvui_render_fill_slice(wvui_color_t c);

/**
 * @brief Fill a solid rectangle, writing only parts inside the current effective clip.
 * @param r  Rectangle, screen coordinates, inclusive
 * @param c  Fill color
 */
void wvui_render_fill_rect(wvui_rect_t r, wvui_color_t c);

/**
 * @brief 1-pixel rectangle outline, used by the dirty rectangle overlay of WVUI_CFG_DEBUG_DIRTY_RECT.
 * @param r  Rectangle, screen coordinates, inclusive
 * @param c  Outline color
 * @note  One wvui_render_fill_rect() call per edge; across slices only the visible part is drawn per slice.
 */
void wvui_render_rect_outline(wvui_rect_t r, wvui_color_t c);

/** Compositing */

/**
 * @brief Set the global opacity factor.
 * @param scale  Factor 0..255; 255 = no effect (default)
 * @note  Every drawing method consumes it once at entry, multiplied with the widget-supplied
 *        opa, never multiplied twice.
 * @note  There is only one global factor; stacking for widget enter/exit animations is
 *        pre-multiplied by the caller before passing in.
 */
void wvui_render_set_opa_scale(uint8_t scale);

/**
 * @brief Get the global opacity factor.
 * @return  Factor 0..255, default 255
 */
uint8_t wvui_render_opa_scale(void);

/**
 * @brief Scale an opacity by the global factor.
 * @param opa  Widget opacity 0..255
 * @return  Scaled opacity 0..255
 */
uint8_t wvui_render_opa_apply(uint8_t opa);

/**
 * @brief RGB565 blend.
 * @param fg   Foreground color
 * @param bg   Background color
 * @param opa  Opacity 0..255; 0 = all background, 255 = all foreground
 * @return  Blended color
 * @note  Uses a256 = alpha + (alpha >> 7) then >> 8 instead of /255; no float, no division.
 */
wvui_color_t wvui_render_blend(wvui_color_t fg, wvui_color_t bg, uint8_t opa);

/**
 * @brief Fill a solid rectangle with opacity, writing only parts inside the current clip.
 * @param r    Rectangle, screen coordinates, inclusive
 * @param c    Fill color
 * @param opa  Opacity 0..255; the render layer multiplies the global factor again; 0 = nothing drawn
 */
void wvui_render_fill_rect_blend(wvui_rect_t r, wvui_color_t c, uint8_t opa);

/**
 * @brief Write one pixel with opacity; points outside the clip are dropped.
 * @param x,y  Screen coordinates
 * @param c    Color
 * @param opa  Opacity 0..255; the render layer multiplies the global factor again; 0 = nothing drawn
 */
void wvui_render_pixel(int16_t x, int16_t y, wvui_color_t c, uint8_t opa);

/** Image format dispatch and decoding: the unified entry is wvui_img_render_auto(). */

typedef enum
{
    WVUI_IMG_ERR_NONE = 0,    /**< No error */
    WVUI_IMG_ERR_NO_BLOB,     /**< blob is empty (drawn before open) */
    WVUI_IMG_ERR_HDR,         /**< Header invalid, or size zero / out of range */
    WVUI_IMG_ERR_UNSUPPORTED, /**< No decoder (including compiled out) */
    WVUI_IMG_ERR_STREAM,      /**< Unknown stream version, or index out of range */
    WVUI_IMG_ERR_READ         /**< Media read too short (resource truncated) */
} wvui_img_err_t;

/** Image drawing statistics. */
typedef struct
{
    uint32_t draws;       /**< Number of calls */
    uint32_t drawn;       /**< Number of draws that had a visible area */
    uint32_t pixels;      /**< Number of drawn pixels; alpha = 0 not counted */
    uint32_t unsupported; /**< Number of unsupported formats */
    uint32_t bad_header;  /**< Number of invalid headers */
    uint32_t bad_stream;  /**< Number of bad streams */
    uint32_t short_reads; /**< Number of short media reads */
    uint8_t last_error;   /**< Cause of the latest failure, see wvui_img_err_t */
} wvui_img_stats_t;

/**
 * @brief Get the image drawing statistics.
 * @return  Read-only statistics struct, process-wide singleton
 */
const wvui_img_stats_t *wvui_img_stats(void);

/**
 * @brief Clear all image drawing statistics.
 */
void wvui_img_stats_reset(void);

/**
 * @brief Get the readable name of an error code.
 * @param err  Error code, see wvui_img_err_t
 * @return  Static string, never NULL; unknown codes give "?"
 */
const char *wvui_img_err_name(uint8_t err);

/**
 * @brief Query whether the format has a decoder under the current build configuration.
 * @param format  Format byte: [compression class:4][pixel format:4]
 * @return  1 = decoder exists; 0 = none
 * @note  Uncompressed supports RGB565 / ARGB8565 / A8(A4/A2/A1); index QOI V3 supports
 *        RGB565 / ARGB8565; QOI_MASK supports A8.
 */
uint8_t wvui_img_format_supported(uint8_t format);

/**
 * @brief Query whether it is a pure alpha bitmap; such bitmaps carry no color and are
 *        tinted with the widget's fg_color at draw time.
 * @param format  Format byte, same rules as wvui_img_format_supported()
 * @return  1 = A1/A2/A4/A8 (including A8 compressed by indexQOI_MASK); 0 = others
 */
uint8_t wvui_img_format_is_alpha(uint8_t format);

/**
 * @brief Get the byte size of one whole pixel.
 * @param format  Format byte, same rules as wvui_img_format_supported()
 * @return  RGB565 = 2, ARGB8565 = 3; pure alpha bitmaps return 0
 */
uint8_t wvui_img_format_pixel_bytes(uint8_t format);

/**
 * @brief Get the image size from the 6-byte header of a blob.
 * @param blob  Opened image resource
 * @param w     Output: width in pixels
 * @param h     Output: height in pixels
 * @note  On failure both w and h get 0; widgets call it in init to compute their geometry.
 */
void wvui_img_size(const wvui_blob_t *blob, int16_t *w, int16_t *h);

/**
 * @brief Draw the image pointed to by a blob at (x, y).
 * @param blob      Image resource
 * @param x,y       Top-left corner of the image, screen coordinates
 * @param fg_color  Tint for pure alpha formats; ignored for the others
 * @param opa       Widget opacity 0..255; the render layer multiplies the global factor once more
 * @return  1 = normal (drawn, or no overlap with the current slice so nothing to draw); 0 = failure
 * @note  Inside a wvui_class_t.draw callback only the rows within the slice are decoded; outside
 *        draw the full-screen slice applies, semantics stay correct but the whole image is decoded.
 * @note  See last_error of wvui_img_stats() for failure causes; unsupported formats, invalid headers
 *        and bad streams are all counted in the statistics, never silently skipped.
 */
uint8_t wvui_img_render_auto(const wvui_blob_t *blob, int16_t x, int16_t y,
                            wvui_color_t fg_color, uint8_t opa);

#endif /* WVUI_RENDER_H */
