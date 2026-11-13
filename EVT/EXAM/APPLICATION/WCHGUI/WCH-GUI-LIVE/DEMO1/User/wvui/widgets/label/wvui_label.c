/**
 * @file wvui_label.c
 * @brief Text label implementation: overlays text only, no background.
 * @note  The text pointer is owned by the caller, the widget does not copy it.
 */

#include "wvui_label.h"
#include "wvui_render.h"

static wvui_label_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_label_rt_t *)wvui_widget_rt(w);
}

static void label_init(wvui_widget_t *w)
{
    wvui_label_rt_t *rt = rt_of(w);

    rt->text = 0;
    rt->font = 0;
    rt->color = WVUI_RGB565(255, 255, 255);
    rt->opa = 255u;
    rt->align = WVUI_LABEL_ALIGN_LEFT;
    rt->line_gap = 0;
}

/* Centering uses >>1 instead of /2: arithmetic right shift floors negative values, off by at most 1 pixel, no division on the hot path. */
static int16_t align_x(const wvui_widget_t *w, int16_t text_w, uint8_t align)
{
    if (align == WVUI_LABEL_ALIGN_CENTER)
    {
        return (int16_t)(w->x + ((int16_t)(w->w - text_w) >> 1));
    }
    if (align == WVUI_LABEL_ALIGN_RIGHT)
    {
        return (int16_t)(w->x + w->w - text_w);
    }
    return w->x;
}

static void label_draw(wvui_widget_t *w)
{
    wvui_label_rt_t *rt = rt_of(w);
    int16_t tw = 0;
    int16_t th = 0;
    int16_t tx;
    int16_t ty;

    if ((rt->font == 0) || (rt->text == 0) || (rt->text[0] == '\0') || (rt->opa == 0u))
    {
        return;
    }

    wvui_font_measure(rt->font, rt->text, &tw, &th);
    tx = align_x(w, tw, rt->align);
    ty = (int16_t)(w->y + ((int16_t)(w->h - th) >> 1));

    /* Clip to the widget rectangle: no overflow when text is wider than the widget. */
    wvui_render_set_clip(wvui_widget_rect(w));
    wvui_font_draw_text(rt->font, tx, ty, rt->text, rt->color, rt->opa);
    wvui_render_clear_clip();
}

/* ---- Per-line dirty marking: skip a line only when both its content and line rect are unchanged ---- */
static void label_invalidate_lines(wvui_widget_t *w, const char *old_text, const char *new_text)
{
    const wvui_label_rt_t *rt = (const wvui_label_rt_t *)wvui_widget_rt_c(w);
    const wvui_font_t *f = rt->font;
    wvui_rect_t box = wvui_widget_rect(w);
    int16_t tw_o = 0;
    int16_t th_o = 0;
    int16_t tw_n = 0;
    int16_t th_n = 0;
    int16_t tx_o;
    int16_t ty_o;
    int16_t tx_n;
    int16_t ty_n;
    uint16_t i;

    if (f == 0)
    {
        wvui_widget_invalidate(w); /* No font: old pixels still need erasing */
        return;
    }
    if ((wvui_font_line_count(old_text) > WVUI_CFG_TEXT_LINE_MAX) ||
        (wvui_font_line_count(new_text) > WVUI_CFG_TEXT_LINE_MAX))
    {
        wvui_widget_invalidate(w); /* Over the line limit: mark the whole block dirty */
        return;
    }

    wvui_font_measure(f, old_text, &tw_o, &th_o);
    wvui_font_measure(f, new_text, &tw_n, &th_n);
    tx_o = align_x(w, tw_o, rt->align);
    ty_o = (int16_t)(w->y + ((int16_t)(w->h - th_o) >> 1));
    tx_n = align_x(w, tw_n, rt->align);
    ty_n = (int16_t)(w->y + ((int16_t)(w->h - th_n) >> 1));

    for (i = 0u; i < (uint16_t)WVUI_CFG_TEXT_LINE_MAX; i++)
    {
        wvui_rect_t r_o;
        wvui_rect_t r_n;
        uint8_t has_o = wvui_font_line_rect(f, old_text, i, tx_o, tw_o, ty_o, &r_o);
        uint8_t has_n = wvui_font_line_rect(f, new_text, i, tx_n, tw_n, ty_n, &r_n);

        if ((has_o == 0u) && (has_n == 0u))
        {
            break; /* Neither text has this line */
        }
        if ((has_o != 0u) && (has_n != 0u) && (wvui_font_line_equal(old_text, new_text, i) != 0u) &&
            (r_o.x0 == r_n.x0) && (r_o.y0 == r_n.y0) && (r_o.x1 == r_n.x1) && (r_o.y1 == r_n.y1))
        {
            continue; /* This line is unchanged */
        }
        if (has_o != 0u)
        {
            wvui_widget_invalidate_rect(w, wvui_rect_clip(r_o, box));
        }
        if (has_n != 0u)
        {
            wvui_widget_invalidate_rect(w, wvui_rect_clip(r_n, box));
        }
    }
}

/* ---- Property get/set (command channel) ---- */
static int8_t label_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
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

    case WVUI_PROP_TEXT:
        if ((len == 0u) || (in == 0)) { return WVUI_PROP_ERR_ARG; }
        /* The command layer guarantees NUL termination (text pool); the label stores the pointer, no copy. */
        wvui_label_set_text(w, (const char *)in);
        return WVUI_PROP_OK;

    case WVUI_PROP_ALIGN:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        if (in[0] > WVUI_LABEL_ALIGN_RIGHT) { return WVUI_PROP_ERR_ARG; }
        wvui_label_set_align(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_label_set_color(w, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_label_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t label_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_label_rt_t *rt = (const wvui_label_rt_t *)wvui_widget_rt_c(w);

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

    case WVUI_PROP_ALIGN:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->align;
        return 1;

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

const wvui_class_t wvui_label_class = {
    .rt_size = (uint16_t)sizeof(wvui_label_rt_t),
    .draw = label_draw,
    .event = 0, /* Decorative widget: no gesture interception */
    .init = label_init,
    .set_pos = 0,
    .on_delete = 0,
    .set_prop = label_set_prop,
    .get_prop = label_get_prop,
    .flags = 0u,
};

/* ---- Property setters ---- */
void wvui_label_set_text(wvui_widget_t *w, const char *text)
{
    wvui_label_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_label_class))
    {
        return;
    }
    rt = rt_of(w);

    /* Same pointer: the same buffer was rewritten, old content is gone, mark the whole block dirty.
     * New pointer: compare old vs new line rects, mark only the changed lines.
     * The widget stores the pointer, never the text. */
    if (rt->text == text)
    {
        wvui_widget_invalidate(w);
    }
    else
    {
        label_invalidate_lines(w, rt->text, text);
    }
    rt->text = text;
}

void wvui_label_set_font(wvui_widget_t *w, const wvui_font_t *font)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_label_class))
    {
        return;
    }
    if (rt_of(w)->font != font)
    {
        rt_of(w)->font = font;
        wvui_widget_invalidate(w);
    }
}

void wvui_label_set_color(wvui_widget_t *w, wvui_color_t color)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_label_class))
    {
        return;
    }
    if (rt_of(w)->color != color)
    {
        rt_of(w)->color = color;
        wvui_widget_invalidate(w);
    }
}

void wvui_label_set_align(wvui_widget_t *w, uint8_t align)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_label_class))
    {
        return;
    }
    if (rt_of(w)->align != align)
    {
        rt_of(w)->align = align;
        wvui_widget_invalidate(w);
    }
}

void wvui_label_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_label_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

const char *wvui_label_text(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_label_class))
    {
        return 0;
    }
    return ((const wvui_label_rt_t *)wvui_widget_rt_c(w))->text;
}
