/**
 * @file wvui_monoimg.c
 * @brief 1bpp monochrome image implementation.
 * @note  Row stride is (w + 7) >> 3; only rows and columns inside the clip area are walked.
 */

#include "wvui_monoimg.h"

#include "wvui_render.h"

static wvui_monoimg_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_monoimg_rt_t *)wvui_widget_rt(w);
}

static const wvui_monoimg_rt_t *rt_of_c(const wvui_widget_t *w)
{
    return (const wvui_monoimg_rt_t *)wvui_widget_rt_c(w);
}

static void monoimg_init(wvui_widget_t *w)
{
    wvui_monoimg_rt_t *rt = rt_of(w);

    rt->bits = 0;
    rt->color = WVUI_RGB565(235, 240, 245);
    rt->bw = 0;
    rt->bh = 0;
    rt->opa = 255u;
    rt->invert = 0u;
}

static void monoimg_draw(wvui_widget_t *w)
{
    const wvui_monoimg_rt_t *rt = rt_of_c(w);
    wvui_rect_t box = wvui_widget_rect(w);
    wvui_rect_t clip;
    int32_t stride;
    int32_t gx0;
    int32_t gx1;
    int32_t gy0;
    int32_t gy1;
    int32_t py;

    if ((rt->bits == 0) || (rt->bw <= 0) || (rt->bh <= 0) || (rt->opa == 0u) ||
        (wvui_rect_is_empty(&box) != 0u))
    {
        return;
    }

    wvui_render_set_clip(box);
    clip = wvui_render_clip(); /* Widget rect ∩ current tile */
    if (wvui_rect_is_empty(&clip) != 0u)
    {
        wvui_render_clear_clip();
        return;
    }

    /* Intersect the bitmap rect with the clip area: bitmap pasted 1:1 from the widget's top-left */
    gx0 = (clip.x0 > box.x0) ? clip.x0 : box.x0;
    gx1 = clip.x1;
    if (gx1 > (int32_t)box.x0 + (int32_t)rt->bw - 1)
    {
        gx1 = (int32_t)box.x0 + (int32_t)rt->bw - 1;
    }
    gy0 = (clip.y0 > box.y0) ? clip.y0 : box.y0;
    gy1 = clip.y1;
    if (gy1 > (int32_t)box.y0 + (int32_t)rt->bh - 1)
    {
        gy1 = (int32_t)box.y0 + (int32_t)rt->bh - 1;
    }
    if ((gx0 > gx1) || (gy0 > gy1))
    {
        wvui_render_clear_clip();
        return;
    }

    stride = ((int32_t)rt->bw + 7) >> 3;
    for (py = gy0; py <= gy1; py++)
    {
        const uint8_t *row = rt->bits + ((int32_t)(py - box.y0) * stride);
        int32_t px;

        for (px = gx0; px <= gx1; px++)
        {
            int32_t bit = px - box.x0;
            uint8_t on = (uint8_t)((row[bit >> 3] >> (7 - (bit & 7))) & 1u);

            if (rt->invert != 0u)
            {
                on = (uint8_t)(on == 0u);
            }
            if (on != 0u)
            {
                wvui_render_pixel((int16_t)px, (int16_t)py, rt->color, rt->opa);
            }
        }
    }
    wvui_render_clear_clip();
}

/* ---- Property read/write (command channel endpoint) ---- */
/* The bitmap is a pointer and cannot go through TLV; bind only via wvui_monoimg_set_bitmap(). */
static int8_t monoimg_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
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

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_monoimg_set_color(w, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_monoimg_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_ENABLED: /* Reused: non-0 = invert */
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_monoimg_set_invert(w, (uint8_t)(in[0] != 0u));
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t monoimg_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_monoimg_rt_t *rt = rt_of_c(w);

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

    case WVUI_PROP_OPA:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->opa;
        return 1;

    case WVUI_PROP_ENABLED:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->invert;
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_monoimg_class = {
    .rt_size = (uint16_t)sizeof(wvui_monoimg_rt_t),
    .draw = monoimg_draw,
    .event = 0, /* Decorative: no gesture interception */
    .init = monoimg_init,
    .set_pos = 0,
    .on_delete = 0,
    .set_prop = monoimg_set_prop,
    .get_prop = monoimg_get_prop,
    .flags = 0u,
};

/* ---- Property setters ---- */
uint8_t wvui_monoimg_set_bitmap(wvui_widget_t *w, const uint8_t *bits, int16_t bw, int16_t bh)
{
    wvui_monoimg_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_monoimg_class))
    {
        return 0u;
    }
    if ((bits == 0) || (bw <= 0) || (bh <= 0))
    {
        return 0u;
    }
    rt = rt_of(w);
    rt->bits = bits;
    rt->bw = bw;
    rt->bh = bh;
    wvui_widget_invalidate(w);
    return 1u;
}

void wvui_monoimg_set_color(wvui_widget_t *w, wvui_color_t color)
{
    wvui_monoimg_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_monoimg_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rt->color != color)
    {
        rt->color = color;
        wvui_widget_invalidate(w);
    }
}

void wvui_monoimg_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_monoimg_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

void wvui_monoimg_set_invert(wvui_widget_t *w, uint8_t invert)
{
    uint8_t v = (uint8_t)((invert != 0u) ? 1u : 0u);

    if (!WVUI_W_IS_TYPE(w, &wvui_monoimg_class))
    {
        return;
    }
    if (rt_of(w)->invert != v)
    {
        rt_of(w)->invert = v;
        wvui_widget_invalidate(w);
    }
}

const uint8_t *wvui_monoimg_bits(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_monoimg_class))
    {
        return 0;
    }
    return rt_of_c(w)->bits;
}

int16_t wvui_monoimg_width(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_monoimg_class))
    {
        return 0;
    }
    return rt_of_c(w)->bw;
}

int16_t wvui_monoimg_height(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_monoimg_class))
    {
        return 0;
    }
    return rt_of_c(w)->bh;
}
