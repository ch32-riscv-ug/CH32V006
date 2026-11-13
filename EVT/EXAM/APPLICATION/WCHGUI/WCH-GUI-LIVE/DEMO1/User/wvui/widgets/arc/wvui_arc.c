/**
 * @file wvui_arc.c
 * @brief Arc implementation: center and radius derivation, dirty marking and property access.
 * @note  Geometry, anti-aliasing and angle tests live in wvui_draw_arc; this file only does stateless redraw.
 */

#include "wvui_arc.h"

#include "wvui_render.h"

static wvui_arc_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_arc_rt_t *)wvui_widget_rt(w);
}

static void arc_init(wvui_widget_t *w)
{
    wvui_arc_rt_t *rt = rt_of(w);

    rt->color = WVUI_RGB565(88, 166, 240);
    rt->a0 = 0u;
    rt->a1 = 384u; /* Default 270° */
    rt->width = 0u;
    rt->radius = 0u;
    rt->opa = 255u;
}

/* Center = rect center; outer radius clamped to 4095, inner radius floored at 0 */
static void arc_geometry(const wvui_widget_t *w, const wvui_arc_rt_t *rt,
                         int16_t *cx, int16_t *cy, uint16_t *r_out, uint16_t *r_in)
{
    int32_t ww = (int32_t)w->w;
    int32_t hh = (int32_t)w->h;
    int32_t s = (ww < hh) ? ww : hh;
    int32_t r = (rt->radius != 0u) ? (int32_t)rt->radius : (s >> 1);
    int32_t in = (rt->width != 0u) ? (r - (int32_t)rt->width) : 0;

    if (r < 1)
    {
        r = 1;
    }
    if (in < 0)
    {
        in = 0;
    }
    *cx = (int16_t)(w->x + (int16_t)(ww >> 1));
    *cy = (int16_t)(w->y + (int16_t)(hh >> 1));
    *r_out = (uint16_t)((r > 4095) ? 4095 : r);
    *r_in = (uint16_t)in;
}

static void arc_draw(wvui_widget_t *w)
{
    const wvui_arc_rt_t *rt = (const wvui_arc_rt_t *)wvui_widget_rt_c(w);
    int16_t cx;
    int16_t cy;
    uint16_t r_out;
    uint16_t r_in;

    if ((rt->opa == 0u) || (w->w <= 0) || (w->h <= 0))
    {
        return;
    }
    arc_geometry(w, rt, &cx, &cy, &r_out, &r_in);

    /* Clip to the widget rect: an explicitly enlarged radius does not spill out */
    wvui_render_set_clip(wvui_widget_rect(w));
    wvui_draw_arc(cx, cy, r_out, r_in, rt->a0, rt->a1, rt->color, rt->opa);
    wvui_render_clear_clip();
}

/* ---- Property access (destination of the command channel) ---- */
/* Properties: BORDER_W stores the ring width; ANGLE_START / ANGLE_END each take one u16 (0..511). */
static int8_t arc_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
{
    wvui_arc_rt_t *rt = rt_of(w);

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

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_arc_set_color(w, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_ANGLE_START:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_arc_set_angles(w, (uint16_t)(wvui_prop_u16(in) & 0x1FFu), rt->a1);
        return WVUI_PROP_OK;

    case WVUI_PROP_ANGLE_END:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_arc_set_angles(w, rt->a0, (uint16_t)(wvui_prop_u16(in) & 0x1FFu));
        return WVUI_PROP_OK;

    case WVUI_PROP_BORDER_W:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_arc_set_width(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_RADIUS:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_arc_set_radius(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_arc_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t arc_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_arc_rt_t *rt = (const wvui_arc_rt_t *)wvui_widget_rt_c(w);

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

    case WVUI_PROP_ANGLE_START:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->a0);
        return 2;

    case WVUI_PROP_ANGLE_END:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->a1);
        return 2;

    case WVUI_PROP_BORDER_W:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->width;
        return 1;

    case WVUI_PROP_RADIUS:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->radius;
        return 1;

    case WVUI_PROP_OPA:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->opa;
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_arc_class = {
    .rt_size = (uint16_t)sizeof(wvui_arc_rt_t),
    .draw = arc_draw,
    .event = 0, /* Decorative widget: takes no touch */
    .init = arc_init,
    .set_pos = 0,
    .on_delete = 0,
    .set_prop = arc_set_prop,
    .get_prop = arc_get_prop,
    .flags = 0u,
};

/* ---- Property setters ---- */
void wvui_arc_set_angles(wvui_widget_t *w, uint16_t a0, uint16_t a1)
{
    wvui_arc_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_arc_class))
    {
        return;
    }
    rt = rt_of(w);
    a0 = (uint16_t)(a0 & 0x1FFu);
    a1 = (uint16_t)(a1 & 0x1FFu);
    if ((rt->a0 == a0) && (rt->a1 == a1))
    {
        return;
    }
    rt->a0 = a0;
    rt->a1 = a1;
    wvui_widget_invalidate(w);
}

void wvui_arc_set_color(wvui_widget_t *w, wvui_color_t color)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_arc_class))
    {
        return;
    }
    if (rt_of(w)->color != color)
    {
        rt_of(w)->color = color;
        wvui_widget_invalidate(w);
    }
}

void wvui_arc_set_width(wvui_widget_t *w, uint8_t width)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_arc_class))
    {
        return;
    }
    if (rt_of(w)->width != width)
    {
        rt_of(w)->width = width;
        wvui_widget_invalidate(w);
    }
}

void wvui_arc_set_radius(wvui_widget_t *w, uint8_t radius)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_arc_class))
    {
        return;
    }
    if (rt_of(w)->radius != radius)
    {
        rt_of(w)->radius = radius;
        wvui_widget_invalidate(w);
    }
}

void wvui_arc_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_arc_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

uint16_t wvui_arc_start(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_arc_class))
    {
        return 0u;
    }
    return ((const wvui_arc_rt_t *)wvui_widget_rt_c(w))->a0;
}

uint16_t wvui_arc_end(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_arc_class))
    {
        return 0u;
    }
    return ((const wvui_arc_rt_t *)wvui_widget_rt_c(w))->a1;
}
