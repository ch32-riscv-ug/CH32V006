

#include "wvui_port_input.h"

#include "GT9xx.h"

#define WVUI_PANEL_W         (320)
#define WVUI_PANEL_H         (240)
#define WVUI_TOUCH_SWAP_XY   (1)
#define WVUI_TOUCH_MIRROR_X  (0)
#define WVUI_TOUCH_MIRROR_Y  (1)

static uint8_t s_was_pressed;

 
static void read_hardware(wvui_indev_t *out)
{
    int16_t x;
    int16_t y;
    uint16_t raw_x;
    uint16_t raw_y;

    if (touchChipInfo.ready == 0u)
    {
        out->pressed = 0u;
        s_was_pressed = 0u;
        return;
    }

    Touch_Scan();

    if ((touchInfo.flag == 0u) || (touchInfo.num == 0u))
    {
        out->pressed = 0u;
        s_was_pressed = 0u;
        return;
    }

    raw_x = touchInfo.x[0];
    raw_y = touchInfo.y[0];

#if WVUI_TOUCH_SWAP_XY
    {
        uint16_t tmp;

        tmp = raw_x;
        raw_x = raw_y;
        raw_y = tmp;
    }
#endif

    x = (int16_t)raw_x;
    y = (int16_t)raw_y;

#if WVUI_TOUCH_MIRROR_X
    x = (int16_t)(WVUI_PANEL_W - 1 - x);
#endif

#if WVUI_TOUCH_MIRROR_Y
    y = (int16_t)(WVUI_PANEL_H - 1 - y);
#endif


    out->x = x;
    out->y = y;
    out->pressed = 1u;

    if (s_was_pressed == 0u)
    {
        s_was_pressed = 1u;
    }
}

static const wvui_input_port_t s_input =
{
    .read = read_hardware
};

const wvui_input_port_t *wvui_port_input(void)
{
    return &s_input;
}
