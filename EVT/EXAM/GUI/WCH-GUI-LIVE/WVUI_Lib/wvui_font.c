/**
 * @file wvui_font.c
 * @brief Font and text engine implementation.
 * @note  No division in the draw loop: row stride = (box_w * bpp + 7) >> 3,
 *        alpha scaling uses multiplication.
 */

#include "wvui_font.h"
#include "wvui_asset.h"
#include "wvui_render.h"

/* Return the code point and advance *p; an invalid sequence skips 1 byte and
 * returns 0. Exposed: the in-file font and the bitmap glyph backend share the
 * same code point rules. */
uint32_t wvui_utf8_next(const char **p)
{
    const uint8_t *s = (const uint8_t *)*p;
    uint32_t cp;
    uint8_t n;
    uint8_t i;

    if (s[0] < 0x80u)
    {
        *p += 1;
        return (uint32_t)s[0];
    }
    if ((s[0] & 0xE0u) == 0xC0u) { cp = (uint32_t)(s[0] & 0x1Fu); n = 1u; }
    else if ((s[0] & 0xF0u) == 0xE0u) { cp = (uint32_t)(s[0] & 0x0Fu); n = 2u; }
    else if ((s[0] & 0xF8u) == 0xF0u) { cp = (uint32_t)(s[0] & 0x07u); n = 3u; }
    else { *p += 1; return 0u; }

    for (i = 1u; i <= n; i++)
    {
        if ((s[i] & 0xC0u) != 0x80u)
        {
            *p += 1; /* Truncated sequence: skip 1 byte */
            return 0u;
        }
        cp = (cp << 6) | (uint32_t)(s[i] & 0x3Fu);
    }
    *p += (uint32_t)(n + 1u);
    return cp;
}

const wvui_font_glyph_t *wvui_font_glyph(const wvui_font_t *f, uint32_t cp)
{
    uint32_t index = 0u;
    uint16_t i;

    if (f == 0)
    {
        return 0;
    }

    for (i = 0u; i < f->range_count; i++)
    {
        const wvui_font_range_t *r = &f->ranges[i];
        uint32_t span = (r->end >= r->start) ? (r->end - r->start + 1u) : 0u;

        if ((cp >= r->start) && (cp <= r->end))
        {
            index += (cp - r->start);
            return (index < f->glyph_count) ? &f->glyph_desc[index] : 0;
        }
        index += span;
    }

    /* Outside the ranges search the ascending sparse table; glyph index =
 * range total + table position, binary search with >>1. */
    if ((f->sparse != 0) && (f->sparse_count > 0u))
    {
        uint16_t lo = 0u;
        uint16_t hi = f->sparse_count;

        while (lo < hi)
        {
            uint16_t mid = (uint16_t)(lo + (uint16_t)((uint16_t)(hi - lo) >> 1));
            uint32_t v = f->sparse[mid];

            if (v == cp)
            {
                index += mid;
                return (index < f->glyph_count) ? &f->glyph_desc[index] : 0;
            }
            if (v < cp)
            {
                lo = (uint16_t)(mid + 1u);
            }
            else
            {
                hi = mid;
            }
        }
    }
    return 0;
}

/* Line pitch = line_height + line_gap; the glyph box stays line_height, do not
 * mix the two. */
static int32_t font_pitch(const wvui_font_t *f)
{
    return (int32_t)f->line_height + (int32_t)f->line_gap;
}

int16_t wvui_font_line_height(const wvui_font_t *f)
{
    return (f != 0) ? (int16_t)font_pitch(f) : 0;
}

int16_t wvui_font_line_width(const wvui_font_t *f, const char *utf8)
{
    const char *p = utf8;
    int32_t w = 0;
    uint8_t have = 0u; /* A glyph was placed on this line already: tracking only sits between two glyphs */

    if ((f == 0) || (utf8 == 0))
    {
        return 0;
    }

    while (*p != '\0')
    {
        const wvui_font_glyph_t *g;
        uint32_t cp;

        if (*p == '\n')
        {
            break;
        }
        cp = wvui_utf8_next(&p);
        if (cp == 0u)
        {
            continue;
        }
        g = wvui_font_glyph(f, cp);
        if (g != 0)
        {
            if ((have != 0u) && (f->tracking != 0u))
            {
                w += (int32_t)f->tracking;
            }
            w += (int32_t)g->adv_w;
            have = 1u;
        }
    }
    return (int16_t)w;
}

void wvui_font_measure(const wvui_font_t *f, const char *utf8, int16_t *w, int16_t *h)
{    const char *p = utf8;
    int32_t cur = 0;
    int32_t maxw = 0;
    int32_t lines = 1;
    uint8_t have = 0u;

    if ((w == 0) || (h == 0))
    {
        return;
    }
    if ((f == 0) || (utf8 == 0))
    {
        *w = 0;
        *h = 0;
        return;
    }

    while (*p != '\0')
    {
        const wvui_font_glyph_t *g;
        uint32_t cp;

        if (*p == '\n')
        {
            p++;
            lines++;
            if (cur > maxw) { maxw = cur; }
            cur = 0;
            have = 0u;
            continue;
        }
        cp = wvui_utf8_next(&p);
        if (cp == 0u)
        {
            continue;
        }
        g = wvui_font_glyph(f, cp);
        if (g != 0)
        {
            if ((have != 0u) && (f->tracking != 0u))
            {
                cur += (int32_t)f->tracking;
            }
            cur += (int32_t)g->adv_w;
            have = 1u;
        }
    }
    if (cur > maxw)
    {
        maxw = cur;
    }

    *w = (int16_t)maxw;
    /* Height covers up to the last line: the last line carries no line_gap,
     * lines * line height + (lines - 1) * gap. */
    *h = (int16_t)(((int32_t)lines * (int32_t)f->line_height) +
                   (((int32_t)lines - 1) * (int32_t)f->line_gap));
}

/* Draw one glyph: row stride = (box_w * bpp + 7) >> 3, the bpp dispatch is
 * hoisted out of the row loop; 2bpp packs 4 px per byte, 4bpp 2 px per byte,
 * unpacked per byte, pixel results identical to per-pixel dispatch. */
static void draw_glyph(const wvui_font_t *f, const wvui_font_glyph_t *g, int16_t x, int16_t y,
                       wvui_color_t fg, uint8_t opa)
{
    uint32_t stride;
    uint16_t row;
    uint8_t bpp = f->bpp;
    wvui_rect_t clip;
    uint16_t row0;   /* first drawn row */
    uint16_t row1;   /* last drawn row (inclusive) */
    int16_t gx0, gx1; /* column range, screen coords, inclusive */

    if ((g->box_w == 0u) || (g->box_h == 0u))
    {
        return; /* Empty glyph: has advance but no bitmap */
    }

    /* Decode only visible rows: pixels outside the slice are not written, saving
 * decode, shift and wvui_render_pixel calls. */
    clip = wvui_render_clip();

    /* Column range: glyph spans [x, x+box_w-1], intersect with the clip */
    gx0 = x;
    gx1 = (int16_t)(x + (int16_t)g->box_w - 1);
    if (gx0 < clip.x0) { gx0 = clip.x0; }
    if (gx1 > clip.x1) { gx1 = clip.x1; }
    if (gx0 > gx1)
    {
        return; /* Glyph entirely outside the chunk */
    }

    /* Row range: intersect with the clip, then convert to row indices */
    {
        int16_t ty0 = y;
        int16_t ty1 = (int16_t)(y + (int16_t)g->box_h - 1);

        if (ty1 < clip.y0)
        {
            return;
        }
        if (ty0 > clip.y1)
        {
            return;
        }
        if (ty0 < clip.y0) { ty0 = clip.y0; }
        if (ty1 > clip.y1) { ty1 = clip.y1; }
        row0 = (uint16_t)(ty0 - y);
        row1 = (uint16_t)(ty1 - y);
    }

    stride = ((uint32_t)g->box_w * (uint32_t)bpp + 7u) >> 3u;

    for (row = row0; row <= row1; row++)
    {
        const uint8_t *bits = f->bitmap_data + g->bitmap_offset + ((uint32_t)row * stride);
        int16_t py = (int16_t)(y + (int16_t)row);
        uint16_t col = (uint16_t)(gx0 - x); /* Start at the first visible column */
        uint16_t col_end = (uint16_t)(gx1 - x);

        /* 2bpp: 4 px per byte, unpacked per byte */
        if (bpp == 2u)
        {
            /* Walk to the group boundary one pixel at a time, then enter the unpack loop;
             * do not re-align col to the group start, it would repaint pixels left of gx0. */
            while ((col <= col_end) && ((col & 3u) != 0u))
            {
                uint8_t b = bits[col >> 2];
                uint8_t a = (uint8_t)((b >> (6u - (uint8_t)(2u * (col & 3u)))) & 0x03u);

                if (a != 0u)
                {
                    wvui_render_pixel((int16_t)(x + (int16_t)col), py, fg,
                                     (uint8_t)(((uint16_t)(a * 85u) * (uint16_t)opa) >> 8));
                }
                col++;
            }
            while ((uint16_t)(col + 3u) <= col_end)
            {
                uint8_t b = bits[col >> 2];
                uint8_t k;

                for (k = 0u; k < 4u; k++)
                {
                    uint8_t a = (uint8_t)((b >> (6u - (uint8_t)(2u * k))) & 0x03u);

                    if (a != 0u)
                    {
                        /* 0..3 -> 0..255, then scaled by opa */
                        wvui_render_pixel((int16_t)(x + (int16_t)col + (int16_t)k), py, fg,
                                         (uint8_t)(((uint16_t)(a * 85u) * (uint16_t)opa) >> 8));
                    }
                }
                col = (uint16_t)(col + 4u);
            }
            /* Fewer than 4 px remain */
            for (; col <= col_end; col++)
            {
                uint8_t b = bits[col >> 2];
                uint8_t a = (uint8_t)((b >> (6u - (uint8_t)(2u * (col & 3u)))) & 0x03u);

                if (a != 0u)
                {
                    wvui_render_pixel((int16_t)(x + (int16_t)col), py, fg,
                                     (uint8_t)(((uint16_t)(a * 85u) * (uint16_t)opa) >> 8));
                }
            }
        }
        else if (bpp == 8u)
        {
            for (; col <= col_end; col++)
            {
                uint8_t a = bits[col];

                if (a != 0u)
                {
                    wvui_render_pixel((int16_t)(x + (int16_t)col), py, fg,
                                     (uint8_t)(((uint16_t)a * (uint16_t)opa) >> 8));
                }
            }
        }
        else if (bpp == 4u)
        {
            for (; col <= col_end; col++)
            {
                uint8_t b = bits[col >> 1];
                uint8_t a = (uint8_t)(((col & 1u) != 0u) ? (b & 0x0Fu) : ((b >> 4) & 0x0Fu));

                a = (uint8_t)(a * 17u); /* 0..15 -> 0..255 */
                if (a != 0u)
                {
                    wvui_render_pixel((int16_t)(x + (int16_t)col), py, fg,
                                     (uint8_t)(((uint16_t)a * (uint16_t)opa) >> 8));
                }
            }
        }
        else
        {
            for (; col <= col_end; col++)
            {
                uint8_t b = bits[col >> 3];
                uint8_t a = (uint8_t)((b >> (7u - (uint8_t)(col & 7u))) & 0x01u);

                if (a != 0u)
                {
                    wvui_render_pixel((int16_t)(x + (int16_t)col), py, fg, opa);
                }
            }
        }
    }
}

void wvui_font_draw_text(const wvui_font_t *f, int16_t x, int16_t y, const char *utf8,
                        wvui_color_t fg, uint8_t opa)
{
    const char *p = utf8;
    int16_t pen_x = x;
    int16_t line_y = y;
    uint8_t have = 0u;

    if ((f == 0) || (utf8 == 0) || (opa == 0u))
    {
        return;
    }

    while (*p != '\0')
    {
        const wvui_font_glyph_t *g;
        uint32_t cp;

        if (*p == '\n')
        {
            p++;
            pen_x = x;
            line_y = (int16_t)(line_y + (int16_t)font_pitch(f));
            have = 0u;
            continue;
        }
        cp = wvui_utf8_next(&p);
        if (cp == 0u)
        {
            continue;
        }
        g = wvui_font_glyph(f, cp);
        if (g == 0)
        {
            continue; /* No glyph: skip */
        }
        if ((have != 0u) && (f->tracking != 0u))
        {
            pen_x = (int16_t)(pen_x + (int16_t)f->tracking);
        }
        draw_glyph(f, g, (int16_t)(pen_x + g->x_ofs), (int16_t)(line_y + g->y_ofs), fg, opa);
        pen_x = (int16_t)(pen_x + (int16_t)g->adv_w);
        have = 1u;
    }
}

/* ---- Prefix metrics and drawing (for word wrapping) ---- */

int16_t wvui_font_prefix_width(const wvui_font_t *f, const char *utf8, uint16_t nbytes)
{
    const char *p = utf8;
    const char *limit;
    int16_t w = 0;
    uint16_t used = 0u;
    uint8_t have = 0u;

    if ((f == 0) || (utf8 == 0) || (nbytes == 0u))
    {
        return 0;
    }
    limit = utf8 + nbytes;
    while ((*p != '\0') && (p < limit))
    {
        const wvui_font_glyph_t *g;
        uint32_t cp;
        const char *before = p;

        if (*p == '\n')
        {
            break; /* On newline: measure this line only */
        }
        cp = wvui_utf8_next(&p);
        used = (uint16_t)(used + (uint16_t)(p - before));
        if (p > limit)
        {
            break; /* Code point crosses the limit: never split one */
        }
        if (cp == 0u)
        {
            continue;
        }
        g = wvui_font_glyph(f, cp);
        if (g == 0)
        {
            continue;
        }
        if ((have != 0u) && (f->tracking != 0u))
        {
            w = (int16_t)(w + (int16_t)f->tracking);
        }
        w = (int16_t)(w + (int16_t)g->adv_w);
        have = 1u;
    }
    return w;
}

void wvui_font_draw_prefix(const wvui_font_t *f, int16_t x, int16_t y, const char *utf8,
                          uint16_t nbytes, wvui_color_t fg, uint8_t opa)
{
    const char *p = utf8;
    const char *limit;
    int16_t pen_x = x;
    int16_t line_y = y;
    uint8_t have = 0u;

    if ((f == 0) || (utf8 == 0) || (opa == 0u) || (nbytes == 0u))
    {
        return;
    }
    limit = utf8 + nbytes;
    while ((*p != '\0') && (p < limit))
    {
        const wvui_font_glyph_t *g;
        uint32_t cp;

        if (*p == '\n')
        {
            p++;
            pen_x = x;
            line_y = (int16_t)(line_y + (int16_t)font_pitch(f));
            have = 0u;
            continue;
        }
        {
            const char *before = p;

            cp = wvui_utf8_next(&p);
            if (p > limit)
            {
                break; /* Code point crosses the limit: draw nothing */
            }
            (void)before;
        }
        if (cp == 0u)
        {
            continue;
        }
        g = wvui_font_glyph(f, cp);
        if (g == 0)
        {
            continue;
        }
        if ((have != 0u) && (f->tracking != 0u))
        {
            pen_x = (int16_t)(pen_x + (int16_t)f->tracking);
        }
        draw_glyph(f, g, (int16_t)(pen_x + g->x_ofs), (int16_t)(line_y + g->y_ofs), fg, opa);
        pen_x = (int16_t)(pen_x + (int16_t)g->adv_w);
        have = 1u;
    }
}

/* ---- Per-line metrics and line comparison ---- */

/* Pointer to the first byte of line index; NULL when the line does not exist.
 * Empty text (NULL or empty string) has no lines, same rule as
 * wvui_font_line_count(). */
static const char *line_start(const char *s, uint16_t index)
{
    if ((s == 0) || (s[0] == '\0'))
    {
        return 0;
    }
    while (index > 0u)
    {
        while ((*s != '\0') && (*s != '\n'))
        {
            s++;
        }
        if (*s == '\0')
        {
            return 0; /* Reached the end before that line */
        }
        s++;
        index--;
    }
    return s;
}

uint16_t wvui_font_line_count(const char *utf8)
{
    uint16_t n;

    if ((utf8 == 0) || (utf8[0] == '\0'))
    {
        return 0u;
    }
    n = 1u;
    while (*utf8 != '\0')
    {
        if (*utf8 == '\n')
        {
            n++;
        }
        utf8++;
    }
    return n;
}

uint8_t wvui_font_line_equal(const char *a, const char *b, uint16_t index)
{
    const char *pa = line_start(a, index);
    const char *pb = line_start(b, index);

    if ((pa == 0) || (pb == 0))
    {
        return (uint8_t)((pa == pb) ? 1u : 0u); /* Line absent on both sides = equal */
    }
    while (*pa == *pb)
    {
        if ((*pa == '\0') || (*pa == '\n'))
        {
            return 1u; /* Byte-identical up to the line end */
        }
        pa++;
        pb++;
    }
    return 0u;
}

uint8_t wvui_font_line_rect(const wvui_font_t *f, const char *utf8, uint16_t index,
                           int16_t x, int16_t width, int16_t line_top, wvui_rect_t *out)
{
    int32_t y0;

    if ((f == 0) || (out == 0) || (width <= 0))
    {
        return 0u;
    }
    if (index >= wvui_font_line_count(utf8))
    {
        return 0u; /* Line does not exist: do not dirty it */
    }

    /* Line number * pitch in int32: int16 would overflow. */
    y0 = (int32_t)line_top + ((int32_t)index * font_pitch(f));
    if ((y0 > 32767) || ((y0 + (int32_t)f->line_height - 1) > 32767))
    {
        return 0u;
    }

    out->x0 = x;
    out->y0 = (int16_t)y0;
    out->x1 = (int16_t)(x + width - 1);
    out->y1 = (int16_t)(y0 + (int32_t)f->line_height - 1);
    return 1u;
}

/* ---- Bitmap glyph backend (simplified): glyphs are assets, reuse the image pipeline ---- */

static uint8_t strip_has(const wvui_font_strip_t *s, uint8_t c)
{
    return (uint8_t)((c >= s->first_char) && (c <= s->last_char));
}

uint16_t wvui_font_strip_line_height(const wvui_font_strip_t *s)
{
    return (s != 0) ? (uint16_t)s->line_height : 0u;
}

void wvui_font_strip_measure(const wvui_font_strip_t *s, const char *utf8, int16_t *w, int16_t *h)
{
    const char *p = utf8;
    int32_t cur = 0;
    int32_t maxw = 0;
    int32_t lines = 1;

    if ((w == 0) || (h == 0))
    {
        return;
    }
    *w = 0;
    *h = 0;
    if ((s == 0) || (utf8 == 0) || (utf8[0] == '\0'))
    {
        return;
    }

    while (*p != '\0')
    {
        uint8_t c = (uint8_t)*p;

        if (c == (uint8_t)'\n')
        {
            if (cur > maxw) { maxw = cur; }
            cur = 0;
            lines++;
            p++;
            continue;
        }
        if (strip_has(s, c))
        {
            cur += (int32_t)s->adv_w;
        }
        p++; /* Characters outside the strip are skipped as glyph-less */
    }
    if (cur > maxw)
    {
        maxw = cur;
    }
    *w = (int16_t)maxw;
    *h = (int16_t)(lines * (int32_t)s->line_height);
}

void wvui_font_strip_draw(const wvui_font_strip_t *s, int16_t x, int16_t y, const char *utf8,
                         wvui_color_t fg, uint8_t opa)
{
    const char *p = utf8;
    int16_t pen_x = x;
    int16_t line_y = y;

    if ((s == 0) || (utf8 == 0) || (opa == 0u))
    {
        return;
    }

    while (*p != '\0')
    {
        uint8_t c = (uint8_t)*p;

        if (c == (uint8_t)'\n')
        {
            pen_x = x;
            line_y = (int16_t)(line_y + (int16_t)s->line_height);
            p++;
            continue;
        }
        if (strip_has(s, c))
        {
            wvui_blob_t blob;

            /* Missing asset or bad header: open returns 0 and the blob is in erased state,
             * so the glyph paints blank; the reason shows in wvui_asset_stats /
             * wvui_img_stats. */
            (void)wvui_asset_open((wvui_asset_id_t)(s->first_id + (wvui_asset_id_t)(c - s->first_char)), &blob);
            (void)wvui_img_render_auto(&blob, pen_x, (int16_t)(line_y + s->y_ofs), fg, opa);
            pen_x = (int16_t)(pen_x + (int16_t)s->adv_w);
        }
        p++;
    }
}
