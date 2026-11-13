/**
 * @file wvui_toast.c
 * @brief Floating toast strip implementation: the fade-out uses a256 fixed-point scaling, the lifetime timer is driven by an anim node.
 * @note  Drawing is stateless; the timer node detaches itself on expiry and the widget object is kept.
 */

#include "wvui_toast.h"

#include "wvui_config.h"
#include "wvui_draw.h"
#include "wvui_layer.h"
#include "wvui_render.h"

static wvui_toast_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_toast_rt_t *)wvui_widget_rt(w);
}

static void toast_stop_anim(wvui_widget_t *w)
{
    wvui_toast_rt_t *rt = rt_of(w);

    if (wvui_anim_running(&rt->an) != 0u)
    {
        wvui_anim_stop(&rt->an);
    }
}

/* Opacity of the current frame: linear fade-out over the last FADE frames (a256 fixed point, division-free). */
static uint8_t toast_opa(const wvui_toast_rt_t *rt)
{
    uint32_t a;

    if (rt->left >= (uint16_t)WVUI_CFG_TOAST_FADE)
    {
        return rt->opa;
    }
    /* a256 form of a = left / 255 */
    a = (uint32_t)rt->left + ((uint32_t)rt->left >> 7);
    return (uint8_t)(((uint32_t)rt->opa * a) >> 8);
}

/* Timer callback: decrement every frame, detach at 0 (the anim engine allows detaching but forbids deleting widgets).
 * Mark dirty only when opa actually changes: opa is constant while left >= FADE. */
static uint8_t toast_step(wvui_anim_t *a)
{
    wvui_widget_t *w = a->owner;
    wvui_toast_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_toast_class))
    {
        return 0u;
    }
    rt = rt_of(w);
    if (rt->left > 0u)
    {
        rt->left = (uint16_t)(rt->left - 1u);
    }

    /* left < FADE means opa differs from the previous frame; with FADE == 0 only the expiry branch marks dirty. */
    if (rt->left < (uint16_t)WVUI_CFG_TOAST_FADE)
    {
        wvui_widget_invalidate(w);
    }
    if (rt->left == 0u)
    {
        rt->active = 0u;
        wvui_layer_detach(w);
        wvui_widget_invalidate(w);
        return 0u;
    }
    return 1u;
}

static void toast_init(wvui_widget_t *w)
{
    wvui_toast_rt_t *rt = rt_of(w);

    wvui_anim_init(&rt->an);
    rt->text = 0;
    rt->font = 0;
    rt->bg = WVUI_RGB565(48, 52, 60);
    rt->fg = WVUI_RGB565(240, 240, 240);
    rt->border = WVUI_RGB565(120, 130, 140);
    rt->left = 0u;
    rt->total = 0u;
    rt->radius = 6u;
    rt->opa = 255u;
    rt->active = 0u;
}

static void toast_delete(wvui_widget_t *w)
{
    toast_stop_anim(w);
}

static void toast_draw(wvui_widget_t *w)
{
    const wvui_toast_rt_t *rt = (const wvui_toast_rt_t *)wvui_widget_rt_c(w);
    wvui_rect_t box = wvui_widget_rect(w);
    uint8_t opa = toast_opa(rt);

    if ((opa == 0u) || (rt->active == 0u) || (wvui_rect_is_empty(&box) != 0u))
    {
        return;
    }

    wvui_draw_round_rect(box, rt->radius, 1u, rt->bg, opa, rt->border, opa);

    if ((rt->font != 0) && (rt->text != 0) && (rt->text[0] != '\0'))
    {
        int16_t tw = 0;
        int16_t th = 0;
        int16_t tx;
        int16_t ty;

        wvui_font_measure(rt->font, rt->text, &tw, &th);
        tx = (int16_t)(box.x0 + ((wvui_rect_w(&box) - tw) >> 1));
        ty = (int16_t)(box.y0 + ((wvui_rect_h(&box) - th) >> 1));
        wvui_render_set_clip(box);
        wvui_font_draw_text(rt->font, tx, ty, rt->text, rt->fg, opa);
        wvui_render_clear_clip();
    }
}

/* ---- Property handlers (command channel) ---- */
static int8_t toast_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
{
    wvui_toast_rt_t *rt = rt_of(w);

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

    case WVUI_PROP_TEXT: /* text pops the toast, the frame count falls back to the default */
        if ((len == 0u) || (in == 0)) { return WVUI_PROP_ERR_ARG; }
        wvui_toast_show(w, (const char *)in, 0u);
        return WVUI_PROP_OK;

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_toast_set_colors(w, wvui_prop_u16(in), rt->fg, rt->border);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_toast_set_colors(w, rt->bg, wvui_prop_u16(in), rt->border);
        return WVUI_PROP_OK;

    case WVUI_PROP_BORDER:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_toast_set_colors(w, rt->bg, rt->fg, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_toast_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_RADIUS:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_toast_set_radius(w, in[0]);
        return WVUI_PROP_OK;

    /* VALUE: frames remaining; writing it = restart timing */
    case WVUI_PROP_VALUE:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_toast_show(w, rt->text, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t toast_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_toast_rt_t *rt = (const wvui_toast_rt_t *)wvui_widget_rt_c(w);

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

    case WVUI_PROP_TEXT:
        return (int16_t)wvui_prop_write_text(out, cap, rt->text);

    case WVUI_PROP_BG:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->bg);
        return 2;

    case WVUI_PROP_COLOR:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->fg);
        return 2;

    case WVUI_PROP_BORDER:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->border);
        return 2;

    case WVUI_PROP_OPA:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->opa;
        return 1;

    case WVUI_PROP_RADIUS:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->radius;
        return 1;

    case WVUI_PROP_VALUE:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->left);
        return 2;

    case WVUI_PROP_ENABLED:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->active;
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_toast_class = {
    .rt_size = (uint16_t)sizeof(wvui_toast_rt_t),
    .draw = toast_draw,
    .event = 0, /* decorative widget: does not intercept gestures */
    .init = toast_init,
    .set_pos = 0,
    .on_delete = toast_delete,
    .set_prop = toast_set_prop,
    .get_prop = toast_get_prop,
    .flags = 0u,
};

/* ---- Property setters ---- */
void wvui_toast_show(wvui_widget_t *w, const char *text, uint16_t frames)
{
    wvui_toast_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_toast_class))
    {
        return;
    }
    rt = rt_of(w);
    rt->text = text;
    rt->total = (frames != 0u) ? frames : (uint16_t)WVUI_CFG_TOAST_FRAMES;
    rt->left = rt->total;
    rt->active = 1u;

    /* Attach to the top layer list, idempotent (already attached means detach first and attach again, still on top); the object must live in persistent memory to survive page switches. */
    (void)wvui_layer_attach(w);
    wvui_widget_invalidate(w);

    /* Timing: idempotent start; the permanent node's callback is the lifetime timer (detach when left reaches 0).
     * Not started with ENABLE_ANIM == 0: the toast shows but never expires (see wvui_config.h). */
    wvui_anim_start_permanent(&rt->an, w, toast_step);
}

void wvui_toast_hide(wvui_widget_t *w)
{
    wvui_toast_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_toast_class))
    {
        return;
    }
    rt = rt_of(w);
    rt->left = 0u;
    rt->active = 0u;
    toast_stop_anim(w);
    wvui_layer_detach(w);
    wvui_widget_invalidate(w);
}

void wvui_toast_set_font(wvui_widget_t *w, const wvui_font_t *font)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_toast_class))
    {
        return;
    }
    if (rt_of(w)->font != font)
    {
        rt_of(w)->font = font;
        wvui_widget_invalidate(w);
    }
}

void wvui_toast_set_colors(wvui_widget_t *w, wvui_color_t bg, wvui_color_t fg,
                          wvui_color_t border)
{
    wvui_toast_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_toast_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->bg == bg) && (rt->fg == fg) && (rt->border == border))
    {
        return;
    }
    rt->bg = bg;
    rt->fg = fg;
    rt->border = border;
    wvui_widget_invalidate(w);
}

void wvui_toast_set_radius(wvui_widget_t *w, uint8_t radius)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_toast_class))
    {
        return;
    }
    if (rt_of(w)->radius != radius)
    {
        rt_of(w)->radius = radius;
        wvui_widget_invalidate(w);
    }
}

void wvui_toast_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_toast_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

uint8_t wvui_toast_is_active(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_toast_class))
    {
        return 0u;
    }
    return ((const wvui_toast_rt_t *)wvui_widget_rt_c(w))->active;
}

uint16_t wvui_toast_left(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_toast_class))
    {
        return 0u;
    }
    return ((const wvui_toast_rt_t *)wvui_widget_rt_c(w))->left;
}
