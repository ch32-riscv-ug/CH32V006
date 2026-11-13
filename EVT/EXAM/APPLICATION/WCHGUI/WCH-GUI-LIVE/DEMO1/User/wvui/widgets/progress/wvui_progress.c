/**
 * @file wvui_progress.c
 * @brief Progress bar implementation.
 * @note  The track is redrawn every frame with the fill on top; fractional width is computed as (w · a256(v)) >> 8.
 */

#include "wvui_progress.h"

#include "wvui_config.h"
#include "wvui_draw.h"
#include "wvui_render.h"

static wvui_progress_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_progress_rt_t *)wvui_widget_rt(w);
}

/* Fixed-point form of v/255: a256 = v + (v >> 7), (w · a256) >> 8 equals w·v/255. */
static uint32_t frac255(uint32_t w, uint32_t v)
{
    return (w * (v + (v >> 7))) >> 8;
}

static void progress_stop_anim(wvui_widget_t *w)
{
    wvui_progress_rt_t *rt = rt_of(w);

    if (wvui_anim_running(&rt->an) != 0u)
    {
        wvui_anim_stop(&rt->an);
    }
}

/* Returns 0 = arrived, the engine removes the node; the widget pointer is taken back from a->owner. */
static uint8_t progress_step(wvui_anim_t *a)
{
    wvui_widget_t *w = a->owner;
    wvui_progress_rt_t *rt = rt_of(w);
    uint8_t d;

    if (rt->display == rt->value)
    {
        return 0u;
    }
    d = (uint8_t)((rt->display < rt->value) ? (rt->value - rt->display)
                                            : (rt->display - rt->value));
    if (d > (uint8_t)WVUI_CFG_PROGRESS_STEP)
    {
        d = (uint8_t)WVUI_CFG_PROGRESS_STEP;
    }
    if (rt->display < rt->value)
    {
        rt->display = (uint8_t)(rt->display + d);
    }
    else
    {
        rt->display = (uint8_t)(rt->display - d);
    }
    wvui_widget_invalidate(w);
    return (uint8_t)((rt->display != rt->value) ? 1u : 0u);
}

static void progress_start_anim(wvui_widget_t *w)
{
    wvui_progress_rt_t *rt = rt_of(w);

    if (rt->display != rt->value)
    {
        /* Idempotent: only refreshes the callback when already on the chain */
        wvui_anim_start(&rt->an, w, progress_step);
    }
}

static void progress_init(wvui_widget_t *w)
{
    wvui_progress_rt_t *rt = rt_of(w);

    wvui_anim_init(&rt->an);
    rt->track_color = WVUI_RGB565(48, 48, 56);
    rt->fill_color = WVUI_RGB565(88, 166, 240);
    rt->value = 0u;
    rt->display = 0u;
    rt->radius = 0u;
    rt->opa = 255u;
}

static void progress_delete(wvui_widget_t *w)
{
    progress_stop_anim(w);
}

static void progress_draw(wvui_widget_t *w)
{
    const wvui_progress_rt_t *rt = (const wvui_progress_rt_t *)wvui_widget_rt_c(w);
    wvui_rect_t track = wvui_widget_rect(w);
    wvui_rect_t fill;
    wvui_rect_t clip;
    int32_t span;
    int32_t r;
    int32_t need;

    if ((rt->opa == 0u) || (wvui_rect_is_empty(&track) != 0u))
    {
        return;
    }

    /* Draw the whole track in one pass (stateless redraw). */
    wvui_draw_round_rect(track, rt->radius, 0u, rt->track_color, rt->opa, 0u, 0u);

    if (rt->display == 0u)
    {
        return;
    }

    /* Direction is chosen by geometry: width >= height means horizontal (default), otherwise fill from the bottom up.
     * Fill length = main axis length · display/255, in a256 fixed point. */
    if (wvui_rect_w(&track) >= wvui_rect_h(&track))
    {
        r = (int32_t)rt->radius;
        if (r > ((int32_t)wvui_rect_h(&track) / 2))
        {
            r = (int32_t)wvui_rect_h(&track) / 2;
        }
        span = (int32_t)frac255((uint32_t)wvui_rect_w(&track), (uint32_t)rt->display);
        if (span <= 0)
        {
            span = 1;
        }
        need = r * 2;
        fill.x0 = track.x0;
        fill.y0 = track.y0;
        fill.y1 = track.y1;
        if (span < need)
        {
            fill.x1 = (int16_t)(track.x0 + need - 1);
            clip.x0 = track.x0;
            clip.y0 = track.y0;
            clip.x1 = (int16_t)(track.x0 + span - 1);
            clip.y1 = track.y1;
            wvui_render_set_clip(track);
            wvui_render_set_clip(clip);
            wvui_draw_round_rect(fill, (uint8_t)r, 0u, rt->fill_color, rt->opa, 0u, 0u);
            wvui_render_clear_clip();
            return;
        }
        fill.x1 = (int16_t)(track.x0 + span - 1);
        if (fill.x1 > track.x1)
        {
            fill.x1 = track.x1; /* Fixed-point rounding can exceed by at most 1 px, clamped */
        }
    }
    else
    {
        r = (int32_t)rt->radius;
        if (r > ((int32_t)wvui_rect_w(&track) / 2))
        {
            r = (int32_t)wvui_rect_w(&track) / 2;
        }
        span = (int32_t)frac255((uint32_t)wvui_rect_h(&track), (uint32_t)rt->display);
        if (span <= 0)
        {
            span = 1;
        }
        need = r * 2;
        fill.x0 = track.x0;
        fill.x1 = track.x1;
        fill.y1 = track.y1;
        if (span < need)
        {
            fill.y0 = (int16_t)(track.y1 - need + 1);
            clip.x0 = track.x0;
            clip.x1 = track.x1;
            clip.y0 = (int16_t)(track.y1 - span + 1);
            clip.y1 = track.y1;
            wvui_render_set_clip(track);
            wvui_render_set_clip(clip);
            wvui_draw_round_rect(fill, (uint8_t)r, 0u, rt->fill_color, rt->opa, 0u, 0u);
            wvui_render_clear_clip();
            return;
        }
        fill.y0 = (int16_t)(track.y1 - span + 1);
        if (fill.y0 < track.y0)
        {
            fill.y0 = track.y0;
        }
    }

    /* Clip to the track rect: rounded corners do not spill over. */
    wvui_render_set_clip(track);
    wvui_draw_round_rect(fill, (uint8_t)r, 0u, rt->fill_color, rt->opa, 0u, 0u);
    wvui_render_clear_clip();
}

/* ---- Property read/write (command channel endpoint) ---- */
static int8_t progress_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
{
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
        wvui_progress_set_value(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_progress_set_colors(w, ((const wvui_progress_rt_t *)wvui_widget_rt_c(w))->track_color,
                                wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_progress_set_colors(w, wvui_prop_u16(in),
                                ((const wvui_progress_rt_t *)wvui_widget_rt_c(w))->fill_color);
        return WVUI_PROP_OK;

    case WVUI_PROP_RADIUS:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_progress_set_radius(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_progress_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t progress_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_progress_rt_t *rt = (const wvui_progress_rt_t *)wvui_widget_rt_c(w);

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
        wvui_prop_write_u16(out, rt->fill_color);
        return 2;

    case WVUI_PROP_BG:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->track_color);
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

const wvui_class_t wvui_progress_class = {
    .rt_size = (uint16_t)sizeof(wvui_progress_rt_t),
    .draw = progress_draw,
    .event = 0, /* Decorative widget: no gesture interception */
    .init = progress_init,
    .set_pos = 0,
    .on_delete = progress_delete,
    .set_prop = progress_set_prop,
    .get_prop = progress_get_prop,
    .flags = 0u,
};

/* ---- Property setters ---- */
void wvui_progress_set_value(wvui_widget_t *w, uint8_t value)
{
    wvui_progress_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_progress_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rt->value == value)
    {
        return;
    }
    /* Mark the whole block dirty: the old fill's right edge is covered by the track. */
    rt->value = value;
    wvui_widget_invalidate(w);
    progress_start_anim(w);
}

void wvui_progress_add(wvui_widget_t *w, uint8_t delta)
{
    const wvui_progress_rt_t *rt = (const wvui_progress_rt_t *)wvui_widget_rt_c(w);
    uint16_t v;

    if (!WVUI_W_IS_TYPE(w, &wvui_progress_class))
    {
        return;
    }
    v = (uint16_t)((uint16_t)rt->value + (uint16_t)delta);
    wvui_progress_set_value(w, (uint8_t)((v > 255u) ? 255u : v));
}

void wvui_progress_sub(wvui_widget_t *w, uint8_t delta)
{
    const wvui_progress_rt_t *rt = (const wvui_progress_rt_t *)wvui_widget_rt_c(w);

    if (!WVUI_W_IS_TYPE(w, &wvui_progress_class))
    {
        return;
    }
    wvui_progress_set_value(w, (uint8_t)((rt->value > delta) ? (uint8_t)(rt->value - delta) : 0u));
}

void wvui_progress_set_colors(wvui_widget_t *w, wvui_color_t track, wvui_color_t fill)
{
    wvui_progress_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_progress_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->track_color == track) && (rt->fill_color == fill))
    {
        return;
    }
    rt->track_color = track;
    rt->fill_color = fill;
    wvui_widget_invalidate(w);
}

void wvui_progress_set_radius(wvui_widget_t *w, uint8_t radius)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_progress_class))
    {
        return;
    }
    if (rt_of(w)->radius != radius)
    {
        rt_of(w)->radius = radius;
        wvui_widget_invalidate(w);
    }
}

void wvui_progress_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_progress_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

uint8_t wvui_progress_value(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_progress_class))
    {
        return 0u;
    }
    return ((const wvui_progress_rt_t *)wvui_widget_rt_c(w))->value;
}

uint8_t wvui_progress_display(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_progress_class))
    {
        return 0u;
    }
    return ((const wvui_progress_rt_t *)wvui_widget_rt_c(w))->display;
}
