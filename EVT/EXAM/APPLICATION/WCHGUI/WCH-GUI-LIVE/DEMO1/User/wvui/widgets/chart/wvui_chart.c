/**
 * @file wvui_chart.c
 * @brief Line chart implementation.
 * @note  Stateless redraw, no background; x = left edge + i, y uses a256 fixed-point, ring index uses a mask.
 */

#include "wvui_chart.h"

#include "wvui_draw.h"
#include "wvui_render.h"

static wvui_chart_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_chart_rt_t *)wvui_widget_rt(w);
}

/* Value to y (0 at the bottom, 255 at the top): y1 − ((h − 1) · a256(v)) >> 8, a256 fixed-point. */
static int16_t value_to_y(const wvui_widget_t *w, uint8_t v)
{
    int32_t span = (int32_t)w->h - 1;
    int32_t a256 = (int32_t)v + ((int32_t)v >> 7);

    if (span < 0)
    {
        span = 0;
    }
    return (int16_t)((int32_t)w->y + w->h - 1 - ((span * a256) >> 8));
}

/* Sample value at screen position i; in ring mode counted from the oldest sample. */
static uint8_t sample_at(const wvui_widget_t *w, const wvui_chart_rt_t *rt, uint16_t i)
{
    uint16_t idx;

    if (rt->samples == 0)
    {
        return 0u;
    }
    if ((rt->ring != 0u) && ((rt->count & (uint16_t)(rt->count - 1u)) == 0u))
    {
        idx = (uint16_t)((rt->head + i) & (uint16_t)(rt->count - 1u));
    }
    else
    {
        idx = i;
    }
    if (idx >= rt->count)
    {
        return 0u;
    }
    (void)w;
    return rt->samples[idx];
}

static void chart_init(wvui_widget_t *w)
{
    wvui_chart_rt_t *rt = rt_of(w);

    rt->samples = 0;
    rt->count = 0u;
    rt->head = 0u;
    rt->line_color = WVUI_RGB565(88, 166, 240);
    rt->base_color = WVUI_RGB565(60, 60, 66);
    rt->ring = 1u;
    rt->opa = 255u;
}

static void chart_draw(wvui_widget_t *w)
{
    const wvui_chart_rt_t *rt = (const wvui_chart_rt_t *)wvui_widget_rt_c(w);
    wvui_rect_t box = wvui_widget_rect(w);
    int16_t prev_x = 0;
    int16_t prev_y = 0;
    uint16_t n;
    uint16_t i;
    uint16_t first;

    if ((rt->opa == 0u) || (rt->samples == 0) || (rt->count == 0u) ||
        (wvui_rect_is_empty(&box) != 0u))
    {
        return;
    }

    n = rt->count;
    if (n > (uint16_t)wvui_rect_w(&box))
    {
        n = (uint16_t)wvui_rect_w(&box); /* One sample per pixel: more would not fit */
    }
    /* Samples that do not fit are dropped, the window is right-aligned; hence the start is count − n. */
    first = (uint16_t)(rt->count - n);

    wvui_render_set_clip(box);

    /* Baseline (zero value): not drawn when same color as the line */
    if (rt->base_color != rt->line_color)
    {
        wvui_draw_line(box.x0, (int16_t)(box.y1), box.x1, (int16_t)(box.y1), 1u,
                      WVUI_CAP_BUTT, rt->base_color, rt->opa);
    }

    for (i = 0u; i < n; i++)
    {
        int16_t x = (int16_t)(box.x0 + i);
        int16_t y = value_to_y(w, sample_at(w, rt, (uint16_t)(first + i)));

        if (i > 0u)
        {
            wvui_draw_line(prev_x, prev_y, x, y, 1u, WVUI_CAP_ROUND, rt->line_color, rt->opa);
        }
        prev_x = x;
        prev_y = y;
    }

    wvui_render_clear_clip();
}

/* ---- Property handlers: expose only the properties expressible as a single value ---- */
static int8_t chart_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
{
    wvui_chart_rt_t *rt = rt_of(w);

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

    case WVUI_PROP_VALUE: /* Append one sample */
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_chart_push(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_chart_set_colors(w, wvui_prop_u16(in), rt->base_color);
        return WVUI_PROP_OK;

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_chart_set_colors(w, rt->line_color, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_chart_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_ENABLED:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_chart_set_ring(w, in[0]);
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t chart_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_chart_rt_t *rt = (const wvui_chart_rt_t *)wvui_widget_rt_c(w);

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

    case WVUI_PROP_VALUE: /* Latest sample; in ring mode samples[0] is the oldest */
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        if ((rt->samples == 0) || (rt->count == 0u))
        {
            out[0] = 0u;
        }
        else if (rt->ring != 0u)
        {
            uint16_t idx = (uint16_t)((rt->head + rt->count - 1u)
                                      & (uint16_t)(rt->count - 1u));
            out[0] = rt->samples[idx];
        }
        else
        {
            out[0] = rt->samples[rt->count - 1u]; /* Linear mode: push appends at the end */
        }
        return 1;

    case WVUI_PROP_COLOR:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->line_color);
        return 2;

    case WVUI_PROP_BG:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->base_color);
        return 2;

    case WVUI_PROP_OPA:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->opa;
        return 1;

    case WVUI_PROP_ENABLED:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->ring;
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_chart_class = {
    .rt_size = (uint16_t)sizeof(wvui_chart_rt_t),
    .draw = chart_draw,
    .event = 0, /* Decorative widget: does not intercept gestures */
    .init = chart_init,
    .set_pos = 0,
    .on_delete = 0,
    .set_prop = chart_set_prop,
    .get_prop = chart_get_prop,
    .flags = 0u,
};

/* ---- Data access ---- */
uint8_t wvui_chart_bind(wvui_widget_t *w, const uint8_t *samples, uint16_t count)
{
    wvui_chart_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_chart_class) || (samples == 0) || (count == 0u))
    {
        return 0u;
    }
    /* Ring indexing relies on a mask; reject binding when count is not a power of two. */
    if ((count & (uint16_t)(count - 1u)) != 0u)
    {
        return 0u;
    }
    rt = rt_of(w);
    rt->samples = samples;
    rt->count = count;
    rt->head = 0u;
    wvui_widget_invalidate(w);
    return 1u;
}

void wvui_chart_set(wvui_widget_t *w, uint16_t index, uint8_t value)
{
    wvui_chart_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_chart_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->samples == 0) || (index >= rt->count))
    {
        return;
    }
    /* The data is owned by the caller; write through this API, otherwise the widget does not redraw. */
    ((uint8_t *)rt->samples)[index] = value;
    wvui_widget_invalidate(w);
}

void wvui_chart_push(wvui_widget_t *w, uint8_t value)
{
    wvui_chart_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_chart_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->samples == 0) || (rt->count == 0u))
    {
        return;
    }
    if (rt->ring != 0u)
    {
        ((uint8_t *)rt->samples)[rt->head] = value;
        rt->head = (uint16_t)((rt->head + 1u) & (uint16_t)(rt->count - 1u));
    }
    else
    {
        /* Linear mode: shift all left by one, append the new value at the end, drop the oldest. */
        uint16_t i;
        uint8_t *s = (uint8_t *)rt->samples;

        for (i = 1u; i < rt->count; i++)
        {
            s[i - 1u] = s[i];
        }
        s[rt->count - 1u] = value;
    }
    wvui_widget_invalidate(w);
}

void wvui_chart_clear(wvui_widget_t *w)
{
    wvui_chart_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_chart_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rt->samples != 0)
    {
        uint16_t i;
        uint8_t *s = (uint8_t *)rt->samples;

        for (i = 0u; i < rt->count; i++)
        {
            s[i] = 0u;
        }
    }
    wvui_widget_invalidate(w);
}

void wvui_chart_set_colors(wvui_widget_t *w, wvui_color_t line, wvui_color_t base)
{
    wvui_chart_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_chart_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->line_color == line) && (rt->base_color == base))
    {
        return;
    }
    rt->line_color = line;
    rt->base_color = base;
    wvui_widget_invalidate(w);
}

void wvui_chart_set_ring(wvui_widget_t *w, uint8_t ring)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_chart_class))
    {
        return;
    }
    if (rt_of(w)->ring != (uint8_t)(ring != 0u))
    {
        rt_of(w)->ring = (uint8_t)(ring != 0u);
        wvui_widget_invalidate(w);
    }
}

void wvui_chart_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_chart_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

uint16_t wvui_chart_count(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_chart_class))
    {
        return 0u;
    }
    return ((const wvui_chart_rt_t *)wvui_widget_rt_c(w))->count;
}

uint16_t wvui_chart_head(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_chart_class))
    {
        return 0u;
    }
    return ((const wvui_chart_rt_t *)wvui_widget_rt_c(w))->head;
}
