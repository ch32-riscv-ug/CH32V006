#ifndef WVUI_INPUT_H
#define WVUI_INPUT_H

/**
 * @file wvui_input.h
 * @brief Input layer: builds press, hold, release, drag and swipe events from port samples.
 * @note  The port only reports {x, y, pressed}; time counts in frames, no millisecond channel.
 */

#include <stdint.h>
#include "wvui_config.h"
#include "wvui_input_port.h"
#include "wvui_widget.h"

/** Input-side counters: last_pressed_id / last_grab_id of 0 means no such object. */
typedef struct
{
    uint32_t presses;         /**< Press captures, including failed ones */
    uint32_t bubble_depth;    /**< Total bubble depth */
    uint32_t drag_asks;       /**< DRAG_BEGIN ask count */
    uint32_t drag_grabs;      /**< Gesture takeovers */
    uint32_t swipes;          /**< Synthesized swipe count */
    uint16_t last_pressed_id; /**< id of the latest captured object */
    uint16_t last_grab_id;    /**< id of the latest grabber */
    uint16_t last_hit_id;     /**< id of the object hit on press */
} wvui_input_stats_t;

/**
 * @brief Get a pointer to the input-side counters.
 * @return Pointer to the internal counters; clear with wvui_input_stats_reset()
 */
const wvui_input_stats_t *wvui_input_stats(void);

/**
 * @brief Clear the counters; keep the press and grab states unchanged.
 * @note  wvui_stats_reset() calls this function.
 */
void wvui_input_stats_reset(void);

/**
 * @brief Clear the press and grab states.
 * @note  No event is dispatched; called on page switch.
 */
void wvui_input_reset(void);

/**
 * @brief Feed one port sample per frame; events are synthesized internally.
 * @param in  Sample of this frame, see wvui_indev_t for the fields
 * @note  When the drag threshold is crossed, DRAG_BEGIN is asked once along the ancestor chain; long press is left to widgets from the first STAY on.
 * @note  Order: hit -> press capture (the first non-zero return is the holder) -> gesture takeover -> swipe synthesis.
 */
void wvui_input_feed(const wvui_indev_t *in);

/**
 * @brief Get the current press-captured object.
 * @return Holder, or the grabber after takeover
 * @note   Cleared by wvui_input_forget() when the object is deleted.
 */
wvui_widget_t *wvui_input_pressed(void);

/**
 * @brief Get the current gesture grabber.
 * @return Grabber; NULL = no takeover
 */
wvui_widget_t *wvui_input_grab(void);

/**
 * @brief Take over the gesture explicitly.
 * @param w  Target widget; returns 0 on NULL
 * @return   1 = takeover succeeded; 0 = invalid argument or already the grabber
 * @note     A container can take over by returning 1 from the DRAG_BEGIN callback; widgets may also call this directly in their own logic.
 * @note     On takeover the original pressed object gets one RELEASED and no CLICKED; the grabber gets one PRESSED
 *           with the press start point as coordinates.
 */
uint8_t wvui_indev_grab(wvui_widget_t *w);

/** @brief Clear pressed / grab references when a widget is deleted. */
void wvui_input_forget(wvui_widget_t *w);

/**
 * @brief Hit test: drill down to the deepest hit object.
 * @param x  Screen coordinate, inclusive
 * @param y  Screen coordinate, inclusive
 * @return   Deepest hit object; NULL = no hit
 * @note     Recursive drill-down + ancestor visible-area clip + deepest first + z-order within a layer + container veto + hotspot + top layer and modal first.
 * @note     With a modal active, only the modal subtree is tested; a miss on everything counts as hitting the modal object itself (full screen).
 */
wvui_widget_t *wvui_input_hit_test(int16_t x, int16_t y);

#endif /* WVUI_INPUT_H */
