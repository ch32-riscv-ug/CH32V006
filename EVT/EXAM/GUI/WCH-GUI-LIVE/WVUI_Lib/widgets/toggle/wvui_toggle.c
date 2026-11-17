/**
 * @file wvui_toggle.c
 * @brief Toggle implementation: track, knob, and position transition.
 * @note  Division-free: the knob position is derived by shifting the Q8 (0..255) travel ratio.
 */

#include "wvui_toggle.h"

#include "wvui_draw.h"
#include "wvui_render.h"

static wvui_toggle_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_toggle_rt_t *)wvui_widget_rt(w);
}

static void toggle_stop_anim(wvui_widget_t *w)
{
    wvui_toggle_rt_t *rt = rt_of(w);

    if (wvui_anim_running(&rt->an) != 0u)
    {
        wvui_anim_stop(&rt->an);
    }
}

/* Per-frame approach: v += (target - v) >> 2, snap when |d| <= 2; return 0 = in place (detached by the engine). */
static uint8_t toggle_step(wvui_anim_t *a)
{
    wvui_widget_t *w = a->owner;
    wvui_toggle_rt_t *rt = rt_of(w);
    int32_t target = (rt->on != 0u) ? 255 : 0;
    int32_t d = target - (int32_t)rt->pos;

    if (d == 0)
    {
        return 0u;
    }
    if ((d <= 2) && (d >= -2))
    {
        rt->pos = (uint8_t)target;
    }
    else
    {
        /* Use a step of +/-1 when d >> 2 gives 0; with |d| > 2 the arithmetic shift converges backwards too. */
        int32_t step = d >> 2;

        if (step == 0)
        {
            step = (d > 0) ? 1 : -1;
        }
        rt->pos = (uint8_t)((int32_t)rt->pos + step);
    }
    wvui_widget_invalidate(w);
    return (uint8_t)((rt->pos != (uint8_t)target) ? 1u : 0u);
}

static void toggle_start_anim(wvui_widget_t *w)
{
    wvui_toggle_rt_t *rt = rt_of(w);

    if (rt->pos != (uint8_t)((rt->on != 0u) ? 255 : 0))
    {
        wvui_anim_start(&rt->an, w, toggle_step); /* idempotent: does not attach again when already in the chain */
    }
}

static void toggle_init(wvui_widget_t *w)
{
    wvui_toggle_rt_t *rt = rt_of(w);

    wvui_anim_init(&rt->an);
    rt->track_off = WVUI_RGB565(70, 78, 88);
    rt->track_on = WVUI_RGB565(40, 130, 200);
    rt->knob_color = WVUI_RGB565(240, 240, 240);
    rt->on = 0u;
    rt->pos = 0u;
    rt->radius = 0u; /* 0 = capsule (h/2) */
    rt->opa = 255u;
    rt->enabled = 1u;
    rt->pressed = 0u;
    rt->on_change = 0;
    rt->user = 0;
}

static void toggle_delete(wvui_widget_t *w)
{
    toggle_stop_anim(w);
}

static void toggle_draw(wvui_widget_t *w)
{
    const wvui_toggle_rt_t *rt = (const wvui_toggle_rt_t *)wvui_widget_rt_c(w);
    wvui_rect_t track = wvui_widget_rect(w);
    wvui_rect_t knob;
    int32_t h;
    int32_t d;      /* knob diameter */
    int32_t travel; /* knob travel */
    uint8_t r;

    if ((rt->opa == 0u) || (wvui_rect_is_empty(&track) != 0u))
    {
        return;
    }

    h = (int32_t)wvui_rect_h(&track);
    r = rt->radius;
    if (r == 0u)
    {
        r = (uint8_t)((h > 255) ? 255 : (h >> 1)); /* 0 = capsule */
    }

    /* Track: drawn once in full; the color picks one of two by on, one paint pass over the same area. */
    wvui_draw_round_rect(track, r, 0u,
                        (rt->on != 0u) ? rt->track_on : rt->track_off, rt->opa, 0u, 0u);

    /* Knob: diameter = inner height - 2 * inner padding, radius = diameter / 2. */
    d = h - 4;
    if (d < 2)
    {
        d = 2;
    }
    travel = (int32_t)wvui_rect_w(&track) - d - 4;
    if (travel < 0)
    {
        travel = 0;
    }

    knob.x0 = (int16_t)(track.x0 + 2 + (int16_t)((travel * (int32_t)rt->pos) >> 8));
    knob.y0 = (int16_t)(track.y0 + 2);
    knob.x1 = (int16_t)(knob.x0 + d - 1);
    knob.y1 = (int16_t)(knob.y0 + d - 1);

    wvui_render_set_clip(track); /* the knob is clipped to the track */
    wvui_draw_round_rect(knob, (uint8_t)(d >> 1), 0u, rt->knob_color, rt->opa, 0u, 0u);
    wvui_render_clear_clip();
}

/* ---- Events: CLICKED only, events return 0 when disabled ---- */
static uint8_t toggle_event(wvui_widget_t *w, wvui_event_t e, wvui_indev_t *d)
{
    wvui_toggle_rt_t *rt = rt_of(w);

    (void)d;

    switch (e)
    {
    case WVUI_EVENT_PRESSED:
        if (rt->enabled == 0u)
        {
            return 0u;
        }
        if (rt->pressed == 0u)
        {
            rt->pressed = 1u;
            wvui_widget_invalidate(w);
        }
        return 1u;

    case WVUI_EVENT_STAY:
        return (rt->enabled != 0u) ? 1u : 0u;

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
        wvui_toggle_flip(w);
        return 1u;

    default:
        return 0u;
    }
}

/* ---- Property handlers: BG = off track color, COLOR = on track color, KNOB = knob color ---- */
static int8_t toggle_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
{
    wvui_toggle_rt_t *rt = rt_of(w);

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
        wvui_toggle_set_on(w, (uint8_t)(in[0] != 0u));
        return WVUI_PROP_OK;

    case WVUI_PROP_BG: /* track color when off */
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_toggle_set_colors(w, wvui_prop_u16(in), rt->track_on, rt->knob_color);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR: /* track color when on */
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_toggle_set_colors(w, rt->track_off, wvui_prop_u16(in), rt->knob_color);
        return WVUI_PROP_OK;

    case WVUI_PROP_KNOB:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_toggle_set_colors(w, rt->track_off, rt->track_on, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_RADIUS:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_toggle_set_radius(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_toggle_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_ENABLED:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_toggle_set_enabled(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_PRESSED:
        return WVUI_PROP_ERR_RO; /* the pressed state belongs to the input layer */

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t toggle_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_toggle_rt_t *rt = (const wvui_toggle_rt_t *)wvui_widget_rt_c(w);

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
        wvui_prop_write_u16(out, rt->track_off);
        return 2;

    case WVUI_PROP_COLOR:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->track_on);
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

const wvui_class_t wvui_toggle_class = {
    .rt_size = (uint16_t)sizeof(wvui_toggle_rt_t),
    .draw = toggle_draw,
    .event = toggle_event,
    .init = toggle_init,
    .set_pos = 0,
    .on_delete = toggle_delete,
    .set_prop = toggle_set_prop,
    .get_prop = toggle_get_prop,
    .flags = 0u,
};

/* ---- Property setters ---- */
void wvui_toggle_set_on(wvui_widget_t *w, uint8_t on)
{
    wvui_toggle_rt_t *rt;
    uint8_t v = (uint8_t)(on != 0u);

    if (!WVUI_W_IS_TYPE(w, &wvui_toggle_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rt->on == v)
    {
        return; /* same state: no animation, no callback */
    }
    rt->on = v;
    wvui_widget_invalidate(w); /* mark the whole block dirty: the knob's old position is repainted */
    toggle_start_anim(w);
    if (rt->on_change != 0)
    {
        rt->on_change(w, v, rt->user);
    }
}

void wvui_toggle_flip(wvui_widget_t *w)
{
    const wvui_toggle_rt_t *rt = (const wvui_toggle_rt_t *)wvui_widget_rt_c(w);

    if (!WVUI_W_IS_TYPE(w, &wvui_toggle_class))
    {
        return;
    }
    wvui_toggle_set_on(w, (uint8_t)(rt->on == 0u));
}

void wvui_toggle_set_colors(wvui_widget_t *w, wvui_color_t track_off, wvui_color_t track_on,
                           wvui_color_t knob)
{
    wvui_toggle_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_toggle_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->track_off == track_off) && (rt->track_on == track_on) && (rt->knob_color == knob))
    {
        return;
    }
    rt->track_off = track_off;
    rt->track_on = track_on;
    rt->knob_color = knob;
    wvui_widget_invalidate(w);
}

void wvui_toggle_set_radius(wvui_widget_t *w, uint8_t radius)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_toggle_class))
    {
        return;
    }
    if (rt_of(w)->radius != radius)
    {
        rt_of(w)->radius = radius;
        wvui_widget_invalidate(w);
    }
}

void wvui_toggle_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_toggle_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

void wvui_toggle_set_enabled(wvui_widget_t *w, uint8_t enabled)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_toggle_class))
    {
        return;
    }
    if (rt_of(w)->enabled != (uint8_t)(enabled != 0u))
    {
        rt_of(w)->enabled = (uint8_t)(enabled != 0u);
        wvui_widget_invalidate(w);
    }
}

void wvui_toggle_set_change_cb(wvui_widget_t *w, wvui_toggle_change_cb_t cb, void *user)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_toggle_class))
    {
        return;
    }
    rt_of(w)->on_change = cb;
    rt_of(w)->user = user;
}

uint8_t wvui_toggle_is_on(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_toggle_class))
    {
        return 0u;
    }
    return ((const wvui_toggle_rt_t *)wvui_widget_rt_c(w))->on;
}

uint8_t wvui_toggle_is_pressed(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_toggle_class))
    {
        return 0u;
    }
    return ((const wvui_toggle_rt_t *)wvui_widget_rt_c(w))->pressed;
}

uint8_t wvui_toggle_knob_pos(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_toggle_class))
    {
        return 0u;
    }
    return ((const wvui_toggle_rt_t *)wvui_widget_rt_c(w))->pos;
}
