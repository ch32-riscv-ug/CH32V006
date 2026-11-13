#ifndef WVUI_ARENA_H
#define WVUI_ARENA_H

/**
 * @file wvui_arena.h
 * @brief Dual-region bump arena: permanent region + transient region, page lifecycle rides on it.
 * @note  No per-object free: memory is reclaimed only on reset, leftovers still count into peak.
 */

#include <stdint.h>
#include "wvui_config.h"

/** Alignment granularity: sizeof(void*), guarantees room for a pointer. */
#define WVUI_ARENA_ALIGN ((uint32_t)sizeof(void *))

/**
 * @brief A caller-provided byte region: bump allocation, reclaimed as a whole on reset.
 * @note  Transient region size > 0 means a stateless page; only a permanent region means a permanent page.
 */
typedef struct
{
    uint8_t *base;    /**< Start of the byte region */
    uint32_t size;    /**< Size of the byte region */
    uint32_t cursor;  /**< Forward-only bump cursor */
    uint32_t peak;    /**< Historical high-water mark, kept across reset */
    uint32_t fails;   /**< Allocation failure count, non-zero means a wrong budget */
} wvui_arena_t;

/**
 * @brief Bind a block of static memory to the arena; re-init clears the cursor and peak / fails.
 * @param a           arena to initialize
 * @param mem         byte region; must be aligned to WVUI_ARENA_ALIGN
 * @param size_bytes  byte count; treated as 0 when mem is 0
 */
void wvui_arena_init(wvui_arena_t *a, void *mem, uint32_t size_bytes);

/**
 * @brief Bump out a block from the cursor, returns an aligned pointer.
 * @param nbytes  requested byte count
 * @return        Pointer aligned to WVUI_ARENA_ALIGN; NULL when out of space
 * @note          On failure only fails is incremented, no bytes written, no out-of-bounds access; segments keep at most align-1 padding bytes.
 */
void *wvui_arena_alloc(wvui_arena_t *a, uint32_t nbytes);

/**
 * @brief Reset the cursor to zero, reclaiming the transient region.
 * @note  peak and fails are kept: peak = permanent + largest transient page, must hold across resets.
 * @note  Call only at page-state lifecycle boundaries; deleting a single object reclaims nothing.
 */
void wvui_arena_reset(wvui_arena_t *a);

/**
 * @brief Get the number of bytes in use.
 * @param a  arena; 0 returns 0
 * @return   Current cursor, in bytes
 */
uint32_t wvui_arena_used(const wvui_arena_t *a);
/**
 * @brief Get the historical high-water mark.
 * @param a  arena; 0 returns 0
 * @return   Peak byte count, kept across reset
 */
uint32_t wvui_arena_peak(const wvui_arena_t *a);
/**
 * @brief Get the cumulative allocation failure count.
 * @param a  arena; 0 returns 0
 * @return   Failure count; non-zero means a wrong budget
 */
uint32_t wvui_arena_fails(const wvui_arena_t *a);

/**
 * @brief Check whether the peak stays within budget.
 * @param budget_bytes  budget byte count for this region
 * @return              non-zero = peak within budget
 * @note   In the dev build this also triggers WVUI_ASSERT (empty by default); in the release build only the return value remains.
 */
uint8_t wvui_arena_assert_peak(const wvui_arena_t *a, uint32_t budget_bytes);

/**
 * @brief Compile-time assertion: the sum of region sizes does not exceed WVUI_CFG_RAM_BUDGET (0 = check off).
 * @param total_bytes  sum of the region sizes, may be a constant expression
 * @param tag          string literal, concatenated into the assertion message
 */
#define WVUI_BUDGET_ASSERT(total_bytes, tag) \
    WVUI_STATIC_ASSERT((WVUI_CFG_RAM_BUDGET) == 0 || (uint32_t)(total_bytes) <= (uint32_t)(WVUI_CFG_RAM_BUDGET), \
                      "WVUI RAM budget exceeded (" tag ")")

#endif /* WVUI_ARENA_H */
