#ifndef WVUI_DRIVER_H
#define WVUI_DRIVER_H

/**
 * @file wvui_driver.h
 * @brief Driver layer: port transactions, PFB chunked screen push, frame loop and render stats.
 * @note  Stateless redraw: every chunk paints the background first, then the content; no incremental push.
 */

#include <stdint.h>
#include "wvui_config.h"
#include "wvui_types.h"
#include "wvui_port.h"
#include "wvui_widget.h" /* object tree and descriptors */

/** Return codes of wvui_init(). */
#define WVUI_OK           (0)  /**< Success */
#define WVUI_ERR_ARG      (-1) /**< NULL argument, or incomplete phase callbacks */
#define WVUI_ERR_CFG      (-2) /**< Invalid screen size or PFB capacity */
#define WVUI_ERR_ALIGN    (-3) /**< Alignment not a power of two, or size not an integral multiple of it */
#define WVUI_ERR_PAGED    (-4) /**< Chunk rows not a multiple of align_y, or align_y == 1 */

/** @brief Initialization parameters. */
typedef struct
{
    int16_t w;                /**< Screen width in pixels; >= 1 */
    int16_t h;                /**< Screen height in pixels; >= 1 */
    wvui_color_t bg;           /**< Background color; starting point of every chunk repaint */
    wvui_color_t *pfb;         /**< Static PFB buffer, owned by the caller */
    uint32_t pfb_pixels;      /**< PFB capacity in pixels; >= 1 */
    uint8_t double_buffer;    /**< 1 = allow double buffering; needs an async port and room for two rows */

    /** Timing model of the statistics; affects stats only, never pixels. A zero field
     *  uses the WVUI_CFG_* defaults from wvui_config.h (which are 0 = not modeled unless
     *  the project defines calibrated constants). */
    uint32_t cpu_pixels_per_ms;      /**< Render speed, pixels per millisecond */
    uint32_t fixed_cost_us_per_rect; /**< Fixed cost per transaction, microseconds */

    /** Cost-based merge constant: equivalent pixels counted for one saved transaction.
     *  0 = resolve in order: WVUI_CFG_DIRTY_FIXED_COST_PX, the timing model, the
     *  compile-time fallback; >0 = used as is. */
    uint32_t dirty_fixed_cost_px;
} wvui_cfg_t;

/**
 * @brief Effective configuration; written by wvui_init(), read-only afterwards.
 */
typedef struct
{
    int16_t w, h;                    /**< Screen size in pixels */
    uint16_t pfb_lines;              /**< Number of PFB rows given by the caller */
    uint16_t chunk_rows;             /**< Chunk row count; halved with double buffering */
    uint8_t double_buffer;           /**< 1 = double buffering actually in use */
    uint8_t async;                   /**< Async capability of the port, passed through as is */
    uint8_t align_x, align_y, paged; /**< Port capabilities: alignment and paging */
    uint32_t chunk_pixels;           /**< Chunk pixel limit; halved with double buffering */
} wvui_info_t;

/**
 * @brief Render statistics counters, read-only.
 * @note  With WVUI_CFG_ENABLE_RENDER_STATS == 0 the struct is kept and every increment is
 *        compiled out; reads return 0.
 */
typedef struct
{
    uint32_t rects;            /**< Number of pushed rectangles */
    uint32_t area_pixels;      /**< Pushed area in pixels (after alignment expansion) */
    uint32_t set_window_calls; /**< Transaction count, equal to set_window calls */
    uint32_t flush_calls;      /**< Number of chunked stream flushes */
    uint32_t c3_splits;        /**< Number of horizontal second-chunk fallbacks */
    uint32_t dirty_rects;      /**< Number of stored dirty rectangles (after merge) */
    uint32_t dirty_area;       /**< Stored dirty area in pixels (after alignment expansion) */
    uint32_t dirty_merges;     /**< Number of cost-based merges */
    uint32_t draw_calls;       /**< Number of draw callback calls, one per slice */
    uint32_t frames;           /**< Number of frames */

    /** Counters of the central animation engine, merged in when wvui_stats() reads. */
    uint32_t anim_steps;       /**< Cumulative number of steps */
    uint16_t anim_active;      /**< Number of active nodes */

    /** Input-side counters, merged in by wvui_frame() every frame. */
    uint32_t in_presses;       /**< Number of press captures */
    uint32_t in_bubble_depth;  /**< Cumulative bubble depth */
    uint32_t in_drag_asks;     /**< Number of DRAG_BEGIN queries */
    uint32_t in_drag_grabs;    /**< Number of gesture takeovers */
    uint32_t in_swipes;        /**< Number of SWIPE_* events */
    uint16_t in_pressed_id;    /**< Captured object id; 0 = none */
    uint16_t in_grab_id;       /**< Grabber object id; 0 = none */
    uint16_t in_hit_id;        /**< Object id hit by the press */

    uint64_t bus_bytes;        /**< Number of bytes pushed to the port */
    uint64_t cpu_us;           /**< Modeled CPU render time, microseconds */
    uint64_t bus_us;           /**< Modeled bus occupancy time, microseconds */
    uint64_t fixed_us;         /**< Cumulative transaction fixed cost, microseconds */
    uint64_t wait_us;          /**< CPU waiting on the bus; blocking port = bus_us */
    uint64_t wall_us;          /**< Total virtual clock time, microseconds */
    uint64_t overlap_us;       /**< (cpu + bus + fixed) - wall; >0 = real overlap */
    uint64_t idle_us;          /**< Idle time; 0 for pure screen push */
} wvui_stats_t;

/**
 * @brief Initialize and validate the port capabilities.
 * @param port  Port implementation; all five phase callbacks must be non-NULL
 * @param cfg   Initialization parameters; pfb is owned by the caller
 * @return      WVUI_OK, or WVUI_ERR_ARG / WVUI_ERR_CFG / WVUI_ERR_ALIGN / WVUI_ERR_PAGED
 * @note        align_x / align_y must be powers of two (1 = no alignment) and the screen
 *              size an integral multiple of each; with paged != 0 also align_y > 1 and
 *              chunk rows a multiple of align_y.
 * @note        On success call wvui_driver_bind_arena() first, then create widgets.
 */
int wvui_init(const wvui_port_t *port, const wvui_cfg_t *cfg);

/**
 * @brief Unbind everything.
 * @note  The PFB stays owned by the caller; counters are cleared on the next wvui_init().
 */
void wvui_deinit(void);

/**
 * @brief Query whether the driver is initialized.
 * @return 1 = initialized; 0 = not initialized
 */
uint8_t wvui_ready(void);

/**
 * @brief Push a solid rectangle immediately: clip -> alignment expansion -> chunking -> port transaction.
 * @param r  Target rectangle, screen coordinates, inclusive; out-of-bounds parts are clipped
 * @param c  Fill color
 * @note  Does not touch the dirty rectangle table; regular refresh goes through invalidate + frame loop.
 * @note  Edge pixels added by alignment expansion get the background color; a rectangle wider
 *        than the PFB is chunked horizontally again.
 */
void wvui_push_rect(wvui_rect_t r, wvui_color_t c);

/**
 * @brief Push the whole screen as one solid color.
 * @param c  Fill color
 */
void wvui_push_screen(wvui_color_t c);

/**
 * @brief Get the screen size set by wvui_init().
 * @param w  Output width in pixels; may be NULL
 * @param h  Output height in pixels; may be NULL
 */
void wvui_screen_size(int16_t *w, int16_t *h);

/**
 * @brief Get the read-only view of the render statistics counters.
 * @return Pointer to the internal statistics; readable even before initialization
 * @note   Merges in the animation and input counters on the call and refreshes wall_us / overlap_us.
 * @note   When statistics are compiled out, returns a constant copy of all zeros.
 */
const wvui_stats_t *wvui_stats(void);

/**
 * @brief Clear the counters and reset the virtual clock.
 * @note  Does not reset configuration or the port binding; wvui_init() calls it once internally.
 */
void wvui_stats_reset(void);

/**
 * @brief Get the read-only view of the effective configuration.
 * @return Pointer to the internal wvui_info_t; lifetime same as the driver
 */
const wvui_info_t *wvui_info(void);

/**
 * @brief Bind the object memory arenas.
 * @param persistent  Persistent arena; may be NULL (pure transient page mode)
 * @param transient   Transient arena; cursor resets to zero when a page leaves
 * @note  Must be called after wvui_init() and before creating widgets.
 * @note  No-op under the STATIC / HEAP memory profiles.
 */
void wvui_driver_bind_arena(wvui_arena_t *persistent, wvui_arena_t *transient);

/**
 * @brief Bind the input port (optional).
 * @param port  Input port; read is called once per frame by the kernel
 * @note  Once bound, wvui_frame(NULL) and wvui_frame_poll() call port->read() once per frame;
 *        unbound, both skip input and only redraw.
 */
void wvui_driver_bind_input(const wvui_input_port_t *port);

/**
 * @brief Invalidate a rectangle: clip -> alignment expansion -> cost-based merge.
 * @param r  Dirty rectangle, screen coordinates, inclusive
 * @note  Dirty areas are consumed together at the end of the frame; widgets call it via wvui_widget_invalidate().
 * @note  No-op when uninitialized.
 */
void wvui_invalidate(wvui_rect_t r);

/**
 * @brief Drop all pending invalidation.
 * @note  Only for cases just repainted full screen (page switch, scene rebuild):
 *        the area is already covered and leftover dirty rectangles would only
 *        push extra content next frame; the normal frame loop does not call it,
 *        invalidation is the only refresh entry.
 */
void wvui_invalidate_clear(void);

/**
 * @brief Instantiate widgets in batch from a descriptor array (array order = z order).
 * @param descs       Descriptor array, const
 * @param count       Number of array elements
 * @param persistent  Non-zero = allocate from the persistent arena, 0 = from the transient one
 * @return            Number of widgets created; under count when the arena runs out
 * @note  A failed creation does not abort the loop; failures are counted in the arena failure counter.
 */
uint16_t wvui_build(const wvui_widget_desc_t *descs, uint16_t count, uint8_t persistent);

/**
 * @brief Run one frame: page boundary -> input -> commands -> page tick -> animation -> dirty rectangle consumption.
 * @param in  Input sample of this frame; non-NULL ignores the bound port, NULL samples from the
 *            bound port, and with no port bound the frame only redraws
 * @note  Dirty areas are consumed and cleared only at the end of the frame; invalidations between frames are not dropped.
 * @note  During hold, input, commands, on_tick and animation still advance; only the screen push is skipped.
 * @note  Returns immediately when uninitialized.
 */
void wvui_frame(const wvui_indev_t *in);

/**
 * @brief Run one frame, input sampled from the bound input port.
 * @note  Equivalent to wvui_frame(NULL).
 * @note  Use this for the regular frame loop; use wvui_frame() to inject a sample per frame.
 */
void wvui_frame_poll(void);

/**
 * @brief Make screen pushing pending, nested counter +1.
 * @note  While pending, dirty areas only accumulate; they still merge by cost, worst case one full-screen chunk.
 * @note  The screen keeps the old content until the next consumption after the pending state ends.
 */
void wvui_render_hold(void);

/**
 * @brief Drop one level of pending state.
 * @note  Does not push immediately: left to the end of this frame or to normal consumption in later frames.
 */
void wvui_render_release(void);

/**
 * @brief Drop all pending levels and consume the pending dirty areas on the spot.
 * @note  The nested counter is cleared in one call; also consumes dirty areas on the spot when not pending.
 * @note  Use wvui_render_release() at the end of page on_enter: commit pushes once here,
 *        the page then invalidates the full screen again, the screen may be pushed twice in full.
 */
void wvui_render_commit(void);

/**
 * @brief Query the pending state.
 * @return 1 = hold pending; 0 = not pending
 */
uint8_t wvui_render_held(void);

/** Callback type for batched updates. */
typedef void (*wvui_update_fn)(void *user);

/**
 * @brief Batched update: hold -> fn(user) -> commit.
 * @param fn    Update callback; when NULL only one hold + commit runs
 * @param user  Context passed through to fn
 * @note  fn may modify any number of widgets; commit pushes them out in one go.
 */
void wvui_render_batch(wvui_update_fn fn, void *user);

/**
 * @brief Reset the transient arena: restore the whole stateless scene in a fixed sequence.
 * @note  Sequence: release pressed state -> close modals -> drop anim nodes -> mark the erase range
 *        -> zero the cursor -> invalidate the erase (the two empty steps are kept, see wvui_page.c);
 *        the order is fixed, the empty steps must be kept explicitly.
 * @note  This function does not invalidate; the caller pushes the full screen right after.
 */
void wvui_transient_reset(void);

#endif /* WVUI_DRIVER_H */
