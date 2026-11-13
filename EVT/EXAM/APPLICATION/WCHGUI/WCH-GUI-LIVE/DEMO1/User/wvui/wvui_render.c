/**
 * @file wvui_render.c
 * @brief Render layer implementation: holds the current slice and the PFB buffer to write.
 * @note  Hot path has no float, no malloc, no / or %; clipping is all integer comparison.
 */

#include "wvui_render.h"
#include "wvui_slice.h"

typedef struct
{
    wvui_color_t *plane;      /* PFB buffer to write for the current chunk */
    uint32_t plane_pixels;   /* Its capacity, in pixels */
    wvui_slice_t slice;       /* Current slice, screen coordinates, inclusive */
    wvui_rect_t clip;         /* Effective clip = slice intersect set_clip, inclusive */
    wvui_slice_t full;        /* Full-screen slice: returned by wvui_draw_slice() outside chunks */
    int16_t stride;          /* PFB row stride == slice width */
    uint8_t in_chunk;        /* 1 = chunk rendering in progress */
    uint8_t opa_scale;       /* Global opacity factor; 255 = no effect */
} wvui_render_state_t;

/* Explicit opa_scale = 255: the .bss default 0 = fully transparent, and a missed
 * wvui_render_init() would then write not a single pixel. */
static wvui_render_state_t s_r = { .opa_scale = 255u };

void wvui_render_init(int16_t screen_w, int16_t screen_h)
{
    s_r.plane = 0;
    s_r.plane_pixels = 0;
    s_r.stride = 0;
    s_r.in_chunk = 0;
    s_r.opa_scale = 255u;

    s_r.full.x0 = 0;
    s_r.full.y0 = 0;
    s_r.full.x1 = (int16_t)(screen_w - 1);
    s_r.full.y1 = (int16_t)(screen_h - 1);
    s_r.slice = s_r.full;
}

void wvui_render_deinit(void)
{
    s_r.plane = 0;
    s_r.plane_pixels = 0;
    s_r.stride = 0;
    s_r.in_chunk = 0;
    s_r.slice = s_r.full;
}

void wvui_render_begin_chunk(wvui_color_t *plane, uint32_t plane_pixels, wvui_rect_t chunk)
{
    s_r.plane = plane;
    s_r.plane_pixels = plane_pixels;
    s_r.slice.x0 = chunk.x0;
    s_r.slice.y0 = chunk.y0;
    s_r.slice.x1 = chunk.x1;
    s_r.slice.y1 = chunk.y1;
    s_r.stride = wvui_rect_w(&chunk);
    s_r.in_chunk = 1;

    /* Default clip = the whole slice; widgets can only narrow it. */
    s_r.clip = chunk;
}

void wvui_render_set_clip(wvui_rect_t r)
{
    /* Store current clip intersected with r: narrowing only. Intersect with the current
     * clip, not with the slice, so a child's set_clip cannot overwrite the container's clip. */
    s_r.clip = wvui_rect_clip(s_r.clip, r);
}

void wvui_render_clear_clip(void)
{
    s_r.clip.x0 = s_r.slice.x0;
    s_r.clip.y0 = s_r.slice.y0;
    s_r.clip.x1 = s_r.slice.x1;
    s_r.clip.y1 = s_r.slice.y1;
}

wvui_rect_t wvui_render_clip(void)
{
    return s_r.clip;
}

void wvui_render_end_chunk(void)
{
    s_r.plane = 0;
    s_r.plane_pixels = 0;
    s_r.in_chunk = 0;
    s_r.slice = s_r.full;
    s_r.stride = 0;
}

uint8_t wvui_render_in_chunk(void)
{
    return s_r.in_chunk;
}

/* Return the current slice during chunk rendering, otherwise the full-screen slice; never NULL. */
const wvui_slice_t *wvui_draw_slice(void)
{
    return s_r.in_chunk ? &s_r.slice : &s_r.full;
}

/* ---- Row fill ---- */
/* PFB stores host byte order: on little-endian two adjacent pixels share one 32-bit
 * word, so 2 pixels are written at a time. 4-byte misaligned positions write 16-bit;
 * on non-little-endian targets the whole section is compiled out. */
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define WVUI_FILL_WORD_OK 1
/* Use may_alias instead of memcpy: the latter degrades to a byte-wise copy when it
 * cannot prove alignment; the check below guarantees alignment. */
typedef uint32_t wvui_fill_u32_t __attribute__((may_alias));
#else
#define WVUI_FILL_WORD_OK 0
#endif

static void fill_span(wvui_color_t *dst, uint32_t n, wvui_color_t c)
{
    uint32_t i = 0u;

    if ((dst == 0) || (n == 0u))
    {
        return;
    }

#if WVUI_FILL_WORD_OK
    /* Pad to 4-byte alignment first, at most one pixel */
    if ((((uintptr_t)dst) & 3u) != 0u)
    {
        dst[0] = c;
        i = 1u;
    }

    if ((i + 1u) < n)
    {
        uint32_t w = ((uint32_t)c << 16) | (uint32_t)c;
        uint32_t pairs = (n - i) >> 1u;
        wvui_fill_u32_t *d = (wvui_fill_u32_t *)(void *)&dst[i];
        uint32_t k;

        for (k = 0u; k < pairs; k++)
        {
            d[k] = w;
        }
        i += (pairs << 1u);
    }
#endif

    /* Remaining 0 or 1 pixel; non-little-endian targets take this branch */
    for (; i < n; i++)
    {
        dst[i] = c;
    }
}

void wvui_render_fill_slice(wvui_color_t c)
{
    uint32_t n;

    if ((s_r.plane == 0) || (s_r.in_chunk == 0))
    {
        return;
    }

    n = (uint32_t)s_r.stride * (uint32_t)wvui_slice_h(&s_r.slice);
    if (n > s_r.plane_pixels)
    {
        n = s_r.plane_pixels; /* Defense: the driver guarantees this never happens, release builds still write in bounds */
    }

    /* The whole slice is contiguous in the PFB, one fill_span call suffices */
    fill_span(s_r.plane, n, c);
}

void wvui_render_set_opa_scale(uint8_t scale)
{
    s_r.opa_scale = (scale == 0u) ? 0u : scale;
}

uint8_t wvui_render_opa_scale(void)
{
    return s_r.opa_scale;
}

uint8_t wvui_render_opa_apply(uint8_t opa)
{
    uint32_t s = (uint32_t)s_r.opa_scale;

    if (s >= 255u)
    {
        return opa; /* Common case: zero overhead */
    }
    /* s + 1 makes s == 255 the identity, so full opacity is never clipped */
    return (uint8_t)(((uint32_t)opa * (s + 1u)) >> 8);
}

wvui_color_t wvui_render_blend(wvui_color_t fg, wvui_color_t bg, uint8_t opa)
{
    uint32_t a;
    uint32_t ia;
    uint32_t r;
    uint32_t g;
    uint32_t b;

    if (opa == 0u)
    {
        return bg;
    }
    if (opa == 255u)
    {
        return fg;
    }

    /* a256 = alpha + (alpha >> 7): maps 0..255 to 0..256, >> 8 equals /255, no division. */
    a = (uint32_t)opa + ((uint32_t)opa >> 7);
    ia = 256u - a;

    r = ((((uint32_t)(fg >> 11) & 0x1Fu) * a) + (((uint32_t)(bg >> 11) & 0x1Fu) * ia)) >> 8;
    g = ((((uint32_t)(fg >> 5) & 0x3Fu) * a) + (((uint32_t)(bg >> 5) & 0x3Fu) * ia)) >> 8;
    b = ((((uint32_t)fg & 0x1Fu) * a) + (((uint32_t)bg & 0x1Fu) * ia)) >> 8;

    return (wvui_color_t)(((r & 0x1Fu) << 11) | ((g & 0x3Fu) << 5) | (b & 0x1Fu));
}

void wvui_render_pixel(int16_t x, int16_t y, wvui_color_t c, uint8_t opa)
{
    uint32_t idx;
    uint8_t a;

    if ((s_r.plane == 0) || (s_r.in_chunk == 0))
    {
        return;
    }
    if ((x < s_r.clip.x0) || (x > s_r.clip.x1) || (y < s_r.clip.y0) || (y > s_r.clip.y1))
    {
        return; /* outside the clip, no write */
    }

    idx = ((uint32_t)(y - s_r.slice.y0) * (uint32_t)s_r.stride) + (uint32_t)(x - s_r.slice.x0);
    if (idx >= s_r.plane_pixels)
    {
        return;
    }

    a = wvui_render_opa_apply(opa);
    if (a == 0u)
    {
        return;
    }
    s_r.plane[idx] = (a == 255u) ? c : wvui_render_blend(c, s_r.plane[idx], a);
}

void wvui_render_fill_rect_blend(wvui_rect_t r, wvui_color_t c, uint8_t opa)
{
    int16_t y;
    int16_t x0;
    int16_t x1;
    uint8_t a = wvui_render_opa_apply(opa);

    if ((s_r.plane == 0) || (s_r.in_chunk == 0) || (a == 0u))
    {
        return;
    }

    if (r.x0 < s_r.clip.x0) { x0 = s_r.clip.x0; } else { x0 = r.x0; }
    if (r.x1 > s_r.clip.x1) { x1 = s_r.clip.x1; } else { x1 = r.x1; }
    if ((x0 > x1) || (r.y0 > s_r.clip.y1) || (r.y1 < s_r.clip.y0))
    {
        return;
    }

    for (y = r.y0; y <= r.y1; y++)
    {
        wvui_color_t *row;
        uint32_t n;

        if ((y < s_r.clip.y0) || (y > s_r.clip.y1))
        {
            continue;
        }
        row = s_r.plane + ((uint32_t)(y - s_r.slice.y0) * (uint32_t)s_r.stride);
        row += (uint32_t)(x0 - s_r.slice.x0);
        n = (uint32_t)(x1 - x0 + 1);

        if (a == 255u)
        {
            fill_span(row, n, c);
        }
        else
        {
            uint32_t i;

            for (i = 0u; i < n; i++)
            {
                row[i] = wvui_render_blend(c, row[i], a);
            }
        }
    }
}

void wvui_render_fill_rect(wvui_rect_t r, wvui_color_t c)
{
    int16_t y;
    int16_t x0, x1;

    if ((s_r.plane == 0) || (s_r.in_chunk == 0))
    {
        return;
    }

    /* Route through the blend path while the global factor is active; only that makes container fade-out correct. */
    if (s_r.opa_scale < 255u)
    {
        wvui_render_fill_rect_blend(r, c, 255u);
        return;
    }

    /* Intersect with the current clip; rows and columns outside are skipped without writing. */
    if (r.x0 < s_r.clip.x0) { x0 = s_r.clip.x0; } else { x0 = r.x0; }
    if (r.x1 > s_r.clip.x1) { x1 = s_r.clip.x1; } else { x1 = r.x1; }
    if ((x0 > x1) || (r.y0 > s_r.clip.y1) || (r.y1 < s_r.clip.y0))
    {
        return;
    }

    for (y = r.y0; y <= r.y1; y++)
    {
        wvui_color_t *row;

        if ((y < s_r.clip.y0) || (y > s_r.clip.y1))
        {
            continue;
        }

        /* Row start pointer = (y - slice top) * stride + (x0 - slice left). */
        row = s_r.plane + ((uint32_t)(y - s_r.slice.y0) * (uint32_t)s_r.stride);
        row += (uint32_t)(x0 - s_r.slice.x0);

        fill_span(row, (uint32_t)(x1 - x0 + 1), c);
    }
}

void wvui_render_rect_outline(wvui_rect_t r, wvui_color_t c)
{
    wvui_rect_t seg;

    if ((r.x0 > r.x1) || (r.y0 > r.y1))
    {
        return;
    }

    /* Four 1-pixel edges, one fill_rect call each: a dirty rectangle spanning
     * slices draws only its visible part in each slice. */
    seg = r;
    seg.y1 = r.y0;
    wvui_render_fill_rect(seg, c);

    seg = r;
    seg.y0 = r.y1;
    wvui_render_fill_rect(seg, c);

    seg = r;
    seg.x1 = r.x0;
    wvui_render_fill_rect(seg, c);

    seg = r;
    seg.x0 = r.x1;
    wvui_render_fill_rect(seg, c);
}
