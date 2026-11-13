#include "debug.h"
#include "wvui_app.h"

#define APP_REFRESH_MS (20u)

static uint16_t s_last_ms;
static uint8_t s_started;

int app_init(void)
{
    wvui_rect_t all;
    int16_t w = 0;
    int16_t h = 0;

    wvui_screen_size(&w, &h);

    all.x0 = 0;
    all.y0 = 0;
    all.x1 = (int16_t)(w - 1);
    all.y1 = (int16_t)(h - 1);

    if (wvui_page_reset(wvui_page1()) != 1u)
    {
        printf("FAIL: page1\r\n");
        return 1;
    }

    wvui_invalidate(all);

    return 0;
}

void wvui_app_tick(void)
{
    uint16_t now = (uint16_t)(TIM_GetCounter(TIM2) >> 3);

    if (s_started == 0u)
    {
        s_started = 1u;
        s_last_ms = (uint16_t)(now - APP_REFRESH_MS);
    }

    if ((uint16_t)(now - s_last_ms) >= APP_REFRESH_MS)
    {
        s_last_ms = now;
        wvui_frame_poll();
    }
}
