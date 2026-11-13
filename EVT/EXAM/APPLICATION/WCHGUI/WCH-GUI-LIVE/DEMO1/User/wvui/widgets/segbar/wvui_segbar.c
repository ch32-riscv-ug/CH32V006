/**
 * @file wvui_segbar.c
 * @brief Segment bar implementation: level mapping in Q16 fixed point, window wrapping by conditional subtraction.
 * @note  Phase advancement runs in a permanent anim node, on the chain only when win is non-zero and win < count.
 */

#include "wvui_segbar.h"

#include "wvui_draw.h"
#include "wvui_render.h"

static wvui_segbar_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_segbar_rt_t *)wvui_widget_rt(w);
}

static const wvui_segbar_rt_t *rt_of_c(const wvui_widget_t *w)
{
    return (const wvui_segbar_rt_t *)wvui_widget_rt_c(w);
}

/* Effective block pitch: 0 = auto; at least blk_w + 1, otherwise adjacent blocks touch. */
static int32_t segbar_pitch(const wvui_segbar_rt_t *rt)
{
    int32_t p = (rt->pitch != 0u) ? (int32_t)rt->pitch : ((int32_t)rt->blk_w + 1);

    return (p > (int32_t)rt->blk_w) ? p : ((int32_t)rt->blk_w + 1);
}

/* Level mapping: value 0..255 -> lit blocks 0..count, rounded, /255 via Q16 fixed point. */
static uint8_t segbar_lit(uint8_t value, uint8_t count)
{
    uint32_t v = ((uint32_t)value * (uint32_t)count * 257u) + 32768u;

    return (uint8_t)(v >> 16);
}

/* Window test: whether i falls inside the wrapped range [offset, offset + win), conditional subtraction instead of %. */
static uint8_t segbar_windowed(const wvui_segbar_rt_t *rt, uint8_t i)
{
    uint8_t d = (i >= rt->offset) ? (uint8_t)(i - rt->offset)
                                  : (uint8_t)((uint8_t)(i + rt->count) - rt->offset);

    return (uint8_t)(d < rt->win);
}

static void segbar_stop_anim(wvui_widget_t *w)
{
    wvui_segbar_rt_t *rt = rt_of(w);

    if (wvui_anim_running(&rt->an) != 0u)
    {
        wvui_anim_stop(&rt->an);
    }
}

/* Permanent node: dirty only on the frame that moves the phase, static frames are not redrawn. */
static uint8_t segbar_step(wvui_anim_t *a)
{
    wvui_widget_t *w = a->owner;
    wvui_segbar_rt_t *rt = rt_of(w);

    if ((rt->win == 0u) || (rt->win >= rt->count))
    {
        return 0u; /* window closed / all lit: remove, idempotent */
    }
    rt->tick = (uint8_t)(rt->tick + 1u);
    if (rt->tick < rt->period)
    {
        return 1u;
    }
    rt->tick = 0u;
    rt->offset = (uint8_t)(rt->offset + 1u);
    if (rt->offset >= rt->count)
    {
        rt->offset = 0u;
    }
    wvui_widget_invalidate(w);
    return 1u;
}

static void segbar_start_anim(wvui_widget_t *w)
{
    wvui_segbar_rt_t *rt = rt_of(w);

    if ((rt->win != 0u) && (rt->win < rt->count))
    {
        wvui_anim_start_permanent(&rt->an, w, segbar_step);
    }
}

static void segbar_init(wvui_widget_t *w)
{
    wvui_segbar_rt_t *rt = rt_of(w);

    wvui_anim_init(&rt->an);
    rt->on = WVUI_RGB565(90, 130, 230);
    rt->off = WVUI_RGB565(32, 38, 54);
    rt->value = 0u;
    rt->count = 8u;
    rt->lit = 0u;
    rt->blk_w = 5u;
    rt->blk_h = 8u;
    rt->pitch = 8u;
    rt->radius = 1u;
    rt->win = 0u;
    rt->offset = 0u;
    rt->period = 4u;
    rt->tick = 0u;
    rt->opa = 255u;
}

static void segbar_delete(wvui_widget_t *w)
{
    segbar_stop_anim(w);
}

static void segbar_draw(wvui_widget_t *w)
{
    const wvui_segbar_rt_t *rt = rt_of_c(w);
    wvui_rect_t box = wvui_widget_rect(w);
    int32_t pitch = segbar_pitch(rt);
    int32_t bh;
    int32_t y0;
    uint8_t i;

    if ((rt->opa == 0u) || (rt->count == 0u) || (rt->blk_w == 0u) ||
        (rt->blk_h == 0u) || (wvui_rect_is_empty(&box) != 0u))
    {
        return;
    }

    bh = (int32_t)rt->blk_h;
    if (bh > (int32_t)wvui_rect_h(&box))
    {
        bh = (int32_t)wvui_rect_h(&box); /* block taller than the widget: clamp to the widget height */
    }
    y0 = (int32_t)box.y0 + (((int32_t)wvui_rect_h(&box) - bh) >> 1);

    wvui_render_set_clip(box);
    for (i = 0u; i < rt->count; i++)
    {
        wvui_rect_t b;
        wvui_color_t c;
        uint8_t lit;

        if (rt->win != 0u)
        {
            lit = segbar_windowed(rt, i);
        }
        else
        {
            lit = (uint8_t)(i < rt->lit);
        }
        if ((lit == 0u) && (rt->off == rt->on))
        {
            continue; /* same color: no unlit blocks drawn */
        }
        c = (lit != 0u) ? rt->on : rt->off;

        b.x0 = (int16_t)((int32_t)box.x0 + ((int32_t)i * pitch));
        b.y0 = (int16_t)y0;
        b.x1 = (int16_t)((int32_t)b.x0 + (int32_t)rt->blk_w - 1);
        b.y1 = (int16_t)(y0 + bh - 1);
        if (b.x0 > box.x1)
        {
            break; /* blocks that no longer fit are not drawn */
        }
        wvui_draw_round_rect(b, rt->radius, 0u, c, rt->opa, 0u, 0u);
    }
    wvui_render_clear_clip();
}

/* ---- Property access: VALUE = level value, payload u8 ---- */
/* COLOR = lit block, BG = unlit block; win and period have no property id, use the setters. */
static int8_t segbar_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
{
    wvui_segbar_rt_t *rt = rt_of(w);

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
        wvui_segbar_set_value(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_segbar_set_colors(w, wvui_prop_u16(in), rt->off);
        return WVUI_PROP_OK;

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_segbar_set_colors(w, rt->on, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_RADIUS:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        if (rt->radius != in[0])
        {
            rt->radius = in[0];
            wvui_widget_invalidate(w);
        }
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_segbar_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t segbar_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_segbar_rt_t *rt = rt_of_c(w);

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
        wvui_prop_write_u16(out, rt->on);
        return 2;

    case WVUI_PROP_BG:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->off);
        return 2;

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

const wvui_class_t wvui_segbar_class = {
    .rt_size = (uint16_t)sizeof(wvui_segbar_rt_t),
    .draw = segbar_draw,
    .event = 0, /* decorative: no gesture interception */
    .init = segbar_init,
    .set_pos = 0,
    .on_delete = segbar_delete,
    .set_prop = segbar_set_prop,
    .get_prop = segbar_get_prop,
    .flags = 0u,
};

/* ---- Setters ---- */
void wvui_segbar_set_blocks(wvui_widget_t *w, uint8_t count, uint8_t blk_w, uint8_t blk_h,
                           uint8_t pitch, uint8_t radius)
{
    wvui_segbar_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_segbar_class))
    {
        return;
    }
    rt = rt_of(w);
    if (count > WVUI_SEGBAR_MAX_BLOCKS)
    {
        count = (uint8_t)WVUI_SEGBAR_MAX_BLOCKS;
    }
    if (count < 1u)
    {
        count = 1u;
    }
    if (blk_w < 1u)
    {
        blk_w = 1u;
    }
    if (blk_h < 1u)
    {
        blk_h = 1u;
    }
    if ((rt->count == count) && (rt->blk_w == blk_w) && (rt->blk_h == blk_h) &&
        (rt->pitch == pitch) && (rt->radius == radius))
    {
        return;
    }
    rt->count = count;
    rt->blk_w = blk_w;
    rt->blk_h = blk_h;
    rt->pitch = pitch;
    rt->radius = radius;
    if (rt->offset >= rt->count)
    {
        rt->offset = 0u;
    }
    if (rt->win > rt->count)
    {
        rt->win = rt->count;
    }
    rt->lit = segbar_lit(rt->value, rt->count);
    if (rt->win != 0u)
    {
        /* Block count changed: the window width may switch between all-lit and scrollable. */
        if (rt->win < rt->count)
        {
            segbar_start_anim(w);
        }
        else
        {
            segbar_stop_anim(w);
        }
    }
    wvui_widget_invalidate(w);
}

void wvui_segbar_set_value(wvui_widget_t *w, uint8_t value)
{
    wvui_segbar_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_segbar_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rt->value == value)
    {
        return;
    }
    rt->value = value;
    rt->lit = segbar_lit(value, rt->count);
    wvui_widget_invalidate(w);
}

void wvui_segbar_set_window(wvui_widget_t *w, uint8_t win, uint8_t period_frames)
{
    wvui_segbar_rt_t *rt;
    uint8_t was_off;

    if (!WVUI_W_IS_TYPE(w, &wvui_segbar_class))
    {
        return;
    }
    rt = rt_of(w);
    if (win > rt->count)
    {
        win = rt->count;
    }
    if (period_frames < 1u)
    {
        period_frames = 1u;
    }
    was_off = (uint8_t)(rt->win == 0u);

    if (win == 0u)
    {
        rt->win = 0u;
        rt->tick = 0u;
        segbar_stop_anim(w); /* closing the window = remove the node */
        wvui_widget_invalidate(w);
        return;
    }
    rt->win = win;
    rt->period = period_frames;
    if (was_off != 0u)
    {
        rt->offset = 0u; /* a newly opened window starts at block 0 */
    }
    rt->tick = 0u;
    if (win >= rt->count)
    {
        segbar_stop_anim(w); /* all lit: static, no anim node used */
    }
    else
    {
        segbar_start_anim(w);
    }
    wvui_widget_invalidate(w);
}

void wvui_segbar_set_offset(wvui_widget_t *w, uint8_t offset)
{
    wvui_segbar_rt_t *rt;
    uint8_t v;

    if (!WVUI_W_IS_TYPE(w, &wvui_segbar_class))
    {
        return;
    }
    rt = rt_of(w);
    v = offset;
    if (rt->count != 0u)
    {
        /* Remainder by subtraction: count is 1..32, offset is u8, at most 255 loop iterations, runs only in the setter. */
        while (v >= rt->count)
        {
            v = (uint8_t)(v - rt->count);
        }
    }
    if (rt->offset != v)
    {
        rt->offset = v;
        rt->tick = 0u;
        wvui_widget_invalidate(w);
    }
}

void wvui_segbar_set_colors(wvui_widget_t *w, wvui_color_t on, wvui_color_t off)
{
    wvui_segbar_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_segbar_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->on == on) && (rt->off == off))
    {
        return;
    }
    rt->on = on;
    rt->off = off;
    wvui_widget_invalidate(w);
}

void wvui_segbar_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_segbar_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

uint8_t wvui_segbar_value(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_segbar_class))
    {
        return 0u;
    }
    return rt_of_c(w)->value;
}

uint8_t wvui_segbar_window(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_segbar_class))
    {
        return 0u;
    }
    return rt_of_c(w)->win;
}

uint8_t wvui_segbar_offset(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_segbar_class))
    {
        return 0u;
    }
    return rt_of_c(w)->offset;
}
