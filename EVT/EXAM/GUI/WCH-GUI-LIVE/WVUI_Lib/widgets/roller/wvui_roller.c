/**
 * @file wvui_roller.c
 * @brief Roller selection implementation: redraws the visible rows and the band highlight each frame from the scroll position.
 * @note  Release only snaps, no inertia: velocity is cleared to zero at RELEASED.
 */

#include "wvui_roller.h"

#include "wvui_render.h"

static wvui_roller_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_roller_rt_t *)wvui_widget_rt(w);
}

/* Range [0, (count-1)·row_h]: drawing shifts content up by pad = (h − row_h)/2,
 * pad + row_h/2 = h/2 cancels out, so item i centered corresponds to pos = i·row_h. */
static void roller_update_range(wvui_widget_t *w)
{
    wvui_roller_rt_t *rt = rt_of(w);
    int32_t max = ((int32_t)rt->count - 1) * (int32_t)rt->row_h;

    if (max < 0)
    {
        max = 0;
    }
    wvui_scroll_set_range(&rt->sc, max);
}

/* Top and bottom padding: degenerates to 0 at row height 1. */
static int32_t roller_pad(const wvui_widget_t *w, const wvui_roller_rt_t *rt)
{
    int32_t pad = ((int32_t)w->h - (int32_t)rt->row_h) >> 1;

    return (pad > 0) ? pad : 0;
}

/* Index at the middle band: the largest i with i·row_h ≤ pos + row_h/2, a half row picks the larger slot, row-by-row accumulation, no division. */
static uint16_t roller_index_at_center(const wvui_widget_t *w, const wvui_roller_rt_t *rt)
{
    int32_t limit;
    int32_t acc = 0;
    uint16_t idx = 0u;
    uint16_t i;

    if (rt->row_h <= 0)
    {
        return 0xFFFFu;
    }
    limit = rt->sc.pos + ((int32_t)rt->row_h >> 1);
    for (i = 0u; i < rt->count; i++)
    {
        if (acc <= limit)
        {
            idx = i;
        }
        acc += rt->row_h;
    }
    (void)w;
    return (rt->count > 0u) ? idx : 0xFFFFu;
}

/* Snap position of slot i = i·row_h, clamped to the range. */
static int32_t roller_target_for(const wvui_widget_t *w, const wvui_roller_rt_t *rt, uint16_t i)
{
    int32_t target = (int32_t)i * (int32_t)rt->row_h;

    if (target < 0)
    {
        target = 0;
    }
    if (target > rt->sc.max)
    {
        target = rt->sc.max;
    }
    (void)w;
    return target;
}

static void roller_stop_anim(wvui_widget_t *w)
{
    wvui_roller_rt_t *rt = rt_of(w);

    if (wvui_anim_running(&rt->an) != 0u)
    {
        wvui_anim_stop(&rt->an);
    }
}

/* Snap stepping: exponential approach + snap convergence. */
static uint8_t roller_step(wvui_anim_t *a)
{
    wvui_widget_t *w = a->owner;
    wvui_roller_rt_t *rt;
    uint16_t idx;
    int32_t target;
    int32_t d;
    int32_t step;

    if (!WVUI_W_IS_TYPE(w, &wvui_roller_class))
    {
        return 0u;
    }
    rt = rt_of(w);

    /* Target recomputed each frame from the current band item, the old target is not reused. */
    idx = roller_index_at_center(w, rt);
    if (idx == 0xFFFFu)
    {
        return 0u;
    }
    target = roller_target_for(w, rt, idx);
    d = target - rt->sc.pos;
    if (d == 0)
    {
        return 0u;
    }
    if ((d <= 1) && (d >= -1))
    {
        rt->sc.pos = target; /* Snapped into place */
    }
    else
    {
        step = d >> 2;
        if (step == 0)
        {
            step = (d > 0) ? 1 : -1;
        }
        rt->sc.pos += step;
    }
    wvui_widget_invalidate(w);
    return (uint8_t)((rt->sc.pos != target) ? 1u : 0u);
}

static void roller_start_anim(wvui_widget_t *w)
{
    wvui_anim_start(&rt_of(w)->an, w, roller_step); /* Idempotent */
}

static void roller_init(wvui_widget_t *w)
{
    wvui_roller_rt_t *rt = rt_of(w);

    wvui_anim_init(&rt->an);
    wvui_scroll_init(&rt->sc, 0);
    rt->items = 0;
    rt->font = 0;
    rt->band = WVUI_RGB565(40, 130, 200);
    rt->fg = WVUI_RGB565(170, 180, 195);
    rt->sel_fg = WVUI_RGB565(255, 255, 255);
    rt->count = 0u;
    rt->selected = 0u;
    rt->row_h = 20;
    rt->opa = 255u;
    rt->enabled = 1u;
    rt->pressed = 0u;
    rt->on_select = 0;
    rt->user = 0;
}

static void roller_delete(wvui_widget_t *w)
{
    roller_stop_anim(w);
}

static void roller_draw(wvui_widget_t *w)
{
    const wvui_roller_rt_t *rt = (const wvui_roller_rt_t *)wvui_widget_rt_c(w);
    wvui_rect_t box = wvui_widget_rect(w);
    int32_t y_top;
    int32_t band_y0;
    int32_t band_y1;
    uint16_t i;

    if ((rt->opa == 0u) || (rt->items == 0) || (rt->count == 0u) ||
        (wvui_rect_is_empty(&box) != 0u))
    {
        return;
    }

    y_top = (int32_t)box.y0 + roller_pad(w, rt) - (int32_t)rt->sc.pos;
    band_y0 = (int32_t)box.y0 + ((int32_t)wvui_rect_h(&box) >> 1) - ((int32_t)rt->row_h >> 1);
    band_y1 = band_y0 + (int32_t)rt->row_h - 1;

    wvui_render_set_clip(box);

    /* Band highlight: draw the background first, then the text. */
    {
        wvui_rect_t band;

        band.x0 = box.x0;
        band.x1 = box.x1;
        band.y0 = (int16_t)((band_y0 < box.y0) ? box.y0 : band_y0);
        band.y1 = (int16_t)((band_y1 > box.y1) ? box.y1 : band_y1);
        wvui_render_fill_rect_blend(band, rt->band, rt->opa);
    }

    for (i = 0u; i < rt->count; i++)
    {
        int32_t y0 = y_top + ((int32_t)i * (int32_t)rt->row_h);
        int32_t y1 = y0 + (int32_t)rt->row_h - 1;
        uint8_t in_band;

        if (y1 < (int32_t)box.y0)
        {
            continue;
        }
        if (y0 > (int32_t)box.y1)
        {
            break;
        }

        in_band = (uint8_t)((i == rt->selected) ? 1u : 0u);
        if ((rt->font != 0) && (rt->items[i] != 0))
        {
            int16_t tw = 0;
            int16_t th = 0;
            int16_t ty;

            wvui_font_measure(rt->font, rt->items[i], &tw, &th);
            ty = (int16_t)(y0 + (((int32_t)rt->row_h - (int32_t)th) >> 1));
            /* Band row centered, other rows left-aligned */
            wvui_font_draw_text(rt->font,
                               (int16_t)((in_band != 0u)
                                             ? (box.x0 + ((wvui_rect_w(&box) - tw) >> 1))
                                             : (box.x0 + 4)),
                               ty, rt->items[i],
                               (in_band != 0u) ? rt->sel_fg : rt->fg, rt->opa);
        }
    }
    wvui_render_clear_clip();
}

/* Release: snap straight to the nearest slot, no inertia. */
static void roller_settle(wvui_widget_t *w)
{
    wvui_roller_rt_t *rt = rt_of(w);
    uint16_t idx = roller_index_at_center(w, rt);
    int32_t target;

    if (idx == 0xFFFFu)
    {
        return;
    }
    target = roller_target_for(w, rt, idx);
    if (rt->sc.pos != target)
    {
        roller_start_anim(w);
    }
    if ((rt->selected != idx) && (idx < rt->count))
    {
        rt->selected = idx;
        wvui_widget_invalidate(w);
        if (rt->on_select != 0)
        {
            rt->on_select(w, idx, rt->user);
        }
    }
}

static uint8_t roller_event(wvui_widget_t *w, wvui_event_t e, wvui_indev_t *d)
{
    wvui_roller_rt_t *rt = rt_of(w);

    switch (e)
    {
    case WVUI_EVENT_PRESSED:
        if (rt->enabled == 0u)
        {
            return 0u;
        }
        rt->pressed = 1u;
        rt->moved = 0u;
        roller_stop_anim(w); /* Stop snapping on press: following the finger wins */
        wvui_scroll_press(&rt->sc, (int16_t)((d != 0) ? d->y : 0));
        return 1u;

    case WVUI_EVENT_STAY:
        if ((rt->enabled == 0u) || (rt->pressed == 0u) || (d == 0))
        {
            return 0u;
        }
        if (wvui_scroll_stay(&rt->sc, d->y) != 0u)
        {
            rt->moved = 1u; /* Past the threshold: drag, not tap-select */
            (void)wvui_scroll_take_delta(&rt->sc);
            wvui_widget_invalidate(w);
        }
        return 1u;

    case WVUI_EVENT_RELEASED:
        if (rt->pressed != 0u)
        {
            rt->pressed = 0u;
            (void)wvui_scroll_release(&rt->sc); /* Discard velocity: no inertia */
            rt->sc.vel = 0;
            roller_settle(w);
        }
        return (rt->enabled != 0u) ? 1u : 0u;

    case WVUI_EVENT_CLICKED:
        if (rt->enabled == 0u)
        {
            return 0u;
        }
        /* CLICKED after a drag does not tap-select: the core appends CLICKED to press+release,
         * reselecting here would overwrite the snap already started at RELEASED. */
        if ((rt->moved == 0u) && (d != 0))
        {
            int32_t local = (int32_t)d->y - (int32_t)w->y - roller_pad(w, rt) +
                            (int32_t)rt->sc.pos;
            int32_t acc = 0;
            uint16_t i;

            for (i = 0u; i < rt->count; i++)
            {
                acc += rt->row_h;
                if (local < acc)
                {
                    (void)wvui_scroll_set(&rt->sc, roller_target_for(w, rt, i));
                    roller_settle(w);
                    break;
                }
            }
        }
        rt->pressed = 0u;
        rt->moved = 0u;
        return 1u;

    default:
        return 0u;
    }
}

/* ---- Property read/write: VALUE = selected index, payload u8 ---- */
static int8_t roller_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
{
    wvui_roller_rt_t *rt = rt_of(w);

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
        roller_update_range(w);
        wvui_widget_invalidate(w);
        return WVUI_PROP_OK;

    case WVUI_PROP_VALUE:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_roller_set_selected(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_roller_set_colors(w, wvui_prop_u16(in), rt->fg, rt->sel_fg);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_roller_set_colors(w, rt->band, wvui_prop_u16(in), rt->sel_fg);
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_roller_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_ENABLED:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_roller_set_enabled(w, in[0]);
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t roller_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_roller_rt_t *rt = (const wvui_roller_rt_t *)wvui_widget_rt_c(w);

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
        wvui_prop_write_u16(out, rt->band);
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

const wvui_class_t wvui_roller_class = {
    .rt_size = (uint16_t)sizeof(wvui_roller_rt_t),
    .draw = roller_draw,
    .event = roller_event,
    .init = roller_init,
    .set_pos = 0,
    .on_delete = roller_delete,
    .set_prop = roller_set_prop,
    .get_prop = roller_get_prop,
    .flags = 0u,
};

/* ---- Property setters ---- */
void wvui_roller_refresh(wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_roller_class))
    {
        return;
    }
    roller_update_range(w);
    wvui_widget_invalidate(w);
}

void wvui_roller_bind(wvui_widget_t *w, const char *const *items, uint16_t count)
{
    wvui_roller_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_roller_class))
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
    roller_update_range(w);
    (void)wvui_scroll_set(&rt->sc, roller_target_for(w, rt, rt->selected));
    wvui_widget_invalidate(w);
}

void wvui_roller_set_font(wvui_widget_t *w, const wvui_font_t *font)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_roller_class))
    {
        return;
    }
    if (rt_of(w)->font != font)
    {
        rt_of(w)->font = font;
        wvui_widget_invalidate(w);
    }
}

void wvui_roller_set_row_height(wvui_widget_t *w, int16_t row_h)
{
    wvui_roller_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_roller_class))
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
    roller_update_range(w);
    (void)wvui_scroll_set(&rt->sc, roller_target_for(w, rt, rt->selected));
    wvui_widget_invalidate(w);
}

void wvui_roller_set_selected(wvui_widget_t *w, uint16_t index)
{
    wvui_roller_rt_t *rt;
    uint16_t v;

    if (!WVUI_W_IS_TYPE(w, &wvui_roller_class))
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
    roller_stop_anim(w);
    (void)wvui_scroll_set(&rt->sc, roller_target_for(w, rt, v)); /* Snap immediately */
    wvui_widget_invalidate(w);
    if (rt->on_select != 0)
    {
        rt->on_select(w, v, rt->user);
    }
}

void wvui_roller_set_colors(wvui_widget_t *w, wvui_color_t band, wvui_color_t fg,
                           wvui_color_t sel_fg)
{
    wvui_roller_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_roller_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->band == band) && (rt->fg == fg) && (rt->sel_fg == sel_fg))
    {
        return;
    }
    rt->band = band;
    rt->fg = fg;
    rt->sel_fg = sel_fg;
    wvui_widget_invalidate(w);
}

void wvui_roller_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_roller_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

void wvui_roller_set_enabled(wvui_widget_t *w, uint8_t enabled)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_roller_class))
    {
        return;
    }
    if (rt_of(w)->enabled != (uint8_t)(enabled != 0u))
    {
        rt_of(w)->enabled = (uint8_t)(enabled != 0u);
        wvui_widget_invalidate(w);
    }
}

void wvui_roller_set_select_cb(wvui_widget_t *w, wvui_roller_select_cb_t cb, void *user)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_roller_class))
    {
        return;
    }
    rt_of(w)->on_select = cb;
    rt_of(w)->user = user;
}

uint16_t wvui_roller_count(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_roller_class))
    {
        return 0u;
    }
    return ((const wvui_roller_rt_t *)wvui_widget_rt_c(w))->count;
}

uint16_t wvui_roller_selected(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_roller_class))
    {
        return 0u;
    }
    return ((const wvui_roller_rt_t *)wvui_widget_rt_c(w))->selected;
}

int32_t wvui_roller_scroll(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_roller_class))
    {
        return 0;
    }
    return ((const wvui_roller_rt_t *)wvui_widget_rt_c(w))->sc.pos;
}

const char *wvui_roller_item(const wvui_widget_t *w, uint16_t index)
{
    const wvui_roller_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_roller_class))
    {
        return 0;
    }
    rt = (const wvui_roller_rt_t *)wvui_widget_rt_c(w);
    if ((rt->items == 0) || (index >= rt->count))
    {
        return 0;
    }
    return rt->items[index];
}
