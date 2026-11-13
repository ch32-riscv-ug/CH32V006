/**
 * @file wvui_mem.c
 * @brief Memory backend implementation (three modes, one chosen at compile time).
 * @note  Allocation and reclamation go only through wvui_mem.h; the two unselected modes compile out entirely.
 */

#include "wvui_mem.h"

#if (WVUI_CFG_MEM_MODE == WVUI_MEM_ARENA)

/* ---- ARENA mode (default): dual-arena bump ---- */
wvui_arena_t *wvui_mem_s_persist;
wvui_arena_t *wvui_mem_s_transient;

wvui_arena_t *wvui_mem_transient_arena(void)
{
    return wvui_mem_s_transient;
}

uint8_t wvui_mem_can_alloc(void)
{
    return 1u; /* Independent of bind state */
}

uint32_t wvui_mem_used(void)
{
    return wvui_arena_used(wvui_mem_s_persist) + wvui_arena_used(wvui_mem_s_transient);
}

uint32_t wvui_mem_peak(void)
{
    /* Upper bound: the two arenas never peak together; only overestimates, never underestimates. */
    return wvui_arena_peak(wvui_mem_s_persist) + wvui_arena_peak(wvui_mem_s_transient);
}

uint32_t wvui_mem_fails(void)
{
    return wvui_arena_fails(wvui_mem_s_persist) + wvui_arena_fails(wvui_mem_s_transient);
}

#elif (WVUI_CFG_MEM_MODE == WVUI_MEM_HEAP)

/* ---- HEAP mode: real allocation, real reclamation ---- */
static uint32_t s_used;  /* The heap does not report block sizes; stats rely on caller-provided nbytes */
static uint32_t s_peak;
static uint32_t s_fails;

void wvui_mem_bind(wvui_arena_t *persistent, wvui_arena_t *transient)
{
    (void)persistent;
    (void)transient;
}

void *wvui_mem_alloc(uint32_t nbytes, uint8_t persistent)
{
    void *p;

    (void)persistent;
    if (nbytes == 0u)
    {
        return 0;
    }
    p = WVUI_MEM_ALLOC(nbytes);
    if (p == 0)
    {
        s_fails++;
        return 0; /* No partial allocation on failure */
    }
    s_used += nbytes;
    if (s_used > s_peak)
    {
        s_peak = s_used;
    }
    return p;
}

void wvui_mem_free(void *p, uint32_t nbytes)
{
    if (p == 0)
    {
        return;
    }
    /* Decrement stats before freeing: nbytes pairs strictly with the allocation. */
    s_used = (s_used >= nbytes) ? (s_used - nbytes) : 0u;
    WVUI_MEM_FREE(p);
}

void wvui_mem_transient_release(void)
{
    /* No-op: objects are freed one by one; writing here would double-free. */
}

wvui_arena_t *wvui_mem_transient_arena(void)
{
    return 0;
}

uint8_t wvui_mem_can_alloc(void)
{
    return 1u;
}

uint32_t wvui_mem_used(void)
{
    return s_used;
}

uint32_t wvui_mem_peak(void)
{
    return s_peak;
}

uint32_t wvui_mem_fails(void)
{
    return s_fails;
}

#else /* WVUI_MEM_STATIC */

/* ---- STATIC mode: zero allocation ---- */
void wvui_mem_bind(wvui_arena_t *persistent, wvui_arena_t *transient)
{
    (void)persistent;
    (void)transient;
}

void *wvui_mem_alloc(uint32_t nbytes, uint8_t persistent)
{
    (void)nbytes;
    (void)persistent;
        /* Create failure is expected semantics in this mode; not counted in fails. */
    return 0;
}

void wvui_mem_free(void *p, uint32_t nbytes)
{
    (void)p;
    (void)nbytes;
}

void wvui_mem_transient_release(void)
{
    /* Objects come from the application's static storage. */
}

wvui_arena_t *wvui_mem_transient_arena(void)
{
    return 0;
}

uint8_t wvui_mem_can_alloc(void)
{
    return 0u;
}

uint32_t wvui_mem_used(void)
{
    return 0u;
}

uint32_t wvui_mem_peak(void)
{
    return 0u;
}

uint32_t wvui_mem_fails(void)
{
    return 0u;
}

#endif /* WVUI_CFG_MEM_MODE */
