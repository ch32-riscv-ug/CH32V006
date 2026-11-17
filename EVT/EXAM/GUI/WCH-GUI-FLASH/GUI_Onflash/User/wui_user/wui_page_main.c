#include "wui_user/wui_page_main.h"

// ================= Key Handler =================

static void main_on_key(uint32_t key)
{
    (void)key;
}

// ================= Page Functions =================

void wui_page_main_init(void)
{
    wui_page_init(widgets_main, sizeof(widgets_main) / sizeof(widgets_main[0]));
    wui_register_key_handler(main_on_key);
}

void wui_page_main_loop(void)
{
}
