/**
 * @file wvui_list.c
 * @brief  Text list implementation: stateless redraw of the visible rows.
 * @note   Scroll and inertia reuse wvui_scroll, plus one anim node as driver.
 */

#include "wvui_list.h"

#include "wvui_render.h"

static wvui_list_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_list_rt_t *)wvui_widget_rt(w);
}

/* Scroll range = content height - widget height, negative values clamp to 0. */
static void list_update_range(wvui_widget_t *w)
{
    wvui_list_rt_t *rt = rt_of(w);
    int32_t content = (int32_t)rt->count * (int32_t)rt->row_h;
    int32_t max = content - (int32_t)w->h;

    if (max < 0)
    {
        max = 0;
    }
    wvui_scroll_set_range(&rt->sc, max);
}

static void list_stop_anim(wvui_widget_t *w)
{
    wvui_list_rt_t *rt = rt_of(w);

    if (wvui_anim_running(&rt->an) != 0u)
    {
        wvui_anim_stop(&rt->an);
    }
}

/* Per-frame step for inertia and bounce-back (anim callback); returns
 * 0 = at rest, the engine detaches the node. */
static uint8_t list_step(wvui_anim_t *a)
{
    wvui_widget_t *w = a->owner;
    wvui_list_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_list_class))
    {
        return 0u;
    }
    rt = rt_of(w);
    if (wvui_scroll_step(&rt->sc) == 0u)
    {
        return 0u;
    }
    wvui_widget_invalidate(w); /* rows draw themselves: scrolling means redraw */
    return 1u;
}

static void list_start_anim(wvui_widget_t *w)
{
    wvui_anim_start(&rt_of(w)->an, w, list_step); /* idempotent */
}

static void list_init(wvui_widget_t *w)
{
    wvui_list_rt_t *rt = rt_of(w);

    wvui_anim_init(&rt->an);
    wvui_scroll_init(&rt->sc, 0);
    rt->items = 0;
    rt->font = 0;
    rt->bg = WVUI_RGB565(40, 130, 200);
    rt->fg = WVUI_RGB565(210, 220, 230);
    rt->sel_fg = WVUI_RGB565(255, 255, 255);
    rt->count = 0u;
    rt->selected = 0u;
    rt->row_h = 20;
    rt->press_y = 0;
    rt->pressed = 0u;
    rt->moved = 0u;
    rt->opa = 255u;
    rt->enabled = 1u;
    rt->on_select = 0;
    rt->user = 0;
}

static void list_delete(wvui_widget_t *w)
{
    list_stop_anim(w);
}

static void list_draw(wvui_widget_t *w)
{
    const wvui_list_rt_t *rt = (const wvui_list_rt_t *)wvui_widget_rt_c(w);
    wvui_rect_t box = wvui_widget_rect(w);
    int32_t y_top;
    uint16_t i;

    if ((rt->opa == 0u) || (rt->items == 0) || (rt->count == 0u) ||
        (wvui_rect_is_empty(&box) != 0u))
    {
        return;
    }

    y_top = (int32_t)box.y0 - (int32_t)rt->sc.pos;

    wvui_render_set_clip(box);
    for (i = 0u; i < rt->count; i++)
    {
        int32_t y0 = y_top + ((int32_t)i * (int32_t)rt->row_h);
        int32_t y1 = y0 + (int32_t)rt->row_h - 1;

        if (y1 < (int32_t)box.y0)
        {
            continue;
        }
        if (y0 > (int32_t)box.y1)
        {
            break;
        }

        if (i == rt->selected)
        {
            wvui_rect_t sel;

            sel.x0 = box.x0;
            sel.x1 = box.x1;
            sel.y0 = (int16_t)((y0 < box.y0) ? box.y0 : y0);
            sel.y1 = (int16_t)((y1 > box.y1) ? box.y1 : y1);
            wvui_render_fill_rect_blend(sel, rt->bg, rt->opa);
        }

        if ((rt->font != 0) && (rt->items[i] != 0))
        {
            int16_t tw = 0;
            int16_t th = 0;
            int16_t ty;

            wvui_font_measure(rt->font, rt->items[i], &tw, &th);
            ty = (int16_t)(y0 + (((int32_t)rt->row_h - (int32_t)th) >> 1));
            wvui_font_draw_text(rt->font, (int16_t)(box.x0 + 4), ty, rt->items[i],
                               (i == rt->selected) ? rt->sel_fg : rt->fg, rt->opa);
            (void)tw;
        }
    }
    wvui_render_clear_clip();
}

/* Resolve a row from the press position: accumulate row by row, no division,
 * unaffected by later scrolling. */
static uint16_t list_row_at(const wvui_widget_t *w, int16_t y)
{
    const wvui_list_rt_t *rt = (const wvui_list_rt_t *)wvui_widget_rt_c(w);
    int32_t local = (int32_t)y - (int32_t)w->y + (int32_t)rt->sc.pos;
    int32_t acc = 0;
    uint16_t i;

    if ((rt->row_h <= 0) || (local < 0))
    {
        return 0xFFFFu;
    }
    for (i = 0u; i < rt->count; i++)
    {
        acc += rt->row_h;
        if (local < acc)
        {
            return i;
        }
    }
    return 0xFFFFu;
}

/* Scroll the selected row into view (instant, no animation). */
static void list_scroll_into_view(wvui_widget_t *w)
{
    wvui_list_rt_t *rt = rt_of(w);
    int32_t top = (int32_t)rt->selected * (int32_t)rt->row_h;
    int32_t bottom = top + (int32_t)rt->row_h;
    int32_t pos = rt->sc.pos;

    if (top < pos)
    {
        pos = top;
    }
    else if (bottom > (pos + (int32_t)w->h))
    {
        pos = bottom - (int32_t)w->h;
    }
    (void)wvui_scroll_set(&rt->sc, pos);
}

static uint8_t list_event(wvui_widget_t *w, wvui_event_t e, wvui_indev_t *d)
{
    wvui_list_rt_t *rt = rt_of(w);

    switch (e)
    {
    case WVUI_EVENT_PRESSED:
        if (rt->enabled == 0u)
        {
            return 0u;
        }
        rt->pressed = 1u;
        rt->moved = 0u;
        rt->press_y = (int16_t)((d != 0) ? d->y : 0);
        wvui_scroll_press(&rt->sc, rt->press_y);
        return 1u;

    case WVUI_EVENT_STAY:
        if ((rt->enabled == 0u) || (rt->pressed == 0u) || (d == 0))
        {
            return 0u;
        }
        if (wvui_scroll_stay(&rt->sc, d->y) != 0u)
        {
            rt->moved = 1u; /* threshold exceeded: counts as a drag */
            (void)wvui_scroll_take_delta(&rt->sc); /* take the delta and update velocity */
            wvui_widget_invalidate(w);
        }
        return 1u;

    case WVUI_EVENT_RELEASED:
        if (rt->pressed != 0u)
        {
            rt->pressed = 0u;
            if (wvui_scroll_release(&rt->sc) != 0u)
            {
                list_start_anim(w); /* inertia / bounce-back */
            }
        }
        return (rt->enabled != 0u) ? 1u : 0u;

    case WVUI_EVENT_CLICKED:
        if (rt->enabled == 0u)
        {
            return 0u;
        }
        if (rt->moved == 0u)
        {
            uint16_t row = list_row_at(w, rt->press_y);

            if (row != 0xFFFFu)
            {
                wvui_list_set_selected(w, row);
            }
        }
        rt->pressed = 0u;
        rt->moved = 0u;
        return 1u;

    case WVUI_EVENT_SWIPE_UP:
    case WVUI_EVENT_SWIPE_DOWN:
        if ((rt->enabled == 0u) || (d == 0))
        {
            return 0u;
        }
        if (wvui_scroll_swipe(&rt->sc, d->y) != 0u)
        {
            list_start_anim(w);
        }
        return 1u;

    default:
        return 0u;
    }
}

/* ---- Property read/write: VALUE = selected item index, payload u8 ---- */
static int8_t list_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
{
    wvui_list_rt_t *rt = rt_of(w);

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
        list_update_range(w);
        wvui_widget_invalidate(w);
        return WVUI_PROP_OK;

    case WVUI_PROP_VALUE:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_list_set_selected(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_list_set_colors(w, wvui_prop_u16(in), rt->fg, rt->sel_fg);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_list_set_colors(w, rt->bg, wvui_prop_u16(in), rt->sel_fg);
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_list_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_ENABLED:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_list_set_enabled(w, in[0]);
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t list_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_list_rt_t *rt = (const wvui_list_rt_t *)wvui_widget_rt_c(w);

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

    case WVUI_PROP_BG:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->bg);
        return 2;

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

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_list_class = {
    .rt_size = (uint16_t)sizeof(wvui_list_rt_t),
    .draw = list_draw,
    .event = list_event,
    .init = list_init,
    .set_pos = 0,
    .on_delete = list_delete,
    .set_prop = list_set_prop,
    .get_prop = list_get_prop,
    .flags = 0u,
};

/* ---- Property setters ---- */
void wvui_list_refresh(wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_list_class))
    {
        return;
    }
    list_update_range(w);
    wvui_widget_invalidate(w);
}

void wvui_list_bind(wvui_widget_t *w, const char *const *items, uint16_t count)
{
    wvui_list_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_list_class))
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
    list_update_range(w);
    (void)wvui_scroll_set(&rt->sc, 0); /* a new table returns to the top */
    wvui_widget_invalidate(w);
}

void wvui_list_set_font(wvui_widget_t *w, const wvui_font_t *font)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_list_class))
    {
        return;
    }
    if (rt_of(w)->font != font)
    {
        rt_of(w)->font = font;
        wvui_widget_invalidate(w);
    }
}

void wvui_list_set_row_height(wvui_widget_t *w, int16_t row_h)
{
    wvui_list_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_list_class))
    {
        return;
    }
    if (row_h < 1)
    {
        row_h = 1;
    }
    rt = rt_of(w);
    if (rt->row_h == row_h)
    {
        return;
    }
    rt->row_h = row_h;
    list_update_range(w);
    wvui_widget_invalidate(w);
}

void wvui_list_set_selected(wvui_widget_t *w, uint16_t index)
{
    wvui_list_rt_t *rt;
    uint16_t v;

    if (!WVUI_W_IS_TYPE(w, &wvui_list_class))
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
    list_scroll_into_view(w);
    wvui_widget_invalidate(w);
    if (rt->on_select != 0)
    {
        rt->on_select(w, v, rt->user);
    }
}

void wvui_list_set_colors(wvui_widget_t *w, wvui_color_t bg, wvui_color_t fg, wvui_color_t sel_fg)
{
    wvui_list_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_list_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->bg == bg) && (rt->fg == fg) && (rt->sel_fg == sel_fg))
    {
        return;
    }
    rt->bg = bg;
    rt->fg = fg;
    rt->sel_fg = sel_fg;
    wvui_widget_invalidate(w);
}

void wvui_list_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_list_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

void wvui_list_set_enabled(wvui_widget_t *w, uint8_t enabled)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_list_class))
    {
        return;
    }
    if (rt_of(w)->enabled != (uint8_t)(enabled != 0u))
    {
        rt_of(w)->enabled = (uint8_t)(enabled != 0u);
        wvui_widget_invalidate(w);
    }
}

void wvui_list_set_select_cb(wvui_widget_t *w, wvui_list_select_cb_t cb, void *user)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_list_class))
    {
        return;
    }
    rt_of(w)->on_select = cb;
    rt_of(w)->user = user;
}

uint16_t wvui_list_count(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_list_class))
    {
        return 0u;
    }
    return ((const wvui_list_rt_t *)wvui_widget_rt_c(w))->count;
}

uint16_t wvui_list_selected(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_list_class))
    {
        return 0u;
    }
    return ((const wvui_list_rt_t *)wvui_widget_rt_c(w))->selected;
}

int32_t wvui_list_scroll(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_list_class))
    {
        return 0;
    }
    return ((const wvui_list_rt_t *)wvui_widget_rt_c(w))->sc.pos;
}

const char *wvui_list_item(const wvui_widget_t *w, uint16_t index)
{
    const wvui_list_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_list_class))
    {
        return 0;
    }
    rt = (const wvui_list_rt_t *)wvui_widget_rt_c(w);
    if ((rt->items == 0) || (index >= rt->count))
    {
        return 0;
    }
    return rt->items[index];
}
