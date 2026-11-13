/**
 * @file wvui_driver.c
 * @brief Driver implementation: port transactions, chunked PFB screen pushes and render stats.
 * @note  Set the window once per rectangle and push chunks in row order; a rectangle
 *        wider than the PFB falls back to one transaction per horizontal band.
 */

#include "wvui_driver.h"
#include "wvui_anim.h"
#include "wvui_arena.h"
#include "wvui_cmd.h"
#include "wvui_dirty.h"
#include "wvui_input.h"
#include "wvui_layer.h" /* top-level chain (draw order) */
#include "wvui_page.h"
#include "wvui_render.h"
#include "wvui_slice.h"
#include "wvui_widget.h"

/* Color depth is fixed to RGB565: wvui_color_t is 16-bit.
 * To switch depth, change the typedef in wvui_types.h and this constant. */
#define WVUI_BYTES_PER_PIXEL (2u)

/* WVUI_STAT: compiled out entirely (virtual clock advance included) when
 * WVUI_CFG_ENABLE_RENDER_STATS == 0. Stats off cost zero ROM/RAM. */
#if WVUI_CFG_ENABLE_RENDER_STATS
#define WVUI_STAT(code) do { code; } while (0)
#else
#define WVUI_STAT(code) ((void)0)
#endif

typedef struct
{
    wvui_port_t  port;      /* Port copy */
    wvui_cfg_t   cfg;
    wvui_info_t  info;

    /* Gone entirely when stats are off, no RAM; wvui_stats() then returns an all-zero const copy. */
#if WVUI_CFG_ENABLE_RENDER_STATS
    wvui_stats_t stats;
#endif

    uint8_t  ready;

    /* Virtual clock in us. For stats and overlap checks only, not for pixels. */
    uint64_t now_us;
    uint64_t busy_until_us; /* Expected finish time of the async transfer */

    /* Q16 reciprocals of the timing model: one division in init, the hot path
 * only multiplies. 0 = not modeled. */
    uint32_t cpu_us_per_px_q16;
    uint32_t bus_us_per_byte_q16;

    /* Transient arena pointer: wvui_transient_reset() rewinds the cursor; the
 * persistent area is owned by the widget module. */
#if (WVUI_CFG_MEM_MODE == WVUI_MEM_ARENA)
    wvui_arena_t *transient;
#endif

    /* Input port (optional); the kernel calls read once per frame. */
    const wvui_input_port_t *input;

    /* Screen update hold depth. A counter, not a flag: wvui_render_batch may nest
 * inside the app's own hold. */
    uint16_t hold_depth;
} wvui_drv_t;

static wvui_drv_t s_d;

/* ---- Small helpers ---- */

static uint8_t is_pow2(uint8_t v)
{
    return (uint8_t)((v != 0u) && ((v & (uint8_t)(v - 1u)) == 0u));
}

#if WVUI_CFG_ENABLE_RENDER_STATS
/* Pixels -> modeled CPU us (Q16 reciprocal multiply, no division on the hot path). */
static uint32_t us_from_pixels(uint32_t pixels)
{
    if (s_d.cpu_us_per_px_q16 == 0u)
    {
        return 0u;
    }
    return (uint32_t)(((uint64_t)pixels * (uint64_t)s_d.cpu_us_per_px_q16) >> 16);
}

/* Bytes -> modeled bus us (bus_bytes_per_ms == 0 means not modeled). */
static uint32_t us_from_bytes(uint32_t bytes)
{
    if (s_d.bus_us_per_byte_q16 == 0u)
    {
        return 0u;
    }
    return (uint32_t)(((uint64_t)bytes * (uint64_t)s_d.bus_us_per_byte_q16) >> 16);
}
#endif /* WVUI_CFG_ENABLE_RENDER_STATS */

/* Align expansion: x0 rounds down and x1 rounds up to align-1 (endpoints included).
 * Valid for power-of-2 alignment only, checked by wvui_init(). */
static wvui_rect_t align_expand(wvui_rect_t r)
{
    if (s_d.port.align_x > 1u)
    {
        uint16_t m = (uint16_t)(s_d.port.align_x - 1u);
        r.x0 = (int16_t)((uint16_t)r.x0 & (uint16_t)~m);
        r.x1 = (int16_t)((uint16_t)r.x1 | m);
    }
    if (s_d.port.align_y > 1u)
    {
        uint16_t m = (uint16_t)(s_d.port.align_y - 1u);
        r.y0 = (int16_t)((uint16_t)r.y0 & (uint16_t)~m);
        r.y1 = (int16_t)((uint16_t)r.y1 | m);
    }
    return r;
}

/* The functions below are only called via WVUI_STAT: compiled out with stats off
 * to silence unused-function warnings. */
#if WVUI_CFG_ENABLE_RENDER_STATS

/* Start of a transfer. Async: advances in the background, busy_until records the
 * expected finish time; blocking port: flush already waits inside, so advance the
 * clock and count it into wait_us. */
static void stat_bus_flush(uint32_t pixels, uint32_t chunks)
{
    uint32_t bytes = pixels * WVUI_BYTES_PER_PIXEL;
    uint32_t us = us_from_bytes(bytes);

    s_d.stats.flush_calls += chunks;
    s_d.stats.bus_bytes += (uint64_t)bytes;
    s_d.stats.bus_us += (uint64_t)us;

    if (s_d.port.async)
    {
        uint64_t start = (s_d.now_us > s_d.busy_until_us) ? s_d.now_us : s_d.busy_until_us;
        s_d.busy_until_us = start + (uint64_t)us;
    }
    else
    {
        s_d.now_us += (uint64_t)us;
        s_d.stats.wait_us += (uint64_t)us;
    }
}

/* Wait for an in-flight transfer to finish (stats semantics). */
static void stat_bus_settle(void)
{
    if (s_d.now_us < s_d.busy_until_us)
    {
        s_d.stats.wait_us += (s_d.busy_until_us - s_d.now_us);
        s_d.now_us = s_d.busy_until_us;
    }
}

static void stat_cpu_render(uint32_t pixels)
{
    uint32_t us = us_from_pixels(pixels);
    s_d.stats.cpu_us += (uint64_t)us;
    s_d.now_us += (uint64_t)us;
}

#endif /* WVUI_CFG_ENABLE_RENDER_STATS */

/* ---- Chunk jobs ----
 * SOLID: one solid chunk; TREE: walk the object chain and issue one draw per
 * widget intersecting the slice. */
typedef enum
{
    RENDER_SOLID = 0,
    RENDER_TREE = 1
} render_kind_t;

typedef struct
{
    render_kind_t kind;
    wvui_rect_t content; /* SOLID only, not expanded */
    wvui_color_t color;
} render_job_t;

/* Recursive descent: a container draws itself first, then walks the subtree with
 * the clip narrowed step by step. The kernel owns clipping; set_clip in a widget's
 * draw may only narrow the clip given by the kernel.
 * Returns the draw calls made for this subtree, counted into draw_calls. */
static uint32_t draw_widget_in_chunk(wvui_widget_t *w, wvui_rect_t clip, wvui_rect_t slice)
{
    wvui_rect_t mine;
    wvui_rect_t sub;
    uint32_t calls = 0u;

    if (WVUI_W_CLASS(w) == 0)
    {
        return 0u;
    }
    /* Hidden: skip the whole subtree. */
    if ((w->flags & WVUI_WFLAG_HIDDEN) != 0u)
    {
        return 0u;
    }

    mine = wvui_widget_rect(w);
    sub = wvui_rect_clip(clip, mine);
    if ((sub.x0 > sub.x1) || (sub.y0 > sub.y1))
    {
        return 0u; /* Clipped away by an ancestor */
    }
    if (wvui_rect_intersect(sub, slice) == 0u)
    {
        return 0u; /* Outside the slice: skip the whole subtree */
    }

    if (WVUI_W_CLASS(w)->draw != 0)
    {
        wvui_render_set_clip(sub);
        WVUI_W_CLASS(w)->draw(w);
        wvui_render_clear_clip(); /* Reset to the full slice */
        calls++;
    }

    if ((WVUI_W_CLASS(w)->flags & WVUI_CLASS_CHILD_OWNER) != 0u)
    {
        wvui_widget_t *c;
        for (c = *wvui_widget_children(w); c != 0; c = c->next)
        {
            calls += draw_widget_in_chunk(c, sub, slice);
        }
    }
    return calls;
}

/* Draw the top-level chain by slice, not culled by its own rectangle: popups
 * paint beyond the rectangle, so the rectangle is not the draw extent.
 * The clip is set to the slice; widgets narrow it via wvui_render_set_clip(). */
static uint32_t draw_layer_widget_in_chunk(wvui_widget_t *w, wvui_rect_t slice)
{
    uint32_t calls = 0u;

    if ((WVUI_W_CLASS(w) == 0) || (WVUI_W_CLASS(w)->draw == 0))
    {
        return 0u;
    }
    wvui_render_set_clip(slice);
    WVUI_W_CLASS(w)->draw(w);
    wvui_render_clear_clip();
    calls++;

    if ((WVUI_W_CLASS(w)->flags & WVUI_CLASS_CHILD_OWNER) != 0u)
    {
        wvui_widget_t *c;

        for (c = *wvui_widget_children(w); c != 0; c = c->next)
        {
            calls += draw_widget_in_chunk(c, slice, slice);
        }
    }
    return calls;
}

static void draw_tree_in_chunk(void)
{
    const wvui_slice_t *sl = wvui_draw_slice();
    wvui_rect_t slice;
    wvui_rect_t screen;
    wvui_widget_t *w;
    uint32_t calls = 0u;

    if (wvui_render_in_chunk() == 0u)
    {
        return;
    }

/* Slice and rect differ in field order: copy field by field, no pointer cast. */
    slice.x0 = sl->x0;
    slice.y0 = sl->y0;
    slice.x1 = sl->x1;
    slice.y1 = sl->y1;

    /* Ancestor visible area uses the full int16 range (no ancestor clipping on the
 * root chain); parts off screen are cut by the slice. */
    screen.x0 = -32768;
    screen.y0 = -32768;
    screen.x1 = 32767;
    screen.y1 = 32767;

    /* The page fade factor applies to normal layers only; the top-level chain lives
     * across pages, so reset it to 255 before drawing.
     * Without a fade, wvui_page_opa() returns 255. */
    {
        uint8_t page_opa = wvui_page_opa();
        if (page_opa < 255u)
        {
            wvui_render_set_opa_scale(page_opa);
        }
    }
    for (w = wvui_widget_first(); w != 0; w = w->next)
    {
        calls += draw_widget_in_chunk(w, screen, slice);
    }
    wvui_render_set_opa_scale(255u);

    /* The top-level chain draws after normal layers; chain order is the z-order,
 * no layer number needed. */
    for (w = wvui_layer_first(); w != 0; w = w->next)
    {
        calls += draw_layer_widget_in_chunk(w, slice);
    }
    WVUI_STAT(s_d.stats.draw_calls += calls);
}

static void render_chunk(const render_job_t *job)
{
    if (job->kind == RENDER_SOLID)
    {
        /* Precondition: wvui_render_begin_chunk just set the clip to the full slice.
         * Skip the background fill when content covers the slice; alignment-expanded
         * edges do not cover it, so paint the background first. */
        wvui_rect_t chunk = wvui_render_clip();

        if ((job->content.x0 <= chunk.x0) && (job->content.y0 <= chunk.y0) &&
            (job->content.x1 >= chunk.x1) && (job->content.y1 >= chunk.y1))
        {
            /* fill_rect clips to the slice, so fill the whole chunk directly */
            wvui_render_fill_rect(job->content, job->color);
        }
        else
        {
            wvui_render_fill_slice(s_d.cfg.bg); /* Background first */
            wvui_render_fill_rect(job->content, job->color);
        }
    }
    else
    {
        wvui_render_fill_slice(s_d.cfg.bg); /* Background first */
        draw_tree_in_chunk();
    }

#if WVUI_CFG_DEBUG_DIRTY_RECT
    /* Dirty-rect visualization (debug): drawn after the content, per slice only the
 * visible part. TREE jobs only. */
    if (job->kind == RENDER_TREE)
    {
        wvui_render_rect_outline(job->content, WVUI_RGB565(255, 0, 255));
    }
#endif
}

/* ---- One transaction ----
 * One set_window for the whole rectangle plus N chunked flushes.
 * win = alignment-expanded window; job = what each chunk draws
 * (alignment-expanded edges keep only background color). */
static void push_transaction(wvui_rect_t win, const render_job_t *job, uint32_t chunk_pixels,
                            uint16_t rows)
{
    wvui_rect_t qa = win;
    int16_t y;
    uint8_t ping = 0;             /* Double buffering: which buffer is being written */
    uint8_t pfb_busy[2];

    /* rows comes from the caller (precomputed in init), no division here.
     * Normal path: rows = chunk_rows = chunk pixels / screen width, a multiple of
     * align_y; for any rect_w <= screen width, rows * rect_w <= chunk_pixels, so the
     * PFB holds. Rectangles wider than the PFB use rows = 1 (fallback path, counted). */
    if (rows < 1u)
    {
        rows = 1u;
    }

    pfb_busy[0] = 0u;
    pfb_busy[1] = 0u;

    /* Set the window once for the whole rectangle; then push sequentially through
 * auto-incrementing window addressing. */
    s_d.port.set_window(qa.x0, qa.y0, qa.x1, qa.y1);
    WVUI_STAT(s_d.stats.set_window_calls++);
    WVUI_STAT(s_d.stats.fixed_us += (uint64_t)s_d.cfg.fixed_cost_us_per_rect);
    WVUI_STAT(s_d.now_us += (uint64_t)s_d.cfg.fixed_cost_us_per_rect);

    s_d.port.flush_begin();

    for (y = qa.y0; y <= qa.y1; y = (int16_t)(y + (int16_t)rows))
    {
        wvui_rect_t chunk = qa;
        wvui_color_t *plane;
        uint32_t pixels;

        chunk.y0 = y;
        chunk.y1 = (int16_t)(y + (int16_t)rows - 1);
        if (chunk.y1 > qa.y1)
        {
            chunk.y1 = qa.y1;
        }

        if (s_d.info.double_buffer && pfb_busy[ping])
        {
            /* Wait for a buffer's transfer to finish before reusing that PFB.
             * Blocking ports never reach this branch (double buffering applies to async
             * ports only). */
            s_d.port.flush_wait();
            WVUI_STAT(stat_bus_settle());
            pfb_busy[0] = 0u;
            pfb_busy[1] = 0u;
        }

        plane = s_d.cfg.pfb + (s_d.info.double_buffer ? ((uint32_t)ping * chunk_pixels) : 0u);
        pixels = (uint32_t)wvui_rect_w(&chunk) * (uint32_t)wvui_rect_h(&chunk);

        /* Stateless CPU redraw: paint the background, then the content; the SOLID content
         * rectangle is not expanded, so alignment-expanded edges keep background only. */
        wvui_render_begin_chunk(plane, chunk_pixels, chunk);
        render_chunk(job);
        wvui_render_end_chunk();

        WVUI_STAT(stat_cpu_render(pixels));

        s_d.port.flush(plane, pixels);
        WVUI_STAT(stat_bus_flush(pixels, 1u));

        if (s_d.info.double_buffer)
        {
            pfb_busy[ping] = 1u;
            ping = (uint8_t)(ping ^ 1u);
        }
    }

    /* End the transaction: wait for the last transfer, then flush_end
 * (the last two of the five phases). */
    s_d.port.flush_wait();
    WVUI_STAT(stat_bus_settle());
    s_d.port.flush_end();
}

/* ---- Public API ---- */

int wvui_init(const wvui_port_t *port, const wvui_cfg_t *cfg)
{
    uint32_t half;
    uint32_t chunk_pixels;
    uint16_t pfb_lines;
    uint16_t chunk_rows;
    uint8_t use_db;

    if ((port == 0) || (cfg == 0) || (cfg->pfb == 0))
    {
        return WVUI_ERR_ARG;
    }
    /* All five phase callbacks must be present (blocking ports supply empty stubs);
 * the hot path performs no NULL checks. */
    if ((port->set_window == 0) || (port->flush_begin == 0) || (port->flush == 0) ||
        (port->flush_wait == 0) || (port->flush_end == 0))
    {
        return WVUI_ERR_ARG;
    }
    if ((cfg->w < 1) || (cfg->h < 1) || (cfg->pfb_pixels < 1u))
    {
        return WVUI_ERR_CFG;
    }

    /* Port capabilities can only be checked at runtime. */
    if ((is_pow2(port->align_x) == 0u) || (is_pow2(port->align_y) == 0u))
    {
        return WVUI_ERR_ALIGN;
    }
    if ((((uint16_t)cfg->w % port->align_x) != 0u) || (((uint16_t)cfg->h % port->align_y) != 0u))
    {
        return WVUI_ERR_ALIGN;
    }

    /* Double buffering applies to async ports only and needs one row per buffer;
 * otherwise use the whole PFB. */
    half = cfg->pfb_pixels / 2u;
    if ((port->async != 0u) && (cfg->double_buffer != 0u) && (half >= (uint32_t)cfg->w))
    {
        use_db = 1u;
        chunk_pixels = half;
    }
    else
    {
        use_db = 0u;
        chunk_pixels = cfg->pfb_pixels;
    }

    pfb_lines = (uint16_t)(chunk_pixels / (uint32_t)cfg->w);
    if (pfb_lines < 1u)
    {
        pfb_lines = 1u;
    }
    chunk_rows = pfb_lines;

    /* Paged ports require chunk boundaries to fall on whole pages. */
    if (port->paged != 0u)
    {
        if (port->align_y <= 1u)
        {
            return WVUI_ERR_PAGED;
        }
        if ((chunk_rows % port->align_y) != 0u)
        {
            return WVUI_ERR_PAGED;
        }
    }

    s_d.port = *port;
    s_d.cfg = *cfg;

    /* Time-model defaults: a zero cfg field / bus field falls back to the wvui_config.h
     * constants, so projects with calibrated numbers keep them out of the app config.
     * With the macros at their default 0 this reproduces "0 = not modeled" exactly. */
    if (s_d.cfg.cpu_pixels_per_ms == 0u)
    {
        s_d.cfg.cpu_pixels_per_ms = WVUI_CFG_CPU_PX_PER_MS;
    }
    if (s_d.cfg.fixed_cost_us_per_rect == 0u)
    {
        s_d.cfg.fixed_cost_us_per_rect = WVUI_CFG_FIXED_US_PER_RECT;
    }
    if (s_d.port.bus_bytes_per_ms == 0u)
    {
        s_d.port.bus_bytes_per_ms = WVUI_CFG_BUS_BYTES_PER_MS;
    }
    s_d.info.w = cfg->w;
    s_d.info.h = cfg->h;
    s_d.info.pfb_lines = (uint16_t)(cfg->pfb_pixels / (uint32_t)cfg->w);
    if (s_d.info.pfb_lines < 1u)
    {
        s_d.info.pfb_lines = 1u;
    }
    s_d.info.chunk_rows = chunk_rows;
    s_d.info.double_buffer = use_db;
    s_d.info.async = port->async;
    s_d.info.align_x = port->align_x;
    s_d.info.align_y = port->align_y;
    s_d.info.paged = port->paged;
    s_d.info.chunk_pixels = chunk_pixels;

    /* Q16 reciprocals of the timing model: division on the cold path only. */
    s_d.cpu_us_per_px_q16 = (cfg->cpu_pixels_per_ms != 0u)
                                ? ((1000u << 16) / cfg->cpu_pixels_per_ms)
                                : 0u;
    s_d.bus_us_per_byte_q16 = (s_d.port.bus_bytes_per_ms != 0u)
                                  ? ((1000u << 16) / s_d.port.bus_bytes_per_ms)
                                  : 0u;

    s_d.hold_depth = 0u; /* Do not inherit the hold state */
    s_d.ready = 1u;
    wvui_stats_reset();
    wvui_cmd_reset();
    wvui_render_init(cfg->w, cfg->h);
    wvui_dirty_bind(cfg->w, cfg->h, port);

    /* Cost priority: explicit cfg value > WVUI_CFG_DIRTY_FIXED_COST_PX >
     * derived from fixed cost / per-pixel cost > WVUI_CFG_DIRTY_FIXED_COST_PX_FALLBACK.
     * Per-pixel cost = CPU per pixel + bus per byte x2, Q16 fixed point;
     * division only in init (cold path), the hot path just compares. */
    {
        uint32_t per_px_q16 = s_d.cpu_us_per_px_q16 + (uint32_t)(WVUI_BYTES_PER_PIXEL * s_d.bus_us_per_byte_q16);
        uint32_t fixed_px = cfg->dirty_fixed_cost_px;

        if (fixed_px == 0u)
        {
            fixed_px = (uint32_t)WVUI_CFG_DIRTY_FIXED_COST_PX;
        }
        if (fixed_px == 0u)
        {
            if (per_px_q16 > 0u)
            {
                fixed_px = (uint32_t)(((uint64_t)s_d.cfg.fixed_cost_us_per_rect << 16) /
                                      (uint64_t)per_px_q16);
            }
            if (fixed_px == 0u)
            {
                fixed_px = (uint32_t)WVUI_CFG_DIRTY_FIXED_COST_PX_FALLBACK;
            }
        }
        wvui_dirty_set_fixed_cost_pixels(fixed_px);
    }

    wvui_input_reset();
    return WVUI_OK;
}

void wvui_deinit(void)
{
    s_d.ready = 0u;
    wvui_render_deinit();
}

uint8_t wvui_ready(void)
{
    return s_d.ready;
}

const wvui_info_t *wvui_info(void)
{
    return &s_d.info;
}

void wvui_stats_reset(void)
{
#if WVUI_CFG_ENABLE_RENDER_STATS
    uint32_t i;
    uint8_t *p = (uint8_t *)&s_d.stats;

    for (i = 0u; i < (uint32_t)sizeof(s_d.stats); i++)
    {
        p[i] = 0u;
    }
#endif
    /* The input layer and the animation engine own their counters; clear them here
     * too so two runs of the same scene yield identical stats. */
    wvui_input_stats_reset();
    wvui_anim_stats_reset();
    s_d.now_us = 0u;
    s_d.busy_until_us = 0u;
}

const wvui_stats_t *wvui_stats(void)
{
#if WVUI_CFG_ENABLE_RENDER_STATS
    uint64_t busy = s_d.stats.cpu_us + s_d.stats.bus_us + s_d.stats.fixed_us;

    /* Animation engine counters are merged in at read time. */
    s_d.stats.anim_steps = wvui_anim_steps();
    s_d.stats.anim_active = wvui_anim_active();

    s_d.stats.wall_us = s_d.now_us;
    s_d.stats.overlap_us = (busy > s_d.now_us) ? (busy - s_d.now_us) : 0u;
    s_d.stats.idle_us = (s_d.now_us > busy) ? (s_d.now_us - busy) : 0u;
    return &s_d.stats;
#else
    /* Stats compiled out entirely: return an all-zero const copy (0 B RAM). */
    static const wvui_stats_t k_no_stats;
    return &k_no_stats;
#endif
}

/* Push an already-clipped, alignment-expanded rectangle to the screen: one
 * transaction on the normal path; if the PFB cannot hold one row, fall back to
 * horizontal re-chunking. */
static void push_any(wvui_rect_t aligned, const render_job_t *job)
{
    uint32_t chunk_pixels = s_d.info.chunk_pixels;

    WVUI_STAT(s_d.stats.rects++);
    WVUI_STAT(s_d.stats.area_pixels += wvui_rect_area(aligned));

    WVUI_ASSERT((uint32_t)wvui_rect_w(&aligned) <= chunk_pixels);
    if ((uint32_t)wvui_rect_w(&aligned) <= chunk_pixels)
    {
        push_transaction(aligned, job, chunk_pixels, s_d.info.chunk_rows);
        return;
    }

    {
        int16_t x;
        WVUI_STAT(s_d.stats.c3_splits++);
        for (x = aligned.x0; x <= aligned.x1; x = (int16_t)(x + (int16_t)chunk_pixels))
        {
            wvui_rect_t band = aligned;
            band.x0 = x;
            band.x1 = (int16_t)(x + (int16_t)chunk_pixels - 1);
            if (band.x1 > aligned.x1)
            {
                band.x1 = aligned.x1;
            }
            /* One transaction per band, each with its own set_window: the pixel stream must
             * match the window addressing order, so the window cannot be set once and split
             * horizontally. The fallback path fixes one row per band to avoid division on
             * the hot path. */
            push_transaction(band, job, chunk_pixels, 1u);
        }
    }
}

void wvui_push_rect(wvui_rect_t r, wvui_color_t c)
{
    wvui_rect_t content;
    wvui_rect_t aligned;
    render_job_t job;

    if (s_d.ready == 0u)
    {
        return;
    }

    /* 1. Clip to the screen (endpoints included) */
    content = r;
    if (content.x0 < 0) { content.x0 = 0; }
    if (content.y0 < 0) { content.y0 = 0; }
    if (content.x1 > (int16_t)(s_d.cfg.w - 1)) { content.x1 = (int16_t)(s_d.cfg.w - 1); }
    if (content.y1 > (int16_t)(s_d.cfg.h - 1)) { content.y1 = (int16_t)(s_d.cfg.h - 1); }
    if ((content.x0 > content.x1) || (content.y0 > content.y1))
    {
        return;
    }

    /* 2. Align expansion: init checked the screen size against align, so the
 * expansion stays on screen. */
    aligned = align_expand(content);

    job.kind = RENDER_SOLID;
    job.content = content;
    job.color = c;
    push_any(aligned, &job);
}

/* Consume one dirty rect within a frame: render through the object tree. */
static void push_dirty_rect(wvui_rect_t aligned)
{
    render_job_t job;

    job.kind = RENDER_TREE;
    job.content = aligned;
    job.color = s_d.cfg.bg;
    push_any(aligned, &job);
}

/* Consume all pending dirty rects: collect stats, push chunk by chunk, clear.
 * Called once at frame end. */
static void consume_dirty(void)
{
    uint8_t i;
    uint8_t n;

    if (s_d.ready == 0u)
    {
        return;
    }

    n = wvui_dirty_count();
    WVUI_STAT(s_d.stats.dirty_rects += (uint32_t)n);
    WVUI_STAT(s_d.stats.dirty_area += wvui_dirty_total_area());
    WVUI_STAT(s_d.stats.dirty_merges += wvui_dirty_merges());

    for (i = 0u; i < n; i++)
    {
        push_dirty_rect(wvui_dirty_get(i));
    }

    /* Clear only after pushing: dirty rects are the sole refresh path. */
    wvui_dirty_reset();
}

void wvui_push_screen(wvui_color_t c)
{
    wvui_rect_t r;

    r.x0 = 0;
    r.y0 = 0;
    r.x1 = (int16_t)(s_d.cfg.w - 1);
    r.y1 = (int16_t)(s_d.cfg.h - 1);
    wvui_push_rect(r, c);
}

void wvui_screen_size(int16_t *w, int16_t *h)
{
    if (w != 0)
    {
        *w = s_d.cfg.w;
    }
    if (h != 0)
    {
        *h = s_d.cfg.h;
    }
}

/* ---- Object tree, dirty-rect consumption and frame loop ---- */

void wvui_driver_bind_arena(wvui_arena_t *persistent, wvui_arena_t *transient)
{
#if (WVUI_CFG_MEM_MODE == WVUI_MEM_ARENA)
    s_d.transient = transient;
#endif
    /* One entry point for all three memory modes: in STATIC / HEAP modes,
     * wvui_widget_bind_arena() -> wvui_mem_bind() is a no-op, so switching the memory
     * model does not change the startup sequence. */
    wvui_widget_bind_arena(persistent, transient);
}

void wvui_driver_bind_input(const wvui_input_port_t *port)
{
    s_d.input = port;
}

void wvui_invalidate(wvui_rect_t r)
{
    if (s_d.ready == 0u)
    {
        return;
    }
    /* Clipping, align expansion and cost-based merging all happen in wvui_dirty_add();
 * consumption happens in one sweep at frame end. */
    wvui_dirty_add(r);
}

void wvui_invalidate_clear(void)
{
    wvui_dirty_reset();
}

/* ---- Batched updates (atomic refresh) ---- */
void wvui_render_hold(void)
{
    if (s_d.hold_depth < 0xFFFFu)
    {
        s_d.hold_depth++;
    }
}

void wvui_render_release(void)
{
    if (s_d.hold_depth > 0u)
    {
        s_d.hold_depth--;
    }
}

void wvui_render_commit(void)
{
    /* Reset the hold depth. */
    s_d.hold_depth = 0u;
    consume_dirty();
}

uint8_t wvui_render_held(void)
{
    return (s_d.hold_depth != 0u) ? 1u : 0u;
}

void wvui_render_batch(wvui_update_fn fn, void *user)
{
    wvui_render_hold();
    if (fn != 0)
    {
        fn(user);
    }
    wvui_render_commit();
}

uint16_t wvui_build(const wvui_widget_desc_t *descs, uint16_t count, uint8_t persistent)
{
    uint16_t i;
    uint16_t made = 0u;

    if (descs == 0)
    {
        return 0u;
    }
    for (i = 0u; i < count; i++)
    {
        if (wvui_widget_create(&descs[i], persistent) != 0)
        {
            made++;
        }
        /* A failed create (arena full) does not break the loop; failures count in
 * wvui_arena_fails(). */
    }
    return made;
}

void wvui_kernel_forget_widget(wvui_widget_t *w)
{
    /* Reference reclamation is centralized here: a missed wvui_anim_stop still leaves
     * no dangling node. Slots: pressed state, gesture grabber, anim nodes, the
     * top-level chain and modal slots. Every new slot of this kind must be added here. */
    wvui_input_forget(w);
    wvui_anim_forget_widget(w);
    wvui_layer_unlink_widget(w);
}

void wvui_frame(const wvui_indev_t *in)
{
    wvui_indev_t sample;

    if (s_d.ready == 0u)
    {
        return;
    }

    /* Do not clear dirty rects at frame start: clear only after consuming at frame
     * end, otherwise dirty marks made between frames would be lost.
     * Frame order: page boundary -> input -> commands -> page tick -> animation ->
     * consume dirty rects -> clear.
     * The page boundary must come first: wvui_page_apply() hands this frame's input
     * to the new page. */

    /* 0. Page boundary: a pending page switch runs here; without one this is a
 * single pointer check. */
    (void)wvui_page_apply();

    /* 1. Input: read once per the port contract when no sample is given */
    if ((in == 0) && (s_d.input != 0) && (s_d.input->read != 0))
    {
        s_d.input->read(&sample);
        in = &sample;
    }
    if (in != 0)
    {
        wvui_input_feed(in);
    }

#if WVUI_CFG_ENABLE_RENDER_STATS
    /* Input-side counters merge into the stats every frame. */
    {
        const wvui_input_stats_t *is = wvui_input_stats();
        s_d.stats.in_presses = is->presses;
        s_d.stats.in_bubble_depth = is->bubble_depth;
        s_d.stats.in_drag_asks = is->drag_asks;
        s_d.stats.in_drag_grabs = is->drag_grabs;
        s_d.stats.in_swipes = is->swipes;
        s_d.stats.in_pressed_id = is->last_pressed_id;
        s_d.stats.in_grab_id = is->last_grab_id;
        s_d.stats.in_hit_id = is->last_hit_id;
    }
#endif

    /* 2. Command channel: decode frames from the byte ring and dispatch to object
     * ops. After input and before stats: a command dirtying a property redraws
     * within this frame. */
#if WVUI_CFG_ENABLE_CMD_CHANNEL
    (void)wvui_cmd_poll();
#endif
    /* 3. Empty step: keeps the numbering fixed (frame order)
     * 4. Current page on_tick: runs before animation stepping, so animations started
     * in on_tick take their first step this frame. */
    wvui_page_tick();
    /* 5. Step the animation engine before stats: animations must count their dirty
     *    rects into this frame's dirty_rects, and after input and commands so
     *    animations just started by input take their first step this frame. */
    wvui_anim_step_all();

#if WVUI_CFG_DEBUG_PERF_STRESS
    /* Worst-case stress: re-dirty all top-level widgets every frame to measure the
     * costliest frame. Before stats: these dirty marks must count into this frame's
     * dirty_rects. */
    {
        wvui_widget_t *w;
        for (w = wvui_widget_first(); w != 0; w = w->next)
        {
            wvui_invalidate(wvui_widget_rect(w));
        }
    }
#endif

    /* 6. Stats: dirty count / total area / merges are recorded at the consumption
 * point; only the frame count ticks here. */
    WVUI_STAT(s_d.stats.frames++);

    /* 7. Consume dirty rects: render chunk by chunk and push to screen; during hold
 * only accumulate, consume next frame or at commit. */
    if (s_d.hold_depth == 0u)
    {
        consume_dirty();
    }
}

void wvui_frame_poll(void)
{
    /* The frame order lives once: do not copy the seven steps here. */
    wvui_frame(0);
}

void wvui_transient_reset(void)
{
    /* The reset sequence core lives in page_leave_current() in wvui_page.c: unlink
     * persistent pages, destroy stateless pages in place and rewind the cursor.
     * Cleanup not covered by the page layer is done by the driver: unwinding
     * transient anim nodes and rewinding the transient cursor. */
#if WVUI_CFG_ENABLE_PAGES
    wvui_page_leave();
#else
    /* Single-page mode: no page layer, so reset just unlinks the whole top-level
     * chain; screen erasure is the caller's subsequent full-screen push. */
    wvui_widget_reset_top();
#endif

    /* Unwind by address range: drop anim nodes in the transient area, covering nodes
     * missed because their owner is unreachable (the normal path already stopped
     * them via wvui_page_leave()). Needed in ARENA mode only; HEAP mode cleans up in
     * wvui_kernel_forget_widget(). */
#if (WVUI_CFG_MEM_MODE == WVUI_MEM_ARENA)
    if (s_d.transient != 0)
    {
        wvui_anim_drop_range(s_d.transient->base,
                            (s_d.transient->base != 0) ? (s_d.transient->base + s_d.transient->size)
                                                       : 0);
    }
#endif

    /* Transient release: the only spot that forks by memory model.
     * ARENA rewinds the cursor (whole block reclaimed); HEAP / STATIC are no-ops
     * (objects free themselves or stay app-owned).
     * Persistent pages run it too: widgets the app creates by hand may fall in the
     * transient area. */
    wvui_mem_transient_release();
}
