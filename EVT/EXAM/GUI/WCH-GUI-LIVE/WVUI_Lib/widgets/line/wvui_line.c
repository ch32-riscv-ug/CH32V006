/**
 * @file wvui_line.c
 * @brief  Line implementation: bounding box, dirtying and property access.
 * @note   Coverage, AA ringing and end caps live in wvui_draw_line; this file draws no background.
 */

#include "wvui_line.h"

#include "wvui_render.h"

static wvui_line_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_line_rt_t *)wvui_widget_rt(w);
}

/* Bounding box = endpoints blown up by half the line width + 2 px AA margin;
 * endpoints may sit outside the widget rectangle */
static wvui_rect_t line_bbox(const wvui_line_rt_t *rt)
{
    wvui_rect_t r;
    int16_t ext = (int16_t)((rt->width >> 1) + 2);

    r.x0 = (int16_t)(((rt->x0 < rt->x1) ? rt->x0 : rt->x1) - ext);
    r.y0 = (int16_t)(((rt->y0 < rt->y1) ? rt->y0 : rt->y1) - ext);
    r.x1 = (int16_t)(((rt->x0 > rt->x1) ? rt->x0 : rt->x1) + ext);
    r.y1 = (int16_t)(((rt->y0 > rt->y1) ? rt->y0 : rt->y1) + ext);
    return r;
}

void wvui_line_reset_points(wvui_widget_t *w)
{
    wvui_line_rt_t *rt = rt_of(w);

    if (w->w >= w->h)
    {
        rt->x0 = w->x;
        rt->x1 = (int16_t)(w->x + w->w - 1);
        rt->y0 = (int16_t)(w->y + (w->h >> 1));
        rt->y1 = rt->y0;
    }
    else
    {
        rt->y0 = w->y;
        rt->y1 = (int16_t)(w->y + w->h - 1);
        rt->x0 = (int16_t)(w->x + (w->w >> 1));
        rt->x1 = rt->x0;
    }
}

static void line_init(wvui_widget_t *w)
{
    wvui_line_rt_t *rt = rt_of(w);

    rt->color = WVUI_RGB565(136, 136, 136);
    rt->width = 1u;
    rt->cap = WVUI_LINE_CAP_BUTT;
    rt->opa = 255u;
    wvui_line_reset_points(w);
}

static void line_draw(wvui_widget_t *w)
{
    const wvui_line_rt_t *rt = (const wvui_line_rt_t *)wvui_widget_rt_c(w);

    if ((rt->opa == 0u) || (rt->width == 0u))
    {
        return;
    }

    /* Clip to the widget rectangle: parts of the line outside are not drawn */
    wvui_render_set_clip(wvui_widget_rect(w));
    wvui_draw_line(rt->x0, rt->y0, rt->x1, rt->y1, rt->width,
                  (wvui_line_cap_t)rt->cap, rt->color, rt->opa);
    wvui_render_clear_clip();
}

/* ---- Property read/write (command channel, same path as the setters) ---- */
/* POS and SIZE must recompute the default endpoints, otherwise only the
 * bounding box gets longer while the line keeps its old length. */
static int8_t line_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
{
    switch (prop)
    {
    case WVUI_PROP_POS:
        if (len != 4u) { return WVUI_PROP_ERR_ARG; }
        wvui_widget_set_pos(w, wvui_prop_i16(in), wvui_prop_i16(in + 2));
        wvui_line_reset_points(w);
        wvui_widget_invalidate(w);
        return WVUI_PROP_OK;

    case WVUI_PROP_SIZE:
        if (len != 4u) { return WVUI_PROP_ERR_ARG; }
        w->w = wvui_prop_i16(in);
        w->h = wvui_prop_i16(in + 2);
        wvui_line_reset_points(w);
        wvui_widget_invalidate(w);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_line_set_color(w, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_line_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t line_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_line_rt_t *rt = (const wvui_line_rt_t *)wvui_widget_rt_c(w);

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

    case WVUI_PROP_COLOR:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->color);
        return 2;

    case WVUI_PROP_OPA:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->opa;
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_line_class = {
    .rt_size = (uint16_t)sizeof(wvui_line_rt_t),
    .draw = line_draw,
    .event = 0, /* decorative widget: no touch input */
    .init = line_init,
    .set_pos = 0,
    .on_delete = 0,
    .set_prop = line_set_prop,
    .get_prop = line_get_prop,
    .flags = 0u,
};

/* ---- Property setters: dirty old, change value, dirty new; moving or
 * resizing leaves no stale pixels ---- */
void wvui_line_set_points(wvui_widget_t *w, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    wvui_line_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_line_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->x0 == x0) && (rt->y0 == y0) && (rt->x1 == x1) && (rt->y1 == y1))
    {
        return;
    }

    wvui_widget_invalidate_rect(w, line_bbox(rt));
    rt->x0 = x0;
    rt->y0 = y0;
    rt->x1 = x1;
    rt->y1 = y1;
    wvui_widget_invalidate_rect(w, line_bbox(rt));
}

void wvui_line_set_width(wvui_widget_t *w, uint8_t width)
{
    wvui_line_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_line_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rt->width == width)
    {
        return;
    }
    wvui_widget_invalidate_rect(w, line_bbox(rt));
    rt->width = width;
    wvui_widget_invalidate_rect(w, line_bbox(rt));
}

void wvui_line_set_cap(wvui_widget_t *w, uint8_t cap)
{
    wvui_line_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_line_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->cap == cap) || (cap > WVUI_LINE_CAP_ROUND))
    {
        return;
    }
    wvui_widget_invalidate_rect(w, line_bbox(rt));
    rt->cap = cap;
    wvui_widget_invalidate_rect(w, line_bbox(rt));
}

void wvui_line_set_color(wvui_widget_t *w, wvui_color_t color)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_line_class))
    {
        return;
    }
    if (rt_of(w)->color != color)
    {
        rt_of(w)->color = color;
        wvui_widget_invalidate(w);
    }
}

void wvui_line_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_line_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}
