/**
 * @file wvui_dropdown.c
 * @brief Dropdown implementation: closed state draws one row, open state draws the popup beyond the widget rect.
 * @note  When drawing the popup the clip is widened to the popup rect; the driver does not cull top-layer objects by widget rect.
 */

#include "wvui_dropdown.h"

#include "wvui_draw.h"
#include "wvui_driver.h" /* wvui_screen_size: decide whether to pop up */
#include "wvui_layer.h"
#include "wvui_render.h"

static wvui_dropdown_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_dropdown_rt_t *)wvui_widget_rt(w);
}

static void dropdown_init(wvui_widget_t *w)
{
    wvui_dropdown_rt_t *rt = rt_of(w);

    rt->items = 0;
    rt->count = 0u;
    rt->selected = 0u;
    rt->font = 0;
    rt->bg = WVUI_RGB565(48, 52, 60);
    rt->fg = WVUI_RGB565(235, 240, 245);
    rt->border = WVUI_RGB565(120, 130, 140);
    rt->sel_bg = WVUI_RGB565(40, 130, 200);
    rt->sel_fg = WVUI_RGB565(255, 255, 255);
    rt->row_h = 20u;
    rt->max_visible = 5u;
    rt->open = 0u;
    rt->pressed = 0u;
    rt->pressed_row = 0u;
    rt->radius = 4u;
    rt->opa = 255u;
    rt->enabled = 1u;
    rt->on_select = 0;
    rt->user = 0;
}

/* Visible rows = min(count, max_visible), truncated, no scrolling. */
static uint8_t dropdown_visible_rows(const wvui_dropdown_rt_t *rt)
{
    uint8_t n = (uint8_t)((rt->count > 255u) ? 255u : rt->count);

    if (n > rt->max_visible)
    {
        n = rt->max_visible;
    }
    return n;
}

/* Popup rect: downward first, upward when it does not fit. */
static void dropdown_popup_rect(const wvui_widget_t *w, const wvui_dropdown_rt_t *rt,
                                wvui_rect_t *out)
{
    int32_t h = (int32_t)dropdown_visible_rows(rt) * (int32_t)rt->row_h;
    int32_t y0 = (int32_t)w->y + (int32_t)w->h + 2;
    int16_t scr_w = 0;
    int16_t scr_h = 0;

    if (h <= 0)
    {
        h = (int32_t)rt->row_h;
    }
    /* Screen height comes from wvui_screen_size(): the core only provides slicing and clipping, widgets cannot get the screen size. */
    wvui_screen_size(&scr_w, &scr_h);
    if ((scr_h > 0) && ((y0 + h) > (int32_t)scr_h))
    {
        y0 = (int32_t)w->y - h - 2;
    }
    if (y0 < 0)
    {
        y0 = 0; /* If it still does not fit, stick to the top; parts beyond the screen are clipped by slicing */
    }
    out->x0 = w->x;
    out->x1 = (int16_t)((int32_t)w->x + (int32_t)w->w - 1);
    out->y0 = (int16_t)y0;
    out->y1 = (int16_t)(y0 + h - 1);
}

/* Mark widget rect + popup rect dirty: the driver redraws only dirty areas, and the popup lies outside the widget rect. */
static void dropdown_invalidate(wvui_widget_t *w)
{
    wvui_dropdown_rt_t *rt = rt_of(w);

    wvui_widget_invalidate(w);
    if (rt->open != 0u)
    {
        wvui_rect_t pop;

        dropdown_popup_rect(w, rt, &pop);
        /* Use wvui_invalidate(): the widget version intersects with the widget rect first, leaving the popup dirty area always empty. */
        wvui_invalidate(pop);
    }
}

static void dropdown_text(const wvui_widget_t *w, const wvui_dropdown_rt_t *rt, const char *s,
                          wvui_rect_t area, wvui_color_t color)
{
    int16_t tw = 0;
    int16_t th = 0;
    int16_t ty;

    if ((rt->font == 0) || (s == 0))
    {
        return;
    }
    wvui_font_measure(rt->font, s, &tw, &th);
    ty = (int16_t)(area.y0 + ((wvui_rect_h(&area) - th) >> 1));
    wvui_render_set_clip(area);
    wvui_font_draw_text(rt->font, (int16_t)(area.x0 + 4), ty, s, color, rt->opa);
    wvui_render_clear_clip();
    (void)w;
}

static void dropdown_draw(wvui_widget_t *w)
{
    const wvui_dropdown_rt_t *rt = (const wvui_dropdown_rt_t *)wvui_widget_rt_c(w);
    wvui_rect_t box = wvui_widget_rect(w);

    if ((rt->opa == 0u) || (wvui_rect_is_empty(&box) != 0u))
    {
        return;
    }

    /* Shared by both states: fill + border + current item text + right arrow */
    wvui_draw_round_rect(box, rt->radius, 1u, rt->bg, rt->opa, rt->border, rt->opa);
    if ((rt->items != 0) && (rt->selected < rt->count))
    {
        dropdown_text(w, rt, rt->items[rt->selected], box, rt->fg);
    }
    {
        /* Arrow: a triangle made of two lines, no font needed; points up when expanded */
        int16_t cx = (int16_t)(box.x1 - 8);
        int16_t cy = (int16_t)(box.y0 + (wvui_rect_h(&box) >> 1));
        int16_t arm = 4;

        if (rt->open != 0u)
        {
            wvui_draw_line((int16_t)(cx - arm), (int16_t)(cy + 2), cx, (int16_t)(cy - 2), 2u,
                          WVUI_CAP_ROUND, rt->border, rt->opa);
            wvui_draw_line(cx, (int16_t)(cy - 2), (int16_t)(cx + arm), (int16_t)(cy + 2), 2u,
                          WVUI_CAP_ROUND, rt->border, rt->opa);
        }
        else
        {
            wvui_draw_line((int16_t)(cx - arm), (int16_t)(cy - 2), cx, (int16_t)(cy + 2), 2u,
                          WVUI_CAP_ROUND, rt->border, rt->opa);
            wvui_draw_line(cx, (int16_t)(cy + 2), (int16_t)(cx + arm), (int16_t)(cy - 2), 2u,
                          WVUI_CAP_ROUND, rt->border, rt->opa);
        }
    }

    if (rt->open != 0u)
    {
        wvui_rect_t pop;
        uint8_t rows;
        uint8_t i;

        dropdown_popup_rect(w, rt, &pop);
        rows = dropdown_visible_rows(rt);

        /* The driver does not cull top-layer objects by widget rect; narrow the clip to the popup rect. */
        wvui_render_set_clip(pop);
        wvui_draw_round_rect(pop, rt->radius, 1u, rt->bg, rt->opa, rt->border, rt->opa);
        for (i = 0u; i < rows; i++)
        {
            wvui_rect_t row;

            row.x0 = (int16_t)(pop.x0 + 1);
            row.x1 = (int16_t)(pop.x1 - 1);
            row.y0 = (int16_t)((int32_t)pop.y0 + ((int32_t)i * (int32_t)rt->row_h));
            row.y1 = (int16_t)((int32_t)row.y0 + (int32_t)rt->row_h - 1);
            if (row.y1 > pop.y1)
            {
                row.y1 = pop.y1;
            }
            if (i == rt->selected)
            {
                wvui_render_fill_rect_blend(row, rt->sel_bg, rt->opa);
            }
            if ((rt->items != 0) && (i < rt->count))
            {
                dropdown_text(w, rt, rt->items[i], row,
                              (i == rt->selected) ? rt->sel_fg : rt->fg);
            }
        }
        wvui_render_clear_clip();
    }
}

/* Which popup row the point is on (0xFFFF = not in the popup). */
static uint16_t dropdown_row_at(const wvui_widget_t *w, const wvui_dropdown_rt_t *rt, int16_t y)
{
    wvui_rect_t pop;
    uint8_t rows;

    dropdown_popup_rect(w, rt, &pop);
    if ((y < pop.y0) || (y > pop.y1))
    {
        return 0xFFFFu;
    }
    rows = dropdown_visible_rows(rt);
    {
        int32_t local = (int32_t)y - (int32_t)pop.y0;
        int32_t acc = 0;
        uint8_t i;

        for (i = 0u; i < rows; i++)
        {
            acc += rt->row_h;
            if (local < acc)
            {
                return i;
            }
        }
    }
    return 0xFFFFu;
}

static uint8_t dropdown_in_face(const wvui_widget_t *w, int16_t x, int16_t y)
{
    return (uint8_t)(((x >= w->x) && (x <= (int16_t)(w->x + w->w - 1)) && (y >= w->y) &&
                      (y <= (int16_t)(w->y + w->h - 1)))
                         ? 1u
                         : 0u);
}

static uint8_t dropdown_event(wvui_widget_t *w, wvui_event_t e, wvui_indev_t *d)
{
    wvui_dropdown_rt_t *rt = rt_of(w);

    switch (e)
    {
    case WVUI_EVENT_PRESSED:
        if (d == 0)
        {
            return 0u;
        }
        if (rt->open == 0u)
        {
            if (dropdown_in_face(w, d->x, d->y) != 0u)
            {
                rt->pressed = 1u;
                dropdown_invalidate(w);
                return 1u;
            }
            return 0u;
        }
        /* Open state: remember the row when pressed on it; presses outside are ignored, collapse on CLICKED */
        {
            uint16_t row = dropdown_row_at(w, rt, d->y);

            rt->pressed = (uint8_t)((row != 0xFFFFu) ? 2u : 0u);
            rt->pressed_row = (uint8_t)((row != 0xFFFFu) ? row : 0u);
        }
        dropdown_invalidate(w);
        return 1u; /* Modal: intercept all events */

    case WVUI_EVENT_RELEASED:
        if (rt->pressed != 0u)
        {
            rt->pressed = 0u;
            dropdown_invalidate(w);
        }
        return (rt->open != 0u) ? 1u : 0u;

    case WVUI_EVENT_CLICKED:
        if (d == 0)
        {
            return 0u;
        }
        if (rt->open == 0u)
        {
            if (dropdown_in_face(w, d->x, d->y) != 0u)
            {
                wvui_dropdown_open(w);
                return 1u;
            }
            return 0u;
        }
        /* Open state: select when clicked in the popup, collapse when clicked elsewhere (including the widget itself). */
        {
            uint16_t row = dropdown_row_at(w, rt, d->y);

            if (row != 0xFFFFu)
            {
                wvui_dropdown_close(w);
                wvui_dropdown_set_selected(w, row);
            }
            else
            {
                wvui_dropdown_close(w);
            }
        }
        return 1u;

    case WVUI_EVENT_MODAL_CLOSE:
        /* Replaced by another modal: collapse on its own, no callback. */
        rt->open = 0u;
        rt->pressed = 0u;
        dropdown_invalidate(w);
        return 1u;

    default:
        return 0u;
    }
}

/* ---- Property handlers: VALUE = selected index, payload u8 ---- */
static int8_t dropdown_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in,
                                uint16_t len)
{
    wvui_dropdown_rt_t *rt = rt_of(w);

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
        dropdown_invalidate(w);
        return WVUI_PROP_OK;

    case WVUI_PROP_VALUE:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_dropdown_set_selected(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_dropdown_set_colors(w, wvui_prop_u16(in), rt->fg, rt->border);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_dropdown_set_colors(w, rt->bg, wvui_prop_u16(in), rt->border);
        return WVUI_PROP_OK;

    case WVUI_PROP_BORDER:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_dropdown_set_colors(w, rt->bg, rt->fg, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_RADIUS:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_dropdown_set_radius(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_dropdown_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_ENABLED:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_dropdown_set_enabled(w, in[0]);
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t dropdown_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_dropdown_rt_t *rt = (const wvui_dropdown_rt_t *)wvui_widget_rt_c(w);

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
        out[0] = (uint8_t)((rt->selected > 255u) ? 255u : rt->selected);
        return 1;

    case WVUI_PROP_TEXT: /* Current item text, read-only */
        return (int16_t)wvui_prop_write_text(
            out, cap, ((rt->items != 0) && (rt->selected < rt->count))
                          ? rt->items[rt->selected]
                          : 0);

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
        out[0] = rt->enabled;
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_dropdown_class = {
    .rt_size = (uint16_t)sizeof(wvui_dropdown_rt_t),
    .draw = dropdown_draw,
    .event = dropdown_event,
    .init = dropdown_init,
    .set_pos = 0,
    .on_delete = 0,
    .set_prop = dropdown_set_prop,
    .get_prop = dropdown_get_prop,
    .flags = 0u,
};

/* ---- Open / collapse / property setters ---- */
void wvui_dropdown_open(wvui_widget_t *w)
{
    wvui_dropdown_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_dropdown_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->count == 0u) || (rt->enabled == 0u))
    {
        return;
    }
    rt->open = 1u;
    rt->pressed = 0u;
    (void)wvui_layer_attach(w); /* Join the top-layer chain, idempotent */
    (void)wvui_modal_open(w);   /* Clicks outside are judged by the core as hitting this widget, collapse in CLICKED */
    dropdown_invalidate(w);
}

void wvui_dropdown_close(wvui_widget_t *w)
{
    wvui_dropdown_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_dropdown_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rt->open == 0u)
    {
        return;
    }
    /* Mark dirty with the still-open geometry first to erase the popup; likewise bypass the widget-rect intersection. */
    {
        wvui_rect_t pop;

        dropdown_popup_rect(w, rt, &pop);
        wvui_invalidate(pop);
    }
    rt->open = 0u;
    rt->pressed = 0u;
    wvui_modal_close(w);
    wvui_layer_detach(w);
    /* Was detached by wvui_widget_detach() when joining the top-layer chain: without rejoining the root chain it is on no chain. */
    wvui_widget_attach_root(w);
    dropdown_invalidate(w);
}

void wvui_dropdown_toggle(wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_dropdown_class))
    {
        return;
    }
    if (rt_of(w)->open != 0u)
    {
        wvui_dropdown_close(w);
    }
    else
    {
        wvui_dropdown_open(w);
    }
}

void wvui_dropdown_bind(wvui_widget_t *w, const char *const *items, uint16_t count)
{
    wvui_dropdown_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_dropdown_class))
    {
        return;
    }
    rt = rt_of(w);
    if (items == 0)
    {
        count = 0u;
    }
    rt->items = items;
    rt->count = count;
    if (rt->selected >= count)
    {
        rt->selected = (count > 0u) ? (uint16_t)(count - 1u) : 0u;
    }
    if (count == 0u)
    {
        wvui_dropdown_close(w); /* An empty table must collapse, otherwise an empty popup remains */
    }
    dropdown_invalidate(w);
}

void wvui_dropdown_set_selected(wvui_widget_t *w, uint16_t index)
{
    wvui_dropdown_rt_t *rt;
    uint16_t v;

    if (!WVUI_W_IS_TYPE(w, &wvui_dropdown_class))
    {
        return;
    }
    rt = rt_of(w);
    v = index;
    if ((rt->count > 0u) && (v >= rt->count))
    {
        v = (uint16_t)(rt->count - 1u);
    }
    if (rt->selected == v)
    {
        return;
    }
    rt->selected = v;
    dropdown_invalidate(w);
    if (rt->on_select != 0)
    {
        rt->on_select(w, v, rt->user);
    }
}

void wvui_dropdown_set_font(wvui_widget_t *w, const wvui_font_t *font)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_dropdown_class))
    {
        return;
    }
    if (rt_of(w)->font != font)
    {
        rt_of(w)->font = font;
        dropdown_invalidate(w);
    }
}

void wvui_dropdown_set_row_height(wvui_widget_t *w, uint8_t row_h)
{
    wvui_dropdown_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_dropdown_class))
    {
        return;
    }
    rt = rt_of(w);
    if (row_h < 4u)
    {
        row_h = 4u;
    }
    if (rt->row_h != row_h)
    {
        rt->row_h = row_h;
        dropdown_invalidate(w);
    }
}

void wvui_dropdown_set_max_visible(wvui_widget_t *w, uint8_t rows)
{
    wvui_dropdown_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_dropdown_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rows < 1u)
    {
        rows = 1u;
    }
    if (rt->max_visible != rows)
    {
        rt->max_visible = rows;
        dropdown_invalidate(w);
    }
}

void wvui_dropdown_set_colors(wvui_widget_t *w, wvui_color_t bg, wvui_color_t fg,
                             wvui_color_t border)
{
    wvui_dropdown_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_dropdown_class))
    {
        return;
    }
    rt = rt_of(w);
    rt->bg = bg;
    rt->fg = fg;
    rt->border = border;
    dropdown_invalidate(w);
}

void wvui_dropdown_set_sel_colors(wvui_widget_t *w, wvui_color_t sel_bg, wvui_color_t sel_fg)
{
    wvui_dropdown_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_dropdown_class))
    {
        return;
    }
    rt = rt_of(w);
    rt->sel_bg = sel_bg;
    rt->sel_fg = sel_fg;
    dropdown_invalidate(w);
}

void wvui_dropdown_set_radius(wvui_widget_t *w, uint8_t radius)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_dropdown_class))
    {
        return;
    }
    if (rt_of(w)->radius != radius)
    {
        rt_of(w)->radius = radius;
        dropdown_invalidate(w);
    }
}

void wvui_dropdown_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_dropdown_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        dropdown_invalidate(w);
    }
}

void wvui_dropdown_set_enabled(wvui_widget_t *w, uint8_t enabled)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_dropdown_class))
    {
        return;
    }
    if (rt_of(w)->enabled != (uint8_t)(enabled != 0u))
    {
        rt_of(w)->enabled = (uint8_t)(enabled != 0u);
        if (rt_of(w)->enabled == 0u)
        {
            wvui_dropdown_close(w);
        }
        dropdown_invalidate(w);
    }
}

void wvui_dropdown_set_select_cb(wvui_widget_t *w, wvui_dropdown_cb_t cb, void *user)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_dropdown_class))
    {
        return;
    }
    rt_of(w)->on_select = cb;
    rt_of(w)->user = user;
}

uint16_t wvui_dropdown_count(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_dropdown_class))
    {
        return 0u;
    }
    return ((const wvui_dropdown_rt_t *)wvui_widget_rt_c(w))->count;
}

uint16_t wvui_dropdown_selected(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_dropdown_class))
    {
        return 0u;
    }
    return ((const wvui_dropdown_rt_t *)wvui_widget_rt_c(w))->selected;
}

uint8_t wvui_dropdown_is_open(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_dropdown_class))
    {
        return 0u;
    }
    return ((const wvui_dropdown_rt_t *)wvui_widget_rt_c(w))->open;
}

const char *wvui_dropdown_item(const wvui_widget_t *w, uint16_t index)
{
    const wvui_dropdown_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_dropdown_class))
    {
        return 0;
    }
    rt = (const wvui_dropdown_rt_t *)wvui_widget_rt_c(w);
    if ((rt->items == 0) || (index >= rt->count))
    {
        return 0;
    }
    return rt->items[index];
}
