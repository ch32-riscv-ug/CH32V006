#include "wvui_app.h"
#include "wvui_font_mont14low.h"

#define PAGE_TITLE_ID (1u)
#define PAGE_NAV_ID (2u)
#define PAGE_BAR_ID (3u)
#define PAGE_T1_ID (4u)
#define PAGE_T2_ID (5u)
#define PAGE_T3_ID (6u)
#define PAGE_T4_ID (7u)

static const wvui_widget_desc_t s_descs[7] = {
    { .type = WVUI_TYPE_LABEL,    .id = PAGE_TITLE_ID, .x = 16,  .y = 10,  .w = 74,  .h = 18 },
    { .type = WVUI_TYPE_BTN,      .id = PAGE_T1_ID,    .x = 40,  .y = 50,  .w = 100, .h = 40 },
    { .type = WVUI_TYPE_BTN,      .id = PAGE_T2_ID,    .x = 180, .y = 50,  .w = 100, .h = 40 },
    { .type = WVUI_TYPE_BTN,      .id = PAGE_T3_ID,    .x = 40,  .y = 100, .w = 100, .h = 40 },
    { .type = WVUI_TYPE_BTN,      .id = PAGE_T4_ID,    .x = 180, .y = 100, .w = 100, .h = 40 },
    { .type = WVUI_TYPE_PROGRESS, .id = PAGE_BAR_ID,   .x = 20,  .y = 156, .w = 280, .h = 14 },
    { .type = WVUI_TYPE_BTN,      .id = PAGE_NAV_ID,   .x = 100, .y = 190, .w = 120, .h = 36 },
};

static void page_back_clicked(wvui_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    wvui_page_go(wvui_page1());
}

static void btn_action(wvui_widget_t *w, void *user)
{
    wvui_widget_t *bar;
    uintptr_t act = (uintptr_t)user;

    (void)w;

    bar = wvui_widget_find_by_id(PAGE_BAR_ID);
    if (bar == NULL)
    {
        return;
    }

    if (act == 1u)
    {
        wvui_progress_add(bar, 32u);
    }
    else if (act == 2u)
    {
        wvui_progress_sub(bar, 32u);
    }
    else if (act == 3u)
    {
        wvui_progress_set_value(bar, 255u);
    }
    else
    {
        wvui_progress_set_value(bar, 0u);
    }
}

static void set_btn(wvui_widget_t *w, const char *text, uintptr_t act)
{
    wvui_btn_set_text(w, text);
    wvui_btn_set_font(w, &wvui_font_mont14low);
    wvui_btn_set_colors(w, WVUI_RGB565(252, 236, 222), WVUI_RGB565(240, 205, 180),
                       WVUI_RGB565(90, 50, 20));
    wvui_btn_set_click_cb(w, btn_action, (void *)act);
}

static void page_enter(const wvui_page_t *self)
{
    wvui_widget_t *w;

    (void)self;

    w = wvui_widget_find_by_id(PAGE_TITLE_ID);

    {
        wvui_label_set_font(w, &wvui_font_mont14low);
        wvui_label_set_text(w, "touch test");
        wvui_label_set_color(w, WVUI_RGB565(30, 45, 65));
    }

    w = wvui_widget_find_by_id(PAGE_T1_ID);

    {
        set_btn(w, "1", 1u);
    }

    w = wvui_widget_find_by_id(PAGE_T2_ID);

    {
        set_btn(w, "2", 2u);
    }

    w = wvui_widget_find_by_id(PAGE_T3_ID);

    {
        set_btn(w, "3", 3u);
    }

    w = wvui_widget_find_by_id(PAGE_T4_ID);

    {
        set_btn(w, "4", 4u);
    }

    w = wvui_widget_find_by_id(PAGE_BAR_ID);

    {
        wvui_progress_set_colors(w, WVUI_RGB565(215, 222, 232), WVUI_RGB565(235, 150, 70));
        wvui_progress_set_radius(w, 7);
        wvui_progress_set_value(w, 48);
    }

    w = wvui_widget_find_by_id(PAGE_NAV_ID);

    {
        wvui_btn_set_text(w, "page 1");
        wvui_btn_set_font(w, &wvui_font_mont14low);
        wvui_btn_set_colors(w, WVUI_RGB565(220, 242, 230), WVUI_RGB565(185, 220, 200),
                           WVUI_RGB565(30, 70, 50));
        wvui_btn_set_click_cb(w, page_back_clicked, 0);
    }
}

static wvui_page_t s_page = {
    .descs = s_descs, .count = 7u, .on_enter = page_enter,
};

wvui_page_t *wvui_page2(void)
{
    return &s_page;
}
