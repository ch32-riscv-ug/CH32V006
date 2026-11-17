#ifndef WVUI_SCROLL_PANEL_H
#define WVUI_SCROLL_PANEL_H

/**
 * @file wvui_scroll_panel.h
 * @brief Single-axis scroll panel: vertical scrolling only, content moved child by child.
 * @note  Presses on blank areas are captured too; when the content is scrollable DRAG_BEGIN takes over
 *        the gesture and children never receive CLICKED.
 */

#include "wvui_anim.h"
#include "wvui_scroll.h"
#include "wvui_widget.h"

typedef struct
{
    wvui_widget_t *children_head; /**< Must be the first field (container convention) */
    wvui_scroll_t sc;             /**< Press/follow/overscroll damping; also records the flick's initial velocity */
    wvui_anim_t an;               /**< Inertia/rebound driver, advanced every frame, removed when settled */
#if WVUI_CFG_BAR_FADE_FRAMES > 0
    /** Scrollbar fade triad: compiled out entirely when WVUI_CFG_BAR_FADE_FRAMES == 0. */
    wvui_anim_t an_bar;   /**< Scrollbar fade node */
    wvui_ramp_t bar_ramp; /**< Fade progress: opa = 255 − pos */
    uint16_t bar_idle;   /**< Idle frame counter, fade-out starts once WVUI_CFG_BAR_FADE_DELAY is reached */
#endif

    int16_t content_h;  /**< Total content height in pixels, computed on refresh */
    int16_t bar_w;      /**< Scrollbar width in pixels; 0 = not drawn */
    int16_t bar_y;      /**< Thumb y relative to the viewport top, computed every frame */
    int16_t bar_h;      /**< Thumb height = viewport height² / content height, clamped to [8, viewport height] */
    int16_t bar_y_max;  /**< Max thumb y = viewport height − thumb height */
    uint32_t bar_q16;   /**< Q16 reciprocal for position conversion, computed on refresh */
    uint8_t bar_show;   /**< 1 = draw the scrollbar when scrollable */
} wvui_scroll_panel_rt_t;

extern const wvui_class_t wvui_scroll_panel_class;

/**
 * @brief Recompute the content height and scroll range, clamping an overscrolled position back.
 * @note  Must be called after children are added, removed or moved; updates geometry only, marks nothing dirty.
 * @note  Range = [0, content height − viewport height]; negatives are treated as 0, i.e. not scrollable.
 */
void wvui_scroll_panel_refresh(wvui_widget_t *w);

/**
 * @brief Scroll programmatically to a position, landing at once and leaving no inertia.
 * @param pos  Target position in pixels; 0 = top, clamped to [0, max scroll]
 * @note  Inertia/rebound is stopped before positioning, otherwise the next frame pulls it back onto a converging path.
 */
void wvui_scroll_panel_set_scroll(wvui_widget_t *w, int32_t pos);

/**
 * @brief Read back the current scroll position.
 * @return Scroll position in pixels, 0 = top; returns 0 on a type mismatch
 */
int32_t wvui_scroll_panel_scroll(const wvui_widget_t *w);

#endif /* WVUI_SCROLL_PANEL_H */
