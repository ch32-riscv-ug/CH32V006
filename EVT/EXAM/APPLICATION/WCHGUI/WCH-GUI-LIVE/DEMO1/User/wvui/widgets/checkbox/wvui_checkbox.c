/**
 * @file wvui_checkbox.c
 * @brief Checkbox implementation: box, stroke-drawn check mark and state animation.
 * @note  Division-free: geometry uses 1/8 of the side length as the unit, stroke interpolation uses >> 8 fixed-point.
 */

#include "wvui_checkbox.h"

#include "wvui_draw.h"
#include "wvui_render.h"

static wvui_checkbox_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_checkbox_rt_t *)wvui_widget_rt(w);
}

static void checkbox_stop_anim(wvui_widget_t *w)
{
    wvui_checkbox_rt_t *rt = rt_of(w);

    if (wvui_anim_running(&rt->an) != 0u)
    {
        wvui_anim_stop(&rt->an);
    }
}

/* Per-frame approach: v += (target − v) >> 2, snap when |d| <= 2; return 0 = in place (engine detaches). */
static uint8_t checkbox_step(wvui_anim_t *a)
{
    wvui_widget_t *w = a->owner;
    wvui_checkbox_rt_t *rt = rt_of(w);
    int32_t target = (rt->on != 0u) ? 255 : 0;
    int32_t d = target - (int32_t)rt->progress;

    if (d == 0)
    {
        return 0u;
    }
    if ((d <= 2) && (d >= -2))
    {
        rt->progress = (uint8_t)target;
    }
    else
    {
        int32_t step = d >> 2;

        if (step == 0)
        {
            step = (d > 0) ? 1 : -1; /* >> 2 gives 0 when |d| < 4, use a step of ±1 */
        }
        rt->progress = (uint8_t)((int32_t)rt->progress + step);
    }
    wvui_widget_invalidate(w);
    return (uint8_t)((rt->progress != (uint8_t)target) ? 1u : 0u);
}

static void checkbox_start_anim(wvui_widget_t *w)
{
    wvui_checkbox_rt_t *rt = rt_of(w);

    if (rt->progress != (uint8_t)((rt->on != 0u) ? 255 : 0))
    {
        wvui_anim_start(&rt->an, w, checkbox_step); /* Idempotent: no duplicate link when already in the chain */
    }
}

static void checkbox_init(wvui_widget_t *w)
{
    wvui_checkbox_rt_t *rt = rt_of(w);

    wvui_anim_init(&rt->an);
    rt->box_off = WVUI_RGB565(48, 48, 56);
    rt->box_on = WVUI_RGB565(40, 130, 200);
    rt->mark_color = WVUI_RGB565(255, 255, 255);
    rt->border_color = WVUI_RGB565(120, 130, 140);
    rt->on = 0u;
    rt->progress = 0u;
    rt->radius = 3u;
    rt->opa = 255u;
    rt->enabled = 1u;
    rt->pressed = 0u;
    rt->on_change = 0;
    rt->user = 0;
}

static void checkbox_delete(wvui_widget_t *w)
{
    checkbox_stop_anim(w);
}

/* Check mark key points: start / bend / tip; unit m = min(w,h) >> 3, minimum 1. */
static void mark_points(const wvui_rect_t *box, int16_t *x0, int16_t *y0,
                        int16_t *ex, int16_t *ey, int16_t *tx, int16_t *ty)
{
    int32_t w = (int32_t)wvui_rect_w(box);
    int32_t h = (int32_t)wvui_rect_h(box);
    int32_t s = (w < h) ? w : h;
    int32_t m = s >> 3;
    int32_t left = (int32_t)box->x0;
    int32_t top = (int32_t)box->y0;

    if (m < 1)
    {
        m = 1;
    }
    *x0 = (int16_t)(left + m);
    *y0 = (int16_t)(top + (s >> 1));
    *ex = (int16_t)(left + (s >> 1) - m);
    *ey = (int16_t)(top + s - m);
    *tx = (int16_t)(left + s - m);
    *ty = (int16_t)(top + m);
}

/* Stroke: (ax,ay)→(bx,by) draws only the first k/256 of the segment, k clamped to 0..256, division-free. */
static void stroke(wvui_widget_t *w, int16_t ax, int16_t ay, int16_t bx, int16_t by,
                   int32_t k, uint8_t width, wvui_color_t color, uint8_t opa)
{
    int16_t cx;
    int16_t cy;

    if (k <= 0)
    {
        return;
    }
    if (k > 256)
    {
        k = 256;
    }
    cx = (int16_t)((int32_t)ax + (((int32_t)bx - (int32_t)ax) * k >> 8));
    cy = (int16_t)((int32_t)ay + (((int32_t)by - (int32_t)ay) * k >> 8));
    wvui_draw_line(ax, ay, cx, cy, width, WVUI_CAP_BUTT, color, opa);
    (void)w;
}

static void checkbox_draw(wvui_widget_t *w)
{
    const wvui_checkbox_rt_t *rt = (const wvui_checkbox_rt_t *)wvui_widget_rt_c(w);
    wvui_rect_t box = wvui_widget_rect(w);
    int32_t ww;
    int32_t hh;
    int32_t s;
    int32_t m;
    uint8_t stroke_w;
    int16_t x0;
    int16_t y0;
    int16_t ex;
    int16_t ey;
    int16_t tx;
    int16_t ty;

    if ((rt->opa == 0u) || (wvui_rect_is_empty(&box) != 0u))
    {
        return;
    }

    /* Box: fill + 1-pixel border, drawn in a single call. */
    wvui_draw_round_rect(box, rt->radius, 1u,
                        (rt->on != 0u) ? rt->box_on : rt->box_off, rt->opa,
                        rt->border_color, rt->opa);

    if (rt->progress == 0u)
    {
        return; /* Unchecked: draw the box only */
    }

    ww = (int32_t)wvui_rect_w(&box);
    hh = (int32_t)wvui_rect_h(&box);
    s = (ww < hh) ? ww : hh;
    m = s >> 3;
    if (m < 1)
    {
        m = 1;
    }
    stroke_w = (uint8_t)((m > 255) ? 255 : m);
    if (stroke_w == 0u)
    {
        stroke_w = 1u;
    }

    mark_points(&box, &x0, &y0, &ex, &ey, &tx, &ty);

    wvui_render_set_clip(box); /* Check mark clipped to the box */
    if (rt->progress <= 128u)
    {
        /* Phase 1: short edge grows from start to bend, k = 2·progress ≤ 256. */
        stroke(w, x0, y0, ex, ey, (int32_t)rt->progress * 2, stroke_w, rt->mark_color, rt->opa);
    }
    else
    {
        /* Phase 2: short edge full, long edge grows from bend to tip. (progress − 128)·2 tops out at 254,
         * off the tip by 1/128; progress snaps to exactly 255, so draw the full segment with k = 256 when done. */
        int32_t k = ((int32_t)rt->progress - 128) * 2;

        if (rt->progress == 255u)
        {
            k = 256;
        }
        wvui_draw_line(x0, y0, ex, ey, stroke_w, WVUI_CAP_BUTT, rt->mark_color, rt->opa);
        stroke(w, ex, ey, tx, ty, k, stroke_w, rt->mark_color, rt->opa);
    }
    wvui_render_clear_clip();
}

/* ---- Events: only CLICKED, events return 0 when disabled ---- */
static uint8_t checkbox_event(wvui_widget_t *w, wvui_event_t e, wvui_indev_t *d)
{
    wvui_checkbox_rt_t *rt = rt_of(w);

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
        wvui_checkbox_flip(w);
        return 1u;

    default:
        return 0u;
    }
}

/* ---- Property handlers: BG = unchecked fill, COLOR = checked fill, KNOB = check mark color, BORDER = border color ---- */
static int8_t checkbox_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
{
    wvui_checkbox_rt_t *rt = rt_of(w);

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
        wvui_checkbox_set_on(w, (uint8_t)(in[0] != 0u));
        return WVUI_PROP_OK;

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_checkbox_set_colors(w, wvui_prop_u16(in), rt->box_on, rt->mark_color,
                                rt->border_color);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_checkbox_set_colors(w, rt->box_off, wvui_prop_u16(in), rt->mark_color,
                                rt->border_color);
        return WVUI_PROP_OK;

    case WVUI_PROP_KNOB:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_checkbox_set_colors(w, rt->box_off, rt->box_on, wvui_prop_u16(in),
                                rt->border_color);
        return WVUI_PROP_OK;

    case WVUI_PROP_BORDER:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_checkbox_set_colors(w, rt->box_off, rt->box_on, rt->mark_color, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_RADIUS:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_checkbox_set_radius(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_checkbox_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_ENABLED:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_checkbox_set_enabled(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_PRESSED:
        return WVUI_PROP_ERR_RO;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t checkbox_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_checkbox_rt_t *rt = (const wvui_checkbox_rt_t *)wvui_widget_rt_c(w);

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
        wvui_prop_write_u16(out, rt->box_off);
        return 2;

    case WVUI_PROP_COLOR:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->box_on);
        return 2;

    case WVUI_PROP_KNOB:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->mark_color);
        return 2;

    case WVUI_PROP_BORDER:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->border_color);
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

const wvui_class_t wvui_checkbox_class = {
    .rt_size = (uint16_t)sizeof(wvui_checkbox_rt_t),
    .draw = checkbox_draw,
    .event = checkbox_event,
    .init = checkbox_init,
    .set_pos = 0,
    .on_delete = checkbox_delete,
    .set_prop = checkbox_set_prop,
    .get_prop = checkbox_get_prop,
    .flags = 0u,
};

/* ---- Property setters ---- */
void wvui_checkbox_set_on(wvui_widget_t *w, uint8_t on)
{
    wvui_checkbox_rt_t *rt;
    uint8_t v = (uint8_t)(on != 0u);

    if (!WVUI_W_IS_TYPE(w, &wvui_checkbox_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rt->on == v)
    {
        return; /* Same state: no animation, no callback */
    }
    rt->on = v;
    wvui_widget_invalidate(w); /* Mark the whole widget dirty: the box fill changes with it */
    checkbox_start_anim(w);
    if (rt->on_change != 0)
    {
        rt->on_change(w, v, rt->user);
    }
}

void wvui_checkbox_flip(wvui_widget_t *w)
{
    const wvui_checkbox_rt_t *rt = (const wvui_checkbox_rt_t *)wvui_widget_rt_c(w);

    if (!WVUI_W_IS_TYPE(w, &wvui_checkbox_class))
    {
        return;
    }
    wvui_checkbox_set_on(w, (uint8_t)(rt->on == 0u));
}

void wvui_checkbox_set_colors(wvui_widget_t *w, wvui_color_t box_off, wvui_color_t box_on,
                             wvui_color_t mark, wvui_color_t border)
{
    wvui_checkbox_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_checkbox_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->box_off == box_off) && (rt->box_on == box_on) &&
        (rt->mark_color == mark) && (rt->border_color == border))
    {
        return;
    }
    rt->box_off = box_off;
    rt->box_on = box_on;
    rt->mark_color = mark;
    rt->border_color = border;
    wvui_widget_invalidate(w);
}

void wvui_checkbox_set_radius(wvui_widget_t *w, uint8_t radius)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_checkbox_class))
    {
        return;
    }
    if (rt_of(w)->radius != radius)
    {
        rt_of(w)->radius = radius;
        wvui_widget_invalidate(w);
    }
}

void wvui_checkbox_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_checkbox_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

void wvui_checkbox_set_enabled(wvui_widget_t *w, uint8_t enabled)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_checkbox_class))
    {
        return;
    }
    if (rt_of(w)->enabled != (uint8_t)(enabled != 0u))
    {
        rt_of(w)->enabled = (uint8_t)(enabled != 0u);
        wvui_widget_invalidate(w);
    }
}

void wvui_checkbox_set_change_cb(wvui_widget_t *w, wvui_checkbox_change_cb_t cb, void *user)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_checkbox_class))
    {
        return;
    }
    rt_of(w)->on_change = cb;
    rt_of(w)->user = user;
}

uint8_t wvui_checkbox_is_on(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_checkbox_class))
    {
        return 0u;
    }
    return ((const wvui_checkbox_rt_t *)wvui_widget_rt_c(w))->on;
}

uint8_t wvui_checkbox_is_pressed(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_checkbox_class))
    {
        return 0u;
    }
    return ((const wvui_checkbox_rt_t *)wvui_widget_rt_c(w))->pressed;
}

uint8_t wvui_checkbox_progress(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_checkbox_class))
    {
        return 0u;
    }
    return ((const wvui_checkbox_rt_t *)wvui_widget_rt_c(w))->progress;
}
