#ifndef WVUI_SLICE_H
#define WVUI_SLICE_H

/**
 * @file wvui_slice.h
 * @brief Render slice query: widgets decode only visible rows, RAM stays independent of screen height.
 */

#include <stdint.h>
#include "wvui_config.h"

/**
 * @brief Screen coordinate range of the current slice, end points inclusive, same rules as wvui_rect_t.
 * @note  Fields pair per axis (x0,x1,y0,y1): same size as wvui_rect_t but not interchangeable.
 */
typedef struct
{
    int16_t x0;
    int16_t x1;  /**< Right edge; x0 > x1 means empty */
    int16_t y0;
    int16_t y1;  /**< Bottom edge; y0 > y1 means empty */
} wvui_slice_t;

/**
 * @brief Get the slice width.
 * @return Width = x1 - x0 + 1
 */
static inline int16_t wvui_slice_w(const wvui_slice_t *s) { return (int16_t)(s->x1 - s->x0 + 1); }
/**
 * @brief Get the slice height.
 * @return Height = y1 - y0 + 1
 */
static inline int16_t wvui_slice_h(const wvui_slice_t *s) { return (int16_t)(s->y1 - s->y0 + 1); }

/**
 * @brief Get the current render slice, never returns NULL.
 * @return Read-only slice pointer; owned by the render layer, the caller must not free or keep it
 * @note  Represents the current block only inside wvui_class_t.draw; outside draw it returns a full-screen slice, a conservative upper bound.
 * @note  The x axis may be split again; do not assume x0 == 0 or x1 == screen width - 1; draw is a pure function,
 *        it never marks dirty, never touches the object tree, never depends on call count; call it once per slice.
 */
const wvui_slice_t *wvui_draw_slice(void);

WVUI_STATIC_ASSERT(sizeof(wvui_slice_t) == 8, "wvui_slice_t is 4x int16, end-points inclusive");

#endif /* WVUI_SLICE_H */
