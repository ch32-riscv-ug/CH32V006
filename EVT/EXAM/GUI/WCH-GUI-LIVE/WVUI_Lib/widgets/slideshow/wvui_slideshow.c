/**
 * @file wvui_slideshow.c
 * @brief Auto slideshow implementation.
 * @note  Draws one image while idle, both once per frame in transition; the offset accrues every frame and steps 1 px once slide_frames is filled.
 */

#include "wvui_slideshow.h"

#include "wvui_config.h"
#include "wvui_render.h"

static wvui_slideshow_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_slideshow_rt_t *)wvui_widget_rt(w);
}

static void slideshow_stop(wvui_widget_t *w)
{
    wvui_slideshow_rt_t *rt = rt_of(w);

    if (wvui_anim_running(&rt->an) != 0u)
    {
        wvui_anim_stop(&rt->an);
    }
}

static uint16_t slideshow_next_index(const wvui_slideshow_rt_t *rt)
{
    uint16_t n = (uint16_t)(rt->index + 1u);

    return (n >= rt->count) ? 0u : n;
}

/* Opens the asset by ID, then follows the streaming decode path of wvui_img. */
static void slideshow_blit(const wvui_slideshow_rt_t *rt, wvui_asset_id_t id, int16_t x,
                           int16_t y)
{
    wvui_blob_t b;

    /* ID 0 is a valid asset and must not be treated as unset; leave blank when it cannot be opened. */
    if (wvui_asset_open(id, &b) == 0u)
    {
        return;
    }
    (void)wvui_img_render_auto(&b, x, y, 0u, rt->opa);
}

/* Hold for hold frames, transition for slide frames, then move on to the next image. */
static uint8_t slideshow_step(wvui_anim_t *a)
{
    wvui_widget_t *w = a->owner;
    wvui_slideshow_rt_t *rt;
    int32_t span;

    if (!WVUI_W_IS_TYPE(w, &wvui_slideshow_class))
    {
        return 0u;
    }
    rt = rt_of(w);
    if ((rt->count < 2u) || (rt->autoplay == 0u))
    {
        return 0u; /* no slideshow below two images */
    }
    span = (int32_t)w->w; /* total span = widget width */

    if (rt->phase == 0u)
    {
        if (rt->hold_left != 0u)
        {
            rt->hold_left--; /* holding: stay on the chain and count */
            return 1u;
        }
        if (rt->slide_cfg == 0u)
        {
            rt->index = slideshow_next_index(rt); /* hard cut */
            rt->hold_left = rt->hold_cfg;
            wvui_widget_invalidate(w);
            return 1u;
        }
        rt->phase = 1u;
        rt->offset = 0;
        rt->slide_left = rt->slide_cfg;
        rt->frac = 0u;
        wvui_widget_invalidate(w);
        return 1u;
    }

    /* Transition: Bresenham-style accrual, division-free; exactly slide_cfg frames finish the span. */
    if (rt->slide_left != 0u)
    {
        rt->slide_left--;
    }
    rt->frac = (uint16_t)((uint32_t)rt->frac + (uint32_t)span);
    while ((rt->slide_cfg != 0u) && (rt->frac >= (uint16_t)rt->slide_cfg))
    {
        rt->frac = (uint16_t)(rt->frac - (uint16_t)rt->slide_cfg);
        if ((int32_t)rt->offset < span)
        {
            rt->offset = (int16_t)((int32_t)rt->offset + 1);
        }
    }
    if (rt->slide_left == 0u)
    {
        rt->offset = 0;
        rt->phase = 0u;
        rt->index = slideshow_next_index(rt);
        /* Use the configured value, not the compile-time default, otherwise set_timing takes effect only once. */
        rt->hold_left = rt->hold_cfg;
    }
    wvui_widget_invalidate(w);
    return 1u;
}

static void slideshow_start(wvui_widget_t *w)
{
    wvui_slideshow_rt_t *rt = rt_of(w);

    if ((rt->autoplay != 0u) && (rt->count >= 2u))
    {
        /* Permanent: an auto slideshow has no end condition; not started with ENABLE_ANIM == 0, stays on the current image. */
        wvui_anim_start_permanent(&rt->an, w, slideshow_step);
    }
    else
    {
        slideshow_stop(w);
    }
}

static void slideshow_init(wvui_widget_t *w)
{
    wvui_slideshow_rt_t *rt = rt_of(w);

    wvui_anim_init(&rt->an);
    rt->ids = 0;
    rt->count = 0u;
    rt->index = 0u;
    rt->offset = 0;
    rt->bg = 0u; /* 0 = no background fill */
    rt->hold_cfg = WVUI_CFG_SLIDESHOW_HOLD; /* default from the build constant */
    rt->hold_left = rt->hold_cfg;
    rt->slide_cfg = WVUI_CFG_SLIDESHOW_SLIDE;
    rt->phase = 0u;
    rt->autoplay = 1u;
    rt->opa = 255u;
}

static void slideshow_delete(wvui_widget_t *w)
{
    slideshow_stop(w);
}

static void slideshow_draw(wvui_widget_t *w)
{
    const wvui_slideshow_rt_t *rt = (const wvui_slideshow_rt_t *)wvui_widget_rt_c(w);
    wvui_rect_t box = wvui_widget_rect(w);

    if ((rt->opa == 0u) || (rt->ids == 0) || (rt->count == 0u) ||
        (wvui_rect_is_empty(&box) != 0u))
    {
        return;
    }
    wvui_render_set_clip(box);
    if (rt->bg != 0u)
    {
        wvui_render_fill_rect(box, rt->bg);
    }

    if ((rt->phase == 0u) || (rt->offset == 0))
    {
        slideshow_blit(rt, rt->ids[rt->index], box.x0, box.y0);
    }
    else
    {
        /* Both images on stage: the old one slides out to the left, the new one slides in from the right, each stream-decoded, no extra memory. */
        uint16_t next = slideshow_next_index(rt);

        slideshow_blit(rt, rt->ids[rt->index], (int16_t)(box.x0 - rt->offset), box.y0);
        slideshow_blit(rt, rt->ids[next], (int16_t)(box.x0 + wvui_rect_w(&box) - rt->offset),
                       box.y0);
    }
    wvui_render_clear_clip();
}

/* ---- Property handlers: VALUE = index, ENABLED = auto play ---- */
static int8_t slideshow_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in,
                                 uint16_t len)
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
        wvui_slideshow_set_index(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_ENABLED:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_slideshow_set_autoplay(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_slideshow_set_colors(w, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_slideshow_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    /* VALUE32 reuse: low byte hold frames, high byte slide frames */
    case WVUI_PROP_VALUE32:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_slideshow_set_timing(w, in[0], in[1]);
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t slideshow_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out,
                                  uint16_t cap)
{
    const wvui_slideshow_rt_t *rt = (const wvui_slideshow_rt_t *)wvui_widget_rt_c(w);

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
        out[0] = (uint8_t)((rt->index > 255u) ? 255u : rt->index);
        return 1;

    case WVUI_PROP_ENABLED:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->autoplay;
        return 1;

    case WVUI_PROP_BG:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->bg);
        return 2;

    case WVUI_PROP_OPA:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->opa;
        return 1;

    case WVUI_PROP_VALUE32:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->hold_cfg;
        out[1] = rt->slide_cfg;
        return 2;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_slideshow_class = {
    .rt_size = (uint16_t)sizeof(wvui_slideshow_rt_t),
    .draw = slideshow_draw,
    .event = 0, /* decorative: does not intercept gestures */
    .init = slideshow_init,
    .set_pos = 0,
    .on_delete = slideshow_delete,
    .set_prop = slideshow_set_prop,
    .get_prop = slideshow_get_prop,
    .flags = 0u,
};

/* ---- Property setters ---- */
void wvui_slideshow_bind(wvui_widget_t *w, const wvui_asset_id_t *ids, uint16_t count)
{
    wvui_slideshow_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_slideshow_class))
    {
        return;
    }
    rt = rt_of(w);
    if (ids == 0)
    {
        count = 0u;
    }
    rt->ids = ids;
    rt->count = count;
    rt->index = 0u;
    rt->offset = 0;
    rt->phase = 0u;
    slideshow_start(w);
    wvui_widget_invalidate(w);
}

void wvui_slideshow_next(wvui_widget_t *w)
{
    wvui_slideshow_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_slideshow_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rt->count < 2u)
    {
        return;
    }
    /* Manual switch = hard cut: clean up the transition state before advancing one image. */
    rt->offset = 0;
    rt->phase = 0u;
    rt->index = slideshow_next_index(rt);
    rt->hold_left = rt->hold_cfg;
    wvui_widget_invalidate(w);
}

void wvui_slideshow_set_index(wvui_widget_t *w, uint16_t index)
{
    wvui_slideshow_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_slideshow_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->count == 0u) || (index >= rt->count))
    {
        return;
    }
    if (rt->index != index)
    {
        rt->index = index;
        rt->offset = 0;
        rt->phase = 0u;
        rt->hold_left = rt->hold_cfg;
        wvui_widget_invalidate(w);
    }
}

void wvui_slideshow_set_timing(wvui_widget_t *w, uint8_t hold_frames, uint8_t slide_frames)
{
    wvui_slideshow_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_slideshow_class))
    {
        return;
    }
    rt = rt_of(w);
    rt->hold_cfg = (hold_frames == 0u) ? 1u : hold_frames;
    rt->hold_left = rt->hold_cfg; /* 0 hold frames clamps to 1 */
    rt->slide_cfg = slide_frames;
}

void wvui_slideshow_set_autoplay(wvui_widget_t *w, uint8_t on)
{
    wvui_slideshow_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_slideshow_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rt->autoplay != (uint8_t)(on != 0u))
    {
        rt->autoplay = (uint8_t)(on != 0u);
        slideshow_start(w);
        wvui_widget_invalidate(w);
    }
}

void wvui_slideshow_set_colors(wvui_widget_t *w, wvui_color_t bg)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_slideshow_class))
    {
        return;
    }
    if (rt_of(w)->bg != bg)
    {
        rt_of(w)->bg = bg;
        wvui_widget_invalidate(w);
    }
}

void wvui_slideshow_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_slideshow_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

uint16_t wvui_slideshow_count(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_slideshow_class))
    {
        return 0u;
    }
    return ((const wvui_slideshow_rt_t *)wvui_widget_rt_c(w))->count;
}

uint16_t wvui_slideshow_index(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_slideshow_class))
    {
        return 0u;
    }
    return ((const wvui_slideshow_rt_t *)wvui_widget_rt_c(w))->index;
}

uint8_t wvui_slideshow_is_playing(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_slideshow_class))
    {
        return 0u;
    }
    return ((const wvui_slideshow_rt_t *)wvui_widget_rt_c(w))->autoplay;
}

int16_t wvui_slideshow_offset(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_slideshow_class))
    {
        return 0;
    }
    return ((const wvui_slideshow_rt_t *)wvui_widget_rt_c(w))->offset;
}
