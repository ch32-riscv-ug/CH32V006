#ifndef WVUI_MEM_H
#define WVUI_MEM_H

/**
 * @file wvui_mem.h
 * @brief Memory backend for widget objects: binding, allocation, reclamation and usage stats.
 * @note  The mode is selected at compile time by WVUI_CFG_MEM_MODE, never switched at runtime.
 */

#include <stdint.h>

#include "wvui_config.h"
#include "wvui_arena.h" /* Arena struct for the ARENA mode */

/** HEAP mode allocator: paired macros first, then FreeRTOS, finally stdlib. */
#if (WVUI_CFG_MEM_MODE == WVUI_MEM_HEAP)
#if defined(WVUI_MEM_ALLOC) && defined(WVUI_MEM_FREE)
/* Application-provided allocator. */
#elif defined(WVUI_MEM_ALLOC) || defined(WVUI_MEM_FREE)
#error "WVUI_MEM_ALLOC and WVUI_MEM_FREE must be defined together"
#elif WVUI_CFG_MEM_HEAP_FREERTOS
#include "FreeRTOS.h" /* pvPortMalloc / vPortFree */
#define WVUI_MEM_ALLOC(n) pvPortMalloc(n)
#define WVUI_MEM_FREE(p)  vPortFree(p)
#else
#include <stdlib.h> /* Host side and bare metal with libc */
#define WVUI_MEM_ALLOC(n) malloc(n)
#define WVUI_MEM_FREE(p)  free(p)
#endif
#endif /* WVUI_MEM_HEAP */

#if (WVUI_CFG_MEM_MODE == WVUI_MEM_ARENA)
/* Kernel private: the two arena pointers are defined in wvui_mem.c; apps use wvui_driver_bind_arena(). */
extern wvui_arena_t *wvui_mem_s_persist;
extern wvui_arena_t *wvui_mem_s_transient;

/**
 * @brief Bind the persistent and transient arenas; both parameters may be NULL.
 * @param persistent  Persistent arena; NULL = pure transient page mode
 * @param transient   Transient arena; its cursor resets to zero when the page leaves
 */
static inline void wvui_mem_bind(wvui_arena_t *persistent, wvui_arena_t *transient)
{
    wvui_mem_s_persist = persistent;
    wvui_mem_s_transient = transient;
}

/**
 * @brief Allocate one widget block from the given arena.
 * @param nbytes      Requested byte count; 0 = nothing allocated
 * @param persistent  1 = persistent arena, 0 = transient arena
 * @return            WVUI_ARENA_ALIGN-aligned pointer; NULL when space is short or no arena is bound
 */
static inline void *wvui_mem_alloc(uint32_t nbytes, uint8_t persistent)
{
    wvui_arena_t *a = (persistent != 0u) ? wvui_mem_s_persist : wvui_mem_s_transient;

    return (a != 0) ? wvui_arena_alloc(a, nbytes) : 0;
}

/**
 * @brief Return a widget block: the ARENA mode never reclaims memory, both parameters are ignored.
 * @param p       Allocated pointer
 * @param nbytes  Byte count given at request time
 */
static inline void wvui_mem_free(void *p, uint32_t nbytes)
{
    (void)p;
    (void)nbytes;
}

/**
 * @brief Reset the transient arena cursor to zero.
 * @note  The persistent arena is unaffected; all objects allocated in the transient arena become invalid after the call.
 */
static inline void wvui_mem_transient_release(void)
{
    wvui_arena_reset(wvui_mem_s_transient);
}
#else
/**
 * @brief Bind the arenas; no-op in STATIC / HEAP mode.
 * @param persistent  Ignored, may be NULL
 * @param transient   Ignored, may be NULL
 */
void wvui_mem_bind(wvui_arena_t *persistent, wvui_arena_t *transient);

/**
 * @brief Allocate one widget block.
 * @param nbytes      Requested byte count; 0 = nothing allocated
 * @param persistent  Ignored (objects are not arena-bound)
 * @return            WVUI_ARENA_ALIGN-aligned pointer; NULL on failure
 * @note              STATIC mode always returns NULL: objects must come from WVUI_WIDGET_STORAGE, this mode cannot switch pages.
 * @note              HEAP mode goes through WVUI_MEM_ALLOC(nbytes), failures count in fails.
 */
void *wvui_mem_alloc(uint32_t nbytes, uint8_t persistent);

/**
 * @brief Return one widget block; no-op in STATIC mode.
 * @param p       Allocated pointer; NULL = do nothing
 * @param nbytes  Byte count requested at creation; the HEAP mode subtracts it from stats
 */
void wvui_mem_free(void *p, uint32_t nbytes);

/**
 * @brief Reset the transient arena; no-op in STATIC / HEAP mode.
 * @note  HEAP mode objects were each freed in wvui_widget_delete().
 */
void wvui_mem_transient_release(void);
#endif /* WVUI_CFG_MEM_MODE == WVUI_MEM_ARENA */

/**
 * @brief Get the transient arena struct; always NULL in STATIC / HEAP mode.
 */
wvui_arena_t *wvui_mem_transient_arena(void);

/**
 * @brief Whether this mode can get a new object.
 * @return  1 = yes; 0 = STATIC mode, wvui_widget_create() is sure to fail
 * @note    STATIC mode thus declares objects statically with WVUI_WIDGET_STORAGE.
 */
uint8_t wvui_mem_can_alloc(void);

/**
 * @brief Bytes currently occupied by widgets.
 * @return  ARENA mode: sum of both arenas in use; HEAP mode: sum of live allocations; STATIC mode = 0
 */
uint32_t wvui_mem_used(void);

/**
 * @brief Historical high watermark.
 * @return  ARENA mode: sum of both arena peaks, kept across resets; HEAP mode: peak of live allocations; STATIC mode = 0
 * @note    ARENA mode gives an upper bound, only overestimating: the two arenas never peak together.
 */
uint32_t wvui_mem_peak(void);

/**
 * @brief Number of failed allocations.
 * @return  Cumulative failures, only grows; STATIC mode is always 0
 * @note    > 0 means the budget was miscalculated, or other modules used up the heap.
 */
uint32_t wvui_mem_fails(void);

/* Prevents bypassing the value validation in wvui_config.h. */
WVUI_STATIC_ASSERT((WVUI_CFG_MEM_MODE == WVUI_MEM_STATIC) || (WVUI_CFG_MEM_MODE == WVUI_MEM_ARENA) ||
                      (WVUI_CFG_MEM_MODE == WVUI_MEM_HEAP),
                  "WVUI_CFG_MEM_MODE must be 0 (static), 1 (arena) or 2 (heap)");

#endif /* WVUI_MEM_H */
