#ifndef WVUI_SCROLL_H
#define WVUI_SCROLL_H

/**
 * @file wvui_scroll.h
 * @brief Single-axis scroll physics: a pure state machine, no drawing, no dirty
 *        marking, no touching the object tree.
 * @note  Single implementation: containers must not add their own inertia /
 *        rebound; velocity is in px per frame.
 */

#include <stdint.h>

#include "wvui_config.h"

/** Scroll state: pos and max share one coordinate system; pos = 0 aligns the content top. */
typedef struct
{
    int32_t pos;        /**< Current scroll position */
    int32_t max;        /**< Max = content length - viewport length; negative counts as 0 */
    int32_t origin_pos; /**< pos at press (reference) */
    int32_t last_pos;   /**< pos after the last delta was taken */

    int16_t origin_c; /**< Main-axis coordinate at press */
    int16_t vel;      /**< Velocity, px per frame */

    uint8_t tracking;  /**< This press sequence is being tracked */
    uint8_t dragging;  /**< Drag threshold has been crossed */
    uint8_t animating; /**< Inertia / rebound in progress */
} wvui_scroll_t;

/**
 * @brief Initialize and bind the max; call when the content changes.
 * @param max  Max, px; < 0 counts as 0 (not scrollable)
 */
void wvui_scroll_init(wvui_scroll_t *s, int32_t max);

/**
 * @brief Update the max; call when content or viewport changes. pos is clamped
 *        into range, without animation.
 * @param max  New max, px; < 0 counts as 0
 */
void wvui_scroll_set_range(wvui_scroll_t *s, int32_t max);

/**
 * @brief Feed a press: stop inertia and start tracking this press sequence.
 * @param c  Main-axis touch coordinate of the press point; c rising makes pos fall
 */
void wvui_scroll_press(wvui_scroll_t *s, int16_t c);

/**
 * @brief Feed STAY (held and moving); the motion is taken later with
 *        wvui_scroll_take_delta().
 * @param c  Current main-axis touch coordinate
 * @return 0 = drag threshold not reached (this press still belongs to the child
 *         widget); 2 = dragging, whether or not pos changed
 */
uint8_t wvui_scroll_stay(wvui_scroll_t *s, int16_t c);

/**
 * @brief Take the pos change since the last call; once per frame while dragging
 *        or after an inertia step.
 * @return Motion delta, px; includes finger tracking and overscroll damping,
 *         clamped to int16 range
 */
int32_t wvui_scroll_take_delta(wvui_scroll_t *s);

/**
 * @brief Feed a release: end tracking and decide whether an anim node is needed;
 *        no clamping, velocity is kept.
 * @return 1 = start a node (overscroll rebound, or inertia with velocity);
 *         0 = nothing to do
 * @note  Clamping and deceleration happen in wvui_scroll_step().
 */
uint8_t wvui_scroll_release(wvui_scroll_t *s);

/**
 * @brief Set the initial velocity from a quick swipe: a pass without STAY, the
 *        motion is the velocity; call only when the whole sequence had no STAY.
 * @param c  Main-axis coordinate of the release point, same coordinate system as
 *        wvui_scroll_press()
 * @return 1 = velocity and animating set (caller starts the node); 0 = not
 *         scrollable or zero motion
 */
uint8_t wvui_scroll_swipe(wvui_scroll_t *s, int16_t c);

/**
 * @brief Inertia / rebound step, called once per frame by the animation engine.
 * @return 1 = pos changed (caller commits and marks dirty); 0 = at rest,
 *         animating has been cleared
 */
uint8_t wvui_scroll_step(wvui_scroll_t *s);

/**
 * @brief Programmatic positioning, without animation.
 * @param pos  Target position, clamped to [0, max]
 * @return 1 = pos changed
 */
uint8_t wvui_scroll_set(wvui_scroll_t *s, int32_t pos);

/**
 * @brief Tell whether scrolling is in motion.
 * @return 1 = inertia / rebound in progress or being dragged; 0 = at rest
 */
uint8_t wvui_scroll_active(const wvui_scroll_t *s);

/**
 * @brief Normalized 0..255 progress, for drawing the scrollbar.
 * @return 0..255; max <= 0 returns 0
 * @note  Contains one division; call only when drawing the scrollbar.
 */
uint8_t wvui_scroll_progress(const wvui_scroll_t *s);

#endif /* WVUI_SCROLL_H */
