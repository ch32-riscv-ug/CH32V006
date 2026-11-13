#ifndef WVUI_WIDGETS_H
#define WVUI_WIDGETS_H

/**
 * @file wvui_widgets.h
 * @brief Built-in widget type numbers and class-table assembly.
 * @note  A type number is a class table index and never changes once published; 0 = unbound/invalid.
 * @note  The first parameter w of a widget API is the widget itself: no-op when NULL or of a
 *        mismatched type (get returns 0), so @param lists only the remaining parameters.
 */

#include "wvui_widget.h"

/** Type numbers start at 1; new ones continue downward, old numbers are never reused. */
#define WVUI_TYPE_NONE (0u)
#define WVUI_TYPE_LABEL (1u)
#define WVUI_TYPE_BTN (2u)
#define WVUI_TYPE_SCROLL_PANEL (3u) /**< Single-axis scrolling container */
#define WVUI_TYPE_IMG (4u)          /**< Image, resource read streamingly */
#define WVUI_TYPE_LINE (5u)         /**< Anti-aliased line / separator */
#define WVUI_TYPE_PROGRESS (6u)     /**< Progress bar */
#define WVUI_TYPE_TOGGLE (7u)       /**< Switch with animation */
#define WVUI_TYPE_CHECKBOX (8u)     /**< Check box with entrance animation */
#define WVUI_TYPE_INDICATOR (9u)    /**< Status indicator dot */
#define WVUI_TYPE_BOX (10u)         /**< Box with four independently styled corners */
#define WVUI_TYPE_ARC (11u)         /**< Arc / ring sector */
#define WVUI_TYPE_SLIDER (12u)      /**< Slider, drag type */
#define WVUI_TYPE_GAUGE (13u)       /**< Gauge, two arc segments + needle */
#define WVUI_TYPE_CHART (14u)       /**< Line chart, 1 pixel per sample */
#define WVUI_TYPE_STEPPER (15u)     /**< Stepper, fixed-point int32 */
#define WVUI_TYPE_LIST (16u)        /**< Text list with self-drawn rows */
#define WVUI_TYPE_ROLLER (17u)      /**< Roller selector, snaps on release */
#define WVUI_TYPE_TOAST (18u)       /**< Floating hint, non-modal */
#define WVUI_TYPE_MSGBOX (19u)      /**< Modal dialog */
#define WVUI_TYPE_DROPDOWN (20u)    /**< Dropdown selection, collapses on outside tap */
#define WVUI_TYPE_SEGDISP (21u)     /**< Seven-segment display */
#define WVUI_TYPE_MARQUEE (22u)     /**< Marquee */
#define WVUI_TYPE_LABEL_EX (23u)    /**< Word-wrapping label */
#define WVUI_TYPE_GROUP (24u)       /**< Group container */
#define WVUI_TYPE_SLIDESHOW (25u)   /**< Automatic carousel */
/* Number 26 stays empty: type numbers are not reused. */
#define WVUI_TYPE_BARCHART (27u)    /**< Bar chart */
#define WVUI_TYPE_SEGBAR (28u)      /**< Segment bar / endless scrolling strip */
#define WVUI_TYPE_MONOIMG (29u)     /**< 1bpp monochrome bitmap */

/**
 * @brief Largest type number, the upper bound of compile-time guards; move it up when adding widgets.
 * @note  Must be < WVUI_CFG_CLASS_MAX: beyond it the class table has no slot and wvui_widget_create() returns NULL.
 */
#define WVUI_TYPE_MAX WVUI_TYPE_MONOIMG
#if (WVUI_TYPE_MAX >= WVUI_CFG_CLASS_MAX)
#error "WVUI_TYPE_MAX >= WVUI_CFG_CLASS_MAX: raise WVUI_CFG_CLASS_MAX in wvui_config.h"
#endif

/**
 * @brief Report the slot count of the built-in class table now ready (highest non-empty index + 1).
 * @return  slot count, widgets trimmed at the tail are not counted; 0 when all are trimmed
 * @note  Trimming only toggles WVUI_CFG_WIDGET_* (or -D WVUI_WIDGETS=...): for a disabled type
 *        wvui_class_get() returns 0 and wvui_widget_create() returns NULL (same as arena
 *        exhaustion, no error), and its .c does not enter the build.
 * @note  Compatibility entry: the class table is filled at compile time, this function does no
 *        binding; custom widgets go through wvui_class_bind().
 */
uint16_t wvui_widgets_register_builtin(void);

#endif /* WVUI_WIDGETS_H */
