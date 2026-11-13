#ifndef WVUI_DIRTY_H
#define WVUI_DIRTY_H

/**
 * @file wvui_dirty.h
 * @brief Dirty rectangle management: invalidation, cost-based merge, per-frame retrieval.
 * @note  Strategy selected at compile time: 0 = one full-screen block, 1 = a single bounding box, 2 = multiple rectangles with cost-based merge (default).
 */

#include <stdint.h>
#include "wvui_config.h"
#include "wvui_types.h"
#include "wvui_port.h"

/**
 * @brief Bind the screen size and the port alignment granularity; called once by wvui_init().
 * @param screen_w  Screen width, pixels
 * @param screen_h  Screen height, pixels
 * @param port      Port; NULL = no alignment expansion
 */
void wvui_dirty_bind(int16_t screen_w, int16_t screen_h, const wvui_port_t *port);

/**
 * @brief Clear the dirty area list and the per-frame statistics; call after the frame's dirty areas have been consumed.
 */
void wvui_dirty_reset(void);

/**
 * @brief Invalidate a rectangle.
 * @param r  Dirty rectangle, screen coordinates, inclusive; parts out of bounds are clipped off
 * @note  On insertion: screen clipping, alignment expansion, then cost-based merge, in that order; expansion must not be deferred to the flush.
 */
void wvui_dirty_add(wvui_rect_t r);

/**
 * @brief Set the cost model: equivalent pixels saved by avoiding one transaction.
 * @param px  Equivalent pixels; 0 = no merge
 * @note  Merging happens iff the extra merged area is < px; the default is derived by wvui_init() from port capabilities.
 */
void wvui_dirty_set_fixed_cost_pixels(uint32_t px);

/**
 * @brief Read the current cost model parameter.
 * @return Equivalent pixels saved by avoiding one transaction
 */
uint32_t wvui_dirty_fixed_cost_pixels(void);

/**
 * @brief Get the number of dirty areas in this frame.
 * @return 0..WVUI_CFG_DIRTY_MAX_NUM
 */
uint8_t wvui_dirty_count(void);

/**
 * @brief Get the index-th dirty area.
 * @param index  Index, must be < wvui_dirty_count()
 * @return       Dirty rectangle, screen coordinates, inclusive; an empty rectangle (x1 < x0) if index is out of range
 */
wvui_rect_t wvui_dirty_get(uint8_t index);

/**
 * @brief Get the total dirty area of this frame.
 * @return Area, pixels; the value after alignment expansion
 */
uint32_t wvui_dirty_total_area(void);

/**
 * @brief Get the number of cost-based merges in this frame.
 * @return Number of merges
 */
uint32_t wvui_dirty_merges(void);

/**
 * @brief Get the number of forced merges (slots full) in this frame.
 * @return Number of merges; counted by different rules than wvui_dirty_merges(), do not sum them
 */
uint32_t wvui_dirty_forced_merges(void);

/* Geometry helpers (area/containment/intersection/union/clipping) are inline in wvui_types.h, sharing the inclusive-endpoint rule. */

#endif /* WVUI_DIRTY_H */
