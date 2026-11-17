/**
 * @file wvui_page.c
 * @brief Page system implementation.
 * @note  All cold paths; the eight-step leave sequence lives in page_leave_current(), cleanup by wvui_transient_reset().
 */

#include "wvui_page.h"

#include "wvui_anim.h"
#include "wvui_driver.h" /* two symbols */
#include "wvui_input.h"

#if WVUI_CFG_ENABLE_PAGES

/* Action codes: one pending slot; multiple requests in a frame, the last one wins. */
#define WVUI_PAGE_ACT_NONE (0u)
#define WVUI_PAGE_ACT_GO   (1u)
#define WVUI_PAGE_ACT_PUSH (2u)
#define WVUI_PAGE_ACT_BACK (3u)

static wvui_page_t *s_stack[WVUI_CFG_PAGE_STACK];
static uint16_t s_depth; /* 0 = no current page yet */
static uint8_t s_act = WVUI_PAGE_ACT_NONE;
static wvui_page_t *s_act_page;

/* Pinned page registry chain: head != 0 <=> PARKED flag <=> on the list.
 * Only the current path sits in the stack; pinned pages replaced by go() or popped
 * by back() are not in the stack, but their object chains stay alive. */
static wvui_page_t *s_parked;

static wvui_page_t *page_current(void)
{
    return (s_depth > 0u) ? s_stack[s_depth - 1u] : 0;
}

static void page_park(wvui_page_t *p, wvui_widget_t *chain)
{
    p->head = chain;
    if ((p->flags & WVUI_PAGE_F_PARKED) == 0u)
    {
        p->flags |= WVUI_PAGE_F_PARKED;
        p->parked_next = s_parked;
        s_parked = p;
    }
}

static void page_unpark(wvui_page_t *p)
{
    wvui_page_t **pp;

    if ((p->flags & WVUI_PAGE_F_PARKED) == 0u)
    {
        return;
    }
    for (pp = &s_parked; *pp != 0; pp = &(*pp)->parked_next)
    {
        if (*pp == p)
        {
            *pp = p->parked_next;
            break;
        }
    }
    p->parked_next = 0;
    p->flags &= (uint8_t)~WVUI_PAGE_F_PARKED;
}

/* ---- Eight-step sequence, step 1: send RELEASED before clearing pressed state ---- */
static void page_release_pressed(void)
{
    wvui_widget_t *p = wvui_input_pressed();

    if ((p != 0) && (WVUI_W_CLASS(p) != 0) && (WVUI_W_CLASS(p)->event != 0))
    {
        wvui_indev_t d;

        d.x = 0;
        d.y = 0;
        d.pressed = 0u;
        (void)WVUI_W_CLASS(p)->event(p, WVUI_EVENT_RELEASED, &d);
    }
    wvui_input_reset(); /* clear pressed / grab / swipe-decision state */
}

void wvui_page_stop_anims(wvui_widget_t *head)
{
    wvui_widget_t *w;

    for (w = head; w != 0; w = w->next)
    {
        /* Reclaim by owner: a widget may own several nodes, stop all at once. */
        wvui_anim_forget_widget(w);
        if ((WVUI_W_CLASS(w) != 0) && ((WVUI_W_CLASS(w)->flags & WVUI_CLASS_CHILD_OWNER) != 0u))
        {
            wvui_page_stop_anims(*wvui_widget_children(w)); /* recursion depth = nesting depth */
        }
    }
}

/* ---- Leave the current page: eight-step sequence ---- */
/* full_invalidate = 1: the per-widget invalidation of steps 6/8 is replaced by one full-screen invalidation. */
static void page_leave_current(uint8_t full_invalidate)
{
    wvui_page_t *p = page_current();
    wvui_widget_t *head = wvui_widget_first();

    /* 0. Application save hook, outside the eight steps; objects, geometry and state all still here. */
    if ((p != 0) && (p->on_leave != 0))
    {
        p->on_leave(p);
    }

    /* 1. Release pressed state */
    page_release_pressed();

    /* 2. Empty step; 3. Close modals: numbering kept, order is not changeable */

    /* 4. Stop all anim nodes of this page: stop the chain first, then zero the
     *    cursor; must run under both memory modes.
     *    drop_range in wvui_transient_reset() is only a rollback. */
    if (head != 0)
    {
        wvui_page_stop_anims(head);
    }

    /* 5. Empty step: numbering kept */

    /* 6./7. Pinned pages only detach the chain; objects and cls are kept (for the
     *       next attach); erasure marks are supplied by this page. Stateless pages
     *       and orphan widgets: wvui_widget_reset_top() destroys in place and
     *       invalidates one by one. */
    if ((p != 0) && (p->pinned != 0u))
    {
        if (full_invalidate == 0u)
        {
            wvui_widget_t *w;

            for (w = head; w != 0; w = w->next)
            {
                wvui_widget_invalidate(w); /* the container covers its children */
            }
        }
        {
            wvui_widget_t *chain = wvui_widget_detach_top();

            if (chain != 0)
            {
                page_park(p, chain);
            }
            else
            {
                p->head = 0; /* empty chain does not join the list */
                page_unpark(p);
            }
        }
    }
    else
    {
        wvui_widget_reset_top();
        if (p != 0)
        {
            p->head = 0;
            page_unpark(p); /* leave the list once the chain is empty */
        }
        /* 7. Reclaim: ARENA profile zeroes the cursor (whole block); HEAP profile already
         *    freed, this step is a no-op; STATIC profile does not compile. */
        wvui_mem_transient_release();
    }

    /* 8. Erasure is taken over by this frame's dirty rectangle consumption. */
}

/* ---- Enter a page: assembly + on_enter ---- */
static void page_enter(wvui_page_t *p)
{
    if ((p->pinned != 0u) && (p->head != 0))
    {
        wvui_widget_attach_top(p->head); /* pinned page reattach, zero allocation */
        page_unpark(p);                 /* back in the active set, parked state ended */
    }
    else if ((p->flags & WVUI_PAGE_F_BUILD_FAILED) == 0u)
    {
        (void)wvui_build(p->descs, p->count, p->pinned);
        /* The previous page was fully detached: the s_top head is this page's head. */
        p->head = wvui_widget_first();
        if ((p->head == 0) && (p->count > 0u))
        {
            p->flags |= WVUI_PAGE_F_BUILD_FAILED; /* once flagged, no retry */
        }
    }

    if (p->on_enter != 0)
    {
        p->on_enter(p);
    }
}

/* Invalidation is the only refresh entry. */
static void page_invalidate_screen(void)
{
    const wvui_info_t *info = wvui_info();
    wvui_rect_t r;

    if (info == 0)
    {
        return;
    }
    r.x0 = 0;
    r.y0 = 0;
    r.x1 = (int16_t)(info->w - 1);
    r.y1 = (int16_t)(info->h - 1);
    wvui_invalidate(r);
}

/* ---- Page fade-in: duration driven, ramp node embedded in this layer ---- */
/* The factor only applies to the normal layer; each frame during the fade invalidates the
 * full screen; the last frame returns 0 and the engine drops the node. */
#if WVUI_CFG_PAGE_FADE_FRAMES > 0
static wvui_anim_t s_fade;
static wvui_ramp_t s_fade_ramp;
/* Explicit init: the static default must be 255, otherwise the first frame is all black. */
static uint8_t s_fade_opa = 255u;
static uint8_t s_fade_on = 1u;

static uint8_t page_fade_step(wvui_anim_t *a)
{
    uint8_t running;

    (void)a;
    running = wvui_ramp_step(&s_fade_ramp);
    /* Last frame has running == 0; redraw once with 255. */
    s_fade_opa = (running != 0u) ? s_fade_ramp.pos : 255u;
    page_invalidate_screen();
    return running;
}

static void page_fade_cancel(void)
{
    wvui_anim_stop(&s_fade);
    wvui_ramp_finish(&s_fade_ramp);
    s_fade_opa = 255u;
}

static void page_fade_start(void)
{
    if (s_fade_on == 0u)
    {
        page_fade_cancel(); /* hard cut: opacity back to 255 */
        return;
    }
    wvui_ramp_start(&s_fade_ramp, WVUI_RAMP_INC(WVUI_CFG_PAGE_FADE_FRAMES),
                   WVUI_CFG_PAGE_FADE_FRAMES);
    s_fade_opa = 0u; /* this frame starts at 0 */
    wvui_anim_start(&s_fade, 0, page_fade_step); /* owner is NULL */
}
#endif /* WVUI_CFG_PAGE_FADE_FRAMES */

uint8_t wvui_page_opa(void)
{
#if WVUI_CFG_PAGE_FADE_FRAMES > 0
    return s_fade_opa;
#else
    return 255u; /* compiled out, always 255 */
#endif
}

void wvui_page_set_fade(uint8_t on)
{
#if WVUI_CFG_PAGE_FADE_FRAMES > 0
    const uint8_t want = (on != 0u) ? 1u : 0u;

    if (s_fade_on == want)
    {
        return;
    }
    s_fade_on = want;
    if (want == 0u)
    {
        /* End immediately: opacity back to 255, one full-screen invalidation. */
        page_fade_cancel();
        page_invalidate_screen();
    }
#else
    (void)on; /* hard cut at compile time */
#endif
}

uint8_t wvui_page_fade_enabled(void)
{
#if WVUI_CFG_PAGE_FADE_FRAMES > 0
    return s_fade_on;
#else
    return 0u;
#endif
}

/* ---- Public API ---- */
void wvui_page_go(wvui_page_t *p)
{
    if ((p == 0) || ((p->count > 0u) && (p->descs == 0)))
    {
        return;
    }
    s_act = WVUI_PAGE_ACT_GO;
    s_act_page = p;
}

uint8_t wvui_page_push(wvui_page_t *p)
{
    if ((p == 0) || ((p->count > 0u) && (p->descs == 0)))
    {
        return 0u;
    }
    /* A full stack is rejected at request time, so the frame head never sees a half-switched intermediate state. */
    if (s_depth >= (uint16_t)WVUI_CFG_PAGE_STACK)
    {
        return 0u;
    }
    s_act = WVUI_PAGE_ACT_PUSH;
    s_act_page = p;
    return 1u;
}

uint8_t wvui_page_back(void)
{
    if (s_depth < 2u)
    {
        return 0u; /* only the current page in the stack, nothing to go back to */
    }
    s_act = WVUI_PAGE_ACT_BACK;
    s_act_page = 0;
    return 1u;
}

uint8_t wvui_page_reset(wvui_page_t *p)
{
    if ((p == 0) || ((p->count > 0u) && (p->descs == 0)))
    {
        return 0u;
    }

    /* Run the leave sequence first when a current page already exists; no full-screen
     * invalidation, the caller pushes the full screen right after. */
    page_leave_current(0u);
    s_act = WVUI_PAGE_ACT_NONE;
    s_act_page = 0;
    s_depth = 0u;
#if WVUI_CFG_PAGE_FADE_FRAMES > 0
    page_fade_cancel(); /* reset needs no fade-in */
#endif

    /* Do not clear p->head: pinned page objects are built once; clearing would lose
     * objects and persistent arena memory; stateless pages already set it to 0 on leave. */
    page_enter(p);
    s_stack[s_depth] = p;
    s_depth = 1u;
    return 1u;
}

uint8_t wvui_page_apply(void)
{
    wvui_page_t *req = s_act_page;
    wvui_page_t *target = 0;
    uint16_t new_depth = s_depth;
    uint8_t act = s_act;

    if (act == WVUI_PAGE_ACT_NONE)
    {
        return 0u;
    }
    s_act = WVUI_PAGE_ACT_NONE;
    s_act_page = 0;

    /* Compute the new stack shape first (pure arithmetic, no object touched); give up entirely if it cannot be computed. */
    switch (act)
    {
    case WVUI_PAGE_ACT_GO:
        target = req;
        new_depth = (s_depth > 0u) ? s_depth : 1u; /* replace the top, keep the history */
        break;
    case WVUI_PAGE_ACT_PUSH:
        if ((s_depth < (uint16_t)WVUI_CFG_PAGE_STACK) && (req != 0))
        {
            target = req;
            new_depth = (uint16_t)(s_depth + 1u);
        }
        break;
    default: /* BACK */
        if (s_depth >= 2u)
        {
            new_depth = (uint16_t)(s_depth - 1u);
            target = s_stack[new_depth - 1u];
        }
        break;
    }

    if (target == 0)
    {
        return 0u; /* defensive early return */
    }

    /* Run leave-sequence steps 1..7; erasure is taken over by the full-screen invalidation below. */
    page_leave_current(1u);
    s_depth = new_depth;
    s_stack[s_depth - 1u] = target;
    page_enter(target);

    page_invalidate_screen(); /* full-screen invalidation */
#if WVUI_CFG_PAGE_FADE_FRAMES > 0
    page_fade_start(); /* fade-in starts */
#endif
    wvui_input_reset();        /* clear touch registration */
    return 1u;
}

void wvui_page_tick(void)
{
    wvui_page_t *p = page_current();

    if ((p != 0) && (p->on_tick != 0))
    {
        p->on_tick(p);
    }
}

const wvui_page_t *wvui_page_current(void)
{
    return page_current();
}

const wvui_page_t *wvui_page_pending(void)
{
    return (s_act != WVUI_PAGE_ACT_NONE) ? s_act_page : 0;
}

uint16_t wvui_page_depth(void)
{
    return s_depth;
}

void wvui_page_leave(void)
{
    page_leave_current(0u); /* erasure is left to the caller's screen push */
    s_act = WVUI_PAGE_ACT_NONE;
    s_act_page = 0;
    s_depth = 0u;
}

/* ---- Maintain the pinned page chain when objects are deleted ---- */
static uint8_t page_unlink_from(wvui_widget_t **head, wvui_widget_t *w)
{
    wvui_widget_t *p = *head;

    if (p == w)
    {
        *head = w->next;
        return 1u;
    }
    while (p != 0)
    {
        if (p->next == w)
        {
            p->next = w->next;
            return 1u;
        }
        p = p->next;
    }
    return 0u;
}

uint8_t wvui_page_unlink_widget(wvui_widget_t *w)
{
    wvui_page_t *p;

    if (w == 0)
    {
        return 0u;
    }
    /* Search only the pinned pages: the top page's chain is s_top, its detach path is
     * handled elsewhere; pinned pages replaced by go() or popped by back() are not in
     * the stack, but their object chains are still alive. */
    for (p = s_parked; p != 0; p = p->parked_next)
    {
        if ((p->head != 0) && (page_unlink_from(&p->head, w) != 0u))
        {
            if (p->head == 0)
            {
                page_unpark(p); /* leave the list once the chain empties */
            }
            return 1u;
        }
    }
    return 0u;
}

#else /* !WVUI_CFG_ENABLE_PAGES */

/* Empty stubs: signatures match wvui_page.h, so the driver/widget layers need no #if;
 * semantics: always exactly one page (the current top chain), all page switch requests ignored. */

void wvui_page_go(wvui_page_t *p) { (void)p; }
uint8_t wvui_page_push(wvui_page_t *p) { (void)p; return 0u; }
uint8_t wvui_page_back(void) { return 0u; }
uint8_t wvui_page_reset(wvui_page_t *p) { (void)p; return 0u; }
uint8_t wvui_page_apply(void) { return 0u; }
void wvui_page_tick(void) {}
const wvui_page_t *wvui_page_current(void) { return 0; }
const wvui_page_t *wvui_page_pending(void) { return 0; }
uint16_t wvui_page_depth(void) { return 0u; }
uint8_t wvui_page_opa(void) { return 255u; }
void wvui_page_set_fade(uint8_t on) { (void)on; }
uint8_t wvui_page_fade_enabled(void) { return 0u; }
void wvui_page_leave(void) {}
uint8_t wvui_page_unlink_widget(wvui_widget_t *w) { (void)w; return 0u; }
void wvui_page_stop_anims(wvui_widget_t *head) { (void)head; }

#endif /* WVUI_CFG_ENABLE_PAGES */
