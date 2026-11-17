/**
 * @file wvui_barchart.c
 * @brief Bar chart implementation.
 * @note  Stateless redraw, paints no background of its own; auto split does one division per draw.
 */

#include "wvui_barchart.h"

#include "wvui_draw.h"
#include "wvui_render.h"

static wvui_barchart_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_barchart_rt_t *)wvui_widget_rt(w);
}

static const wvui_barchart_rt_t *rt_of_c(const wvui_widget_t *w)
{
    return (const wvui_barchart_rt_t *)wvui_widget_rt_c(w);
}

/* Value → extent: span = usable px − 1, a256 fixed point, v = 255 fills exactly span. */
static int16_t value_to_extent(int16_t span, uint8_t v)
{
    int32_t a256 = (int32_t)v + ((int32_t)v >> 7); /* 0..255 → 0..256 */

    if (span <= 0)
    {
        return 0;
    }
    return (int16_t)(((int32_t)span * a256) >> 8);
}

static void draw_bar(const wvui_barchart_rt_t *rt, wvui_rect_t slot, wvui_rect_t bar)
{
    if ((rt->track_color != rt->bar_color) && (wvui_rect_is_empty(&slot) == 0u))
    {
        wvui_draw_round_rect(slot, 0u, 0u, rt->track_color, rt->opa, rt->track_color, 0u);
    }
    if (wvui_rect_is_empty(&bar) == 0u)
    {
        wvui_draw_round_rect(bar, rt->radius, 0u, rt->bar_color, rt->opa, rt->bar_color, 0u);
    }
}

static void barchart_draw(wvui_widget_t *w)
{
    const wvui_barchart_rt_t *rt = rt_of_c(w);
    wvui_rect_t box = wvui_widget_rect(w);
    int32_t slot;
    int32_t thick;
    uint16_t i;

    if ((rt->opa == 0u) || (rt->samples == 0) || (rt->count == 0u) ||
        (wvui_rect_is_empty(&box) != 0u))
    {
        return;
    }

    wvui_render_set_clip(box);

    /* Fixed bar width: no division; auto split: one division per draw. */
    if (rt->bar_w != 0u)
    {
        slot = (int32_t)rt->bar_w + (int32_t)rt->gap;
        thick = (int32_t)rt->bar_w;
    }
    else
    {
        int32_t span = (rt->vertical != 0u) ? (int32_t)wvui_rect_w(&box)
                                            : (int32_t)wvui_rect_h(&box);
        slot = span / (int32_t)rt->count;
        if (slot < 1)
        {
            slot = 1; /* More bars than pixels: 1 px each, no overlap */
        }
        thick = (slot > (int32_t)rt->gap) ? (slot - (int32_t)rt->gap) : 1;
    }
    if (thick < 1)
    {
        thick = 1;
    }

    for (i = 0u; i < rt->count; i++)
    {
        const uint8_t v = rt->samples[i];
        int32_t pos = (int32_t)((rt->vertical != 0u) ? box.x0 : box.y0) + (int32_t)i * slot;
        wvui_rect_t slab;
        wvui_rect_t bar;

        /* Truncate when it does not fit (normal path in fixed-width mode) */
        if (rt->vertical != 0u)
        {
            if (pos > (int32_t)box.x1)
            {
                break;
            }
        }
        else
        {
            if (pos > (int32_t)box.y1)
            {
                break;
            }
        }

        if (rt->vertical != 0u)
        {
            int16_t x0 = (int16_t)pos;
            int16_t x1 = (int16_t)(pos + thick - 1);
            int16_t ext;

            if (x1 > box.x1)
            {
                x1 = box.x1;
            }
            slab.x0 = x0;
            slab.y0 = box.y0;
            slab.x1 = x1;
            slab.y1 = box.y1;

            ext = value_to_extent((int16_t)(wvui_rect_h(&box) - 1), v);
            /* v == 0: no bar, track only; otherwise a 1 px dot would remain */
            bar.x0 = x0;
            bar.x1 = x1;
            bar.y0 = (v == 0u) ? (int16_t)(box.y1 + 1) : (int16_t)(box.y1 - ext);
            bar.y1 = box.y1;
        }
        else
        {
            int16_t y0 = (int16_t)pos;
            int16_t y1 = (int16_t)(pos + thick - 1);
            int16_t ext;

            if (y1 > box.y1)
            {
                y1 = box.y1;
            }
            slab.x0 = box.x0;
            slab.y0 = y0;
            slab.x1 = box.x1;
            slab.y1 = y1;

            ext = value_to_extent((int16_t)(wvui_rect_w(&box) - 1), v);
            bar.y0 = y0;
            bar.y1 = y1;
            bar.x0 = box.x0;
            bar.x1 = (v == 0u) ? (int16_t)(box.x0 - 1) : (int16_t)(box.x0 + ext);
        }

        draw_bar(rt, slab, bar);
    }

    /* Baseline drawn last: the full-height tracks would cover it; not drawn when base_color == track_color. */
    if (rt->base_color != rt->track_color)
    {
        if (rt->vertical != 0u)
        {
            wvui_draw_line(box.x0, box.y1, box.x1, box.y1, 1u, WVUI_CAP_BUTT,
                          rt->base_color, rt->opa);
        }
        else
        {
            wvui_draw_line(box.x0, box.y0, box.x0, box.y1, 1u, WVUI_CAP_BUTT,
                          rt->base_color, rt->opa);
        }
    }

    wvui_render_clear_clip();
}

/* ---- Property access: WVUI_PROP_VALUE rewrites the last bar ---- */
static int8_t barchart_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
{
    const wvui_barchart_rt_t *rt = rt_of_c(w);

    switch (prop)
    {
    case WVUI_PROP_POS:
        if (len != 4u) { return WVUI_PROP_ERR_ARG; }
        wvui_widget_set_pos(w, wvui_prop_i16(in), wvui_prop_i16(in + 2));
        return WVUI_PROP_OK;

    case WVUI_PROP_SIZE:
        if (len != 4u) { return WVUI_PROP_ERR_ARG; }
        w->w = wvui_prop_i16(in);
        w->h = wvui_prop_i16(in + 2);
        wvui_widget_invalidate(w);
        return WVUI_PROP_OK;

    case WVUI_PROP_VALUE:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        if ((rt->samples == 0) || (rt->count == 0u)) { return WVUI_PROP_ERR_ARG; }
        wvui_barchart_set(w, (uint16_t)(rt->count - 1u), in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_barchart_set_colors(w, wvui_prop_u16(in), rt->track_color);
        return WVUI_PROP_OK;

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_barchart_set_colors(w, rt->bar_color, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_BORDER:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_barchart_set_base_color(w, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_RADIUS:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_barchart_set_radius(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_barchart_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_ENABLED:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_barchart_set_vertical(w, in[0]);
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t barchart_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_barchart_rt_t *rt = rt_of_c(w);

    switch (prop)
    {
    case WVUI_PROP_POS:
        if (cap < 4u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, (uint16_t)w->x);
        wvui_prop_write_u16(out + 2, (uint16_t)w->y);
        return 4;

    case WVUI_PROP_SIZE:
        if (cap < 4u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, (uint16_t)w->w);
        wvui_prop_write_u16(out + 2, (uint16_t)w->h);
        return 4;

    case WVUI_PROP_VALUE:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        if ((rt->samples == 0) || (rt->count == 0u)) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->samples[rt->count - 1u];
        return 1;

    case WVUI_PROP_COLOR:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->bar_color);
        return 2;

    case WVUI_PROP_BG:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->track_color);
        return 2;

    case WVUI_PROP_BORDER:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->base_color);
        return 2;

    case WVUI_PROP_RADIUS:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->radius;
        return 1;

    case WVUI_PROP_OPA:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->opa;
        return 1;

    case WVUI_PROP_ENABLED:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->vertical;
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static void barchart_init(wvui_widget_t *w)
{
    wvui_barchart_rt_t *rt = rt_of(w);

    rt->samples = 0;
    rt->count = 0u;
    rt->bar_color = WVUI_RGB565(88, 166, 240);
    rt->track_color = WVUI_RGB565(38, 42, 52);
    rt->base_color = WVUI_RGB565(70, 76, 88);
    rt->bar_w = 0u; /* Default: auto even split */
    rt->gap = 2u;
    rt->radius = 0u;
    rt->vertical = 1u;
    rt->opa = 255u;
}

const wvui_class_t wvui_barchart_class = {
    .rt_size = (uint16_t)sizeof(wvui_barchart_rt_t),
    .draw = barchart_draw,
    .event = 0, /* Decorative widget: no gesture interception */
    .init = barchart_init,
    .set_pos = 0,
    .on_delete = 0,
    .set_prop = barchart_set_prop,
    .get_prop = barchart_get_prop,
    .flags = 0u,
};

/* ---- Data interface ---- */
uint8_t wvui_barchart_bind(wvui_widget_t *w, const uint8_t *samples, uint16_t count)
{
    wvui_barchart_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_barchart_class) || (samples == 0) || (count == 0u))
    {
        return 0u;
    }
    rt = rt_of(w);
    rt->samples = samples;
    rt->count = count;
    wvui_widget_invalidate(w);
    return 1u;
}

void wvui_barchart_set(wvui_widget_t *w, uint16_t index, uint8_t value)
{
    wvui_barchart_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_barchart_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->samples == 0) || (index >= rt->count))
    {
        return;
    }
    if (rt->samples[index] == value)
    {
        return; /* Value unchanged: no redraw, saves one screen flush */
    }
    /* Data is owned by the caller; changes must go through this API or the widget will not redraw. */
    ((uint8_t *)rt->samples)[index] = value;
    wvui_widget_invalidate(w);
}

void wvui_barchart_set_colors(wvui_widget_t *w, wvui_color_t bar, wvui_color_t track)
{
    wvui_barchart_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_barchart_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->bar_color == bar) && (rt->track_color == track))
    {
        return;
    }
    rt->bar_color = bar;
    rt->track_color = track;
    wvui_widget_invalidate(w);
}

void wvui_barchart_set_base_color(wvui_widget_t *w, wvui_color_t base)
{
    wvui_barchart_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_barchart_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rt->base_color != base)
    {
        rt->base_color = base;
        wvui_widget_invalidate(w);
    }
}

void wvui_barchart_set_bar(wvui_widget_t *w, uint8_t bar_w, uint8_t gap)
{
    wvui_barchart_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_barchart_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->bar_w != bar_w) || (rt->gap != gap))
    {
        rt->bar_w = bar_w;
        rt->gap = gap;
        wvui_widget_invalidate(w);
    }
}

void wvui_barchart_set_radius(wvui_widget_t *w, uint8_t radius)
{
    wvui_barchart_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_barchart_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rt->radius != radius)
    {
        rt->radius = radius;
        wvui_widget_invalidate(w);
    }
}

void wvui_barchart_set_vertical(wvui_widget_t *w, uint8_t vertical)
{
    wvui_barchart_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_barchart_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rt->vertical != (uint8_t)(vertical != 0u))
    {
        rt->vertical = (uint8_t)(vertical != 0u);
        wvui_widget_invalidate(w);
    }
}

void wvui_barchart_set_opa(wvui_widget_t *w, uint8_t opa)
{
    wvui_barchart_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_barchart_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rt->opa != opa)
    {
        rt->opa = opa;
        wvui_widget_invalidate(w);
    }
}

uint8_t wvui_barchart_value(const wvui_widget_t *w, uint16_t index)
{
    const wvui_barchart_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_barchart_class))
    {
        return 0u;
    }
    rt = rt_of_c(w);
    if ((rt->samples == 0) || (index >= rt->count))
    {
        return 0u;
    }
    return rt->samples[index];
}

uint16_t wvui_barchart_count(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_barchart_class))
    {
        return 0u;
    }
    return rt_of_c(w)->count;
}
