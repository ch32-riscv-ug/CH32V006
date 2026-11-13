/**
 * @file wvui_label_ex.c
 * @brief  Word-wrapping label implementation: wrap results cached in a line table.
 * @note   Draws from the cached line table every frame; wrapping happens only in refresh.
 */

#include "wvui_label_ex.h"

#include "wvui_config.h"
#include "wvui_render.h"
#include "widgets/label/wvui_label.h" /* reuses WVUI_LABEL_ALIGN_* and the align rules */

static wvui_label_ex_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_label_ex_rt_t *)wvui_widget_rt(w);
}

/* Word boundary: space, tab or newline. */
static uint8_t is_space(char c)
{
    return (uint8_t)((c == ' ') || (c == '\t') || (c == '\n'));
}

/* Find the longest prefix of width <= limit within max bytes, backing off
 * byte by byte. UTF-8 safety comes from the prefix API: a code point cut in
 * half is not counted. */
static uint16_t fit_prefix(const wvui_font_t *f, const char *s, uint16_t max,
                           int16_t limit)
{
    uint16_t n;

    for (n = max; n > 0u; n--)
    {
        if (wvui_font_prefix_width(f, s, n) <= limit)
        {
            return n;
        }
    }
    return 1u; /* always keep 1 byte, prevents an endless loop */
}

/* Re-wrap: break text into at most WVUI_CFG_LABEL_EX_LINES lines by widget width. */
void wvui_label_ex_refresh(wvui_widget_t *w)
{
    wvui_label_ex_rt_t *rt;
    uint16_t cur = 0u;
    int16_t limit;

    if (!WVUI_W_IS_TYPE(w, &wvui_label_ex_class))
    {
        return;
    }
    rt = rt_of(w);
    rt->lines = 0u;
    rt->truncated = 0u;
    if ((rt->text == 0) || (rt->text[0] == '\0') || (rt->font == 0))
    {
        wvui_widget_invalidate(w);
        return;
    }
    limit = (int16_t)(w->w - 2); /* 1 px padding on each side */
    if (limit < 1)
    {
        limit = 1;
    }

    while ((rt->text[cur] != '\0') && (rt->lines < (uint8_t)WVUI_CFG_LABEL_EX_LINES))
    {
        uint16_t start = cur;
        uint16_t len;

        if (rt->text[cur] == '\n')
        {
            /* Explicit newline: empty lines still count as one. */
            rt->line_off[rt->lines] = cur;
            rt->line_len[rt->lines] = 0u;
            rt->lines++;
            cur++;
            continue;
        }

        /* Advance word by word: keep going while it fits, stop or hard-break otherwise. */
        len = 0u;
        while (rt->text[start + len] != '\0')
        {
            uint16_t word_end = (uint16_t)(start + len);
            uint16_t try_len;
            uint8_t ended;

            /* Skip the spaces in front of the word. */
            while (is_space(rt->text[word_end]) != 0u)
            {
                if (rt->text[word_end] == '\n')
                {
                    break;
                }
                word_end++;
            }
            if (rt->text[word_end] == '\n')
            {
                break; /* this line stops at the explicit newline */
            }
            if (rt->text[word_end] == '\0')
            {
                break;
            }
            /* End of word: space, newline or end of string. */
            ended = 0u;
            while ((rt->text[word_end + ended] != '\0') &&
                   (is_space(rt->text[word_end + ended]) == 0u))
            {
                ended++;
            }
            try_len = (uint16_t)((word_end - start) + ended);
            if (wvui_font_prefix_width(rt->font, &rt->text[start], try_len) <= limit)
            {
                len = try_len; /* the word fits */
                if (rt->text[start + len] == '\0')
                {
                    break;
                }
                continue;
            }
            /* Does not fit: end the line here; hard-break if even the first
             * word does not fit. */
            if (len == 0u)
            {
                len = fit_prefix(rt->font, &rt->text[start], ended, limit);
            }
            break;
        }
        if (len == 0u)
        {
            len = fit_prefix(rt->font, &rt->text[start], (uint16_t)0xFFFFu, limit);
        }

        rt->line_off[rt->lines] = start;
        rt->line_len[rt->lines] = len;
        rt->lines++;
        cur = (uint16_t)(start + len);
        /* Drop the trailing newline, else the next pass counts it as an
         * extra empty line; a newline at line start is already recorded as
         * an empty line by the branch above. */
        if (rt->text[cur] == '\n')
        {
            cur++;
        }
        /* Drop trailing spaces: no leading blanks on the next line */
        while ((rt->text[cur] == ' ') || (rt->text[cur] == '\t'))
        {
            cur++;
        }
    }
    if (rt->text[cur] != '\0')
    {
        rt->truncated = 1u; /* text remains but the line limit is hit */
    }
    wvui_widget_invalidate(w);
}

static void label_ex_init(wvui_widget_t *w)
{
    wvui_label_ex_rt_t *rt = rt_of(w);

    rt->text = 0;
    rt->font = 0;
    rt->color = WVUI_RGB565(230, 235, 240);
    rt->lines = 0u;
    rt->align = WVUI_LABEL_ALIGN_LEFT;
    rt->opa = 255u;
    rt->truncated = 0u;
}

static void label_ex_draw(wvui_widget_t *w)
{
    const wvui_label_ex_rt_t *rt = (const wvui_label_ex_rt_t *)wvui_widget_rt_c(w);
    wvui_rect_t box = wvui_widget_rect(w);
    int16_t lh;
    uint8_t i;

    if ((rt->opa == 0u) || (rt->font == 0) || (rt->text == 0) || (rt->lines == 0u) ||
        (wvui_rect_is_empty(&box) != 0u))
    {
        return;
    }
    lh = wvui_font_line_height(rt->font);

    wvui_render_set_clip(box);
    for (i = 0u; i < rt->lines; i++)
    {
        int16_t lw = wvui_font_prefix_width(rt->font, &rt->text[rt->line_off[i]],
                                           rt->line_len[i]);
        int16_t x = box.x0;
        int16_t y = (int16_t)((int32_t)box.y0 + ((int32_t)i * (int32_t)lh));

        if (rt->align == WVUI_LABEL_ALIGN_CENTER)
        {
            x = (int16_t)(box.x0 + ((wvui_rect_w(&box) - lw) >> 1));
        }
        else if (rt->align == WVUI_LABEL_ALIGN_RIGHT)
        {
            x = (int16_t)(box.x1 - lw);
        }
        if (rt->line_len[i] != 0u)
        {
            wvui_font_draw_prefix(rt->font, x, y, &rt->text[rt->line_off[i]],
                                 rt->line_len[i], rt->color, rt->opa);
        }
    }
    wvui_render_clear_clip();
}

/* ---- Property read/write (command channel) ---- */
static int8_t label_ex_set_prop(wvui_widget_t *w, uint8_t prop, const uint8_t *in,
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
        wvui_label_ex_refresh(w); /* width changed -> every wrap changes */
        return WVUI_PROP_OK;

    case WVUI_PROP_TEXT:
        if ((len == 0u) || (in == 0)) { return WVUI_PROP_ERR_ARG; }
        wvui_label_ex_set_text(w, (const char *)in);
        return WVUI_PROP_OK;

    case WVUI_PROP_COLOR:
        if (len != 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_label_ex_set_color(w, wvui_prop_u16(in));
        return WVUI_PROP_OK;

    case WVUI_PROP_ALIGN:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_label_ex_set_align(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_OPA:
        if (len != 1u) { return WVUI_PROP_ERR_ARG; }
        wvui_label_ex_set_opa(w, in[0]);
        return WVUI_PROP_OK;

    case WVUI_PROP_ENABLED: /* read-only: truncation flag */
        return WVUI_PROP_ERR_RO;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

static int16_t label_ex_get_prop(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap)
{
    const wvui_label_ex_rt_t *rt = (const wvui_label_ex_rt_t *)wvui_widget_rt_c(w);

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

    case WVUI_PROP_VALUE: /* line count (read-only) */
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->lines;
        return 1;

    case WVUI_PROP_COLOR:
        if (cap < 2u) { return WVUI_PROP_ERR_ARG; }
        wvui_prop_write_u16(out, rt->color);
        return 2;

    case WVUI_PROP_ALIGN:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->align;
        return 1;

    case WVUI_PROP_OPA:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->opa;
        return 1;

    case WVUI_PROP_ENABLED:
        if (cap < 1u) { return WVUI_PROP_ERR_ARG; }
        out[0] = rt->truncated;
        return 1;

    default:
        return WVUI_PROP_ERR_UNSUP;
    }
}

const wvui_class_t wvui_label_ex_class = {
    .rt_size = (uint16_t)sizeof(wvui_label_ex_rt_t),
    .draw = label_ex_draw,
    .event = 0, /* decorative widget: no gesture interception */
    .init = label_ex_init,
    .set_pos = 0,
    .on_delete = 0,
    .set_prop = label_ex_set_prop,
    .get_prop = label_ex_get_prop,
    .flags = 0u,
};

/* ---- Property setters ---- */
void wvui_label_ex_set_text(wvui_widget_t *w, const char *text)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_label_ex_class))
    {
        return;
    }
    if (rt_of(w)->text != text)
    {
        rt_of(w)->text = text;
        wvui_label_ex_refresh(w); /* pointer changed: re-wrap */
    }
}

void wvui_label_ex_set_font(wvui_widget_t *w, const wvui_font_t *font)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_label_ex_class))
    {
        return;
    }
    if (rt_of(w)->font != font)
    {
        rt_of(w)->font = font;
        wvui_label_ex_refresh(w);
    }
}

void wvui_label_ex_set_color(wvui_widget_t *w, wvui_color_t color)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_label_ex_class))
    {
        return;
    }
    if (rt_of(w)->color != color)
    {
        rt_of(w)->color = color;
        wvui_widget_invalidate(w);
    }
}

void wvui_label_ex_set_align(wvui_widget_t *w, uint8_t align)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_label_ex_class))
    {
        return;
    }
    if (rt_of(w)->align != align)
    {
        rt_of(w)->align = align;
        wvui_widget_invalidate(w);
    }
}

void wvui_label_ex_set_opa(wvui_widget_t *w, uint8_t opa)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_label_ex_class))
    {
        return;
    }
    if (rt_of(w)->opa != opa)
    {
        rt_of(w)->opa = opa;
        wvui_widget_invalidate(w);
    }
}

const char *wvui_label_ex_text(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_label_ex_class))
    {
        return "";
    }
    return ((const wvui_label_ex_rt_t *)wvui_widget_rt_c(w))->text;
}

uint8_t wvui_label_ex_line_count(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_label_ex_class))
    {
        return 0u;
    }
    return ((const wvui_label_ex_rt_t *)wvui_widget_rt_c(w))->lines;
}

uint8_t wvui_label_ex_truncated(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_label_ex_class))
    {
        return 0u;
    }
    return ((const wvui_label_ex_rt_t *)wvui_widget_rt_c(w))->truncated;
}

int16_t wvui_label_ex_line_width(const wvui_widget_t *w, uint8_t index)
{
    const wvui_label_ex_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_label_ex_class))
    {
        return 0;
    }
    rt = (const wvui_label_ex_rt_t *)wvui_widget_rt_c(w);
    if ((rt->font == 0) || (rt->text == 0) || (index >= rt->lines))
    {
        return 0;
    }
    return wvui_font_prefix_width(rt->font, &rt->text[rt->line_off[index]],
                                 rt->line_len[index]);
}
