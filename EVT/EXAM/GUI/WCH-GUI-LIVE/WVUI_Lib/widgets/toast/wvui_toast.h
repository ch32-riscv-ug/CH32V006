#ifndef WVUI_TOAST_H
#define WVUI_TOAST_H

/**
 * @file wvui_toast.h
 * @brief Floating toast strip: attaches to the top layer list, counts frames, and detaches itself when expired.
 * @note  Non-modal and takes no hit test (clicks still reach the layer below); the object must be allocated in persistent memory.
 */

#include "wvui_anim.h"
#include "wvui_font.h"
#include "wvui_widget.h"

typedef struct
{
    wvui_anim_t an;       /**< Lifetime timer (embedded) */
    const char *text;    /**< Text, owned by the caller */
    const wvui_font_t *font; /**< Font for drawing; 0 = draw no text */
    wvui_color_t bg;      /**< Background color */
    wvui_color_t fg;      /**< Text color */
    wvui_color_t border;  /**< Border color */
    uint16_t left;       /**< Frames remaining; 0 = expired */
    uint16_t total;      /**< Total frames of this run, used to compute the fade-out ratio */
    uint8_t radius;      /**< Corner radius in pixels; default 6 */
    uint8_t opa;         /**< Opacity 0..255; default 255 */
    uint8_t active;      /**< 1 = showing; 0 = dismissed */
} wvui_toast_rt_t;

extern const wvui_class_t wvui_toast_class;

/**
 * @brief Show a toast strip: attach to the top layer list and run for frames frames.
 * @param text    Text; set a font first, otherwise only the fill and border are drawn
 * @param frames  Lifetime in frames; 0 = use WVUI_CFG_TOAST_FRAMES (90 frames)
 * @note  Fades out linearly over the last WVUI_CFG_TOAST_FADE (20) frames by the frames remaining.
 * @note  Only restarts timing and attaching: no allocation, the object is reusable.
 */
void wvui_toast_show(wvui_widget_t *w, const char *text, uint16_t frames);

/**
 * @brief Dismiss at once: stop timing, detach, and redraw.
 * @note  The object is not deleted and stays reusable; show() may be called again.
 */
void wvui_toast_hide(wvui_widget_t *w);

/**
 * @brief Set the font for drawing.
 * @param font  Font; 0 = draw no text, keep only the fill and border
 * @note  There is no default font for the text, so it must be set explicitly.
 */
void wvui_toast_set_font(wvui_widget_t *w, const wvui_font_t *font);

/**
 * @brief Set the background, text, and border colors.
 * @param bg      Background color
 * @param fg      Text color
 * @param border  Border color, 1 pixel wide
 */
void wvui_toast_set_colors(wvui_widget_t *w, wvui_color_t bg, wvui_color_t fg,
                          wvui_color_t border);

/**
 * @brief Set the corner radius.
 * @param radius  Corner radius in pixels; default 6, 0 = square corners
 * @note  Clamps to min(w, h) / 2 while drawing.
 */
void wvui_toast_set_radius(wvui_widget_t *w, uint8_t radius);

/**
 * @brief Set the opacity ceiling.
 * @param opa  Widget opacity 0..255
 * @note  Scales down again by the frames remaining during the fade-out, so the actual value never exceeds opa.
 */
void wvui_toast_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Read back whether the toast is showing.
 * @return 1 = showing; 0 = dismissed or wrong type
 */
uint8_t wvui_toast_is_active(const wvui_widget_t *w);

/**
 * @brief Read back the frames remaining.
 * @return Lifetime frames remaining, 0 = expired; 0 on wrong type
 */
uint16_t wvui_toast_left(const wvui_widget_t *w);

#endif /* WVUI_TOAST_H */
