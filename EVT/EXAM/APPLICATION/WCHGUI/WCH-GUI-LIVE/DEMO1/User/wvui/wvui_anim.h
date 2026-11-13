#ifndef WVUI_ANIM_H
#define WVUI_ANIM_H

/**
 * @file wvui_anim.h
 * @brief Central animation engine: per-frame callback chain plus fixed-frame tweens.
 * @note  With WVUI_CFG_ENABLE_ANIM == 0 nodes have no next / linked; transitions run synchronously to completion at the call site, permanent animations do not start.
 */

#include <stdint.h>

#include "wvui_config.h"
#include "wvui_widget.h" /* wvui_widget_t (owner) */

typedef struct wvui_anim_s wvui_anim_t;

/**
 * @brief Per-frame callback.
 * @param a  The node itself
 * @return  1 = keep going, called again next frame; 0 = done, detached by the engine
 * @note    May only detach itself (wvui_anim_stop(a) or return 0); detaching another node truncates this frame's traversal.
 * @note    Do not delete widgets (including owner) inside the callback; set a flag and defer destruction to the next frame.
 */
typedef uint8_t (*wvui_anim_step_fn)(wvui_anim_t *a);

/** Animation node: embedded in widget runtime, position is arbitrary. */
struct wvui_anim_s
{
#if WVUI_CFG_ENABLE_ANIM
    struct wvui_anim_s *next;  /**< Next node in the chain; valid only while linked != 0 */
    wvui_widget_t *owner;      /**< Reclaimed by owner, may be NULL */
    wvui_anim_step_fn step;    /**< Per-frame callback, NULL means done */
    uint8_t linked;           /**< 1 = linked into the chain */
#else
    wvui_widget_t *owner;    /**< Used by the callback to fetch the widget */
    wvui_anim_step_fn step;  /**< Callback kept even with animations off */
#endif
};

/**
 * @brief Initialize a node; each widget calls this once in its own init.
 * @note  An uninitialized node cannot be started.
 */
void wvui_anim_init(wvui_anim_t *a);

/**
 * @brief Start or refresh an animation node, idempotent and never fails.
 * @param owner  Owning widget, may be NULL
 * @param step   Per-frame callback, NULL means done
 * @note  If already linked only owner / step are refreshed; otherwise the node joins the active chain (at the head) and starts stepping next frame.
 * @note  With owner == NULL wvui_anim_forget_widget() cannot detach it; detach by address with wvui_anim_drop_range() instead.
 */
void wvui_anim_start(wvui_anim_t *a, wvui_widget_t *owner, wvui_anim_step_fn step);

/**
 * @brief Start a permanent animation (blink / marquee / auto carousel / toast lifetime / fade-out).
 * @param owner  Owning widget, may be NULL
 * @note  Not started when ENABLE_ANIM == 0: permanent animations have no end condition, so the static state is decided by the widget's initial value.
 * @note  Transitions are detached when the callback returns 0; permanent ones detach when the widget sets values (making the callback return 0) or deletes the widget.
 */
#if WVUI_CFG_ENABLE_ANIM
static inline void wvui_anim_start_permanent(wvui_anim_t *a, wvui_widget_t *owner,
                                            wvui_anim_step_fn step)
{
    wvui_anim_start(a, owner, step);
}
#else
void wvui_anim_start_permanent(wvui_anim_t *a, wvui_widget_t *owner, wvui_anim_step_fn step);
#endif

/**
 * @brief Detach a node; idempotent, no-op when not linked.
 */
void wvui_anim_stop(wvui_anim_t *a);

/**
 * @brief Query whether a node is on the active chain; always 0 with ENABLE_ANIM == 0.
 * @param a  Node, may be NULL
 * @return 1 = on the chain; 0 = not
 */
static inline uint8_t wvui_anim_running(const wvui_anim_t *a)
{
#if WVUI_CFG_ENABLE_ANIM
    return (uint8_t)((a != 0) ? a->linked : 0u);
#else
    (void)a;
    return 0u; /* this build never joins the active chain */
#endif
}

/**
 * @brief Step all nodes, called once per frame by wvui_frame.
 */
void wvui_anim_step_all(void);

/** Fixed-frame tween state: one embedded per widget, advanced by wvui_ramp_*(). */
typedef struct
{
    uint16_t inc;   /**< Per-frame increment (Q8) */
    uint16_t left;  /**< Remaining frames */
    uint16_t acc;   /**< Q8 accumulator, pos = acc >> 8 */
    uint8_t pos;    /**< Progress 0..255 */
    uint8_t done;   /**< 1 = target value reached */
} wvui_ramp_t;

/** Per-frame increment; frames must be a compile-time constant, 0 yields 0. */
#define WVUI_RAMP_INC(frames) \
    ((uint16_t)(((frames) != 0) ? (((uint32_t)255u * 256u) / (uint32_t)(frames)) : 0u))

/**
 * @brief Start a tween: call wvui_ramp_start() first, then wvui_anim_start() to start the node.
 * @param inc     Per-frame increment, usually WVUI_RAMP_INC(frames)
 * @param frames  Total frame count; 0 = complete at once with pos = 255
 */
void wvui_ramp_start(wvui_ramp_t *r, uint16_t inc, uint16_t frames);

/**
 * @brief Advance one frame.
 * @return 1 = another frame is needed; 0 = done, pos already holds the target value this frame
 * @note  Idempotent: always returns 0 once done; on the frame returning 0 the caller must still redraw as usual.
 */
uint8_t wvui_ramp_step(wvui_ramp_t *r);

/**
 * @brief Jump to the target value, used when cancelling an animation.
 * @note  The next wvui_ramp_step() then returns 0 and pos is already 255.
 */
void wvui_ramp_finish(wvui_ramp_t *r);

/**
 * @brief Detach all nodes with owner == w, called by wvui_kernel_forget_widget().
 * @param w  The widget being deleted
 * @note  Even a missed wvui_anim_stop() leaves no node pointing to the deleted widget.
 */
void wvui_anim_forget_widget(wvui_widget_t *w);

/**
 * @brief Detach all nodes whose address falls in [lo, hi).
 * @param lo  Lower bound, inclusive
 * @param hi  Upper bound, exclusive
 * @note  Nodes are embedded in the arena; after the arena cursor resets the chain still points at them and the next step would call through dangling pointers.
 * @note  Called by wvui_transient_reset() before the arena cursor resets.
 */
void wvui_anim_drop_range(const void *lo, const void *hi);

/**
 * @brief Number of currently active nodes (chain length); O(chain length), call only from stats and diagnostics paths.
 */
uint16_t wvui_anim_active(void);

/**
 * @brief Cumulative step count: frames stepped since start.
 */
uint32_t wvui_anim_steps(void);

/**
 * @brief Clear the counters only, leave the chain alone; called by wvui_stats_reset().
 */
void wvui_anim_stats_reset(void);

#endif /* WVUI_ANIM_H */
