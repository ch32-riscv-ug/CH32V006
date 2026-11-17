/**
 * @file wvui_dirty.c
 * @brief Dirty rectangle implementation: insert-merge and per-frame statistics.
 * @note  Merge passes on insertion are bounded by WVUI_CFG_DIRTY_MERGE_MAX.
 */

#include "wvui_dirty.h"

/* Fallback cost constant: one bus transaction is about 125 pixels; the value derived by wvui_init() overrides it. */
#define WVUI_DIRTY_DEFAULT_FIXED_PX (WVUI_CFG_DIRTY_FIXED_COST_PX_FALLBACK)

static wvui_rect_t s_rects[WVUI_CFG_DIRTY_MAX_NUM];
static uint8_t s_count;
static uint32_t s_area;
static uint32_t s_merges;
static uint32_t s_forced;
static uint32_t s_fixed_px = WVUI_DIRTY_DEFAULT_FIXED_PX;

static int16_t s_w;
static int16_t s_h;
static uint8_t s_align_x = 1u;
static uint8_t s_align_y = 1u;

/* ---- Clipping and alignment ---- */
static wvui_rect_t clip_to_screen(wvui_rect_t r)
{
    if (r.x0 < 0) { r.x0 = 0; }
    if (r.y0 < 0) { r.y0 = 0; }
    if (r.x1 > (int16_t)(s_w - 1)) { r.x1 = (int16_t)(s_w - 1); }
    if (r.y1 > (int16_t)(s_h - 1)) { r.y1 = (int16_t)(s_h - 1); }
    return r;
}

/* Alignment expansion: round x0/y0 down and x1/y1 up to the align-1 boundary. */
static wvui_rect_t align_expand(wvui_rect_t r)
{
    if (s_align_x > 1u)
    {
        uint16_t m = (uint16_t)(s_align_x - 1u);
        r.x0 = (int16_t)((uint16_t)r.x0 & (uint16_t)~m);
        r.x1 = (int16_t)((uint16_t)r.x1 | m);
    }
    if (s_align_y > 1u)
    {
        uint16_t m = (uint16_t)(s_align_y - 1u);
        r.y0 = (int16_t)((uint16_t)r.y0 & (uint16_t)~m);
        r.y1 = (int16_t)((uint16_t)r.y1 | m);
    }
    return r;
}

/* ---- Bind and frame start ---- */
void wvui_dirty_bind(int16_t screen_w, int16_t screen_h, const wvui_port_t *port)
{
    s_w = screen_w;
    s_h = screen_h;
    s_align_x = (port != 0) ? port->align_x : 1u;
    s_align_y = (port != 0) ? port->align_y : 1u;
    if (s_align_x == 0u) { s_align_x = 1u; }
    if (s_align_y == 0u) { s_align_y = 1u; }
}

void wvui_dirty_reset(void)
{
    s_count = 0u;
    s_area = 0u;
    s_merges = 0u;
    s_forced = 0u;
}

void wvui_dirty_set_fixed_cost_pixels(uint32_t px)
{
    s_fixed_px = px;
}

uint32_t wvui_dirty_fixed_cost_pixels(void)
{
    return s_fixed_px;
}

uint8_t wvui_dirty_count(void)
{
    return s_count;
}

wvui_rect_t wvui_dirty_get(uint8_t index)
{
    wvui_rect_t empty;

    empty.x0 = 0;
    empty.y0 = 0;
    empty.x1 = -1;
    empty.y1 = -1;
    return (index < s_count) ? s_rects[index] : empty;
}

uint32_t wvui_dirty_total_area(void)
{
    return s_area;
}

uint32_t wvui_dirty_merges(void)
{
    return s_merges;
}

uint32_t wvui_dirty_forced_merges(void)
{
    return s_forced;
}

/* ---- Insertion ---- */
static uint8_t push_slot(wvui_rect_t r)
{
    if (s_count >= (uint8_t)WVUI_CFG_DIRTY_MAX_NUM)
    {
        return 0u;
    }
    s_rects[s_count] = r;
    s_count = (uint8_t)(s_count + 1u);
    s_area += wvui_rect_area(r);
    return 1u;
}

#if (WVUI_CFG_DIRTY_STRATEGY != 0)
static uint32_t recalc_area(void)
{
    uint8_t i;
    uint32_t a = 0u;

    for (i = 0u; i < s_count; i++)
    {
        a += wvui_rect_area(s_rects[i]);
    }
    s_area = a;
    return a;
}
#endif

#if (WVUI_CFG_DIRTY_STRATEGY == 2)
/* Pick the slot whose merged extra area is smallest; return 0xFF if there is no slot, extra area written to *extra. */
static uint8_t best_merge_slot(wvui_rect_t r, uint32_t *extra)
{
    uint8_t i;
    uint8_t best = 0xFFu;
    uint32_t best_extra = 0u;

    for (i = 0u; i < s_count; i++)
    {
        wvui_rect_t u = wvui_rect_union(s_rects[i], r);
        uint32_t ua = wvui_rect_area(u);
        uint32_t ra = wvui_rect_area(s_rects[i]) + wvui_rect_area(r);
        uint32_t e = (ua > ra) ? (ua - ra) : 0u;

        if ((best == 0xFFu) || (e < best_extra))
        {
            best = i;
            best_extra = e;
        }
    }
    *extra = best_extra;
    return best;
}

static void add_plain(wvui_rect_t r)
{
    uint8_t i;
    uint8_t slot;
    uint32_t extra = 0u;
    uint32_t passes;

    /* Already contained by some slot: drop it directly. */
    for (i = 0u; i < s_count; i++)
    {
        if (wvui_rect_contains(s_rects[i], r))
        {
            return;
        }
    }

    slot = best_merge_slot(r, &extra);

    /* Open a new slot when not full and the merged extra area >= fixed cost; overlap does not force a merge. */
    if ((s_count < (uint8_t)WVUI_CFG_DIRTY_MAX_NUM) && ((slot == 0xFFu) || (extra >= s_fixed_px)))
    {
        if (push_slot(r) != 0u)
        {
            return;
        }
    }

    if (slot == 0xFFu)
    {
        (void)push_slot(r); /* fallback when s_count == 0 */
        return;
    }

    if (extra < s_fixed_px)
    {
        s_merges++;
    }
    else
    {
        s_forced++; /* slots full: merge into the slot with the smallest extra */
    }

    s_rects[slot] = wvui_rect_union(s_rects[slot], r);

    /* Remove slots absorbed after the merge; pass bound WVUI_CFG_DIRTY_MERGE_MAX. */
    for (passes = 0u; passes < (uint32_t)WVUI_CFG_DIRTY_MERGE_MAX; passes++)
    {
        uint8_t a;
        uint8_t b;
        uint8_t absorbed = 0u;

        for (a = 0u; a < s_count; a++)
        {
            for (b = 0u; b < s_count; b++)
            {
                if ((a == b) || (s_rects[b].x0 > s_rects[b].x1))
                {
                    continue;
                }
                if (wvui_rect_contains(s_rects[a], s_rects[b]))
                {
                    /* b covered by a: fill the hole with the tail element */
                    s_rects[b] = s_rects[s_count - 1u];
                    s_count = (uint8_t)(s_count - 1u);
                    absorbed = 1u;
                    break;
                }
            }
            if (absorbed != 0u)
            {
                break;
            }
        }
        if (absorbed == 0u)
        {
            break;
        }
    }

    (void)recalc_area();
}
#endif /* WVUI_CFG_DIRTY_STRATEGY == 2 */

void wvui_dirty_add(wvui_rect_t r)
{
    wvui_rect_t q;

    if ((s_w <= 0) || (s_h <= 0))
    {
        return; /* not bound yet: drop silently */
    }

    q = clip_to_screen(r);
    if ((q.x0 > q.x1) || (q.y0 > q.y1))
    {
        return; /* fully off-screen */
    }

    q = align_expand(q); /* expansion is done at insertion, not repeated at flush */

#if (WVUI_CFG_DIRTY_STRATEGY == 0)
    /* full: one full-screen block */
    s_count = 0u;
    s_area = 0u;
    q.x0 = 0;
    q.y0 = 0;
    q.x1 = (int16_t)(s_w - 1);
    q.y1 = (int16_t)(s_h - 1);
    (void)push_slot(q);
#elif (WVUI_CFG_DIRTY_STRATEGY == 1)
    /* bbox: merge all dirty areas into one bounding box */
    if (s_count == 0u)
    {
        (void)push_slot(q);
    }
    else
    {
        s_rects[0] = wvui_rect_union(s_rects[0], q);
        (void)recalc_area();
    }
#else
    add_plain(q);
#endif
}
