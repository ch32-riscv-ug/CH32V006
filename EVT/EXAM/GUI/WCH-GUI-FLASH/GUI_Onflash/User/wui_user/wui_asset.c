#include "wui_asset.h"
#include <stddef.h>

#include "LCD_Font.h"
const wui_bitfont_t wui_font_6x12 = { 6, 12, 0x20, 95, (const uint8_t*)asc2_1206 };
const wui_bitfont_t wui_font_8x16 = { 8, 16, 0x20, 95, (const uint8_t*)asc2_1608 };

/*******************************************************************************
 *                          STRING RESOURCES
 ******************************************************************************/

/* Maps string IDs to string pointers */
static const char* wui_string_table[WUI_STR_COUNT] = {
    [WUI_STR_NONE] = NULL,
};

/**
 * @brief Get string by ID
 * @param id - String ID
 * @return String pointer, or NULL if not found
 */
const char* wui_get_string(wui_str_id_t id)
{
    if (id > 0 && id < WUI_STR_COUNT)
    {
        return wui_string_table[id];
    }
    return NULL;
}

/*******************************************************************************
 *                          IMAGE TABLE RESOURCES
 ******************************************************************************/


/*  Maps image table IDs to image ID arrays */
static const uint16_t* wui_imgtable_list[WUI_IMGTABLE_COUNT] = {
    [WUI_IMGTABLE_NONE] = NULL,
};

/**
 * @brief Get image table by ID
 * @param id - Image table ID
 * @return Image table pointer, or NULL if not found
 */
const uint16_t* wui_get_imgtable(wui_imgtable_id_t id)
{
    if (id > 0 && id < WUI_IMGTABLE_COUNT)
    {
        return wui_imgtable_list[id];
    }
    return NULL;
}

/*******************************************************************************
 *                          FONT RESOURCES
 ******************************************************************************/

/**
 * @brief Font table for WUI
 * @details Maps font IDs to font structures
 */
static const wui_font_t wui_font_table[WUI_FONT_COUNT] = {
    [WUI_FONT_NONE] = {0},
};

const wui_font_t* wui_get_font(wui_font_id_t id)
{
    if (id > 0 && id < WUI_FONT_COUNT)
    {
        return &wui_font_table[id];
    }
    return NULL;
}

/*******************************************************************************
 *                      BITMAP FONT RESOURCES
 ******************************************************************************/

/*  Maps bitmap font IDs to font structures */
static const wui_bitfont_t* wui_bitmap_font_table[WUI_BITMAP_FONT_COUNT] = {
    [WUI_BITMAP_FONT_DEFAULT] = &wui_font_8x16,
    [WUI_BITMAP_FONT_6X12]    = &wui_font_6x12,
    [WUI_BITMAP_FONT_8X16]    = &wui_font_8x16,
};

/**
 * @brief Get bitmap font by ID
 * @param id - Bitmap font ID
 * @return Bitmap font structure pointer, or default font if not found
 */
const wui_bitfont_t* wui_get_bitmap_font(wui_bitmap_font_id_t id)
{
    if (id > 0 && id < WUI_BITMAP_FONT_COUNT) {
        return wui_bitmap_font_table[id];
    }
    return wui_bitmap_font_table[WUI_BITMAP_FONT_DEFAULT];
}


/*********************************************************************
 * @fn      wui_get_image
 *
 * @brief   Get image information by index.
 *
 * @param   index - Image index.
 *
 * @return  Pointer to image information structure.
 */
const image_info_t* wui_get_image(uint32_t index)
{
    // return get_image(index);
    uint16_t idx = IMAGE_GET_INDEX(index);
    if (idx >= IMAGE_COUNT) return NULL;
    return &image_table[idx];
}
