/**
 * @file wvui_segdisp.c
 * @brief Seven-segment display implementation: segment-mask-table glyphs, per-digit drawing and number formatting.
 * @note  Each digit draws 7 segments plus the decimal point; number formatting uses a powers-of-10 table with
 *        digit-by-digit subtraction, zero division.
 */

#include "wvui_segdisp.h"

#include "wvui_draw.h"
#include "wvui_render.h"

/* Segment masks: bit0..6 = segments a..g, bit7 = decimal point; glyphs are defined entirely by this
 * table, no font involved. */
static const uint8_t s_seg_glyph[16] = {
    0x3Fu, /* 0: abcdef          */
    0x06u, /* 1: bc              */
    0x5Bu, /* 2: abdeg           */
    0x4Fu, /* 3: abcdg           */
    0x66u, /* 4: bcfg            */
    0x6Du, /* 5: acdfg           */
    0x7Du, /* 6: acdefg          */
    0x07u, /* 7: abc             */
    0x7Fu, /* 8: abcdefg         */
    0x6Fu, /* 9: abcdfg          */
    0x77u, /* A: abcefg          */
    0x7Cu, /* b: cdefg           */
    0x39u, /* C: adef            */
    0x5Eu, /* d: bcdeg           */
    0x79u, /* E: adefg           */
    0x71u  /* F: aefg            */
};

#define SEG_BLANK (0x00u)
#define SEG_MINUS (0x40u) /* middle bar only = minus sign */
#define SEG_COLON (0x00u) /* colon placeholder, drawn as dots at draw time */

static wvui_segdisp_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_segdisp_rt_t *)wvui_widget_rt(w);
}

static uint8_t segdisp_glyph(char c)
{
    if ((c >= '0') && (c <= '9'))
    {
        return s_seg_glyph[(uint8_t)(c - '0')];
    }
    if ((c >= 'A') && (c <= 'F'))
    {
        return s_seg_glyph[(uint8_t)(c - 'A' + 10)];
    }
    if ((c >= 'a') && (c <= 'f'))
    {
        return s_seg_glyph[(uint8_t)(c - 'a' + 10)];
    }
    if (c == '-')
    {
        return SEG_MINUS;
    }
    if (c == ':')
    {
        return SEG_COLON;
    }
    return SEG_BLANK;
}

/* Number -> characters: high digits saturate at 9 beyond the digit count, low digits are never truncated. */
static void segdisp_format(wvui_segdisp_rt_t *rt)
{
    static const uint32_t pow10[10] = { 1u,      10u,      100u,      1000u,     10000u,
                                        100000u, 1000000u, 10000000u, 100000000u,
                                        1000000000u };
    uint32_t v = rt->value;
    uint8_t digits = rt->digits;
    uint8_t dec = rt->decimals;
    uint8_t i;
    uint8_t pos = 0u;

    if (digits > 10u)
    {
        digits = 10u;
    }
    if (dec > digits)
    {
        dec = digits;
    }
    if (digits == 0u)
    {
        rt->text[0] = '\0';
        return;
    }

    /* Digit extraction: from the most significant digit, at most 9 subtractions per digit */
    for (i = digits; i > 0u; i--)
    {
        uint32_t place = pow10[i - 1u];
        uint8_t d = 0u;

        while (v >= place)
        {
            v -= place;
            d++;
            if (d >= 9u)
            {
                break; /* saturate within the digit */
            }
        }
        rt->text[pos++] = (char)('0' + (char)d);
        /* decimal point goes after the last integer digit */
        if ((dec != 0u) && (i == (uint8_t)(dec + 1u)))
        {
            rt->text[pos++] = '.';
        }
    }
    rt->text[pos] = '\0';
}

static void segdisp_init(wvui_widget_t *w)
{
    wvui_segdisp_rt_t *rt = rt_of(w);

    rt->text[0] = '0';
    rt->text[1] = '\0';
    rt->value = 0u;
    rt->on = WVUI_RGB565(255, 80, 60);
    rt->off = WVUI_RGB565(48, 40, 40);
    rt->digits = 4u;
    rt->decimals = 0u;
    rt->seg_w = 16u;
    rt->seg_h = 0u; /* 0 = use widget height */
    rt->seg_t = 3u;
    rt->gap = 0u;   /* 0 = auto */
    rt->opa = 255u;
    segdisp_format(rt);
}

/* Drawn segment thickness: at least 2; a 1-pixel round-cap line is smeared by anti-aliasing. */
static int32_t segdisp_t(const wvui_segdisp_rt_t *rt)
{
    return (rt->seg_t < 2u) ? 2 : (int32_t)rt->seg_t;
}

/* Effective digit gap: 0 = auto; at least 3 pixels so the decimal point fits. */
static int32_t segdisp_gap(const wvui_segdisp_rt_t *rt)
{
    int32_t t = segdisp_t(rt);

    return (rt->gap != 0u) ? (int32_t)rt->gap : ((t > 3) ? t : 3);
}

/* One decimal point: diameter = min(segment thickness, digit gap), centre tucked at the lower
 * right, always clear of the neighbouring digit. */
static void segdisp_dot(const wvui_segdisp_rt_t *rt, int32_t x0, int32_t y1, int32_t d)
{
    int32_t cx = x0 + (d >> 1);
    int32_t cy = y1 - (d >> 1);

    wvui_draw_line((int16_t)cx, (int16_t)cy, (int16_t)cx, (int16_t)cy, (uint8_t)d,
                  WVUI_CAP_ROUND, rt->on, rt->opa);
}

/* Per digit: 7 segments, each drawn as a round-cap AA line. */
static void segdisp_digit(wvui_widget_t *w, const wvui_segdisp_rt_t *rt, wvui_rect_t d,
                          uint8_t mask)
{
    int32_t t = segdisp_t(rt);
    int32_t mid = (int32_t)d.y0 + ((int32_t)(d.y1 - d.y0) >> 1);
    int32_t xl = (int32_t)d.x0 + (t >> 1);
    int32_t xr = (int32_t)d.x1 - (t >> 1);
    int32_t yt = (int32_t)d.y0 + (t >> 1);
    int32_t yb = (int32_t)d.y1 - (t >> 1);
    uint8_t i;

    for (i = 0u; i < 7u; i++)
    {
        uint8_t lit = (uint8_t)((mask & (uint8_t)(1u << i)) != 0u);
        wvui_color_t c = (lit != 0u) ? rt->on : rt->off;
        int16_t x0 = 0;
        int16_t y0 = 0;
        int16_t x1 = 0;
        int16_t y1 = 0;

        if (rt->off == rt->on && lit == 0u)
        {
            continue; /* same color: skip unlit segments */
        }
        switch (i)
        {
        case 0: /* a top bar */
            x0 = (int16_t)((int32_t)d.x0 + t);
            y0 = (int16_t)yt;
            x1 = (int16_t)((int32_t)d.x1 - t);
            y1 = (int16_t)yt;
            break;
        case 1: /* b upper right */
            x0 = (int16_t)xr;
            y0 = (int16_t)((int32_t)d.y0 + t);
            x1 = (int16_t)xr;
            y1 = (int16_t)(mid - (t >> 1));
            break;
        case 2: /* c lower right */
            x0 = (int16_t)xr;
            y0 = (int16_t)(mid + (t >> 1));
            x1 = (int16_t)xr;
            y1 = (int16_t)((int32_t)d.y1 - t);
            break;
        case 3: /* d bottom bar */
            x0 = (int16_t)((int32_t)d.x0 + t);
            y0 = (int16_t)yb;
            x1 = (int16_t)((int32_t)d.x1 - t);
            y1 = (int16_t)yb;
            break;
        case 4: /* e lower left */
            x0 = (int16_t)xl;
            y0 = (int16_t)(mid + (t >> 1));
            x1 = (int16_t)xl;
            y1 = (int16_t)((int32_t)d.y1 - t);
            break;
        case 5: /* f upper left */
            x0 = (int16_t)xl;
            y0 = (int16_t)((int32_t)d.y0 + t);
            x1 = (int16_t)xl;
            y1 = (int16_t)(mid - (t >> 1));
            break;
        default: /* g middle bar */
            x0 = (int16_t)((int32_t)d.x0 + t);
            y0 = (int16_t)mid;
            x1 = (int16_t)((int32_t)d.x1 - t);
            y1 = (int16_t)mid;
            break;
        }
        wvui_draw_line(x0, y0, x1, y1, (uint8_t)t, WVUI_CAP_ROUND, c, rt->opa);
    }
    (void)w;
}

static void segdisp_draw(wvui_widget_t *w)
{
    const wvui_segdisp_rt_t *rt = (const wvui_segdisp_rt_t *)wvui_widget_rt_c(w);
    wvui_rect_t box = wvui_widget_rect(w);
    int32_t dw = (int32_t)rt->seg_w;
    int32_t dh;
    int32_t gap;
    int32_t dotd;
    int32_t x;
    int32_t y0;
    int32_t y1;
    int32_t box_h = (int32_t)wvui_rect_h(&box);
    uint8_t n = 0u;
    uint8_t i;

    if ((rt->opa == 0u) || (dw < 4) || (wvui_rect_is_empty(&box) != 0u))
    {
        return;
    }
    dh = (rt->seg_h != 0u) ? (int32_t)rt->seg_h : box_h;
    if (dh <= 0)
    {
        return;
    }

    gap = segdisp_gap(rt);
    dotd = segdisp_t(rt);
    if (dotd > gap)
    {
        dotd = gap; /* decimal point diameter clamped inside the gap, never over neighbours */
    }
    y0 = (int32_t)box.y0 + ((box_h - dh) >> 1);
    y1 = y0 + dh - 1;
    x = (int32_t)box.x1; /* lay out right to left */

    while (rt->text[n] != '\0')
    {
        n++;
    }

    wvui_render_set_clip(box);
    for (i = n; i > 0u; i--)
    {
        char c = rt->text[i - 1u];
        wvui_rect_t d;

        if (c == '.')
        {
            /* Decimal point falls inside the digit gap, takes no digit width (cursor does not
             * move); a trailing one is clamped back inside the widget. */
            int32_t dx0 = x + 1;

            if ((dx0 + dotd - 1) > (int32_t)box.x1)
            {
                dx0 = (int32_t)box.x1 - dotd + 1;
            }
            segdisp_dot(rt, dx0, y1, dotd);
            continue;
        }

        d.x1 = (int16_t)x;
        d.x0 = (int16_t)(x - dw + 1);
        d.y0 = (int16_t)y0;
        d.y1 = (int16_t)y1;

        if (d.x1 < box.x0)
        {
            break; /* leading digits that do not fit are not drawn */
        }
        if (c == ':')
        {
            /* Colon: two dots at 1/4 and 3/4 height, occupies a digit slot, no segments drawn. */
            int32_t px = (int32_t)d.x0 + (dw >> 1);
            int32_t up = (int32_t)d.y0 + (dh >> 2);
            int32_t dn = (int32_t)d.y1 - (dh >> 2);

            wvui_draw_line((int16_t)px, (int16_t)up, (int16_t)px, (int16_t)up,
                          (uint8_t)segdisp_t(rt), WVUI_CAP_ROUND, rt->on, rt->opa);
            wvui_draw_line((int16_t)px, (int16_t)dn, (int16_t)px, (int16_t)dn,
                          (uint8_t)segdisp_t(rt), WVUI_CAP_ROUND, rt->on, rt->opa);
        }
        else
        {
            segdisp_digit(w, rt, d, segdisp_glyph(c));
        }
        x = (int32_t)d.x0 - 1 - gap;
    }
    wvui_render_clear_clip();
}

/* ---- Property access ---- */
static int32_t segdisp_prop_i32(const uint8_t *p)
{
    return (int32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
                     ((uint32_t)p[3] << 24));
}

static int8_t segdisp_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in,
                               uint16_t len)
{
    wvui_segdisp_rt_t *rt = rt_of(w);

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
        wvui_segdisp_set_text(w, (const char *)in);
        return WVUI_PROP_OK;

    case WVUI_PROP_VALUE32:
        if (len != 4u) { return WVUI_PROP_ERR_ARG; }
        wvui_segdisp_set_value(w, (uint32_t)segdisp_prop_i32(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_segdisp_set_colors(w, wvui_prop_u16(in), rt->off);
        return WVUI_PROP_OK;

    case WVUI_PROP_BG:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_segdisp_set_colors(w, rt->on, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_segdisp_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_RADIUS: /* RADIUS reused as segment thickness */
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_segdisp_set_seg_size(w, rt->seg_w, rt->seg_h, in[0]);
        return WVUI_PROP_OK;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t segdisp_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_segdisp_rt_t *rt = (const wvui_segdisp_rt_t *)wvui_widget_rt_c(w);

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

    case WVUI_PROP_VALUE32:
        if (cap < 4u) { return WVUI_PROP_ERR_ARG; }
        {
            uint32_t u = rt->value;

            out[0] = (uint8_t)(u & 0xFFu);
            out[1] = (uint8_t)((u >> 8) & 0xFFu);
            out[2] = (uint8_t)((u >> 16) & 0xFFu);
            out[3] = (uint8_t)((u >> 24) & 0xFFu);
        }
        return 4;

    case WVUI_PROP_COLOR:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->on);
        return 2;

    case WVUI_PROP_BG:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->off);
        return 2;

    case WVUI_PROP_OPA:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->opa;
        return 1;

    case WVUI_PROP_RADIUS:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->seg_t;
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_segdisp_class = {
    .rt_size = (uint16_t)sizeof(wvui_segdisp_rt_t),
    .draw = segdisp_draw,
    .event = 0, /* decorative widget: no gesture interception */
    .init = segdisp_init,
    .set_pos = 0,
    .on_delete = 0,
    .set_prop = segdisp_set_prop,
    .get_prop = segdisp_get_prop,
    .flags = 0u,
};

/* ---- Setters ---- */
void wvui_segdisp_set_text(wvui_widget_t *w, const char *text)
{
    wvui_segdisp_rt_t *rt;
    uint8_t i = 0u;

    if (!WVUI_W_IS_TYPE(w, &wvui_segdisp_class))
    {
        return;
    }
    rt = rt_of(w);
    if (text == 0)
    {
        text = "";
    }
    while ((text[i] != '\0') && (i < (uint8_t)(sizeof(rt->text) - 1u)))
    {
        rt->text[i] = text[i];
        i++;
    }
    rt->text[i] = '\0';
    wvui_widget_invalidate(w);
}

void wvui_segdisp_set_value(wvui_widget_t *w, uint32_t value)
{
    wvui_segdisp_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_segdisp_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rt->value == value)
    {
        return;
    }
    rt->value = value;
    segdisp_format(rt); /* re-format on value change; drawing reads the text buffer only. */
    wvui_widget_invalidate(w);
}

void wvui_segdisp_set_digits(wvui_widget_t *w, uint8_t digits)
{
    wvui_segdisp_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_segdisp_class))
    {
        return;
    }
    rt = rt_of(w);
    if (digits > 10u)
    {
        digits = 10u;
    }
    if (digits < 1u)
    {
        digits = 1u;
    }
    if (rt->digits != digits)
    {
        rt->digits = digits;
        segdisp_format(rt);
        wvui_widget_invalidate(w);
    }
}

void wvui_segdisp_set_decimals(wvui_widget_t *w, uint8_t decimals)
{
    wvui_segdisp_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_segdisp_class))
    {
        return;
    }
    rt = rt_of(w);
    if (decimals > rt->digits)
    {
        decimals = rt->digits;
    }
    if (rt->decimals != decimals)
    {
        rt->decimals = decimals;
        segdisp_format(rt);
        wvui_widget_invalidate(w);
    }
}

void wvui_segdisp_set_seg_size(wvui_widget_t *w, uint8_t seg_w, uint8_t seg_h, uint8_t seg_t)
{
    wvui_segdisp_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_segdisp_class))
    {
        return;
    }
    rt = rt_of(w);
    if (seg_w < 4u)
    {
        seg_w = 4u;
    }
    if (seg_t < 1u)
    {
        seg_t = 1u;
    }
    if ((rt->seg_w == seg_w) && (rt->seg_h == seg_h) && (rt->seg_t == seg_t))
    {
        return;
    }
    rt->seg_w = seg_w;
    rt->seg_h = seg_h;
    rt->seg_t = seg_t;
    wvui_widget_invalidate(w);
}

void wvui_segdisp_set_gap(wvui_widget_t *w, uint8_t gap)
{
    wvui_segdisp_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_segdisp_class))
    {
        return;
    }
    rt = rt_of(w);
    if (rt->gap != gap)
    {
        rt->gap = gap;
        wvui_widget_invalidate(w);
    }
}

void wvui_segdisp_set_colors(wvui_widget_t *w, wvui_color_t on, wvui_color_t off)
{
    wvui_segdisp_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_segdisp_class))
    {
        return;
    }
    rt = rt_of(w);
    if ((rt->on == on) && (rt->off == off))
    {
        return;
    }
    rt->on = on;
    rt->off = off;
    wvui_widget_invalidate(w);
}

void wvui_segdisp_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_segdisp_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

uint32_t wvui_segdisp_value(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_segdisp_class))
    {
        return 0u;
    }
    return ((const wvui_segdisp_rt_t *)wvui_widget_rt_c(w))->value;
}

const char *wvui_segdisp_text(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_segdisp_class))
    {
        return "";
    }
    return ((const wvui_segdisp_rt_t *)wvui_widget_rt_c(w))->text;
}
