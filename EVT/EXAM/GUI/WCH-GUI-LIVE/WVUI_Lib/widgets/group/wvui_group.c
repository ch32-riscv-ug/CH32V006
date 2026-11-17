/**
 * @file wvui_group.c
 * @brief Group container implementation: draws only its own background and border.
 * @note  Children are drawn by the kernel recursively; a transparent group is the default and draws nothing.
 */

#include "wvui_group.h"

#include "wvui_draw.h"
#include "wvui_render.h"

static wvui_group_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_group_rt_t *)wvui_widget_rt(w);
}

static void group_init(wvui_widget_t *w)
{
    wvui_group_rt_t *rt = rt_of(w);

    rt->children_head = 0;
    rt->bg = 0u;       /* 0 = no background */
    rt->bg_on = 0u;
    rt->border = 0u;   /* 0 = no border */
    rt->border_w = 0u;
    rt->radius = 0u;
    rt->opa = 255u;
}

static void group_draw(wvui_widget_t *w)
{
    const wvui_group_rt_t *rt = (const wvui_group_rt_t *)wvui_widget_rt_c(w);
    wvui_rect_t box = wvui_widget_rect(w);
    uint8_t has_bg = (uint8_t)((rt->bg_on != 0u) || (rt->bg != 0u));

    if ((rt->opa == 0u) || (wvui_rect_is_empty(&box) != 0u))
    {
        return;
    }
    if ((has_bg == 0u) && ((rt->border == 0u) || (rt->border_w == 0u)))
    {
        return; /* Transparent group: default form, draws nothing */
    }
    /* A bg value of 0 is a valid color (pure black); bg_on tells whether to fill. */
    wvui_draw_round_rect(box, rt->radius, rt->border_w, rt->bg, rt->opa, rt->border, rt->opa);
}

/* ---- Property get/set (command channel) ---- */
static int8_t group_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in,
                             uint16_t len)
{
    wvui_group_rt_t *rt = rt_of(w);

    /* ENABLED = visible (1) / hidden (0). */
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

    case WVUI_PROP_ENABLED:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_group_set_hidden(w, (uint8_t)((in[0] != 0u) ? 0u : 1u));
        return WVUI_PROP_OK;

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_group_set_colors(w, wvui_prop_u16(in), rt->border);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR: /* Border color; this widget has no text color */
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_group_set_colors(w, rt->bg, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_BORDER_W:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_group_set_border(w, in[0], rt->radius);
        return WVUI_PROP_OK;

    case WVUI_PROP_RADIUS:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_group_set_border(w, rt->border_w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_group_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_VALUE: /* Read-only: child count */
        return WVUI_PROP_ERR_RO;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t group_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_group_rt_t *rt = (const wvui_group_rt_t *)wvui_widget_rt_c(w);

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

    case WVUI_PROP_ENABLED:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = (uint8_t)((wvui_widget_is_hidden(w) != 0u) ? 0u : 1u);
        return 1;

    case WVUI_PROP_VALUE:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        {
            /* Channel is one byte: saturate to 255 for more than 255 children (a cast would wrap). */
            uint16_t cnt = wvui_group_child_count(w);

            out[0] = (uint8_t)((cnt > 255u) ? 255u : cnt);
        }
        return 1;

    case WVUI_PROP_BG:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->bg);
        return 2;

    case WVUI_PROP_COLOR:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->border);
        return 2;

    case WVUI_PROP_BORDER_W:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->border_w;
        return 1;

    case WVUI_PROP_RADIUS:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->radius;
        return 1;

    case WVUI_PROP_OPA:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->opa;
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_group_class = {
    .rt_size = (uint16_t)sizeof(wvui_group_rt_t),
    .draw = group_draw,
    .event = 0, /* No event callback; never intercepts gestures, kernel recurses into children */
    .init = group_init,
    .set_pos = 0,
    .on_delete = 0,
    .set_prop = group_set_prop,
    .get_prop = group_get_prop,
    .flags = WVUI_CLASS_CHILD_OWNER, /* First field = child chain head */
};

/* ---- Property setters ---- */
void wvui_group_add(wvui_widget_t *g, wvui_widget_t *child)
{
    if (!WVUI_W_IS_TYPE(g, &wvui_group_class) || (child == 0))
    {
        return;
    }
    wvui_widget_append_child(g, child);
}

void wvui_group_set_hidden(wvui_widget_t *g, uint8_t hidden)
{
    if (!WVUI_W_IS_TYPE(g, &wvui_group_class))
    {
        return;
    }
    wvui_widget_set_hidden(g, hidden);
}

uint8_t wvui_group_is_hidden(const wvui_widget_t *g)
{
    if (!WVUI_W_IS_TYPE(g, &wvui_group_class))
    {
        return 0u;
    }
    return wvui_widget_is_hidden(g);
}

void wvui_group_set_colors(wvui_widget_t *g, wvui_color_t bg, wvui_color_t border)
{
    wvui_group_rt_t *rt;

    if (!WVUI_W_IS_TYPE(g, &wvui_group_class))
    {
        return;
    }
    rt = rt_of(g);
    if ((rt->bg == bg) && (rt->border == border))
    {
        return;
    }
    rt->bg = bg;
    rt->border = border;
    wvui_widget_invalidate(g);
}

void wvui_group_set_bg_visible(wvui_widget_t *g, uint8_t visible)
{
    wvui_group_rt_t *rt;
    uint8_t v = (uint8_t)((visible != 0u) ? 1u : 0u);

    if (!WVUI_W_IS_TYPE(g, &wvui_group_class))
    {
        return;
    }
    rt = rt_of(g);
    if (rt->bg_on == v)
    {
        return;
    }
    rt->bg_on = v;
    wvui_widget_invalidate(g);
}

void wvui_group_set_border(wvui_widget_t *g, uint8_t border_w, uint8_t radius)
{
    wvui_group_rt_t *rt;

    if (!WVUI_W_IS_TYPE(g, &wvui_group_class))
    {
        return;
    }
    rt = rt_of(g);
    if ((rt->border_w == border_w) && (rt->radius == radius))
    {
        return;
    }
    rt->border_w = border_w;
    rt->radius = radius;
    wvui_widget_invalidate(g);
}

void wvui_group_set_opa(wvui_widget_t *g, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(g, &wvui_group_class))
    {
        return;
    }
    if (rt_of(g)->opa != opa)
    {
        rt_of(g)->opa = opa;
        wvui_widget_invalidate(g);
    }
}

uint16_t wvui_group_child_count(const wvui_widget_t *g)
{
    const wvui_widget_t *c;
    uint16_t n = 0u;

    if (!WVUI_W_IS_TYPE(g, &wvui_group_class))
    {
        return 0u;
    }
    for (c = *wvui_widget_children((wvui_widget_t *)g); c != 0; c = c->next)
    {
        n++;
    }
    return n;
}
