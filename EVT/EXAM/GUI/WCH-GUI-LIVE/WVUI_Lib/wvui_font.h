#ifndef WVUI_FONT_H
#define WVUI_FONT_H

/**
 * @file wvui_font.h
 * @brief Font and text engine: glyph description, UTF-8 decoding, metrics,
 *        drawing and per-line dirty marking.
 * @note  Lines always split at '\n'; line-top spacing of adjacent lines =
 *        line_height + line_gap (defined only in wvui_font_line_height);
 *        code points without a glyph are skipped.
 */

#include <stdint.h>
#include "wvui_asset.h" /* glyphs of the bitmap backend are asset IDs */
#include "wvui_config.h"
#include "wvui_types.h"

/** Code point range, endpoints included. */
typedef struct
{
    uint32_t start;  /**< First code point */
    uint32_t end;    /**< Last code point */
} wvui_font_range_t;

/** Glyph descriptor: all information for proportional metrics. */
typedef struct
{
    uint32_t adv_w : 8;          /**< Advance width, px, 1..255 */
    uint32_t box_w : 8;          /**< Bitmap box width, px, 0..255 */
    uint32_t box_h : 8;          /**< Bitmap box height, px, 0..255 */
    uint32_t x_ofs : 8;          /**< X offset from the layout pen, px, 0..255 */
    uint32_t y_ofs : 8;          /**< Y offset from the line top, px, 0..255 */
    uint32_t bitmap_offset : 24; /**< Bitmap byte offset in the shared stream, < 16 MB */
} wvui_font_glyph_t;

WVUI_STATIC_ASSERT(sizeof(wvui_font_glyph_t) == 8,
                  "wvui_font_glyph_t must be 8 bytes: five 8-bit fields + 24-bit offset");

/** In-file font: all tables are const arrays, resident in flash. */
typedef struct
{
    uint8_t bpp;                         /**< 1 / 2 / 4 / 8 */
    uint8_t tracking;                    /**< Tracking: extra px between adjacent glyphs; 0 = native advance */
    uint8_t line_gap;                    /**< Line gap: extra px between adjacent lines; 0 = line height only */
    uint16_t line_height;                /**< Glyph box line height, px (line_gap not included) */
    uint16_t baseline;                   /**< Baseline position from the line top, px */
    uint16_t range_count;                /**< Number of range table entries */
    uint16_t sparse_count;               /**< Sparse code point table entries; 0 = none */
    uint16_t glyph_count;                /**< Number of glyph descriptor entries */
    uint16_t max_box_width;              /**< Widest glyph box width, px */
    uint16_t max_box_height;             /**< Tallest glyph box height, px */
    const wvui_font_range_t *ranges;      /**< Range table, ascending, non-overlapping */
    const uint32_t *sparse;              /**< Sparse code point table, ascending, may be NULL */
    const wvui_font_glyph_t *glyph_desc;  /**< Glyph descriptor table */
    const uint8_t *bitmap_data;          /**< Shared bitmap stream */
} wvui_font_t;

/**
 * @brief Look up a glyph by code point.
 * @param f    Font descriptor; NULL returns NULL
 * @param cp   Code point
 * @return     Glyph descriptor; NULL when the code point is absent
 * @note       The range table is checked first; outside the ranges, binary
 *             search on the sparse table.
 */
const wvui_font_glyph_t *wvui_font_glyph(const wvui_font_t *f, uint32_t cp);

/**
 * @brief Decode one UTF-8 code point.
 * @param p  Read/write cursor; advanced to the next code point on success
 * @return   Code point; an invalid sequence skips 1 byte and returns 0
 * @note     Both font backends share this implementation; code point boundaries
 *           must be byte-identical.
 */
uint32_t wvui_utf8_next(const char **p);

/**
 * @brief Get the line pitch: line-top spacing of adjacent lines.
 * @param f  Font descriptor; NULL returns 0
 * @return   line_height + line_gap, px
 * @note     Layout steps by the pitch, not by the line_height field: the glyph
 *           box height stays line_height, the extra gap sits between lines.
 */
int16_t wvui_font_line_height(const wvui_font_t *f);

/**
 * @brief Measure width and height ('\n' may give multiple lines).
 * @param f     Font descriptor; NULL f or utf8 outputs 0
 * @param utf8  NUL-terminated UTF-8 string, may be NULL
 * @param w     Output width, px, may be NULL
 * @param h     Output height = lines * line_height + (lines - 1) * line_gap, may be NULL
 * @note  Returns immediately when w or h is NULL; neither output is written.
 */
void wvui_font_measure(const wvui_font_t *f, const char *utf8, int16_t *w, int16_t *h);

/**
 * @brief Draw a text at (x, y), writing only pixels inside the current slice.
 * @param f     Font descriptor; NULL, NULL utf8 or opa == 0 draws nothing
 * @param x     Left edge of the first line, screen coordinates
 * @param y     Line top of the first line, screen coordinates
 * @param utf8  UTF-8 string
 * @param fg    Foreground color
 * @param opa   Opacity 0..255
 */
void wvui_font_draw_text(const wvui_font_t *f, int16_t x, int16_t y, const char *utf8,
                        wvui_color_t fg, uint8_t opa);

/**
 * @brief Get the single-line width (part before '\n').
 * @param f     Font descriptor; NULL f or utf8 returns 0
 * @param utf8  UTF-8 string
 * @return      Width, px
 */
int16_t wvui_font_line_width(const wvui_font_t *f, const char *utf8);

/**
 * @brief Measure the layout width of the first nbytes bytes.
 * @param f       Font descriptor; NULL f or utf8, or nbytes == 0 returns 0
 * @param utf8    UTF-8 string
 * @param nbytes  Byte count; beyond the string length it counts up to NUL
 * @return        Width, px; content after '\n' is not counted
 * @note          The argument is a byte count, not code points; the wrap loop
 *                advances by bytes.
 */
int16_t wvui_font_prefix_width(const wvui_font_t *f, const char *utf8, uint16_t nbytes);

/**
 * @brief Draw only the first nbytes bytes; '\n' still wraps, otherwise the same
 *        semantics as wvui_font_draw_text().
 * @param f       Font descriptor; NULL, NULL utf8, opa == 0 or nbytes == 0 draws nothing
 * @param x       Left edge of the first line, screen coordinates
 * @param y       Line top of the first line, screen coordinates
 * @param utf8    UTF-8 string
 * @param nbytes  Byte count; beyond the string length it counts up to NUL
 * @param fg      Foreground color
 * @param opa     Opacity 0..255
 */
void wvui_font_draw_prefix(const wvui_font_t *f, int16_t x, int16_t y, const char *utf8,
                          uint16_t nbytes, wvui_color_t fg, uint8_t opa);

/**
 * @brief Get the line count.
 * @param utf8  UTF-8 string, may be NULL
 * @return      Line count; NULL and empty string give 0, "a\n" gives 2
 * @note        Same line count rules as wvui_font_measure().
 */
uint16_t wvui_font_line_count(const char *utf8);

/**
 * @brief Compare line index of two texts byte by byte.
 * @param index  Line number, starting at 0
 * @return       1 = equal; 0 = different
 * @note         Missing on both sides also counts as equal.
 */
uint8_t wvui_font_line_equal(const char *a, const char *b, uint16_t index);

/**
 * @brief Get the layout rect of line index (for per-line dirty marking, screen
 *        coordinates, endpoints included).
 * @param f        Font descriptor; NULL f or out, or width <= 0 returns 0
 * @param utf8     UTF-8 string, may be NULL
 * @param index    Line number, starting at 0
 * @param x        Block left edge, screen coordinates
 * @param width    Block width; per-line dirty marking only needs sufficient
 *                 horizontal coverage
 * @param line_top Line top of the first line
 * @return         1 = line exists; 0 = line missing or out of int16 range
 * @note           Line top = line_top + index * pitch (line_height + line_gap);
 *                 the rect height stays line_height.
 * @note           Requires x_ofs + box_w <= adv_w and y_ofs + box_h <=
 *                 line_height (validated at generation time), otherwise
 *                 overflowing edges get dropped.
 */
uint8_t wvui_font_line_rect(const wvui_font_t *f, const char *utf8, uint16_t index,
                           int16_t x, int16_t width, int16_t line_top, wvui_rect_t *out);

/** Bitmap glyph backend: a run of image IDs with fixed cells, monospaced. */
typedef struct
{
    wvui_asset_id_t first_id;  /**< Glyph ID for first_char, increasing with character code */
    uint8_t first_char;       /**< Range start, inclusive */
    uint8_t last_char;        /**< Range end, inclusive */
    uint8_t cell_w;           /**< Cell width, px */
    uint8_t cell_h;           /**< Cell height, px */
    uint8_t adv_w;            /**< Advance width, including spacing */
    uint8_t line_height;      /**< Line height, px */
    int8_t y_ofs;             /**< Cell Y offset from the line top */
} wvui_font_strip_t;

/**
 * @brief Get the line height.
 * @param s  Bitmap font descriptor; NULL returns 0
 * @return   Line height, px
 */
uint16_t wvui_font_strip_line_height(const wvui_font_strip_t *s);

/**
 * @brief Measure width and height.
 * @param s     Bitmap font descriptor; NULL s or utf8 outputs 0
 * @param utf8  UTF-8 string
 * @param w     Output width = glyphs in a line * adv_w, may be NULL
 * @param h     Output height = lines * line_height, may be NULL
 */
void wvui_font_strip_measure(const wvui_font_strip_t *s, const char *utf8, int16_t *w, int16_t *h);

/**
 * @brief Draw a text at (x, y) with fixed cells.
 * @param s     Bitmap font descriptor; NULL, NULL utf8 or opa == 0 draws nothing
 * @param x     Left edge of the first line, screen coordinates
 * @param y     Line top of the first line, screen coordinates
 * @param utf8  UTF-8 string
 * @param fg    Foreground color (glyphs are usually A8 / mask images)
 * @param opa   Opacity 0..255
 * @note         Each glyph is fetched via wvui_asset_open(); a missing asset
 *               skips that glyph.
 * @note         Every cell is equally wide; only [first_char, last_char] has
 *               glyphs, characters outside are skipped without advancing.
 */
void wvui_font_strip_draw(const wvui_font_strip_t *s, int16_t x, int16_t y, const char *utf8,
                         wvui_color_t fg, uint8_t opa);

#endif /* WVUI_FONT_H */
