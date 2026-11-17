/**
 * @file wvui_widget.c
 * @brief Widget base class implementation: class table, object tree, life cycle.
 * @note  All entry points are cold paths; delete only detaches and destructs, memory is released in one block when the cursor resets.
 */

#include <string.h>

#include "wvui_widget.h"
#include "wvui_driver.h" /* dirty marking goes through the driver's dirty-rect manager */
#include "wvui_layer.h"  /* scene reset clears the top-level list */
#include "wvui_mem.h"    /* the only allocation and release backend */
#include "wvui_page.h"   /* docked-page detach fallback */

/* Allocation and mode-switching logic belongs to wvui_mem.c: this file only holds the object tree. */
static wvui_widget_t *s_top; /* Top-level list: head at the bottom, tail on top */
static uint32_t s_count;

/* ---- Class table: the compile-time array of wvui_widgets.c, no registration call at startup ---- */
uint8_t wvui_class_bind(uint16_t type, const wvui_class_t *cls)
{
#if WVUI_CFG_ENABLE_CLASS_BIND
    /* type 0 is a reserved value: deleted is recorded as type = 0, binding slot 0 would keep deleted objects resolvable. */
    if ((cls == 0) || (type == 0u) || (type >= (uint16_t)WVUI_CFG_CLASS_MAX))
    {
        return 0u;
    }
    wvui_class_table[type] = cls;
    return 1u;
#else
    /* The table is const: runtime registration is unsupported; failing with 0 stays consistent with the other mode. */
    (void)type;
    (void)cls;
    return 0u;
#endif
}

const wvui_class_t *wvui_class_get(uint16_t type)
{
    return (type < (uint16_t)WVUI_CFG_CLASS_MAX) ? wvui_class_table[type] : 0;
}

uint16_t wvui_class_bound_count(void)
{
    uint16_t n = (uint16_t)WVUI_CFG_CLASS_MAX;

    /* Scan back from the table tail: trimmed type numbers leave empty slots; take the first non-empty slot number + 1. */
    while ((n > 0u) && (wvui_class_table[n - 1u] == 0))
    {
        n--;
    }
    return n;
}

void wvui_widget_bind_arena(wvui_arena_t *persistent, wvui_arena_t *transient)
{
    /* ARENA mode hands both areas to the backend; STATIC / HEAP make it a no-op, keeping the startup sequence equal across modes. */
    wvui_mem_bind(persistent, transient);
}

/* ---- Life cycle: the three modes differ only in block origin and release point ---- */
static wvui_widget_t *widget_setup(wvui_widget_t *o, const wvui_class_t *cls, uint16_t type,
                                  uint16_t id, int16_t x, int16_t y, int16_t w, int16_t h)
{
    wvui_widget_t *tail;

    o->next = 0;
    o->parent = 0;
#if WVUI_CFG_WIDGET_COMPACT_TYPE
    /* Compact mode: store the class number (type already validated by the caller as < WVUI_CFG_CLASS_MAX) */
    o->type = (uint8_t)type;
    (void)cls;
#else
    (void)type;
    o->cls = cls;
#endif
    o->x = x;
    o->y = y;
    o->w = w;
    o->h = h;
    o->id = id;
#if WVUI_CFG_ENABLE_INPUT
    o->pad_l = 0;
    o->pad_t = 0;
    o->pad_r = 0;
    o->pad_b = 0;
#endif
    /* flags must be zeroed: initial memory values cannot be relied on in any mode, garbage bits would read as hidden. */
    o->flags = 0u;

    (void)memset(wvui_widget_rt(o), 0, cls->rt_size);

    if (cls->init != 0)
    {
        cls->init(o);
    }

    /* Append to the tail of the top-level list: array order is the z-order */
    if (s_top == 0)
    {
        s_top = o;
    }
    else
    {
        tail = s_top;
        while (tail->next != 0)
        {
            tail = tail->next;
        }
        tail->next = o;
    }
    s_count++;

    wvui_widget_invalidate(o);
    return o;
}

wvui_widget_t *wvui_widget_create_raw(uint16_t type, uint16_t id, int16_t x, int16_t y,
                                    int16_t w, int16_t h, uint8_t persistent)
{
#if (WVUI_CFG_MEM_MODE == WVUI_MEM_STATIC)
    /* STATIC mode has no allocatable memory: objects come from WVUI_WIDGET_STORAGE + STATIC_BIND.
     * Hitting this entry is a caller error: assert in debug, return NULL in release, never touch the class table. */
    (void)type;
    (void)id;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)persistent;
    WVUI_ASSERT(0);
    return 0;
#else
    const wvui_class_t *cls = wvui_class_get(type);
    wvui_widget_t *o;
    uint32_t need;

    if (cls == 0)
    {
        return 0;
    }

    need = (uint32_t)sizeof(wvui_widget_t) + (uint32_t)cls->rt_size;

    o = (wvui_widget_t *)wvui_mem_alloc(need, persistent);
    if (o == 0)
    {
        /* Allocation failure returns NULL without out-of-bounds writes; failure counts stay in the backend's wvui_mem_fails(). */
        return 0;
    }
    return widget_setup(o, cls, type, id, x, y, w, h);
#endif
}

wvui_widget_t *wvui_widget_create(const wvui_widget_desc_t *desc, uint8_t persistent)
{
    if (desc == 0)
    {
        return 0;
    }
    return wvui_widget_create_raw(desc->type, desc->id, desc->x, desc->y, desc->w, desc->h,
                                 persistent);
}

wvui_widget_t *wvui_widget_init_static(wvui_widget_t *storage, uint32_t bytes,
                                     const wvui_widget_desc_t *desc)
{
#if (WVUI_CFG_MEM_MODE == WVUI_MEM_HEAP)
    /* HEAP mode deletes objects with WVUI_MEM_FREE(): handing static memory to free is undefined behavior. */
    (void)storage;
    (void)bytes;
    (void)desc;
    return 0;
#else
    const wvui_class_t *cls;

    if ((storage == 0) || (desc == 0))
    {
        return 0;
    }
    cls = wvui_class_get(desc->type);
    if (cls == 0)
    {
        return 0; /* Type trimmed: same rule as create */
    }
    if (bytes < ((uint32_t)sizeof(wvui_widget_t) + (uint32_t)cls->rt_size))
    {
        /* Declared too small (RT_T mismatches the real runtime): assert, never write past into neighbors */
        WVUI_ASSERT(0);
        return 0;
    }
    return widget_setup(storage, cls, desc->type, desc->id, desc->x, desc->y, desc->w, desc->h);
#endif
}


static uint8_t unlink_from(wvui_widget_t *head_owner_list, wvui_widget_t *w)
{
    wvui_widget_t *p = head_owner_list;

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

static void widget_unlink(wvui_widget_t *w)
{
    wvui_widget_t **head;

    if ((w->parent != 0) && (WVUI_W_CLASS(w->parent) != 0) &&
        ((WVUI_W_CLASS(w->parent)->flags & WVUI_CLASS_CHILD_OWNER) != 0u))
    {
        head = wvui_widget_children(w->parent);
    }
    else
    {
        head = &s_top;
    }

    if (*head == w)
    {
        *head = w->next;
    }
    else if (unlink_from(*head, w) == 0u)
    {
        /* Not in the active set: possibly on a docked page's list (the whole list parked on itself); leaving it attached
         * would leave a dangling reference; child widgets never take this branch, their head comes from parent. */
        (void)wvui_page_unlink_widget(w);
    }

    /* Clear next after detaching: otherwise the child list forms a cycle (the tail points back at itself) and walks loop forever. */
    w->next = 0;
}

void wvui_widget_delete(wvui_widget_t *w)
{
#if (WVUI_CFG_MEM_MODE == WVUI_MEM_HEAP)
    /* HEAP mode releases by the originally requested size; the other two modes have no per-object free */
    uint32_t need = 0u;
#endif

    if (w == 0)
    {
        return;
    }

#if (WVUI_CFG_MEM_MODE == WVUI_MEM_HEAP)
    if (WVUI_W_CLASS(w) != 0)
    {
        need = (uint32_t)sizeof(wvui_widget_t) + (uint32_t)WVUI_W_CLASS(w)->rt_size;
    }
#endif

    /* 1. Destructor callback: cls and fields are still valid, safe to mark dirty. */
    if ((WVUI_W_CLASS(w) != 0) && (WVUI_W_CLASS(w)->on_delete != 0))
    {
        WVUI_W_CLASS(w)->on_delete(w);
    }

    /* 2. Post-order recursion over the child list: delete all descendants first, then self. */
    if ((WVUI_W_CLASS(w) != 0) && ((WVUI_W_CLASS(w)->flags & WVUI_CLASS_CHILD_OWNER) != 0u))
    {
        wvui_widget_t *c = *wvui_widget_children(w);
        while (c != 0)
        {
            wvui_widget_t *next = c->next;
            wvui_widget_delete(c);
            c = next;
        }
        *wvui_widget_children(w) = 0;
    }

    /* 3. Dirty-mark erase (while the geometry is still there) */
    wvui_widget_invalidate(w);

    /* 4. Detach: the top-level list must go first. widget_unlink clears w->next at its end, and next is
     *    a field shared by both lists; in the reverse order wvui_layer_detach() cannot read the successor
     *    and would drop the remaining top-level objects along with it. */
    wvui_layer_unlink_widget(w);
    widget_unlink(w);

    /* 5. Release kernel-side references; the top-level list was already detached in step 4, repeated calls are idempotent. */
    wvui_kernel_forget_widget(w);

    /* 6. Clear fields; ARENA mode waits for the cursor reset, HEAP mode frees at the end of the file. */
    WVUI_W_CLEAR_CLASS(w);
    w->next = 0;
    w->parent = 0;
    w->id = 0;
    w->w = 0;
    w->h = 0;
    if (s_count > 0u)
    {
        s_count--;
    }

#if (WVUI_CFG_MEM_MODE == WVUI_MEM_HEAP)
    /* Free only as the last step: every step above reads w->next, detaches in reverse and calls back owners, an early free would dangle. */
    wvui_mem_free(w, need);
#endif
}

wvui_widget_t *wvui_widget_first(void)
{
    return s_top;
}

uint32_t wvui_widget_count(void)
{
    return s_count;
}

wvui_widget_t *wvui_widget_find_by_id(uint16_t id)
{
    wvui_widget_t *w;

    for (w = s_top; w != 0; w = w->next)
    {
        if (w->id == id)
        {
            return w;
        }
    }
    return 0;
}

void wvui_widget_reset_top(void)
{
#if (WVUI_CFG_MEM_MODE == WVUI_MEM_HEAP)
    /* HEAP mode has no cursor-reset release: must delete one by one, otherwise memory never returns to the heap. */
    while (s_top != 0)
    {
        wvui_widget_delete(s_top);
    }
    wvui_layer_reset();
    s_count = 0u;
#else
    wvui_widget_t *w = s_top;

    while (w != 0)
    {
        wvui_widget_t *next = w->next;
        wvui_widget_invalidate(w); /* Mark the vanished full page dirty */
        WVUI_W_CLEAR_CLASS(w);
        w->next = 0;
        w->parent = 0;
        w = next;
    }
    s_top = 0;
    /* The top-level list is cleared together: only a full scene reset may touch it, otherwise layers survive across scenes. */
    wvui_layer_reset();
    s_count = 0u;
#endif
}

/* ---- Parking and re-attaching of the active set (conventions in wvui_widget.h) ---- */
wvui_widget_t *wvui_widget_detach_top(void)
{
    wvui_widget_t *head = s_top;

    s_top = 0;
    s_count = 0u;
    /* next is kept as-is, meaning the whole list was detached; no dirty mark, the page layer erases the full screen. */
    return head;
}

void wvui_widget_attach_top(wvui_widget_t *head)
{
    wvui_widget_t *p;

    if (head == 0)
    {
        return;
    }
    /* This list must be empty when entering the page (guaranteed by the page layer), otherwise the two pages' lists interfere */
    WVUI_ASSERT(s_top == 0);

    if (s_top == 0)
    {
        s_top = head;
    }
    else
    {
        p = s_top;
        while (p->next != 0)
        {
            p = p->next;
        }
        p->next = head;
    }

    for (p = head; p != 0; p = p->next)
    {
        s_count++; /* Restore the count by the list length */
    }
}

wvui_arena_t *wvui_widget_transient_arena(void)
{
    /* Compatibility entry: the holder lives in wvui_mem.c (STATIC / HEAP modes return NULL) */
    return wvui_mem_transient_arena();
}

/* ---- Geometry / hit area / hit test ---- */
wvui_rect_t wvui_widget_rect(const wvui_widget_t *w)
{
    wvui_rect_t r;

    r.x0 = w->x;
    r.y0 = w->y;
    r.x1 = (int16_t)(w->x + w->w - 1);
    r.y1 = (int16_t)(w->y + w->h - 1);
    return r;
}

wvui_rect_t wvui_widget_hit_rect(const wvui_widget_t *w)
{
#if WVUI_CFG_ENABLE_INPUT
    wvui_rect_t r;

    r.x0 = (int16_t)(w->x - w->pad_l);
    r.y0 = (int16_t)(w->y - w->pad_t);
    r.x1 = (int16_t)(w->x + w->w - 1 + w->pad_r);
    r.y1 = (int16_t)(w->y + w->h - 1 + w->pad_b);
    return r;
#else
    /* No input layer: no hit area, return the rectangle without expansion */
    return wvui_widget_rect(w);
#endif
}

uint8_t wvui_widget_hit_test_point(const wvui_widget_t *w, int16_t x, int16_t y)
{
    wvui_rect_t r = wvui_widget_hit_rect(w);

    return (uint8_t)((x >= r.x0) && (x <= r.x1) && (y >= r.y0) && (y <= r.y1));
}

void wvui_widget_set_pos(wvui_widget_t *w, int16_t x, int16_t y)
{
    if (w == 0)
    {
        return;
    }
    if ((w->x == x) && (w->y == y))
    {
        return;
    }

    wvui_widget_invalidate(w); /* Old position */
    w->x = x;
    w->y = y;
    if ((WVUI_W_CLASS(w) != 0) && (WVUI_W_CLASS(w)->set_pos != 0))
    {
        WVUI_W_CLASS(w)->set_pos(w, x, y);
    }
    wvui_widget_invalidate(w); /* New position */
}

void wvui_widget_set_hit_pad(wvui_widget_t *w, int8_t l, int8_t t, int8_t r, int8_t b)
{
    if (w == 0)
    {
        return;
    }
#if WVUI_CFG_ENABLE_INPUT
    w->pad_l = l;
    w->pad_t = t;
    w->pad_r = r;
    w->pad_b = b;
    /* The hit area affects hit test only, not visuals: no dirty mark; hit results are computed fresh each time, never cached. */
#else
    /* The hit-area fields were removed along with the input layer: keep this signature, no-op */
    (void)l;
    (void)t;
    (void)r;
    (void)b;
#endif
}

/* Detach a single top-level object: keep cls/id/geometry so it can be re-added (wvui_layer_attach). */
uint8_t wvui_widget_detach(wvui_widget_t *w)
{
    if (w == 0)
    {
        return 0u;
    }
    /* Handle the list-head case first: unlink_from() only inspects p->next, missing this step leaves the head on
     * two lists, and delete would cut the whole top-level list along next. */
    if (s_top == w)
    {
        s_top = w->next;
        w->next = 0;
        w->parent = 0;
        return 1u;
    }
    if (unlink_from(s_top, w) == 0u)
    {
        return 0u;
    }
    w->next = 0;
    w->parent = 0;
    return 1u;
}

/* A top-level object off the top-level list must return to the normal layer, otherwise it is not drawn and not hit (relied on by dropdown collapse). */
void wvui_widget_attach_root(wvui_widget_t *w)
{
    wvui_widget_t *p;

    if (w == 0)
    {
        return;
    }
    for (p = s_top; p != 0; p = p->next)
    {
        if (p == w)
        {
            return;
        }
    }
    w->parent = 0;
    w->next = 0;
    if (s_top == 0)
    {
        s_top = w;
    }
    else
    {
        p = s_top;
        while (p->next != 0)
        {
            p = p->next;
        }
        p->next = w;
    }
    wvui_widget_invalidate(w);
}

void wvui_widget_set_hidden(wvui_widget_t *w, uint8_t hidden)
{
    uint8_t v;

    if (w == 0)
    {
        return;
    }
    v = (uint8_t)((hidden != 0u) ? WVUI_WFLAG_HIDDEN : 0u);
    if ((w->flags & WVUI_WFLAG_HIDDEN) == v)
    {
        return;
    }
    wvui_widget_invalidate(w); /* Old state: erase what was drawn */
    if (v != 0u)
    {
        w->flags |= WVUI_WFLAG_HIDDEN;
    }
    else
    {
        w->flags &= (uint8_t)~WVUI_WFLAG_HIDDEN;
    }
    wvui_widget_invalidate(w); /* New state: the subtree needs a redraw */
}

uint8_t wvui_widget_is_hidden(const wvui_widget_t *w)
{
    if (w == 0)
    {
        return 0u;
    }
    return (uint8_t)(((w->flags & WVUI_WFLAG_HIDDEN) != 0u) ? 1u : 0u);
}

void wvui_widget_bring_to_front(wvui_widget_t *w)
{
    wvui_widget_t *p;

    if ((w == 0) || (s_top == w))
    {
        return;
    }

    /* Bring-to-front works only inside the active set: docked pages and other containers' children are never touched. */
    if (unlink_from(s_top, w) == 0u)
    {
        return;
    }
    w->next = 0;
    if (s_top == 0)
    {
        s_top = w;
        return;
    }
    p = s_top;
    while (p->next != 0)
    {
        p = p->next;
    }
    p->next = w;
    /* Bring-to-front changes overlap relations: mark the whole area dirty once */
    wvui_widget_invalidate(w);
}

void wvui_widget_append_child(wvui_widget_t *parent, wvui_widget_t *child)
{
    wvui_widget_t **head;
    wvui_widget_t *p;

    if ((parent == 0) || (child == 0) || (WVUI_W_CLASS(parent) == 0) ||
        ((WVUI_W_CLASS(parent)->flags & WVUI_CLASS_CHILD_OWNER) == 0u))
    {
        return;
    }

    widget_unlink(child);
    child->parent = parent;
    head = wvui_widget_children(parent);
    if (*head == 0)
    {
        *head = child;
    }
    else
    {
        p = *head;
        while (p->next != 0)
        {
            p = p->next;
        }
        p->next = child;
    }
    wvui_widget_invalidate(child);
}

void wvui_widget_invalidate(wvui_widget_t *w)
{
    if ((w == 0) || (w->w <= 0) || (w->h <= 0))
    {
        return;
    }
    wvui_invalidate(wvui_widget_rect(w));
}

void wvui_widget_invalidate_rect(wvui_widget_t *w, wvui_rect_t r)
{
    if ((w == 0) || (w->w <= 0) || (w->h <= 0))
    {
        return;
    }
    /* Intersect with the widget rectangle first so no dirty region lands outside the frame; intersecting an empty rectangle stays empty and the driver discards it. */
    wvui_invalidate(wvui_rect_clip(r, wvui_widget_rect(w)));
}
