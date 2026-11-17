/**
 * @file wvui_scroll.c
 * @brief Single-axis scroll physics: finger tracking, overscroll damping,
 *        inertia decay and rebound.
 * @note  Shifts only: overscroll damping halves the overshoot with >> 1, inertia
 *        decays with vel -= vel >> 3, rebound steps with >> 2.
 */

#include "wvui_scroll.h"

static int32_t abs32(int32_t v)
{
    return (v < 0) ? -v : v;
}

static int32_t clamp_range(const wvui_scroll_t *s, int32_t pos)
{
    if (pos < 0)
    {
        return 0;
    }
    if (pos > s->max)
    {
        return s->max;
    }
    return pos;
}

/* Overscroll damping: halve the overshoot, capped at WVUI_CFG_SCROLL_OVERSCROLL. */
static int32_t damp_overscroll(const wvui_scroll_t *s, int32_t pos)
{
    if (pos < 0)
    {
        int32_t over = -pos;

        if (over > (int32_t)WVUI_CFG_SCROLL_OVERSCROLL)
        {
            over = (int32_t)WVUI_CFG_SCROLL_OVERSCROLL;
        }
        return -(over >> 1);
    }
    if (pos > s->max)
    {
        int32_t over = pos - s->max;

        if (over > (int32_t)WVUI_CFG_SCROLL_OVERSCROLL)
        {
            over = (int32_t)WVUI_CFG_SCROLL_OVERSCROLL;
        }
        return s->max + (over >> 1);
    }
    return pos;
}

void wvui_scroll_init(wvui_scroll_t *s, int32_t max)
{
    if (s == 0)
    {
        return;
    }
    s->pos = 0;
    s->max = (max > 0) ? max : 0;
    s->origin_pos = 0;
    s->last_pos = 0;
    s->origin_c = 0;
    s->vel = 0;
    s->tracking = 0u;
    s->dragging = 0u;
    s->animating = 0u;
}

void wvui_scroll_set_range(wvui_scroll_t *s, int32_t max)
{
    if (s == 0)
    {
        return;
    }
    s->max = (max > 0) ? max : 0;
    s->pos = clamp_range(s, s->pos);
    s->last_pos = s->pos;
    if ((s->pos == 0) || (s->pos == s->max))
    {
        s->vel = 0; /* Clear velocity at the edge, no phantom inertia */
    }
}

void wvui_scroll_press(wvui_scroll_t *s, int16_t c)
{
    if (s == 0)
    {
        return;
    }
    s->tracking = 1u;
    s->dragging = 0u;
    s->animating = 0u; /* A press stops inertia and clears velocity */
    s->vel = 0;
    s->origin_pos = s->pos;
    s->last_pos = s->pos;
    s->origin_c = c;
}

uint8_t wvui_scroll_stay(wvui_scroll_t *s, int16_t c)
{
    int32_t want;

    if ((s == 0) || (s->tracking == 0u))
    {
        return 0u;
    }

    if (s->dragging == 0u)
    {
        if (abs32((int32_t)c - (int32_t)s->origin_c) < (int32_t)WVUI_CFG_DRAG_THRESHOLD)
        {
            return 0u; /* Below threshold: belongs to the child widget */
        }
        s->dragging = 1u;
        /* Count crossing the threshold as one tracking frame, no leading motion gap */
    }

    /* Finger tracking: pos moves opposite the finger (c rising makes pos fall). */
    want = s->origin_pos - ((int32_t)c - (int32_t)s->origin_c);
    s->pos = damp_overscroll(s, want);
    return 2u;
}

int32_t wvui_scroll_take_delta(wvui_scroll_t *s)
{
    int32_t d;

    if (s == 0)
    {
        return 0;
    }
    d = s->pos - s->last_pos;
    s->last_pos = s->pos;
    /* Per-frame velocity: this frame's motion is the velocity estimate, no time
 * slicing needed. */
    if (d > 32767)
    {
        d = 32767;
    }
    if (d < -32768)
    {
        d = -32768;
    }
    s->vel = (int16_t)d;
    return d;
}

uint8_t wvui_scroll_release(wvui_scroll_t *s)
{
    uint8_t was_drag;
    uint8_t out;

    if (s == 0)
    {
        return 0u;
    }

    /* Animation is needed: dragged with non-zero vel, or already overscrolled.
     * wvui_scroll_take_delta wrote vel frame by frame during tracking, so vel here is
     * the release velocity. */
    was_drag = s->dragging;
    out = (uint8_t)((s->pos < 0) || (s->pos > s->max));

    s->tracking = 0u;
    s->dragging = 0u;

    if (((was_drag != 0u) && (s->vel != 0)) || (out != 0u))
    {
        s->animating = 1u;
        return 1u;
    }

    s->vel = 0;
    return 0u;
}

uint8_t wvui_scroll_swipe(wvui_scroll_t *s, int16_t c)
{
    int32_t d_total;
    int32_t v;

    if ((s == 0) || (s->max <= 0))
    {
        return 0u;
    }

    /* Direction: c falling = finger up = pos rising, so velocity takes the negated
 * motion. */
    d_total = (int32_t)c - (int32_t)s->origin_c;
    v = -d_total;

    if (v == 0)
    {
        return 0u;
    }
    if (v > 32767)
    {
        v = 32767;
    }
    if (v < -32768)
    {
        v = -32768;
    }

    s->vel = (int16_t)v;
    s->last_pos = s->pos;
    s->animating = 1u;
    return 1u;
}

uint8_t wvui_scroll_step(wvui_scroll_t *s)
{
    int32_t before;

    if (s == 0)
    {
        return 0u;
    }
    before = s->pos;

    if (s->pos < 0)
    {
        /* Rebound steps back a quarter per frame, at least 1 px: with overshoot < 4,
 * >> 2 yields 0 and would never converge. */
        int32_t step = (0 - s->pos) >> 2;

        if (step < 1)
        {
            step = 1;
        }
        s->pos += step;
        if (s->pos > 0)
        {
            s->pos = 0;
        }
        /* Clear velocity in the overscroll phase: it points outward and would push past
 * the boundary again. */
        s->vel = 0;
    }
    else if (s->pos > s->max)
    {
        int32_t step = (s->pos - s->max) >> 2;

        if (step < 1)
        {
            step = 1;
        }
        s->pos -= step;
        if (s->pos < s->max)
        {
            s->pos = s->max;
        }
        s->vel = 0;
    }
    else if (s->vel != 0)
    {
        int32_t v = (int32_t)s->vel;

        v -= v >> 3; /* x7/8, negative values also approach 0 */
        s->pos += (int32_t)s->vel;
        if (s->pos < 0)
        {
            s->pos = 0;
            v = 0;
        }
        else if (s->pos > s->max)
        {
            s->pos = s->max;
            v = 0;
        }
        if ((v > -(int32_t)WVUI_CFG_SCROLL_MIN_VEL) && (v < (int32_t)WVUI_CFG_SCROLL_MIN_VEL))
        {
            v = 0;
        }
        s->vel = (int16_t)v;
    }

    if (s->pos == before)
    {
        s->animating = 0u;
        return 0u;
    }
    s->animating = 1u;
    return 1u;
}

uint8_t wvui_scroll_set(wvui_scroll_t *s, int32_t pos)
{
    int32_t p;

    if (s == 0)
    {
        return 0u;
    }
    p = clamp_range(s, pos);
    if (p == s->pos)
    {
        return 0u;
    }
    s->pos = p;
    s->last_pos = p;
    s->vel = 0;
    return 1u;
}

uint8_t wvui_scroll_active(const wvui_scroll_t *s)
{
    if (s == 0)
    {
        return 0u;
    }
    return (uint8_t)((s->dragging != 0u) || (s->animating != 0u));
}

uint8_t wvui_scroll_progress(const wvui_scroll_t *s)
{
    if ((s == 0) || (s->max <= 0))
    {
        return 0u;
    }
    if (s->pos <= 0)
    {
        return 0u;
    }
    if (s->pos >= s->max)
    {
        return 255u;
    }
    return (uint8_t)((s->pos * 255) / s->max);
}
