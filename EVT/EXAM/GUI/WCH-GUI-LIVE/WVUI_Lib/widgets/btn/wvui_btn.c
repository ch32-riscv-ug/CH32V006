/**
 * @file wvui_btn.c
 * @brief Button implementation: drawing, events and property access.
 * @note  Draw order: sliced background → base → border → text; the pressed state only changes the base color and refreshes via dirty marking.
 */

#include "wvui_btn.h"
#include "wvui_draw.h"   /* wvui_draw_round_rect(); called only when radius > 0 */
#include "wvui_render.h"

static wvui_btn_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_btn_rt_t *)wvui_widget_rt(w);
}

static void btn_init(wvui_widget_t *w)
{
    wvui_btn_rt_t *rt = rt_of(w);

    rt->text = 0;
    rt->font = 0;
    rt->bg = WVUI_RGB565(70, 78, 88);
    rt->bg_pressed = WVUI_RGB565(40, 130, 200);
    rt->fg = WVUI_RGB565(255, 255, 255);
    rt->border = WVUI_RGB565(120, 130, 140);
    rt->opa = 255u;
    rt->pressed = 0u;
    rt->enabled = 1u;
    rt->pad = 2u;
    rt->radius = 0u;
    rt->on_click = 0;
    rt->user = 0;
}

/* 1 px border: four rectangles, all through the render layer (slice-aware + clipped). */
static void draw_border(wvui_widget_t *w, wvui_color_t c)
{
    wvui_rect_t r = wvui_widget_rect(w);
    wvui_rect_t e;

    e.x0 = r.x0; e.x1 = r.x1; e.y0 = r.y0; e.y1 = r.y0;
    wvui_render_fill_rect(e, c);
    e.y0 = r.y1; e.y1 = r.y1;
    wvui_render_fill_rect(e, c);
    e.y0 = r.y0; e.y1 = r.y1; e.x0 = r.x0; e.x1 = r.x0;
    wvui_render_fill_rect(e, c);
    e.x0 = r.x1; e.x1 = r.x1;
    wvui_render_fill_rect(e, c);
}

static void btn_draw(wvui_widget_t *w)
{
    wvui_btn_rt_t *rt = rt_of(w);
    wvui_rect_t r = wvui_widget_rect(w);
    wvui_color_t bg;

    wvui_render_set_clip(r);

    if (rt->enabled == 0u)
    {
        bg = WVUI_RGB565(60, 60, 60); /* disabled base color */
    }
    else
    {
        bg = (rt->pressed != 0u) ? rt->bg_pressed : rt->bg;
    }

    if (rt->radius > 0u)
    {
        /* Rounded corners call wvui_draw_round_rect() once: base + 1 px border, the radius is clamped to min(w,h)/2 here */
        uint8_t rad = rt->radius;
        uint8_t bw = (uint8_t)((rt->border != bg) ? 1u : 0u);
        uint8_t lim = (uint8_t)((w->w < w->h) ? (w->w >> 1) : (w->h >> 1));

        if (rad > lim)
        {
            rad = lim;
        }

        wvui_draw_round_rect(r, rad, bw, bg, rt->opa, rt->border, (bw != 0u) ? rt->opa : 0u);
    }
    else
    {
        wvui_render_fill_rect_blend(r, bg, rt->opa);

        if (rt->border != bg)
        {
            draw_border(w, rt->border);
        }
    }

    if ((rt->text != 0) && (rt->font != 0) && (rt->text[0] != '\0'))
    {
        int16_t tw = 0;
        int16_t th = 0;
        int16_t tx;
        int16_t ty;

        wvui_font_measure(rt->font, rt->text, &tw, &th);
        tx = (int16_t)(w->x + ((int16_t)(w->w - tw) >> 1));
        ty = (int16_t)(w->y + ((int16_t)(w->h - th) >> 1));
        wvui_font_draw_text(rt->font, tx, ty, rt->text, rt->fg, rt->opa);
    }

    wvui_render_clear_clip();
}

/* Non-zero return = consumed: this widget becomes the pressed_obj and later events
 * are dispatched to it by the kernel directly. The callback must not delete itself
 * or ancestors: the kernel still dereferences the node after the event returns. */
static uint8_t btn_event(wvui_widget_t *w, wvui_event_t e, wvui_indev_t *d)
{
    wvui_btn_rt_t *rt = rt_of(w);

    (void)d;

    switch (e)
    {
    case WVUI_EVENT_PRESSED:
        if (rt->enabled == 0u)
        {
            return 0u; /* Disabled: leave the event to the outer layers */
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
        return 1u;

    case WVUI_EVENT_CLICKED:
        if (rt->pressed != 0u)
        {
            rt->pressed = 0u;
            wvui_widget_invalidate(w);
        }
        if (rt->on_click != 0)
        {
            rt->on_click(w, rt->user);
        }
        return 1u;

    default:
        return 0u;
    }
}

/* ---- Property access (destination of the command channel, same path as the setters) ---- */
/* PRESSED is read-only: the pressed state belongs to the input layer's capture and drift detection; programmatic clicks go through WVUI_CMD_OP_INPUT(). */
static int8_t btn_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
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
        wvui_btn_set_text(w, (const char *)in);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        rt_of(w)->fg = wvui_prop_u16(in);
        wvui_widget_invalidate(w);
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        rt_of(w)->opa = in[0];
        wvui_widget_invalidate(w);
        return WVUI_PROP_OK;

    case WVUI_PROP_ENABLED:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_btn_set_enabled(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        rt_of(w)->bg = wvui_prop_u16(in);
        wvui_widget_invalidate(w);
        return WVUI_PROP_OK;

    case WVUI_PROP_BG_PRESSED:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        rt_of(w)->bg_pressed = wvui_prop_u16(in);
        wvui_widget_invalidate(w);
        return WVUI_PROP_OK;

    case WVUI_PROP_BORDER:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_btn_set_border(w, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_RADIUS:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_btn_set_radius(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_PRESSED:
        return WVUI_PROP_ERR_RO;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t btn_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_btn_rt_t *rt = (const wvui_btn_rt_t *)wvui_widget_rt_c(w);

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
        out[0] = rt->enabled;
        return 1;

    case WVUI_PROP_PRESSED:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->pressed;
        return 1;

    case WVUI_PROP_BG:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->bg);
        return 2;

    case WVUI_PROP_BG_PRESSED:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->bg_pressed);
        return 2;

    case WVUI_PROP_BORDER:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->border);
        return 2;

    case WVUI_PROP_RADIUS:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->radius;
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_btn_class = {
    .rt_size = (uint16_t)sizeof(wvui_btn_rt_t),
    .draw = btn_draw,
    .event = btn_event,
    .init = btn_init,
    .set_pos = 0,
    .on_delete = 0,
    .set_prop = btn_set_prop,
    .get_prop = btn_get_prop,
    /* flags = 0: hit testing uses geometry with a HIT_TEST veto */
    .flags = 0u,
};

/* ---- Property setters ---- */
void wvui_btn_set_text(wvui_widget_t *w, const char *text)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_btn_class))
    {
        return;
    }
    /* Text stores only a pointer: an equal pointer does not mean equal content, so mark dirty unconditionally */
    wvui_widget_invalidate(w);
    rt_of(w)->text = text;
    wvui_widget_invalidate(w);
}

void wvui_btn_set_font(wvui_widget_t *w, const wvui_font_t *font)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_btn_class))
    {
        return;
    }
    if (rt_of(w)->font != font)
    {
        rt_of(w)->font = font;
        wvui_widget_invalidate(w);
    }
}

void wvui_btn_set_colors(wvui_widget_t *w, wvui_color_t bg, wvui_color_t bg_pressed, wvui_color_t fg)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_btn_class))
    {
        return;
    }
    rt_of(w)->bg = bg;
    rt_of(w)->bg_pressed = bg_pressed;
    rt_of(w)->fg = fg;
    wvui_widget_invalidate(w);
}

void wvui_btn_set_border(wvui_widget_t *w, wvui_color_t border)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_btn_class))
    {
        return;
    }
    rt_of(w)->border = border;
    wvui_widget_invalidate(w);
}

void wvui_btn_set_radius(wvui_widget_t *w, uint8_t radius)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_btn_class))
    {
        return;
    }
    if (rt_of(w)->radius != radius)
    {
        rt_of(w)->radius = radius;
        wvui_widget_invalidate(w);
    }
}

uint8_t wvui_btn_radius(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_btn_class))
    {
        return 0u;
    }
    return ((const wvui_btn_rt_t *)wvui_widget_rt_c(w))->radius;
}

void wvui_btn_set_enabled(wvui_widget_t *w, uint8_t enabled)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_btn_class))
    {
        return;
    }
    if (rt_of(w)->enabled != (uint8_t)(enabled != 0u))
    {
        rt_of(w)->enabled = (uint8_t)(enabled != 0u);
        wvui_widget_invalidate(w);
    }
}

void wvui_btn_set_click_cb(wvui_widget_t *w, wvui_btn_click_cb_t cb, void *user)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_btn_class))
    {
        return;
    }
    rt_of(w)->on_click = cb;
    rt_of(w)->user = user;
}

uint8_t wvui_btn_is_pressed(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_btn_class))
    {
        return 0u;
    }
    return ((const wvui_btn_rt_t *)wvui_widget_rt_c(w))->pressed;
}
