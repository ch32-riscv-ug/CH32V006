/**
 * @file wvui_marquee.c
 * @brief  Marquee implementation.
 * @note   Draws one pass per frame by offset; when the text is wider than the
 *         widget a second pass fills the seam.
 */

#include "wvui_marquee.h"

#include "wvui_render.h"

static wvui_marquee_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_marquee_rt_t *)wvui_widget_rt(w);
}

static void marquee_stop(wvui_widget_t *w)
{
    wvui_marquee_rt_t *rt = rt_of(w);

    if (wvui_anim_running(&rt->an) != 0u)
    {
        wvui_anim_stop(&rt->an);
    }
    rt->running = 0u;
}

/* Advance speed per frame; wraps to 0 after covering text_w + gap. */
static uint8_t marquee_step(wvui_anim_t *a)
{
    wvui_widget_t *w = a->owner;
    wvui_marquee_rt_t *rt;
    int32_t span;

    if (!WVUI_W_IS_TYPE(w, &wvui_marquee_class))
    {
        return 0u;
    }
    rt = rt_of(w);
    span = (int32_t)rt->text_w + (int32_t)rt->gap;
    if ((span <= 0) || (rt->speed == 0u))
    {
        rt->running = 0u;
        return 0u;
    }
    rt->offset = (int16_t)((int32_t)rt->offset + (int32_t)rt->speed);
    if ((int32_t)rt->offset >= span)
    {
        rt->offset = (int16_t)((int32_t)rt->offset - span); /* wrap by subtraction, not modulo */
    }
    wvui_widget_invalidate(w);
    return 1u;
}

static void marquee_init(wvui_widget_t *w)
{
    wvui_marquee_rt_t *rt = rt_of(w);

    wvui_anim_init(&rt->an);
    rt->text = 0;
    rt->font = 0;
    rt->fg = WVUI_RGB565(235, 240, 245);
    rt->text_w = 0;
    rt->text_h = 0;
    rt->offset = 0;
    rt->speed = 1u;
    rt->gap = 16u;
    rt->opa = 255u;
    rt->running = 0u;
}

static void marquee_delete(wvui_widget_t *w)
{
    marquee_stop(w);
}

void wvui_marquee_refresh(wvui_widget_t *w)
{
    wvui_marquee_rt_t *rt;
    int16_t tw = 0;
    int16_t th = 0;

    if (!WVUI_W_IS_TYPE(w, &wvui_marquee_class))
    {
        return;
    }
    rt = rt_of(w);
    rt->text_w = 0;
    rt->text_h = 0;
    if ((rt->font != 0) && (rt->text != 0) && (rt->text[0] != '\0'))
    {
        wvui_font_measure(rt->font, rt->text, &tw, &th);
        rt->text_w = tw;
        rt->text_h = th;
        (void)th;
    }

    if ((rt->text_w > w->w) && (rt->speed != 0u))
    {
        if (rt->running == 0u)
        {
            rt->offset = 0;
            rt->running = 1u;
        }
        /* Permanent node: does not start with no animation slot, rests at
         * the text start. */
        wvui_anim_start_permanent(&rt->an, w, marquee_step);
    }
    else
    {
        marquee_stop(w);
        rt->offset = 0;
    }
    wvui_widget_invalidate(w);
}

static void marquee_draw(wvui_widget_t *w)
{
    const wvui_marquee_rt_t *rt = (const wvui_marquee_rt_t *)wvui_widget_rt_c(w);
    wvui_rect_t box = wvui_widget_rect(w);
    int16_t th = rt->text_h; /* cached value: draw is called once per slice */
    int16_t ty;
    int16_t x;

    if ((rt->opa == 0u) || (rt->font == 0) || (rt->text == 0) || (rt->text[0] == '\0') ||
        (wvui_rect_is_empty(&box) != 0u))
    {
        return;
    }
    ty = (int16_t)(box.y0 + ((wvui_rect_h(&box) - th) >> 1));

    wvui_render_set_clip(box);
    x = (int16_t)((int32_t)box.x0 - (int32_t)rt->offset);
    wvui_font_draw_text(rt->font, x, ty, rt->text, rt->fg, rt->opa);
    if (rt->text_w > wvui_rect_w(&box))
    {
        /* Second pass: run at the tail, seamless at the wrap point */
        wvui_font_draw_text(rt->font, (int16_t)((int32_t)x + (int32_t)rt->text_w +
                                               (int32_t)rt->gap),
                           ty, rt->text, rt->fg, rt->opa);
    }
    wvui_render_clear_clip();
}

/* ---- Property read/write (command channel): TEXT = text, VALUE = speed
 * (pixels per frame) ---- */
static int8_t marquee_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in,
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
        wvui_marquee_refresh(w); /* width changed: re-check the scroll condition */
        return WVUI_PROP_OK;

    case WVUI_PROP_TEXT:
        if ((len == 0u) || (in == 0)) { return WVUI_PROP_ERR_ARG; }
        wvui_marquee_set_text(w, (const char *)in);
        return WVUI_PROP_OK;

    case WVUI_PROP_VALUE:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_marquee_set_speed(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_marquee_set_color(w, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_marquee_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t marquee_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_marquee_rt_t *rt = (const wvui_marquee_rt_t *)wvui_widget_rt_c(w);

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

    case WVUI_PROP_VALUE:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->speed;
        return 1;

    case WVUI_PROP_COLOR:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->fg);
        return 2;

    case WVUI_PROP_OPA:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->opa;
        return 1;

    case WVUI_PROP_ENABLED:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->running;
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_marquee_class = {
    .rt_size = (uint16_t)sizeof(wvui_marquee_rt_t),
    .draw = marquee_draw,
    .event = 0, /* decorative: no gesture interception */
    .init = marquee_init,
    .set_pos = 0,
    .on_delete = marquee_delete,
    .set_prop = marquee_set_prop,
    .get_prop = marquee_get_prop,
    .flags = 0u,
};

/* ---- Property setters ---- */
void wvui_marquee_set_text(wvui_widget_t *w, const char *text)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_marquee_class))
    {
        return;
    }
    if (rt_of(w)->text != text)
    {
        rt_of(w)->text = text;
        wvui_marquee_refresh(w); /* pointer changed: re-measure and re-check */
    }
}

void wvui_marquee_set_font(wvui_widget_t *w, const wvui_font_t *font)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_marquee_class))
    {
        return;
    }
    if (rt_of(w)->font != font)
    {
        rt_of(w)->font = font;
        wvui_marquee_refresh(w);
    }
}

void wvui_marquee_set_color(wvui_widget_t *w, wvui_color_t fg)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_marquee_class))
    {
        return;
    }
    if (rt_of(w)->fg != fg)
    {
        rt_of(w)->fg = fg;
        wvui_widget_invalidate(w);
    }
}

void wvui_marquee_set_speed(wvui_widget_t *w, uint8_t px_per_frame)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_marquee_class))
    {
        return;
    }
    if (rt_of(w)->speed != px_per_frame)
    {
        rt_of(w)->speed = px_per_frame;
        wvui_marquee_refresh(w); /* speed 0 = no scrolling */
    }
}

void wvui_marquee_set_gap(wvui_widget_t *w, uint8_t gap)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_marquee_class))
    {
        return;
    }
    if (rt_of(w)->gap != gap)
    {
        rt_of(w)->gap = gap;
        wvui_widget_invalidate(w);
    }
}

void wvui_marquee_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_marquee_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

const char *wvui_marquee_text(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_marquee_class))
    {
        return "";
    }
    return ((const wvui_marquee_rt_t *)wvui_widget_rt_c(w))->text;
}

int16_t wvui_marquee_text_width(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_marquee_class))
    {
        return 0;
    }
    return ((const wvui_marquee_rt_t *)wvui_widget_rt_c(w))->text_w;
}

int16_t wvui_marquee_offset(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_marquee_class))
    {
        return 0;
    }
    return ((const wvui_marquee_rt_t *)wvui_widget_rt_c(w))->offset;
}

uint8_t wvui_marquee_is_running(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_marquee_class))
    {
        return 0u;
    }
    return ((const wvui_marquee_rt_t *)wvui_widget_rt_c(w))->running;
}
