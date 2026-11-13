/**
 * @file wvui_stepper.c
 * @brief Stepper implementation: fixed-point formatting and long-press repeat.
 * @note  Decimal formatting uses a power-of-10 table plus per-digit subtraction; the long press repeats by counting STAY frames.
 */

#include "wvui_stepper.h"

#include "wvui_draw.h"
#include "wvui_render.h"

/* Powers of 10: the integer part needs 10^9, decimal places go up to 4 digits. */
static const int32_t s_pow10[10] = {
    1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000
};

/* Value range +/-9999999: avoids INT32_MIN, and text[16] is enough. */
#define WVUI_STEPPER_LIMIT (9999999)

static wvui_stepper_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_stepper_rt_t *)wvui_widget_rt(w);
}

/* Fixed-point value to string: division-free, float-free, per-digit subtraction splits integer and fraction. */
static void stepper_format(wvui_stepper_rt_t *rt)
{
    int32_t av = (rt->value < 0) ? -rt->value : rt->value;
    int32_t scale = s_pow10[rt->decimals];
    char *p = rt->text;
    int32_t i;
    uint8_t started = 0u;

    if (rt->value < 0)
    {
        *p++ = '-';
    }

    /* Integer part: subtract from 10^9 down to 10^decimals, at most 9 subtractions per digit. */
    for (i = 9; i >= (int32_t)rt->decimals; i--)
    {
        int32_t place = s_pow10[i];
        int32_t digit = 0;

        while (av >= place)
        {
            av -= place;
            digit++;
        }
        if ((digit != 0) || (started != 0u) || (i == (int32_t)rt->decimals))
        {
            *p++ = (char)('0' + (char)digit);
            started = 1u;
        }
    }

    if (rt->decimals != 0u)
    {
        *p++ = '.';
        for (i = (int32_t)rt->decimals - 1; i >= 0; i--)
        {
            int32_t place = s_pow10[i];
            int32_t digit = 0;

            while (av >= place)
            {
                av -= place;
                digit++;
            }
            *p++ = (char)('0' + (char)digit);
        }
    }
    (void)scale;
    *p = '\0';
}

static int32_t stepper_clamp(const wvui_stepper_rt_t *rt, int32_t v)
{
    if (v < rt->min)
    {
        return rt->min;
    }
    if (v > rt->max)
    {
        return rt->max;
    }
    return v;
}

static void stepper_init(wvui_widget_t *w)
{
    wvui_stepper_rt_t *rt = rt_of(w);

    rt->value = 0;
    rt->min = -WVUI_STEPPER_LIMIT;
    rt->max = WVUI_STEPPER_LIMIT;
    rt->step = 1;
    rt->bg = WVUI_RGB565(48, 52, 60);
    rt->fg = WVUI_RGB565(240, 240, 240);
    rt->border = WVUI_RGB565(120, 130, 140);
    rt->decimals = 0u;
    rt->repeat = 6u;
    rt->repeat_count = 0u;
    rt->part = 0u;
    rt->opa = 255u;
    rt->enabled = 1u;
    rt->on_change = 0;
    rt->user = 0;
    stepper_format(rt);
}

/* '-' and '+' glyphs: drawn as antialiased line segments, no font dependency. */
static void stepper_glyph(wvui_widget_t *w, uint8_t plus, wvui_color_t c, uint8_t opa)
{
    int16_t cx = (int16_t)(w->x + (plus != 0u ? (w->w - (w->w >> 2)) : (w->w >> 2)));
    int16_t cy = (int16_t)(w->y + (w->h >> 1));
    int16_t arm = (int16_t)(w->h >> 2);

    if (arm < 2)
    {
        arm = 2;
    }
    wvui_draw_line((int16_t)(cx - arm), cy, (int16_t)(cx + arm), cy, 2u, WVUI_CAP_ROUND, c, opa);
    if (plus != 0u)
    {
        wvui_draw_line(cx, (int16_t)(cy - arm), cx, (int16_t)(cy + arm), 2u, WVUI_CAP_ROUND, c,
                      opa);
    }
}

static void stepper_draw(wvui_widget_t *w)
{
    const wvui_stepper_rt_t *rt = (const wvui_stepper_rt_t *)wvui_widget_rt_c(w);
    wvui_rect_t box = wvui_widget_rect(w);
    static const uint8_t square[4] = { WVUI_BOX_SQUARE, WVUI_BOX_SQUARE, WVUI_BOX_SQUARE,
                                       WVUI_BOX_SQUARE };
    wvui_color_t fg = rt->fg;

    if ((rt->opa == 0u) || (wvui_rect_is_empty(&box) != 0u))
    {
        return;
    }
    if (rt->enabled == 0u)
    {
        fg = WVUI_RGB565(110, 110, 110); /* disabled: glyphs turn gray */
    }

    /* Fill plus 1-pixel border, all four corners square. */
    wvui_draw_box(box, square, 0u, 1u, rt->bg, rt->opa, rt->border, rt->opa);

    wvui_render_set_clip(box);
    stepper_glyph(w, 0u, fg, rt->opa);
    stepper_glyph(w, 1u, fg, rt->opa);
    wvui_render_clear_clip();
}

/* ---- Events: decrement on the left half, increment on the right, long press repeats via STAY ---- */
static uint8_t stepper_half(const wvui_widget_t *w, const wvui_indev_t *d)
{
    int16_t mid;

    if (d == 0)
    {
        return 0u;
    }
    mid = (int16_t)(w->x + (w->w >> 1));
    return (uint8_t)((d->x < mid) ? 1u : 2u);
}

static void stepper_apply(wvui_widget_t *w, uint8_t part)
{
    wvui_stepper_rt_t *rt = rt_of(w);
    int32_t before = rt->value;

    if (part == 1u)
    {
        wvui_stepper_add(w, -rt->step);
    }
    else if (part == 2u)
    {
        wvui_stepper_add(w, rt->step);
    }
    (void)before;
}

static uint8_t stepper_event(wvui_widget_t *w, wvui_event_t e, wvui_indev_t *d)
{
    wvui_stepper_rt_t *rt = rt_of(w);

    switch (e)
    {
    case WVUI_EVENT_PRESSED:
        if (rt->enabled == 0u)
        {
            return 0u;
        }
        rt->part = stepper_half(w, d);
        rt->repeat_count = 0u;
        stepper_apply(w, rt->part); /* one step fires right on press */
        wvui_widget_invalidate(w);
        return 1u;

    case WVUI_EVENT_STAY:
        if ((rt->enabled == 0u) || (rt->part == 0u))
        {
            return 0u;
        }
        rt->repeat_count = (uint8_t)(rt->repeat_count + 1u);
        if (rt->repeat_count >= rt->repeat)
        {
            rt->repeat_count = 0u;
            stepper_apply(w, rt->part); /* one step per repeat frames */
        }
        return 1u;

    case WVUI_EVENT_RELEASED:
    case WVUI_EVENT_CLICKED:
        rt->part = 0u;
        rt->repeat_count = 0u;
        return (rt->enabled != 0u) ? 1u : 0u;

    default:
        return 0u;
    }
}

/* ---- Property handlers: VALUE32 is passed as 4 little-endian bytes, i.e. the fixed-point int32 ---- */
static int32_t prop_i32(const uint8_t *p)
{
    return (int32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
                     ((uint32_t)p[3] << 24));
}

static void prop_write_i32(uint8_t *p, int32_t v)
{
    uint32_t u = (uint32_t)v;

    p[0] = (uint8_t)(u & 0xFFu);
    p[1] = (uint8_t)((u >> 8) & 0xFFu);
    p[2] = (uint8_t)((u >> 16) & 0xFFu);
    p[3] = (uint8_t)((u >> 24) & 0xFFu);
}

static int8_t stepper_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len)
{
    wvui_stepper_rt_t *rt = rt_of(w);

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

    case WVUI_PROP_VALUE32:
        if (len != 4u) { return WVUI_PROP_ERR_ARG; }
        wvui_stepper_set_value(w, prop_i32(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_stepper_set_colors(w, wvui_prop_u16(in), rt->fg, rt->border);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_stepper_set_colors(w, rt->bg, wvui_prop_u16(in), rt->border);
        return WVUI_PROP_OK;

    case WVUI_PROP_BORDER:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_stepper_set_colors(w, rt->bg, rt->fg, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_stepper_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_ENABLED:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_stepper_set_enabled(w, in[0]);
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t stepper_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_stepper_rt_t *rt = (const wvui_stepper_rt_t *)wvui_widget_rt_c(w);

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

    case WVUI_PROP_VALUE32:
        if (cap < 4u) { return WVUI_PROP_ERR_ARG; }
        prop_write_i32(out, rt->value);
        return 4;

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

    case WVUI_PROP_ENABLED:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->enabled;
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_stepper_class = {
    .rt_size = (uint16_t)sizeof(wvui_stepper_rt_t),
    .draw = stepper_draw,
    .event = stepper_event,
    .init = stepper_init,
    .set_pos = 0,
    .on_delete = 0,
    .set_prop = stepper_set_prop,
    .get_prop = stepper_get_prop,
    .flags = 0u,
};

/* ---- Property setters ---- */
void wvui_stepper_set_range(wvui_widget_t *w, int32_t min, int32_t max, uint8_t decimals)
{
    wvui_stepper_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_stepper_class))
    {
        return;
    }
    rt = rt_of(w);
    if (decimals > 4u)
    {
        decimals = 4u;
    }
    if (min > max)
    {
        int32_t t = min;
        min = max;
        max = t;
    }
    if (min < -WVUI_STEPPER_LIMIT) { min = -WVUI_STEPPER_LIMIT; }
    if (max > WVUI_STEPPER_LIMIT) { max = WVUI_STEPPER_LIMIT; }
    rt->min = min;
    rt->max = max;
    rt->decimals = decimals;
    rt->value = stepper_clamp(rt, rt->value);
    stepper_format(rt);
    wvui_widget_invalidate(w);
}

void wvui_stepper_set_step(wvui_widget_t *w, int32_t step)
{
    wvui_stepper_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_stepper_class))
    {
        return;
    }
    rt = rt_of(w);
    if (step < 0)
    {
        step = -step;
    }
    rt->step = (step == 0) ? 1 : step; /* a step of 0 clamps to 1 */
}

void wvui_stepper_set_value(wvui_widget_t *w, int32_t value)
{
    wvui_stepper_rt_t *rt;
    int32_t v;

    if (!WVUI_W_IS_TYPE(w, &wvui_stepper_class))
    {
        return;
    }
    rt = rt_of(w);
    v = stepper_clamp(rt, value);
    if (rt->value == v)
    {
        return;
    }
    rt->value = v;
    stepper_format(rt); /* rebuild the text on value change */
    wvui_widget_invalidate(w);
    if (rt->on_change != 0)
    {
        rt->on_change(w, v, rt->user);
    }
}

void wvui_stepper_add(wvui_widget_t *w, int32_t delta)
{
    const wvui_stepper_rt_t *rt = (const wvui_stepper_rt_t *)wvui_widget_rt_c(w);
    int32_t v;

    if (!WVUI_W_IS_TYPE(w, &wvui_stepper_class))
    {
        return;
    }
    /* The addition may overflow int32: clamp to the range before adding (the range is capped at +/-9999999). */
    v = rt->value;
    if ((delta > 0) && (v > (rt->max - delta))) { v = rt->max; }
    else if ((delta < 0) && (v < (rt->min - delta))) { v = rt->min; }
    else { v += delta; }
    wvui_stepper_set_value(w, v);
}

void wvui_stepper_set_colors(wvui_widget_t *w, wvui_color_t bg, wvui_color_t fg,
                            wvui_color_t border)
{
    wvui_stepper_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_stepper_class))
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

void wvui_stepper_set_repeat(wvui_widget_t *w, uint8_t frames)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_stepper_class))
    {
        return;
    }
    if (frames < 1u)
    {
        /* one step per frame; 0 would leave STAY never advancing */
        frames = 1u;
    }
    rt_of(w)->repeat = frames;
}

void wvui_stepper_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_stepper_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

void wvui_stepper_set_enabled(wvui_widget_t *w, uint8_t enabled)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_stepper_class))
    {
        return;
    }
    if (rt_of(w)->enabled != (uint8_t)(enabled != 0u))
    {
        rt_of(w)->enabled = (uint8_t)(enabled != 0u);
        wvui_widget_invalidate(w);
    }
}

void wvui_stepper_set_change_cb(wvui_widget_t *w, wvui_stepper_change_cb_t cb, void *user)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_stepper_class))
    {
        return;
    }
    rt_of(w)->on_change = cb;
    rt_of(w)->user = user;
}

int32_t wvui_stepper_value(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_stepper_class))
    {
        return 0;
    }
    return ((const wvui_stepper_rt_t *)wvui_widget_rt_c(w))->value;
}

uint8_t wvui_stepper_decimals(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_stepper_class))
    {
        return 0u;
    }
    return ((const wvui_stepper_rt_t *)wvui_widget_rt_c(w))->decimals;
}

const char *wvui_stepper_text(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_stepper_class))
    {
        return "";
    }
    return ((const wvui_stepper_rt_t *)wvui_widget_rt_c(w))->text;
}
