#ifndef WVUI_MSGBOX_H
#define WVUI_MSGBOX_H

/**
 * @file wvui_msgbox.h
 * @brief Modal dialog: title, body text and one or two buttons.
 * @note  On open it joins the top layer chain and turns modal; taps outside the modal do not pass through to widgets below.
 */

#include "wvui_font.h"
#include "wvui_widget.h"

/** Button callback: called only after the dialog was closed. */
typedef void (*wvui_msgbox_cb_t)(wvui_widget_t *w, void *user);

typedef struct
{
    const char *title;             /**< Title pointer, not copied */
    const char *message;           /**< Body text pointer, not copied */
    const char *ok_text;           /**< OK button text; NULL = "OK" */
    const char *cancel_text;       /**< Cancel button text; NULL = "CANCEL" */
    const wvui_font_t *font;        /**< Font; NULL = draw no text */
    wvui_color_t bg;                /**< Dialog background, default dark gray */
    wvui_color_t fg;                /**< Body text color, default light gray */
    wvui_color_t border;            /**< Border color, also used as the title color */
    wvui_color_t btn_bg;            /**< Button idle background, default gray */
    wvui_color_t btn_fg;            /**< Button text color, default white */
    wvui_color_t btn_pressed;       /**< Button pressed background, default blue */
    uint8_t two_buttons;           /**< 1 = two buttons (default), 0 = single button */
    uint8_t pressed;               /**< 0 none, 1 cancel, 2 OK */
    uint8_t radius;                /**< Corner radius in pixels, default 8 */
    uint8_t opa;                   /**< Opacity 0..255, default 255 */
    wvui_msgbox_cb_t on_ok;         /**< OK button callback */
    wvui_msgbox_cb_t on_cancel;     /**< Cancel button callback */
    void *user;                    /**< User pointer passed to the callbacks */
} wvui_msgbox_rt_t;

extern const wvui_class_t wvui_msgbox_class;

/**
 * @brief Open the dialog: join the top layer chain and turn modal.
 * @note  The object must be allocated in persistent memory; the top layer chain survives page switches.
 * @note  After opening it intercepts all gestures; tapping outside the button row does not close it.
 */
void wvui_msgbox_open(wvui_widget_t *w);

/**
 * @brief Close the dialog: clear the modal first, then detach.
 * @note  The object is not destroyed and can be reused with another open.
 */
void wvui_msgbox_close(wvui_widget_t *w);

/**
 * @brief Set the title and body text.
 * @param title    Title; NULL = not drawn
 * @param message  Body text; NULL = not drawn
 * @note  Only stores pointers, no copy; the caller must keep the strings alive.
 * @note  Only marks dirty, does not change the open state.
 */
void wvui_msgbox_set_text(wvui_widget_t *w, const char *title, const char *message);

/**
 * @brief Set the text of both buttons.
 * @param ok_text      OK button text; NULL = "OK"
 * @param cancel_text  Cancel button text; NULL = "CANCEL"
 * @note  In single-button mode only ok_text is used.
 */
void wvui_msgbox_set_button_text(wvui_widget_t *w, const char *ok_text,
                                const char *cancel_text);

/**
 * @brief Set the number of buttons.
 * @param two_buttons  1 = two buttons left/right (default), 0 = one full-row button
 * @note  With two buttons the left half is cancel, the right half is OK.
 */
void wvui_msgbox_set_buttons(wvui_widget_t *w, uint8_t two_buttons);

/**
 * @brief Set the font.
 * @param font  Font; NULL = draw no text
 * @note  Defaults to NULL; title, body text and button text are all drawn with it.
 */
void wvui_msgbox_set_font(wvui_widget_t *w, const wvui_font_t *font);

/**
 * @brief Set the background, body text and border colors.
 * @param bg      Dialog background, default dark gray
 * @param fg      Body text color, default light gray
 * @param border  Border color, also used as the title color
 */
void wvui_msgbox_set_colors(wvui_widget_t *w, wvui_color_t bg, wvui_color_t fg,
                           wvui_color_t border);

/**
 * @brief Set the three button colors.
 * @param bg       Button idle background, default gray
 * @param fg       Button text color, default white
 * @param pressed  Background while pressed, default blue
 */
void wvui_msgbox_set_button_colors(wvui_widget_t *w, wvui_color_t bg, wvui_color_t fg,
                                  wvui_color_t pressed);

/**
 * @brief Set the dialog corner radius.
 * @param radius  Radius in pixels, default 8; 0 = square corners
 * @note  Radius beyond min(w,h)/2 is clamped by the draw layer.
 */
void wvui_msgbox_set_radius(wvui_widget_t *w, uint8_t radius);

/**
 * @brief Set the overall opacity.
 * @param opa  0..255, default 255; 0 = not drawn
 */
void wvui_msgbox_set_opa(wvui_widget_t *w, uint8_t opa);

/**
 * @brief Set the callbacks of both buttons.
 * @param on_ok      OK button callback; NULL = no callback
 * @param on_cancel  Cancel button callback; NULL = no callback
 * @param user       Pointer passed through unchanged to the callbacks
 * @note  Closes first, then calls back; the callback must not delete itself, set a flag and delete on the next frame.
 */
void wvui_msgbox_set_cb(wvui_widget_t *w, wvui_msgbox_cb_t on_ok, wvui_msgbox_cb_t on_cancel,
                       void *user);

/**
 * @brief Read back whether it is open.
 * @return 1 = this widget is the current modal; 0 = closed or the type does not match
 */
uint8_t wvui_msgbox_is_open(const wvui_widget_t *w);

#endif /* WVUI_MSGBOX_H */
