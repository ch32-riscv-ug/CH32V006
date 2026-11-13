#ifndef WUI_PAGE_MAIN_H
#define WUI_PAGE_MAIN_H

#include "wui.h"
#include "wui_user/wui_page.h"
#include "wui_widget.h"
#include "images.h"
#include "wui_asset.h"
#include <stdio.h>

// ================= Widget ID Enum =================
typedef enum
{
    MAIN_BGBG,
    MAIN_TXTTITLE,
    MAIN_PGBPROGRESS,
    MAIN_IMGTIGERANIM,
} main_widget_id_t;

// ================= Declarative Widgets =================
static const wui_widget_desc_t widgets_main[] = {
    [MAIN_BGBG] = {
        .widget_id = MAIN_BGBG,
        .type      = Wui_w_Rect,
        .x         = 0,
        .y         = 0,
        .w         = 320,
        .h         = 240,
        .props.rect = {
            .color = RGB888_TO_RGB565(255, 246, 234),
        },
    },

    [MAIN_TXTTITLE] = {
        .widget_id = MAIN_TXTTITLE,
        .type      = Wui_w_StaticImage,
        .x         = 135,
        .y         = 151,
        .w         = 59,
        .h         = 32,
        .props.staticimg = {
            .img_id = IMAGE_TEXT_WUI_PAGE_MAIN_TXTTITLE,
        },
    },

    [MAIN_PGBPROGRESS] = {
        .widget_id = MAIN_PGBPROGRESS,
        .type      = Wui_w_Rect,
        .x         = 84,
        .y         = 188,
        .w         = 160,
        .h         = 2,
        .props.rect = {
            .color = RGB888_TO_RGB565(247, 178, 51),
        },
    },

    [MAIN_IMGTIGERANIM] = {
        .widget_id = MAIN_IMGTIGERANIM,
        .type      = Wui_w_Anime,
        .x         = 112,
        .y         = 36,
        .w         = 96,
        .h         = 96,
        .props.anime = {
            .start_img_id = IMAGE_IMG_TIGER_CUTE_1,
            .frame_count  = 4,
            .fps          = 3,
            .loop         = 1,
        },
    },

};


#endif // WUI_PAGE_MAIN_H
