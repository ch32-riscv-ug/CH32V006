/**
 * @file wvui_anim.c
 * @brief Central animation engine implementation: steps the active chain per frame plus fixed-frame tweens.
 * @note  Zero division on the hot path; stepping saves next before the callback, so a callback that detaches itself does not truncate this frame's traversal.
 */

#include "wvui_anim.h"

#if WVUI_CFG_ENABLE_ANIM
static wvui_anim_t *s_head;
static uint16_t s_active;
#endif
static uint32_t s_steps;

/* Upper bound of iterations when running synchronously to completion with animations off:
 * truncates callbacks that never return 0, avoiding an infinite loop within one frame.
 * The longest known transition is scroll inertia (vel -= vel>>3, about 40 frames); 1024 frames leaves ample headroom. */
#define WVUI_ANIM_SNAP_MAX 1024u

#if WVUI_CFG_ENABLE_RENDER_STATS
#define WVUI_ASTAT(code) do { code; } while (0)
#else
#define WVUI_ASTAT(code) ((void)0)
#endif

/* ---- Duration-driven tween ---- */
void wvui_ramp_start(wvui_ramp_t *r, uint16_t inc, uint16_t frames)
{
    if (r == 0)
    {
        return;
    }
    r->inc = inc;
    r->left = frames;
    r->acc = 0u;
    r->pos = 0u;
    r->done = 0u;
    if (frames == 0u)
    {
        r->pos = 255u; /* frames = 0: set the target value right away */
        r->done = 1u;
    }
}

uint8_t wvui_ramp_step(wvui_ramp_t *r)
{
    if ((r == 0) || (r->done != 0u))
    {
        return 0u;
    }

    if (r->left > 0u)
    {
        r->left--;
    }
    r->acc = (uint16_t)(r->acc + r->inc);
    r->pos = (uint8_t)(r->acc >> 8);

    if ((r->left == 0u) || (r->pos >= 255u))
    {
        r->pos = 255u; /* target value reached on the last frame */
        r->done = 1u;
    }
    return (uint8_t)(r->done == 0u);
}

void wvui_ramp_finish(wvui_ramp_t *r)
{
    if (r == 0)
    {
        return;
    }
    r->left = 0u;
    r->acc = 0u;
    r->pos = 255u;
    r->done = 1u;
}

void wvui_anim_init(wvui_anim_t *a)
{
    if (a == 0)
    {
        return;
    }
#if WVUI_CFG_ENABLE_ANIM
    /* Detach first if already linked: otherwise the node stays on the chain while
     * claiming to be unlinked, and both the counter and the traversal go wrong. */
    wvui_anim_stop(a);
    a->next = 0;
    a->owner = 0;
    a->step = 0;
    a->linked = 0u;
#else
    /* With animations off a node is only owner + step, clearing them is the uninitialized state. */
    a->owner = 0;
    a->step = 0;
#endif
}

void wvui_anim_start(wvui_anim_t *a, wvui_widget_t *owner, wvui_anim_step_fn step)
{
    if (a == 0)
    {
        return;
    }

    a->owner = owner;
    a->step = step;

#if WVUI_CFG_ENABLE_ANIM
    if (a->linked != 0u)
    {
        return; /* idempotent: only owner/step refreshed */
    }

    a->next = s_head;
    s_head = a;
    a->linked = 1u;
    s_active++;
#else
    /* With animations off do not join the active chain: transitions instead run
     * synchronously to completion at the call site, permanent animations do not start.
     * Call sites still just call wvui_anim_start(), so they need no #if of their own. */
    if (step != 0)
    {
        uint16_t guard = (uint16_t)WVUI_ANIM_SNAP_MAX;

        while ((guard > 0u) && (step(a) != 0u))
        {
            guard--;
        }
    }
#endif
}

#if !WVUI_CFG_ENABLE_ANIM
void wvui_anim_start_permanent(wvui_anim_t *a, wvui_widget_t *owner, wvui_anim_step_fn step)
{
    /* Permanent animations do not start: the callback has no end condition, so the
     * static state is decided only by the widget's initial value. owner / step are
     * still written, matching the semantics of initialized-but-unlinked. */
    if (a != 0)
    {
        a->owner = owner;
        a->step = step;
    }
}
#endif /* !WVUI_CFG_ENABLE_ANIM */

void wvui_anim_stop(wvui_anim_t *a)
{
#if WVUI_CFG_ENABLE_ANIM
    wvui_anim_t **pp;

    if ((a == 0) || (a->linked == 0u))
    {
        return;
    }

    for (pp = &s_head; *pp != 0; pp = &(*pp)->next)
    {
        if (*pp == a)
        {
            *pp = a->next;
            break;
        }
    }

    a->next = 0;
    a->linked = 0u;
    if (s_active > 0u)
    {
        s_active--;
    }
#else
    /* With animations off no node ever joins the active chain: stop is a no-op and call sites need no branching. */
    (void)a;
#endif
}

void wvui_anim_step_all(void)
{
#if WVUI_CFG_ENABLE_ANIM
    wvui_anim_t *a = s_head;

    while (a != 0)
    {
        /* Save next first: the callback may detach itself, after which a->next has changed */
        wvui_anim_t *next = a->next;

        WVUI_ASTAT(s_steps++);

        if ((a->step == 0) || (a->step(a) == 0u))
        {
            wvui_anim_stop(a);
        }
        a = next;
    }
#endif
    /* With animations off: the whole loop is compiled out. */
}

void wvui_anim_forget_widget(wvui_widget_t *w)
{
#if WVUI_CFG_ENABLE_ANIM
    wvui_anim_t **pp = &s_head;

    if (w == 0)
    {
        return;
    }

    while (*pp != 0)
    {
        wvui_anim_t *a = *pp;

        if (a->owner == w)
        {
            *pp = a->next;
            a->next = 0;
            a->linked = 0u;
            if (s_active > 0u)
            {
                s_active--;
            }
        }
        else
        {
            pp = &a->next;
        }
    }
#else
    /* With animations off: no logic to detach nodes by owner. */
    (void)w;
#endif
}

void wvui_anim_drop_range(const void *lo, const void *hi)
{
#if WVUI_CFG_ENABLE_ANIM
    const uint8_t *base = (const uint8_t *)lo;
    const uint8_t *end = (const uint8_t *)hi;
    wvui_anim_t **pp = &s_head;

    if ((lo == 0) || (hi == 0) || (base >= end))
    {
        return;
    }

    while (*pp != 0)
    {
        wvui_anim_t *a = *pp;
        const uint8_t *addr = (const uint8_t *)a;

        if ((addr >= base) && (addr < end))
        {
            *pp = a->next;
            a->next = 0;
            a->linked = 0u; /* the arena reuses this memory right away */
            if (s_active > 0u)
            {
                s_active--;
            }
        }
        else
        {
            pp = &a->next;
        }
    }
#else
    (void)lo;
    (void)hi;
#endif
}

uint16_t wvui_anim_active(void)
{
#if WVUI_CFG_ENABLE_ANIM
    return s_active;
#else
    return 0u;
#endif
}

uint32_t wvui_anim_steps(void)
{
    return s_steps;
}

void wvui_anim_stats_reset(void)
{
    s_steps = 0u;
}
