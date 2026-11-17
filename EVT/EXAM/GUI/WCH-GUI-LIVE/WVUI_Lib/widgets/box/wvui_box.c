/**
 * @file wvui_box.c
 * @brief Box implementation: state, dirty marking and property access.
 * @note  Coverage, corner styles and AA live in wvui_draw_box; this file only does stateless redraw.
 */

#include "wvui_box.h"

#include "wvui_render.h"

static wvui_box_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_box_rt_t *)wvui_widget_rt(w);
}

uint8_t wvui_box_pack_corners(uint8_t lt, uint8_t rt, uint8_t lb, uint8_t rb)
{
    return (uint8_t)((lt & 0x03u) | ((rt & 0x03u) << 2) | ((lb & 0x03u) << 4) |
                     ((rb & 0x03u) << 6));
}

uint8_t wvui_box_corner_of(uint8_t packed, uint8_t corner)
{
    return (uint8_t)((packed >> (corner * 2u)) & 0x03u);
}

static void box_init(wvui_widget_t *w)
{
    wvui_box_rt_t *rt = rt_of(w);

    rt->fill = WVUI_RGB565(40, 44, 52);
    rt->border = WVUI_RGB565(120, 130, 140);
    rt->corners[WVUI_BOX_LT] = WVUI_BOX_ROUND;
    rt->corners[WVUI_BOX_RT] = WVUI_BOX_ROUND;
    rt->corners[WVUI_BOX_LB] = WVUI_BOX_ROUND;
    rt->corners[WVUI_BOX_RB] = WVUI_BOX_ROUND;
    rt->radius = 6u;
    rt->bw = 1u;
    rt->opa = 255u;
}

static void box_draw(wvui_widget_t *w)
{
    const wvui_box_rt_t *rt = (const wvui_box_rt_t *)wvui_widget_rt_c(w);

    if (rt->opa == 0u)
    {
        return;
    }
    wvui_draw_box(wvui_widget_rect(w), rt->corners, rt->radius, rt->bw,
                 rt->fill, rt->opa, rt->border, rt->opa);
}

/* ---- Property access (destination of the command channel, same path as the setters) ---- */
/* Properties: BG fill color, BORDER border color, BORDER_W border width, CORNERS one packed byte. */
static int8_t box_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
{
    wvui_box_rt_t *rt = rt_of(w);

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

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_box_set_colors(w, wvui_prop_u16(in), rt->border);
        return WVUI_PROP_OK;

    case WVUI_PROP_BORDER:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_box_set_colors(w, rt->fill, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_CORNERS:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_box_set_corners(w, wvui_box_corner_of(in[0], WVUI_BOX_LT),
                            wvui_box_corner_of(in[0], WVUI_BOX_RT),
                            wvui_box_corner_of(in[0], WVUI_BOX_LB),
                            wvui_box_corner_of(in[0], WVUI_BOX_RB));
        return WVUI_PROP_OK;

    case WVUI_PROP_RADIUS:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_box_set_radius(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_BORDER_W:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_box_set_border_width(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_box_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t box_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_box_rt_t *rt = (const wvui_box_rt_t *)wvui_widget_rt_c(w);

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

    case WVUI_PROP_BG:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->fill);
        return 2;

    case WVUI_PROP_BORDER:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->border);
        return 2;

    case WVUI_PROP_CORNERS:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = wvui_box_pack_corners(rt->corners[WVUI_BOX_LT], rt->corners[WVUI_BOX_RT],
                                      rt->corners[WVUI_BOX_LB], rt->corners[WVUI_BOX_RB]);
        return 1;

    case WVUI_PROP_RADIUS:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->radius;
        return 1;

    case WVUI_PROP_BORDER_W:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->bw;
        return 1;

    case WVUI_PROP_OPA:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->opa;
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_box_class = {
    .rt_size = (uint16_t)sizeof(wvui_box_rt_t),
    .draw = box_draw,
    .event = 0, /* Decorative widget: takes no touch */
    .init = box_init,
    .set_pos = 0,
    .on_delete = 0,
    .set_prop = box_set_prop,
    .get_prop = box_get_prop,
    .flags = 0u,
};

/* ---- Property setters ---- */
void wvui_box_set_colors(wvui_widget_t *w, wvui_color_t fill, wvui_color_t border)
{
    wvui_box_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_box_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->fill == fill) && (rt->border == border))
    {
        return;
    }
    rt->fill = fill;
    rt->border = border;
    wvui_widget_invalidate(w);
}

void wvui_box_set_corners(wvui_widget_t *w, uint8_t s_lt, uint8_t s_rt, uint8_t s_lb, uint8_t s_rb)
{
    wvui_box_rt_t *b;

    if (!WVUI_W_IS_TYPE(w, &wvui_box_class))
    {
        return;
    }
    b = rt_of(w);
    if ((b->corners[WVUI_BOX_LT] == s_lt) && (b->corners[WVUI_BOX_RT] == s_rt) &&
        (b->corners[WVUI_BOX_LB] == s_lb) && (b->corners[WVUI_BOX_RB] == s_rb))
    {
        return;
    }
    b->corners[WVUI_BOX_LT] = (uint8_t)(s_lt & 0x03u);
    b->corners[WVUI_BOX_RT] = (uint8_t)(s_rt & 0x03u);
    b->corners[WVUI_BOX_LB] = (uint8_t)(s_lb & 0x03u);
    b->corners[WVUI_BOX_RB] = (uint8_t)(s_rb & 0x03u);
    wvui_widget_invalidate(w);
}

void wvui_box_set_corner(wvui_widget_t *w, uint8_t corner, uint8_t style)
{
    wvui_box_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_box_class) || (corner > WVUI_BOX_RB))
    {
        return;
    }
    rt = rt_of(w);
    style = (uint8_t)(style & 0x03u);
    if (rt->corners[corner] == style)
    {
        return;
    }
    rt->corners[corner] = style;
    wvui_widget_invalidate(w);
}

void wvui_box_set_radius(wvui_widget_t *w, uint8_t radius)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_box_class))
    {
        return;
    }
    if (rt_of(w)->radius != radius)
    {
        rt_of(w)->radius = radius;
        wvui_widget_invalidate(w);
    }
}

void wvui_box_set_border_width(wvui_widget_t *w, uint8_t bw)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_box_class))
    {
        return;
    }
    if (rt_of(w)->bw != bw)
    {
        rt_of(w)->bw = bw;
        wvui_widget_invalidate(w);
    }
}

void wvui_box_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_box_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}
