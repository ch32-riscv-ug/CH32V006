#include "debug.h"
#include "wvui_app.h"
#include "wvui_font_mont14low.h"

#define PAGE_TITLE_ID (1u)
#define PAGE_NAV_ID (2u)
#define PAGE_LABEL1_ID (3u)
#define PAGE_LABEL2_ID (4u)
#define PAGE_BAR1_ID (5u)
#define PAGE_BAR2_ID (6u)

static const wvui_widget_desc_t s_descs[6] = {
    { .type = WVUI_TYPE_LABEL,    .id = PAGE_TITLE_ID,  .x = 16,  .y = 10,  .w = 48,  .h = 20 },
    { .type = WVUI_TYPE_LABEL,    .id = PAGE_LABEL1_ID, .x = 16,  .y = 52,  .w = 28,  .h = 20 },
    { .type = WVUI_TYPE_LABEL,    .id = PAGE_LABEL2_ID, .x = 16,  .y = 90,  .w = 28,  .h = 20 },
    { .type = WVUI_TYPE_PROGRESS, .id = PAGE_BAR1_ID,   .x = 56,  .y = 56,  .w = 248, .h = 14 },
    { .type = WVUI_TYPE_PROGRESS, .id = PAGE_BAR2_ID,   .x = 56,  .y = 94,  .w = 248, .h = 14 },
    { .type = WVUI_TYPE_BTN,      .id = PAGE_NAV_ID,    .x = 100, .y = 180, .w = 120, .h = 40 },
};

static void page_next_clicked(wvui_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    wvui_page_go(wvui_page2());
}

static uint32_t s_ms;
static uint16_t s_last_ms;
static uint8_t s_started;

static void page_tick(const wvui_page_t *self)
{
    wvui_widget_t *w;
    uint16_t now;
    uint16_t dt;
    uint8_t v;
    uint16_t t;

    (void)self;

    now = (uint16_t)(TIM_GetCounter(TIM2) >> 3);

    if (s_started == 0u)
    {
        s_started = 1u;
        s_last_ms = now;
        return;
    }

    dt = (uint16_t)(now - s_last_ms);
    s_last_ms = now;
    s_ms += dt;

    t = (uint16_t)((s_ms / 50u) & 0x1FFu);
    v = (uint8_t)((t < 256u) ? t : (511u - t));

    w = wvui_widget_find_by_id(PAGE_BAR1_ID);

    {
        wvui_progress_set_value(w, v);
    }

    w = wvui_widget_find_by_id(PAGE_BAR2_ID);

    {
        wvui_progress_set_value(w, (uint8_t)(255u - v));
    }
}

static void set_label(wvui_widget_t *w, const char *text, wvui_color_t color)
{
    wvui_label_set_font(w, &wvui_font_mont14low);
    wvui_label_set_text(w, text);
    wvui_label_set_color(w, color);
}

static void page_enter(const wvui_page_t *self)
{
    wvui_widget_t *w;

    (void)self;

    s_ms = 0u;
    s_started = 0u;

    w = wvui_widget_find_by_id(PAGE_TITLE_ID);

    {
        set_label(w, "sensor", WVUI_RGB565(30, 45, 65));
    }

    w = wvui_widget_find_by_id(PAGE_LABEL1_ID);

    {
        set_label(w, "ch1", WVUI_RGB565(95, 110, 135));
    }

    w = wvui_widget_find_by_id(PAGE_LABEL2_ID);

    {
        set_label(w, "ch2", WVUI_RGB565(95, 110, 135));
    }

    w = wvui_widget_find_by_id(PAGE_BAR1_ID);

    {
        wvui_progress_set_colors(w, WVUI_RGB565(215, 222, 232), WVUI_RGB565(80, 150, 230));
        wvui_progress_set_radius(w, 7);
    }

    w = wvui_widget_find_by_id(PAGE_BAR2_ID);

    {
        wvui_progress_set_colors(w, WVUI_RGB565(215, 222, 232), WVUI_RGB565(0, 175, 160));
        wvui_progress_set_radius(w, 7);
    }

    w = wvui_widget_find_by_id(PAGE_NAV_ID);

    {
        wvui_btn_set_text(w, "page 2");
        wvui_btn_set_font(w, &wvui_font_mont14low);
        wvui_btn_set_colors(w, WVUI_RGB565(215, 230, 250), WVUI_RGB565(175, 200, 235),
                           WVUI_RGB565(30, 50, 80));
        wvui_btn_set_click_cb(w, page_next_clicked, 0);
    }
}

static wvui_page_t s_page = {
    .descs = s_descs, .count = 6u, .on_enter = page_enter, .on_tick = page_tick,
};

wvui_page_t *wvui_page1(void)
{
    return &s_page;
}
