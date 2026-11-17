/**
 * @file wvui_msgbox.c
 * @brief Modal dialog implementation: three-band layout, button strips and press/release handling.
 * @note  Layout is derived by shifting the widget rect; modality belongs to wvui_layer, this file only decides what to draw and where taps land.
 */

#include "wvui_msgbox.h"

#include "wvui_draw.h"
#include "wvui_layer.h"
#include "wvui_render.h"

static wvui_msgbox_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_msgbox_rt_t *)wvui_widget_rt(w);
}

static void msgbox_init(wvui_widget_t *w)
{
    wvui_msgbox_rt_t *rt = rt_of(w);

    rt->title = 0;
    rt->message = 0;
    rt->ok_text = 0;
    rt->cancel_text = 0;
    rt->font = 0;
    rt->bg = WVUI_RGB565(40, 44, 52);
    rt->fg = WVUI_RGB565(230, 235, 240);
    rt->border = WVUI_RGB565(120, 130, 140);
    rt->btn_bg = WVUI_RGB565(70, 78, 88);
    rt->btn_fg = WVUI_RGB565(255, 255, 255);
    rt->btn_pressed = WVUI_RGB565(40, 130, 200);
    rt->two_buttons = 1u;
    rt->pressed = 0u;
    rt->radius = 8u;
    rt->opa = 255u;
    rt->on_ok = 0;
    rt->on_cancel = 0;
    rt->user = 0;
}

/* Button row height = 1/4 of the widget height (clamped to [12, 32]), the basis of the three-band layout. */
static int32_t msgbox_btn_h(const wvui_widget_t *w)
{
    int32_t h = (int32_t)w->h >> 2;

    if (h < 12)
    {
        h = 12;
    }
    if (h > 32)
    {
        h = 32;
    }
    if (h > (int32_t)w->h - 8)
    {
        h = (int32_t)w->h - 8; /* Button row tops out 8 px above the widget bottom edge */
    }
    if (h < 4)
    {
        h = 4;
    }
    return h;
}

/* Button rect (part: 1 = cancel, 2 = OK). Two buttons split left/right, one button takes the row. */
static void msgbox_button_rect(const wvui_widget_t *w, const wvui_msgbox_rt_t *rt, uint8_t part,
                               wvui_rect_t *out)
{
    int32_t bh = msgbox_btn_h(w);
    int32_t pad = 4;
    int32_t inner_w = (int32_t)w->w - (pad * 2);
    int32_t bw;

    out->y0 = (int16_t)((int32_t)w->y + (int32_t)w->h - pad - bh);
    out->y1 = (int16_t)((int32_t)w->y + (int32_t)w->h - pad - 1);

    if (rt->two_buttons == 0u)
    {
        out->x0 = (int16_t)((int32_t)w->x + pad);
        out->x1 = (int16_t)((int32_t)w->x + pad + inner_w - 1);
        return;
    }
    bw = (inner_w - 4) >> 1; /* 4 px gap in the middle */
    if (part == 1u)
    {
        out->x0 = (int16_t)((int32_t)w->x + pad);
    }
    else
    {
        out->x0 = (int16_t)((int32_t)w->x + pad + bw + 4);
    }
    out->x1 = (int16_t)((int32_t)out->x0 + bw - 1);
}

/* Which button the point lands on (0 = outside the button row). */
static uint8_t msgbox_part_at(const wvui_widget_t *w, const wvui_msgbox_rt_t *rt, int16_t x,
                              int16_t y)
{
    wvui_rect_t r;

    if (rt->two_buttons == 0u)
    {
        msgbox_button_rect(w, rt, 2u, &r);
        return (uint8_t)(((x >= r.x0) && (x <= r.x1) && (y >= r.y0) && (y <= r.y1)) ? 2u : 0u);
    }
    msgbox_button_rect(w, rt, 1u, &r);
    if ((x >= r.x0) && (x <= r.x1) && (y >= r.y0) && (y <= r.y1))
    {
        return 1u;
    }
    msgbox_button_rect(w, rt, 2u, &r);
    if ((x >= r.x0) && (x <= r.x1) && (y >= r.y0) && (y <= r.y1))
    {
        return 2u;
    }
    return 0u;
}

static void msgbox_text(const wvui_widget_t *w, const wvui_msgbox_rt_t *rt, const char *s,
                        wvui_rect_t area, wvui_color_t color)
{
    int16_t tw = 0;
    int16_t th = 0;
    int16_t tx;
    int16_t ty;

    if ((rt->font == 0) || (s == 0) || (s[0] == '\0'))
    {
        return;
    }
    wvui_font_measure(rt->font, s, &tw, &th);
    tx = (int16_t)(area.x0 + ((wvui_rect_w(&area) - tw) >> 1));
    ty = (int16_t)(area.y0 + ((wvui_rect_h(&area) - th) >> 1));
    wvui_render_set_clip(area);
    wvui_font_draw_text(rt->font, tx, ty, s, color, rt->opa);
    wvui_render_clear_clip();
    (void)w;
}

static void msgbox_draw(wvui_widget_t *w)
{
    const wvui_msgbox_rt_t *rt = (const wvui_msgbox_rt_t *)wvui_widget_rt_c(w);
    wvui_rect_t box = wvui_widget_rect(w);
    wvui_rect_t title_area;
    wvui_rect_t msg_area;
    wvui_rect_t btn;
    int32_t bh;
    uint8_t parts;

    if ((rt->opa == 0u) || (wvui_rect_is_empty(&box) != 0u))
    {
        return;
    }

    wvui_draw_round_rect(box, rt->radius, 1u, rt->bg, rt->opa, rt->border, rt->opa);

    bh = msgbox_btn_h(w);
    title_area = box;
    title_area.y1 = (int16_t)((int32_t)box.y0 + 20); /* Title band: fixed 20 px */
    if (title_area.y1 > (int32_t)box.y1)
    {
        title_area.y1 = box.y1;
    }
    msg_area = box;
    msg_area.y0 = (int16_t)((int32_t)title_area.y1 + 1);
    msg_area.y1 = (int16_t)((int32_t)box.y1 - bh - 6);
    if (msg_area.y1 < msg_area.y0)
    {
        msg_area.y1 = msg_area.y0;
    }

    msgbox_text(w, rt, (rt->title != 0) ? rt->title : "", title_area, rt->border);
    msgbox_text(w, rt, rt->message, msg_area, rt->fg);

    parts = (uint8_t)((rt->two_buttons != 0u) ? 2u : 1u);
    {
        uint8_t i;

        for (i = 1u; i <= parts; i++)
        {
            uint8_t part = (uint8_t)((rt->two_buttons != 0u) ? i : 2u);
            const char *label;
            wvui_color_t c;

            msgbox_button_rect(w, rt, part, &btn);
            label = (part == 2u) ? ((rt->ok_text != 0) ? rt->ok_text : "OK")
                                 : ((rt->cancel_text != 0) ? rt->cancel_text : "CANCEL");
            c = (rt->pressed == part) ? rt->btn_pressed : rt->btn_bg;
            wvui_draw_round_rect(btn, 4u, 0u, c, rt->opa, 0u, 0u);
            msgbox_text(w, rt, label, btn, rt->btn_fg);
        }
    }
}

/* ---- Events: highlight on press, act on release ---- */
static uint8_t msgbox_event(wvui_widget_t *w, wvui_event_t e, wvui_indev_t *d)
{
    wvui_msgbox_rt_t *rt = rt_of(w);

    switch (e)
    {
    case WVUI_EVENT_PRESSED:
        rt->pressed = (uint8_t)((d != 0) ? msgbox_part_at(w, rt, d->x, d->y) : 0u);
        wvui_widget_invalidate(w);
        return 1u; /* Modal intercepts everything, including taps outside */

    case WVUI_EVENT_RELEASED:
        if (rt->pressed != 0u)
        {
            rt->pressed = 0u;
            wvui_widget_invalidate(w);
        }
        return 1u;

    case WVUI_EVENT_CLICKED:
    {
        uint8_t part = (uint8_t)((d != 0) ? msgbox_part_at(w, rt, d->x, d->y) : 0u);
        wvui_msgbox_cb_t cb;

        rt->pressed = 0u;
        wvui_widget_invalidate(w);
        if (part == 0u)
        {
            return 1u; /* Blank area: do not close */
        }
        cb = (part == 2u) ? rt->on_ok : rt->on_cancel;
        wvui_msgbox_close(w); /* Close first, then call back */
        if (cb != 0)
        {
            cb(w, rt->user);
        }
        return 1u;
    }

    case WVUI_EVENT_MODAL_CLOSE:
        /* Replaced by another modal: detach + clear modal, no callback (the caller did not choose).
         * Clearing only pressed is not enough, the old dialog would stay on the top chain and keep being drawn. */
        wvui_msgbox_close(w);
        return 1u;

    default:
        return 0u;
    }
}

/* ---- Property read/write (command channel endpoint) ---- */
static int8_t msgbox_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
{
    wvui_msgbox_rt_t *rt = rt_of(w);

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

    case WVUI_PROP_TEXT: /* Body text; also opens the dialog */
        if ((len == 0u) || (in == 0)) { return WVUI_PROP_ERR_ARG; }
        wvui_msgbox_set_text(w, rt->title, (const char *)in);
        wvui_msgbox_open(w);
        return WVUI_PROP_OK;

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_msgbox_set_colors(w, wvui_prop_u16(in), rt->fg, rt->border);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_msgbox_set_colors(w, rt->bg, wvui_prop_u16(in), rt->border);
        return WVUI_PROP_OK;

    case WVUI_PROP_BORDER:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_msgbox_set_colors(w, rt->bg, rt->fg, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_RADIUS:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_msgbox_set_radius(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_msgbox_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_ENABLED: /* 0 = one button, 1 = two buttons */
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_msgbox_set_buttons(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_VALUE: /* Read-only: open state */
        return WVUI_PROP_ERR_RO;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t msgbox_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_msgbox_rt_t *rt = (const wvui_msgbox_rt_t *)wvui_widget_rt_c(w);

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
        return (int16_t)wvui_prop_write_text(out, cap, rt->message);

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
        out[0] = rt->two_buttons;
        return 1;

    case WVUI_PROP_VALUE:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = wvui_msgbox_is_open(w);
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_msgbox_class = {
    .rt_size = (uint16_t)sizeof(wvui_msgbox_rt_t),
    .draw = msgbox_draw,
    .event = msgbox_event,
    .init = msgbox_init,
    .set_pos = 0,
    .on_delete = 0,
    .set_prop = msgbox_set_prop,
    .get_prop = msgbox_get_prop,
    .flags = 0u,
};

/* ---- Open / close / property setters ---- */
void wvui_msgbox_open(wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_msgbox_class))
    {
        return;
    }
    rt_of(w)->pressed = 0u;
    (void)wvui_layer_attach(w); /* Top layer chain, idempotent */
    (void)wvui_modal_open(w);   /* Must be paired with attach */
    wvui_widget_invalidate(w);
}

void wvui_msgbox_close(wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_msgbox_class))
    {
        return;
    }
    rt_of(w)->pressed = 0u;
    wvui_modal_close(w);    /* Clear modal first: hit test recovers */
    wvui_layer_detach(w);   /* Then detach */
    wvui_widget_invalidate(w);
}

void wvui_msgbox_set_text(wvui_widget_t *w, const char *title, const char *message)
{
    wvui_msgbox_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_msgbox_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->title == title) && (rt->message == message))
    {
        return;
    }
    rt->title = title;
    rt->message = message;
    wvui_widget_invalidate(w);
}

void wvui_msgbox_set_button_text(wvui_widget_t *w, const char *ok_text, const char *cancel_text)
{
    wvui_msgbox_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_msgbox_class))
    {
        return;
    }
    rt = rt_of(w);
    rt->ok_text = ok_text;
    rt->cancel_text = cancel_text;
    wvui_widget_invalidate(w);
}

void wvui_msgbox_set_buttons(wvui_widget_t *w, uint8_t two_buttons)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_msgbox_class))
    {
        return;
    }
    if (rt_of(w)->two_buttons != (uint8_t)(two_buttons != 0u))
    {
        rt_of(w)->two_buttons = (uint8_t)(two_buttons != 0u);
        wvui_widget_invalidate(w);
    }
}

void wvui_msgbox_set_font(wvui_widget_t *w, const wvui_font_t *font)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_msgbox_class))
    {
        return;
    }
    if (rt_of(w)->font != font)
    {
        rt_of(w)->font = font;
        wvui_widget_invalidate(w);
    }
}

void wvui_msgbox_set_colors(wvui_widget_t *w, wvui_color_t bg, wvui_color_t fg,
                           wvui_color_t border)
{
    wvui_msgbox_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_msgbox_class))
    {
        return;
    }
    rt = rt_of(w);
    rt->bg = bg;
    rt->fg = fg;
    rt->border = border;
    wvui_widget_invalidate(w);
}

void wvui_msgbox_set_button_colors(wvui_widget_t *w, wvui_color_t bg, wvui_color_t fg,
                                  wvui_color_t pressed)
{
    wvui_msgbox_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_msgbox_class))
    {
        return;
    }
    rt = rt_of(w);
    rt->btn_bg = bg;
    rt->btn_fg = fg;
    rt->btn_pressed = pressed;
    wvui_widget_invalidate(w);
}

void wvui_msgbox_set_radius(wvui_widget_t *w, uint8_t radius)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_msgbox_class))
    {
        return;
    }
    if (rt_of(w)->radius != radius)
    {
        rt_of(w)->radius = radius;
        wvui_widget_invalidate(w);
    }
}

void wvui_msgbox_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_msgbox_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

void wvui_msgbox_set_cb(wvui_widget_t *w, wvui_msgbox_cb_t on_ok, wvui_msgbox_cb_t on_cancel,
                       void *user)
{
    wvui_msgbox_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_msgbox_class))
    {
        return;
    }
    rt = rt_of(w);
    rt->on_ok = on_ok;
    rt->on_cancel = on_cancel;
    rt->user = user;
}

uint8_t wvui_msgbox_is_open(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_msgbox_class))
    {
        return 0u;
    }
    return (uint8_t)((wvui_modal_get() == w) ? 1u : 0u);
}
