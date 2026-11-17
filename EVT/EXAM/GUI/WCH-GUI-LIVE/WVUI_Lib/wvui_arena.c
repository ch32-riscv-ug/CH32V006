/**
 * @file wvui_arena.c
 * @brief Dual-region arena implementation, see wvui_arena.h for the contract.
 */

#include "wvui_arena.h"

/* sizeof expressions cannot go into #if, use a static assert instead. */
WVUI_STATIC_ASSERT((WVUI_ARENA_ALIGN & (WVUI_ARENA_ALIGN - 1u)) == 0u,
                  "WVUI_ARENA_ALIGN must be a power of two (alignment uses a mask)");

void wvui_arena_init(wvui_arena_t *a, void *mem, uint32_t size_bytes)
{
    if (a == 0)
    {
        return;
    }
    a->base = (uint8_t *)mem;
    a->size = (mem != 0) ? size_bytes : 0u;
    a->cursor = 0u;
    a->peak = 0u;
    a->fails = 0u;
}

void *wvui_arena_alloc(wvui_arena_t *a, uint32_t nbytes)
{
    uint32_t aligned;
    uint32_t end;
    void *p;

    if ((a == 0) || (a->base == 0))
    {
        return 0;
    }

    aligned = (a->cursor + (WVUI_ARENA_ALIGN - 1u)) & ~(WVUI_ARENA_ALIGN - 1u);
    end = aligned + nbytes;

    if ((end < aligned) || (end > a->size)) /* overflow and out-of-range caught in one check */
    {
        a->fails++;
        return 0; /* failure writes no memory */
    }

    p = (void *)(a->base + aligned);
    a->cursor = end;
    if (a->cursor > a->peak)
    {
        a->peak = a->cursor;
    }
    return p;
}

void wvui_arena_reset(wvui_arena_t *a)
{
    if (a == 0)
    {
        return;
    }
    a->cursor = 0u;
}

uint32_t wvui_arena_used(const wvui_arena_t *a)
{
    return (a != 0) ? a->cursor : 0u;
}

uint32_t wvui_arena_peak(const wvui_arena_t *a)
{
    return (a != 0) ? a->peak : 0u;
}

uint32_t wvui_arena_fails(const wvui_arena_t *a)
{
    return (a != 0) ? a->fails : 0u;
}

uint8_t wvui_arena_assert_peak(const wvui_arena_t *a, uint32_t budget_bytes)
{
    uint32_t peak = wvui_arena_peak(a);

    WVUI_ASSERT(peak <= budget_bytes);
    return (uint8_t)(peak <= budget_bytes);
}
