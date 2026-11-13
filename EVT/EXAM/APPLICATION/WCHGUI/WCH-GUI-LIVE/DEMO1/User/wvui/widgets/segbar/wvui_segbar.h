#ifndef WVUI_SEGBAR_H
#define WVUI_SEGBAR_H

/**
 * @file wvui_segbar.h
 * @brief Segment bar: a row of blocks shown through a "lit window", usable as a level bar
 *        or an endlessly scrolling marquee window.
 * @note  Decorative widget: event callback is 0, gestures are not intercepted.
 */

#include "wvui_anim.h"
#include "wvui_widget.h"

/** Block count limit: the runtime keeps it in a u8, count is clamped with it. */
#define WVUI_SEGBAR_MAX_BLOCKS (32u)

typedef struct
{
    wvui_color_t on;  /**< Lit block color */
    wvui_color_t off; /**< Unlit block color; not drawn when equal to on */
    wvui_anim_t an;   /**< Permanent anim node of the scrolling window */
    uint8_t value;   /**< Level value 0..255, affects the level mode only */
    uint8_t count;   /**< Block count 1..32; default 8 */
    uint8_t lit;     /**< In level mode, number of currently lit blocks 0..count */
    uint8_t blk_w;   /**< Single block width in pixels; default 5 */
    uint8_t blk_h;   /**< Single block height in pixels; default 8 */
    uint8_t pitch;   /**< Block pitch (step) in pixels; drawn at blk_w + 1 when smaller than blk_w + 1 */
    uint8_t radius;  /**< Corner radius in pixels; 0 = square */
    uint8_t win;     /**< Scrolling window width in blocks; 0 = level mode */
    uint8_t offset;  /**< Scrolling window start in blocks, wraps automatically */
    uint8_t period;  /**< Scroll step: frames per block; default 4, minimum 1 */
    uint8_t tick;    /**< Within-period frame counter, moves once when period is reached */
    uint8_t opa;     /**< Widget opacity 0..255; 0 = not drawn */
} wvui_segbar_rt_t;

extern const wvui_class_t wvui_segbar_class;

/**
 * @brief Set the block geometry.
 * @param count   Block count, 1..32; 0 or over the limit is clamped to the bounds
 * @param blk_w   Single block width in pixels; 0 is clamped to 1
 * @param blk_h   Single block height in pixels; 0 is clamped to 1
 * @param pitch   Block pitch in pixels; 0 = auto = blk_w + 1
 * @param radius  Corner radius in pixels
 * @note  Blocks taller than the widget are clamped to the widget height; the leftover blocks are not drawn.
 */
void wvui_segbar_set_blocks(wvui_widget_t *w, uint8_t count, uint8_t blk_w, uint8_t blk_h,
                           uint8_t pitch, uint8_t radius);

/**
 * @brief Set the level value (used in level mode).
 * @param value  0..255, lit evenly over count blocks
 * @note  Remembered even while the scrolling window is open; takes effect at once when the window closes.
 */
void wvui_segbar_set_value(wvui_widget_t *w, uint8_t value);

/**
 * @brief Open or close the scrolling window (endless scroll mode).
 * @param win            Number of lit blocks in the window; 0 = close and return to level mode,
 *                       >= count means all lit (no animation)
 * @param period_frames  Frames per block; below 1 is clamped to 1
 * @note  Opening resets the phase to zero: the window starts at block 0 and shifts right by one block every
 *        period frames, wrapping back to 0 past the last block.
 * @note  The permanent node runs only when win < count; with win >= count all blocks stay lit and the anim
 *        chain is not used.
 */
void wvui_segbar_set_window(wvui_widget_t *w, uint8_t win, uint8_t period_frames);

/**
 * @brief Set the scroll phase directly (for manual animation / command-driven use).
 * @param offset  Window start in blocks, wrapped by remainder when beyond count
 * @note  Auto-scroll keeps running: the next period expiry continues from the new phase.
 */
void wvui_segbar_set_offset(wvui_widget_t *w, uint8_t offset);

/**
 * @brief Set the lit and unlit block colors.
 * @param on   Lit block color
 * @param off  Unlit block color; not drawn when equal to on
 */
void wvui_segbar_set_colors(wvui_widget_t *w, wvui_color_t on, wvui_color_t off);

/**
 * @brief Set the opacity.
 * @param opa  Widget opacity 0..255; 0 = not drawn
 */
void wvui_segbar_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Read back the level value.
 * @return  0..255; returns 0 on a type mismatch
 */
uint8_t wvui_segbar_value(const wvui_widget_t *w);

/**
 * @brief Read back the scrolling window width.
 * @return  Block count; 0 = level mode, also returns 0 on a type mismatch
 */
uint8_t wvui_segbar_window(const wvui_widget_t *w);

/**
 * @brief Read back the scroll phase.
 * @return  Window start in blocks; returns 0 on a type mismatch
 */
uint8_t wvui_segbar_offset(const wvui_widget_t *w);

#endif /* WVUI_SEGBAR_H */
