/**
 * @file wvui_indicator.c
 * @brief Status indicator dot implementation: a dot that can light up and blink.
 * @note  Division-free: radius = min(w,h) >> 1; blinking uses counter comparison, no modulo.
 */

#include "wvui_indicator.h"

#include "wvui_draw.h"
#include "wvui_render.h"

static wvui_indicator_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_indicator_rt_t *)wvui_widget_rt(w);
}

static void indicator_stop_anim(wvui_widget_t *w)
{
    wvui_indicator_rt_t *rt = rt_of(w);

    if (wvui_anim_running(&rt->an) != 0u)
    {
        wvui_anim_stop(&rt->an);
    }
}

/* Blinking: a permanent anim node returning 1 every frame; dirty is set only on phase flips. */
static uint8_t indicator_step(wvui_anim_t *a)
{
    wvui_widget_t *w = a->owner;
    wvui_indicator_rt_t *rt = rt_of(w);

    if (rt->blink_on == 0u)
    {
        return 0u; /* Blink off: returning 0 detaches the node */
    }
    rt->blink_count = (uint8_t)(rt->blink_count + 1u);
    if (rt->blink_count >= rt->blink_period)
    {
        rt->blink_count = 0u;
        rt->blink_visible = (uint8_t)(rt->blink_visible == 0u);
        wvui_widget_invalidate(w);
    }
    return 1u;
}

static void indicator_start_anim(wvui_widget_t *w)
{
    wvui_indicator_rt_t *rt = rt_of(w);

    if (rt->blink_on != 0u)
    {
        /* Permanent node: does not start without the anim tier, stays at the initial value 1 (steady on). */
        wvui_anim_start_permanent(&rt->an, w, indicator_step);
    }
}

static void indicator_init(wvui_widget_t *w)
{
    wvui_indicator_rt_t *rt = rt_of(w);

    wvui_anim_init(&rt->an);
    rt->on_color = WVUI_RGB565(80, 220, 120);
    rt->off_color = WVUI_RGB565(60, 60, 66);
    rt->on = 0u;
    rt->blink_on = 0u;
    rt->blink_period = 30u;
    rt->blink_count = 0u;
    rt->blink_visible = 1u;
    rt->radius = 0u; /* 0 = a full round dot */
    rt->opa = 255u;
    rt->clickable = 0u;
    rt->pressed = 0u;
    rt->on_change = 0;
    rt->user = 0;
}

static void indicator_delete(wvui_widget_t *w)
{
    indicator_stop_anim(w);
}

static void indicator_draw(wvui_widget_t *w)
{
    const wvui_indicator_rt_t *rt = (const wvui_indicator_rt_t *)wvui_widget_rt_c(w);
    wvui_rect_t dot = wvui_widget_rect(w);
    int32_t ww;
    int32_t hh;
    int32_t s;
    uint8_t r;
    uint8_t lit;

    if ((rt->opa == 0u) || (wvui_rect_is_empty(&dot) != 0u))
    {
        return;
    }

    ww = (int32_t)wvui_rect_w(&dot);
    hh = (int32_t)wvui_rect_h(&dot);
    s = (ww < hh) ? ww : hh;

    r = rt->radius;
    if (r == 0u)
    {
        r = (uint8_t)((s > 255) ? 255 : (s >> 1)); /* 0 = a full round dot */
    }

    lit = (uint8_t)((rt->on != 0u) && ((rt->blink_on == 0u) || (rt->blink_visible != 0u)));

    wvui_draw_round_rect(dot, r, 0u,
                        (lit != 0u) ? rt->on_color : rt->off_color, rt->opa, 0u, 0u);
}

/* ---- Events: read-only by default; after set_clickable(1), handled by press state ---- */
static uint8_t indicator_event(wvui_widget_t *w, wvui_event_t e, wvui_indev_t *d)
{
    wvui_indicator_rt_t *rt = rt_of(w);

    (void)d;

    if (rt->clickable == 0u)
    {
        return 0u; /* Read-only: no event consumed */
    }

    switch (e)
    {
    case WVUI_EVENT_PRESSED:
        if (rt->pressed == 0u)
        {
            rt->pressed = 1u;
            wvui_widget_invalidate(w);
        }
        return 1u;

    case WVUI_EVENT_STAY:
        return 1u;

    case WVUI_EVENT_RELEASED:
        if (rt->pressed != 0u)
        {
            rt->pressed = 0u;
            wvui_widget_invalidate(w);
        }
        return 1u;

    case WVUI_EVENT_CLICKED:
        if (rt->pressed != 0u)
        {
            rt->pressed = 0u;
            wvui_widget_invalidate(w);
        }
        wvui_indicator_flip(w);
        return 1u;

    default:
        return 0u;
    }
}

/* ---- Property get/set (command channel) ---- */
static int8_t indicator_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
{
    wvui_indicator_rt_t *rt = rt_of(w);

    /* BG = off color; COLOR = on color; ENABLED = clickable. */
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
        wvui_indicator_set_on(w, (uint8_t)(in[0] != 0u));
        return WVUI_PROP_OK;

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_indicator_set_colors(w, wvui_prop_u16(in), rt->on_color);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_indicator_set_colors(w, rt->off_color, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_RADIUS:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_indicator_set_radius(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_indicator_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_ENABLED:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_indicator_set_clickable(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_PRESSED:
        return WVUI_PROP_ERR_RO;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t indicator_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_indicator_rt_t *rt = (const wvui_indicator_rt_t *)wvui_widget_rt_c(w);

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
        out[0] = rt->on;
        return 1;

    case WVUI_PROP_BG:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->off_color);
        return 2;

    case WVUI_PROP_COLOR:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->on_color);
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
        out[0] = rt->clickable;
        return 1;

    case WVUI_PROP_PRESSED:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->pressed;
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_indicator_class = {
    .rt_size = (uint16_t)sizeof(wvui_indicator_rt_t),
    .draw = indicator_draw,
    .event = indicator_event, /* Read-only by default */
    .init = indicator_init,
    .set_pos = 0,
    .on_delete = indicator_delete,
    .set_prop = indicator_set_prop,
    .get_prop = indicator_get_prop,
    .flags = 0u,
};

/* ---- Property setters ---- */
void wvui_indicator_set_on(wvui_widget_t *w, uint8_t on)
{
    wvui_indicator_rt_t *rt;
    uint8_t v = (uint8_t)(on != 0u);

    if (!WVUI_W_IS_TYPE(w, &wvui_indicator_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rt->on == v)
    {
        return;
    }
    rt->on = v;
    wvui_widget_invalidate(w);
    if (rt->on_change != 0)
    {
        rt->on_change(w, v, rt->user);
    }
}

void wvui_indicator_flip(wvui_widget_t *w)
{
    const wvui_indicator_rt_t *rt = (const wvui_indicator_rt_t *)wvui_widget_rt_c(w);

    if (!WVUI_W_IS_TYPE(w, &wvui_indicator_class))
    {
        return;
    }
    wvui_indicator_set_on(w, (uint8_t)(rt->on == 0u));
}

void wvui_indicator_set_blink(wvui_widget_t *w, uint8_t blink, uint8_t period_frames)
{
    wvui_indicator_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_indicator_class))
    {
        return;
    }
    rt = rt_of(w);
    if (period_frames < 2u)
    {
        period_frames = 2u; /* Below 2 the phase never flips */
    }
    rt->blink_period = period_frames;
    rt->blink_count = 0u;
    rt->blink_visible = 1u;
    if (blink == 0u)
    {
        rt->blink_on = 0u;
        indicator_stop_anim(w); /* Blink off = detach the node */
        wvui_widget_invalidate(w);
        return;
    }
    rt->blink_on = 1u;
    indicator_start_anim(w);
    wvui_widget_invalidate(w);
}

void wvui_indicator_set_colors(wvui_widget_t *w, wvui_color_t off, wvui_color_t on)
{
    wvui_indicator_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_indicator_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->off_color == off) && (rt->on_color == on))
    {
        return;
    }
    rt->off_color = off;
    rt->on_color = on;
    wvui_widget_invalidate(w);
}

void wvui_indicator_set_radius(wvui_widget_t *w, uint8_t radius)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_indicator_class))
    {
        return;
    }
    if (rt_of(w)->radius != radius)
    {
        rt_of(w)->radius = radius;
        wvui_widget_invalidate(w);
    }
}

void wvui_indicator_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_indicator_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

void wvui_indicator_set_clickable(wvui_widget_t *w, uint8_t clickable)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_indicator_class))
    {
        return;
    }
    if (rt_of(w)->clickable != (uint8_t)(clickable != 0u))
    {
        rt_of(w)->clickable = (uint8_t)(clickable != 0u);
        wvui_widget_invalidate(w);
    }
}

void wvui_indicator_set_change_cb(wvui_widget_t *w, wvui_indicator_change_cb_t cb, void *user)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_indicator_class))
    {
        return;
    }
    rt_of(w)->on_change = cb;
    rt_of(w)->user = user;
}

uint8_t wvui_indicator_is_on(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_indicator_class))
    {
        return 0u;
    }
    return ((const wvui_indicator_rt_t *)wvui_widget_rt_c(w))->on;
}

uint8_t wvui_indicator_is_pressed(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_indicator_class))
    {
        return 0u;
    }
    return ((const wvui_indicator_rt_t *)wvui_widget_rt_c(w))->pressed;
}

uint8_t wvui_indicator_visible(const wvui_widget_t *w)
{
    const wvui_indicator_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_indicator_class))
    {
        return 0u;
    }
    rt = (const wvui_indicator_rt_t *)wvui_widget_rt_c(w);
    return (uint8_t)((rt->on != 0u) && ((rt->blink_on == 0u) || (rt->blink_visible != 0u)));
}
