/**
 * @file wvui_img.c
 * @brief Image widget implementation: opens the asset by ID once per draw, overlays the image without painting a background.
 * @note  In-array backend is zero-copy.
 */

#include "wvui_img.h"

#include "wvui_asset.h"
#include "wvui_render.h"

static wvui_img_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_img_rt_t *)wvui_widget_rt(w);
}

static void img_init(wvui_widget_t *w)
{
    wvui_img_rt_t *rt = rt_of(w);

    rt->asset = 0u;
    rt->color = WVUI_RGB565(255, 255, 255);
    rt->opa = 255u;
}

static void img_draw(wvui_widget_t *w)
{
    const wvui_img_rt_t *rt = (const wvui_img_rt_t *)wvui_widget_rt_c(w);
    wvui_blob_t blob;

    if (rt->opa == 0u)
    {
        return;
    }
    /* On open failure the blob is already erased: the render layer returns 0 and logs the cause into wvui_img_stats(). */
    (void)wvui_asset_open(rt->asset, &blob);
    (void)wvui_img_render_auto(&blob, w->x, w->y, rt->color, rt->opa);
}

/* ---- Property get/set: commands and C code change properties through the same path ---- */
static int8_t img_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
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

    case WVUI_PROP_ASSET:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_img_set_asset(w, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_img_set_color(w, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_img_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t img_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_img_rt_t *rt = (const wvui_img_rt_t *)wvui_widget_rt_c(w);

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

    case WVUI_PROP_ASSET:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->asset);
        return 2;

    case WVUI_PROP_COLOR:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->color);
        return 2;

    case WVUI_PROP_OPA:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->opa;
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_img_class = {
    .rt_size = (uint16_t)sizeof(wvui_img_rt_t),
    .draw = img_draw,
    .event = 0, /* Decorative widget: no gesture interception */
    .init = img_init,
    .set_pos = 0,
    .on_delete = 0,
    .set_prop = img_set_prop,
    .get_prop = img_get_prop,
    .flags = 0u,
};

/* ---- Property setters: the command channel calls the same functions, every setter marks dirty ---- */
void wvui_img_set_asset(wvui_widget_t *w, wvui_asset_id_t id)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_img_class))
    {
        return;
    }
    if (rt_of(w)->asset != id)
    {
        rt_of(w)->asset = id;
        wvui_widget_invalidate(w);
    }
}

void wvui_img_set_color(wvui_widget_t *w, wvui_color_t color)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_img_class))
    {
        return;
    }
    if (rt_of(w)->color != color)
    {
        rt_of(w)->color = color;
        wvui_widget_invalidate(w);
    }
}

void wvui_img_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_img_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

wvui_asset_id_t wvui_img_asset(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_img_class))
    {
        return 0u;
    }
    return ((const wvui_img_rt_t *)wvui_widget_rt_c(w))->asset;
}

uint8_t wvui_img_opa(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_img_class))
    {
        return 0u;
    }
    return ((const wvui_img_rt_t *)wvui_widget_rt_c(w))->opa;
}
