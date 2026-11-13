/**
 * @file wvui_slider.c
 * @brief Slider implementation: track, fill and knob.
 * @note  Zero division: forward pos(v) = (range · a256(v)) >> 8; the inverse x -> value bisects over 0..255.
 */

#include "wvui_slider.h"

#include "wvui_draw.h"
#include "wvui_input.h"

static wvui_slider_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_slider_rt_t *)wvui_widget_rt(w);
}

/* Knob geometry: diameter = inner height, range = inner width − diameter; v=0 hugs the left
 * end, v=255 the right end. */
static void knob_metrics(const wvui_widget_t *w, int32_t *d, int32_t *travel)
{
    int32_t hh = (int32_t)w->h;
    int32_t ww = (int32_t)w->w;
    int32_t kd = hh;

    if (kd < 2)
    {
        kd = 2;
    }
    *d = kd;
    *travel = ww - kd;
    if (*travel < 0)
    {
        *travel = 0;
    }
}

/* Value -> knob left-edge offset, same formula as draw. */
static int32_t knob_offset(const wvui_widget_t *w, uint8_t value)
{
    int32_t d;
    int32_t travel;

    knob_metrics(w, &d, &travel);
    (void)d; /* the offset depends only on the range */
    return (travel * ((int32_t)value + ((int32_t)value >> 7))) >> 8;
}

/* x -> value: bisection over 0..255 (the forward map is monotonic), 8 comparisons, zero division. */
static uint8_t value_from_x(const wvui_widget_t *w, int16_t x)
{
    int32_t d;
    int32_t travel;
    int32_t dx = (int32_t)x - (int32_t)w->x;
    uint8_t lo = 0u;
    uint8_t hi = 255u;

    knob_metrics(w, &d, &travel);
    dx -= (d >> 1); /* knob centre aligned with the finger */

    while (lo < hi)
    {
        uint8_t mid = (uint8_t)(lo + (((hi - lo) + 1u) >> 1));
        int32_t pos = (travel * ((int32_t)mid + ((int32_t)mid >> 7))) >> 8;

        if (pos <= dx)
        {
            lo = mid;
        }
        else
        {
            hi = (uint8_t)(mid - 1u);
        }
    }
    return lo;
}

static void slider_init(wvui_widget_t *w)
{
    wvui_slider_rt_t *rt = rt_of(w);

    rt->track_color = WVUI_RGB565(48, 48, 56);
    rt->fill_color = WVUI_RGB565(88, 166, 240);
    rt->knob_color = WVUI_RGB565(240, 240, 240);
    rt->value = 0u;
    rt->radius = 0u; /* 0 = capsule */
    rt->opa = 255u;
    rt->enabled = 1u;
    rt->pressed = 0u;
    rt->on_change = 0;
    rt->user = 0;
}

static void slider_draw(wvui_widget_t *w)
{
    const wvui_slider_rt_t *rt = (const wvui_slider_rt_t *)wvui_widget_rt_c(w);
    wvui_rect_t track = wvui_widget_rect(w); /* track = widget rect */
    wvui_rect_t fill;
    wvui_rect_t knob;
    int32_t d;
    int32_t travel;
    int32_t off;
    uint8_t r;
    uint8_t rmax;

    if ((rt->opa == 0u) || (wvui_rect_is_empty(&track) != 0u))
    {
        return;
    }

    knob_metrics(w, &d, &travel);
    (void)travel;

    /* Effective radius: 0 = capsule (half height); beyond half height clamps to it, the fill
     * cap uses this r too. */
    rmax = (uint8_t)(d >> 1);
    r = rt->radius;
    if ((r == 0u) || (r > rmax))
    {
        r = rmax;
    }

    /* Track: full-height rounded rectangle */
    wvui_draw_round_rect(track, r, 0u, rt->track_color, rt->opa, 0u, 0u);

    off = knob_offset(w, rt->value);

    /* Fill: the right-end round cap is concentric and same-diameter with the knob.
     * x1 = x0 + off + d/2 + r − 1 puts the cap centre on the knob centre; r ≤ d/2 keeps the
     * whole cap inside the knob disc. */
    if (rt->value > 0u)
    {
        fill.x0 = track.x0;
        fill.y0 = track.y0;
        fill.x1 = (int16_t)(track.x0 + off + (d >> 1) + (int32_t)r - 1);
        fill.y1 = track.y1;
        wvui_draw_round_rect(fill, r, 0u, rt->fill_color, rt->opa, 0u, 0u);
    }

    /* Knob: full-height circle on top of track and fill; d ≤ h keeps it inside the widget rect. */
    knob.x0 = (int16_t)(track.x0 + off);
    knob.y0 = track.y0;
    knob.x1 = (int16_t)(knob.x0 + d - 1);
    knob.y1 = (int16_t)(knob.y0 + d - 1);
    wvui_draw_round_rect(knob, (uint8_t)(d >> 1), 0u, rt->knob_color, rt->opa, 0u, 0u);
}

/* ---- Events: jump on press, follow while held, end on release; DRAG_BEGIN is not handled ---- */
static uint8_t slider_event(wvui_widget_t *w, wvui_event_t e, wvui_indev_t *d)
{
    wvui_slider_rt_t *rt = rt_of(w);

    switch (e)
    {
    case WVUI_EVENT_PRESSED:
        if (rt->enabled == 0u)
        {
            return 0u;
        }
        rt->pressed = 1u;
        if (d != 0)
        {
            wvui_slider_set_value(w, value_from_x(w, d->x)); /* press point becomes the target value */
        }
        wvui_widget_invalidate(w);
        return 1u;

    case WVUI_EVENT_STAY:
        if (rt->enabled == 0u)
        {
            return 0u;
        }
        if (d != 0)
        {
            wvui_slider_set_value(w, value_from_x(w, d->x)); /* follow the finger */
        }
        return 1u;

    case WVUI_EVENT_RELEASED:
        if (rt->pressed != 0u)
        {
            rt->pressed = 0u;
            wvui_widget_invalidate(w);
        }
        return (rt->enabled != 0u) ? 1u : 0u;

    case WVUI_EVENT_CLICKED:
        if (rt->enabled == 0u)
        {
            return 0u;
        }
        if (rt->pressed != 0u)
        {
            rt->pressed = 0u;
            wvui_widget_invalidate(w);
        }
        return 1u;

    default:
        return 0u;
    }
}

/* ---- Property access: BG = track color, COLOR = fill color, KNOB = knob color ---- */
static int8_t slider_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
{
    wvui_slider_rt_t *rt = rt_of(w);

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
        wvui_slider_set_value(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_slider_set_colors(w, wvui_prop_u16(in), rt->fill_color, rt->knob_color);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_slider_set_colors(w, rt->track_color, wvui_prop_u16(in), rt->knob_color);
        return WVUI_PROP_OK;

    case WVUI_PROP_KNOB:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_slider_set_colors(w, rt->track_color, rt->fill_color, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_RADIUS:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_slider_set_radius(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_slider_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_ENABLED:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_slider_set_enabled(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_PRESSED:
        return WVUI_PROP_ERR_RO;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t slider_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_slider_rt_t *rt = (const wvui_slider_rt_t *)wvui_widget_rt_c(w);

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

    case WVUI_PROP_BG:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->track_color);
        return 2;

    case WVUI_PROP_COLOR:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->fill_color);
        return 2;

    case WVUI_PROP_KNOB:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->knob_color);
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
        out[0] = rt->enabled;
        return 1;

    case WVUI_PROP_PRESSED:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->pressed;
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_slider_class = {
    .rt_size = (uint16_t)sizeof(wvui_slider_rt_t),
    .draw = slider_draw,
    .event = slider_event,
    .init = slider_init,
    .set_pos = 0,
    .on_delete = 0,
    .set_prop = slider_set_prop,
    .get_prop = slider_get_prop,
    .flags = 0u,
};

/* ---- Setters ---- */
void wvui_slider_set_value(wvui_widget_t *w, uint8_t value)
{
    wvui_slider_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_slider_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rt->value == value)
    {
        return;
    }
    rt->value = value;
    wvui_widget_invalidate(w);
    if (rt->on_change != 0)
    {
        rt->on_change(w, value, rt->user);
    }
}

void wvui_slider_set_colors(wvui_widget_t *w, wvui_color_t track, wvui_color_t fill,
                           wvui_color_t knob)
{
    wvui_slider_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_slider_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->track_color == track) && (rt->fill_color == fill) && (rt->knob_color == knob))
    {
        return;
    }
    rt->track_color = track;
    rt->fill_color = fill;
    rt->knob_color = knob;
    wvui_widget_invalidate(w);
}

void wvui_slider_set_radius(wvui_widget_t *w, uint8_t radius)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_slider_class))
    {
        return;
    }
    if (rt_of(w)->radius != radius)
    {
        rt_of(w)->radius = radius;
        wvui_widget_invalidate(w);
    }
}

void wvui_slider_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_slider_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

void wvui_slider_set_enabled(wvui_widget_t *w, uint8_t enabled)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_slider_class))
    {
        return;
    }
    if (rt_of(w)->enabled != (uint8_t)(enabled != 0u))
    {
        rt_of(w)->enabled = (uint8_t)(enabled != 0u);
        wvui_widget_invalidate(w);
    }
}

void wvui_slider_set_change_cb(wvui_widget_t *w, wvui_slider_change_cb_t cb, void *user)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_slider_class))
    {
        return;
    }
    rt_of(w)->on_change = cb;
    rt_of(w)->user = user;
}

uint8_t wvui_slider_value(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_slider_class))
    {
        return 0u;
    }
    return ((const wvui_slider_rt_t *)wvui_widget_rt_c(w))->value;
}

uint8_t wvui_slider_is_pressed(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_slider_class))
    {
        return 0u;
    }
    return ((const wvui_slider_rt_t *)wvui_widget_rt_c(w))->pressed;
}
