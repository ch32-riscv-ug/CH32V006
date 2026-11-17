/**
 * @file wvui_gauge.c
 * @brief Gauge implementation.
 * @note  Stateless redraw; value to angle uses a256 fixed-point, needle endpoints use 512-step trigonometry.
 */

#include "wvui_gauge.h"

#include "wvui_render.h"

static wvui_gauge_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_gauge_rt_t *)wvui_widget_rt(w);
}

static void gauge_init(wvui_widget_t *w)
{
    wvui_gauge_rt_t *rt = rt_of(w);

    rt->arc_color = WVUI_RGB565(88, 166, 240);
    rt->track_color = WVUI_RGB565(48, 48, 56);
    rt->needle_color = WVUI_RGB565(240, 80, 80);
    rt->a0 = 384u; /* Start points up; a1 = 128 sweeps 180° (512-step scale) */
    rt->a1 = 128u;
    rt->value = 0u;
    rt->width = 8u;
    rt->needle_w = 3u;
    rt->radius = 0u;
    rt->opa = 255u;
}

/* Center = rect center; outer radius = min(w,h)/2, or the explicit radius. */
static void gauge_geometry(const wvui_widget_t *w, const wvui_gauge_rt_t *rt,
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

/* Value to angle: a0 + ((sweep · a256(v)) >> 8), a256 fixed-point, division-free. */
static uint16_t gauge_angle_of(const wvui_gauge_rt_t *rt)
{
    uint32_t sweep = (uint32_t)((rt->a1 - rt->a0) & 0x1FFu);
    uint32_t a256 = (uint32_t)rt->value + ((uint32_t)rt->value >> 7);

    if (sweep == 0u)
    {
        sweep = 512u; /* a1 == a0 = full circle */
    }
    return (uint16_t)((rt->a0 + ((sweep * a256) >> 8)) & 0x1FFu);
}

static void gauge_draw(wvui_widget_t *w)
{
    const wvui_gauge_rt_t *rt = (const wvui_gauge_rt_t *)wvui_widget_rt_c(w);
    int16_t cx;
    int16_t cy;
    uint16_t r_out;
    uint16_t r_in;
    uint16_t a_split;
    uint32_t sweep;

    if ((rt->opa == 0u) || (w->w <= 0) || (w->h <= 0))
    {
        return;
    }
    gauge_geometry(w, rt, &cx, &cy, &r_out, &r_in);
    sweep = (uint32_t)((rt->a1 - rt->a0) & 0x1FFu);
    if (sweep == 0u)
    {
        sweep = 512u;
    }
    a_split = gauge_angle_of(rt);

    wvui_render_set_clip(wvui_widget_rect(w));

    /* Value arc and track arc are complementary: [a0, a_split] value-arc color, [a_split, a1] track-arc color.
     * Each pixel is painted once, the overlap does not darken under translucency; the two endpoint checks guard zero-length segments. */
    if (a_split != rt->a0)
    {
        wvui_draw_arc(cx, cy, r_out, r_in, rt->a0, a_split, rt->arc_color, rt->opa);
    }
    if (a_split != rt->a1)
    {
        wvui_draw_arc(cx, cy, r_out, r_in, a_split, rt->a1, rt->track_color, rt->opa);
    }

    /* Needle: drawn from the center along a_split to the middle of the ring band */
    if (rt->needle_w != 0u)
    {
        int32_t len = (int32_t)r_out - ((rt->width != 0u) ? ((int32_t)rt->width >> 1) : 0);
        int16_t ex;
        int16_t ey;

        if (len < 2)
        {
            len = 2;
        }
        ex = (int16_t)((int32_t)cx + (((int32_t)wvui_cos512(a_split) * len) >> 15));
        ey = (int16_t)((int32_t)cy + (((int32_t)wvui_sin512(a_split) * len) >> 15));
        wvui_draw_line(cx, cy, ex, ey, rt->needle_w, WVUI_CAP_ROUND, rt->needle_color, rt->opa);
    }

    wvui_render_clear_clip();
}

/* ---- Property handlers: COLOR value arc, BG track arc, KNOB needle, BORDER_W ring width ---- */
static int8_t gauge_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
{
    wvui_gauge_rt_t *rt = rt_of(w);

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
        wvui_gauge_set_value(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_gauge_set_colors(w, rt->track_color, wvui_prop_u16(in), rt->needle_color);
        return WVUI_PROP_OK;

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_gauge_set_colors(w, wvui_prop_u16(in), rt->arc_color, rt->needle_color);
        return WVUI_PROP_OK;

    case WVUI_PROP_KNOB:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_gauge_set_colors(w, rt->track_color, rt->arc_color, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_ANGLE_START:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_gauge_set_angles(w, (uint16_t)(wvui_prop_u16(in) & 0x1FFu), rt->a1);
        return WVUI_PROP_OK;

    case WVUI_PROP_ANGLE_END:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_gauge_set_angles(w, rt->a0, (uint16_t)(wvui_prop_u16(in) & 0x1FFu));
        return WVUI_PROP_OK;

    case WVUI_PROP_BORDER_W:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_gauge_set_width(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_RADIUS:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_gauge_set_radius(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_gauge_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_ENABLED: /* Needle on/off */
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_gauge_set_needle(w, (uint8_t)((in[0] != 0u) ? rt->needle_w : 0u));
        if ((in[0] != 0u) && (rt->needle_w == 0u))
        {
            wvui_gauge_set_needle(w, 3u); /* Use the default width 3 when turning on */
        }
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t gauge_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_gauge_rt_t *rt = (const wvui_gauge_rt_t *)wvui_widget_rt_c(w);

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
        out[0] = rt->value;
        return 1;

    case WVUI_PROP_COLOR:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->arc_color);
        return 2;

    case WVUI_PROP_BG:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->track_color);
        return 2;

    case WVUI_PROP_KNOB:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->needle_color);
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

    case WVUI_PROP_ENABLED:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = (uint8_t)((rt->needle_w != 0u) ? 1u : 0u);
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_gauge_class = {
    .rt_size = (uint16_t)sizeof(wvui_gauge_rt_t),
    .draw = gauge_draw,
    .event = 0, /* Decorative widget: does not intercept gestures */
    .init = gauge_init,
    .set_pos = 0,
    .on_delete = 0,
    .set_prop = gauge_set_prop,
    .get_prop = gauge_get_prop,
    .flags = 0u,
};

/* ---- Property setters ---- */
void wvui_gauge_set_value(wvui_widget_t *w, uint8_t value)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_gauge_class))
    {
        return;
    }
    if (rt_of(w)->value != value)
    {
        rt_of(w)->value = value;
        wvui_widget_invalidate(w);
    }
}

void wvui_gauge_set_angles(wvui_widget_t *w, uint16_t a0, uint16_t a1)
{
    wvui_gauge_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_gauge_class))
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

void wvui_gauge_set_colors(wvui_widget_t *w, wvui_color_t track, wvui_color_t arc,
                          wvui_color_t needle)
{
    wvui_gauge_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_gauge_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->track_color == track) && (rt->arc_color == arc) && (rt->needle_color == needle))
    {
        return;
    }
    rt->track_color = track;
    rt->arc_color = arc;
    rt->needle_color = needle;
    wvui_widget_invalidate(w);
}

void wvui_gauge_set_width(wvui_widget_t *w, uint8_t width)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_gauge_class))
    {
        return;
    }
    if (rt_of(w)->width != width)
    {
        rt_of(w)->width = width;
        wvui_widget_invalidate(w);
    }
}

void wvui_gauge_set_needle(wvui_widget_t *w, uint8_t width)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_gauge_class))
    {
        return;
    }
    if (rt_of(w)->needle_w != width)
    {
        rt_of(w)->needle_w = width;
        wvui_widget_invalidate(w);
    }
}

void wvui_gauge_set_radius(wvui_widget_t *w, uint8_t radius)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_gauge_class))
    {
        return;
    }
    if (rt_of(w)->radius != radius)
    {
        rt_of(w)->radius = radius;
        wvui_widget_invalidate(w);
    }
}

void wvui_gauge_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_gauge_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

uint8_t wvui_gauge_value(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_gauge_class))
    {
        return 0u;
    }
    return ((const wvui_gauge_rt_t *)wvui_widget_rt_c(w))->value;
}

uint16_t wvui_gauge_angle(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_gauge_class))
    {
        return 0u;
    }
    return gauge_angle_of((const wvui_gauge_rt_t *)wvui_widget_rt_c(w));
}
