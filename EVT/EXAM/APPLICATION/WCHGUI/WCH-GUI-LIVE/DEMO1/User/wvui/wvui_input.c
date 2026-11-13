/**
 * @file wvui_input.c
 * @brief Input layer implementation: hit test, press capture, gesture takeover and swipe synthesis.
 * @note  The port only reports {x, y, pressed}; the whole press sequence is judged in this file.
 */

#include "wvui_input.h"

#include "wvui_layer.h"  /* Top layer chain + modal slot */
#include "wvui_types.h"

#if WVUI_CFG_ENABLE_INPUT

static wvui_indev_t s_indev;     /* Sample of this frame, read-only in callbacks */
static wvui_widget_t *s_pressed; /* Holder: atomic widget or grabber */
static wvui_widget_t *s_grab;    /* Grabber; NULL = not taken over */
static uint8_t s_was_pressed;
static uint8_t s_had_stay;    /* Whether STAY happened in this sequence */
static uint8_t s_drag_asked;  /* Whether DRAG_BEGIN was already asked */
static int16_t s_press_x;     /* Press start point */
static int16_t s_press_y;
static wvui_input_stats_t s_stats;

/* Stat increments: compiled out entirely when WVUI_CFG_ENABLE_RENDER_STATS == 0 */
#if WVUI_CFG_ENABLE_RENDER_STATS
#define WVUI_ISTAT(code) do { code; } while (0)
#else
#define WVUI_ISTAT(code) ((void)0)
#endif

/* ---- Helpers ---- */
static int16_t abs16(int16_t v)
{
    return (int16_t)((v < 0) ? -v : v);
}

static uint8_t point_in_rect(wvui_rect_t r, int16_t x, int16_t y)
{
    return (uint8_t)((x >= r.x0) && (x <= r.x1) && (y >= r.y0) && (y <= r.y1));
}

static uint8_t has_children(const wvui_widget_t *w)
{
    return (uint8_t)((WVUI_W_CLASS(w) != 0) && ((WVUI_W_CLASS(w)->flags & WVUI_CLASS_CHILD_OWNER) != 0u));
}

static uint8_t send_event(wvui_widget_t *w, wvui_event_t e)
{
    if ((w == 0) || (WVUI_W_CLASS(w) == 0) || (WVUI_W_CLASS(w)->event == 0))
    {
        return 0u;
    }
    return WVUI_W_CLASS(w)->event(w, e, &s_indev);
}

/* ---- Hit test ---- */
/* clip is the intersection of ancestor visible areas; a child scrolled out of its
 * container never hits, even if its coordinates fall on screen. */
static wvui_widget_t *hit_test_tree(wvui_widget_t *w, int16_t x, int16_t y, wvui_rect_t clip)
{
    wvui_rect_t mine;
    wvui_widget_t *best = 0;

    mine = wvui_widget_hit_rect(w);
    clip = wvui_rect_clip(clip, mine);
    if ((clip.x0 > clip.x1) || (clip.y0 > clip.y1))
    {
        return 0;
    }
    if (point_in_rect(clip, x, y) == 0u)
    {
        return 0;
    }
    if (WVUI_W_CLASS(w) == 0)
    {
        return 0;
    }
    /* Hidden = the whole subtree misses; this check precedes container veto and
     * skips HIT_TEST as well. */
    if ((w->flags & WVUI_WFLAG_HIDDEN) != 0u)
    {
        return 0;
    }

    /* Container veto: return 0 = skip the whole subtree; decorative widgets also
     * return 0 at the press stage to keep bubbling. */
    if (has_children(w) != 0u)
    {
        if ((WVUI_W_CLASS(w)->event != 0) && (send_event(w, WVUI_EVENT_HIT_TEST) == 0u))
        {
            return 0;
        }
    }

#if WVUI_CFG_ENABLE_NESTED_INPUT
    if (has_children(w) != 0u)
    {
        wvui_widget_t *c;
        /* Child chain order = z-order; the last hit wins, children take
         * priority over the container itself. */
        for (c = *wvui_widget_children(w); c != 0; c = c->next)
        {
            wvui_widget_t *h = hit_test_tree(c, x, y, clip);
            if (h != 0)
            {
                best = h;
            }
        }
    }
#endif

    return (best != 0) ? best : w;
}

wvui_widget_t *wvui_input_hit_test(int16_t x, int16_t y)
{
    wvui_widget_t *w;
    wvui_widget_t *hit = 0;
    wvui_rect_t screen;

    /* Off-screen coordinates never hit; the port only reports coordinates
     * within the valid range. */
    screen.x0 = 0;
    screen.y0 = 0;
    screen.x1 = 32767;
    screen.y1 = 32767;

    /* While a modal is active, the whole normal-layer chain is excluded; the
     * modal slot must be read before testing the normal layers. */
    {
        wvui_widget_t *m = wvui_modal_get();

        if (m == 0)
        {
            /* Top layer chain z-order = join order, the last hit wins. */
            for (w = wvui_widget_first(); w != 0; w = w->next)
            {
                wvui_widget_t *h = hit_test_tree(w, x, y, screen);
                if (h != 0)
                {
                    hit = h;
                }
            }
        }

        /* The top layer chain is tested after the normal layers, top wins at
         * equal depth; a miss on the modal counts as hitting the modal object
         * itself (hit area = full screen), dismissing on outside tap is left
         * to the widget. */
        for (w = wvui_layer_first(); w != 0; w = w->next)
        {
            wvui_widget_t *h = hit_test_tree(w, x, y, screen);
            if (h != 0)
            {
                hit = h;
            }
        }
        if (m != 0)
        {
            return (hit != 0) ? hit : m;
        }
    }
    return hit;
}

/* ---- Press capture ---- */
/* Bubble up the parent chain from the deepest hit object; the first non-zero
 * return is the holder. */
static void dispatch_pressed(wvui_widget_t *hit)
{
    wvui_widget_t *w = hit;
    uint32_t depth = 0u;

    s_pressed = 0;
    while (w != 0)
    {
        depth++;
        if (send_event(w, WVUI_EVENT_PRESSED) != 0u)
        {
            s_pressed = w;
            break;
        }
        w = w->parent;
    }

    WVUI_ISTAT(s_stats.presses++);
    WVUI_ISTAT(s_stats.bubble_depth += depth);
    WVUI_ISTAT(s_stats.last_pressed_id = (s_pressed != 0) ? s_pressed->id : 0u);
    WVUI_ISTAT(s_stats.last_hit_id = (hit != 0) ? hit->id : 0u);
}

/* ---- Gesture takeover ---- */

/* On takeover the original holder gets one RELEASED and no CLICKED; the
 * grabber gets one PRESSED with the press start point as coordinates. */
static void grab_gesture(wvui_widget_t *w)
{
    wvui_indev_t saved;

    if ((s_grab == w) || (w == 0))
    {
        return;
    }

    if ((s_pressed != 0) && (s_pressed != w))
    {
        (void)send_event(s_pressed, WVUI_EVENT_RELEASED);
    }

    s_grab = w;
    s_pressed = w;
    WVUI_ISTAT(s_stats.drag_grabs++);
    WVUI_ISTAT(s_stats.last_grab_id = w->id);

    saved = s_indev;
    s_indev.x = s_press_x;
    s_indev.y = s_press_y;
    (void)send_event(w, WVUI_EVENT_PRESSED);
    s_indev = saved;
}

uint8_t wvui_indev_grab(wvui_widget_t *w)
{
    if (w == 0)
    {
        return 0u;
    }
    if (s_grab == w)
    {
        return 0u; /* Already the grabber: idempotent */
    }
    grab_gesture(w);
    return 1u;
}

/* Ask ancestors only, not self (no nested scrolling). */
static void ask_drag_begin(void)
{
    wvui_widget_t *w;

    s_drag_asked = 1u;
    WVUI_ISTAT(s_stats.drag_asks++);

    w = (s_pressed != 0) ? s_pressed->parent : 0;
    while (w != 0)
    {
        if (send_event(w, WVUI_EVENT_DRAG_BEGIN) != 0u)
        {
            grab_gesture(w); /* Same as calling grab inside the callback */
            return;
        }
        w = w->parent;
    }
}

/* Whether the movement crosses the drag threshold (either axis). */
static uint8_t over_drag_threshold(void)
{
    return (uint8_t)((abs16((int16_t)(s_indev.x - s_press_x)) >= (int16_t)WVUI_CFG_DRAG_THRESHOLD) ||
                     (abs16((int16_t)(s_indev.y - s_press_y)) >= (int16_t)WVUI_CFG_DRAG_THRESHOLD));
}

/* ---- Swipe synthesis ---- */
static void dispatch_swipe(wvui_widget_t *w)
{
    int16_t dx = (int16_t)(s_indev.x - s_press_x);
    int16_t dy = (int16_t)(s_indev.y - s_press_y);
    wvui_event_t e;

    if (abs16(dx) >= abs16(dy))
    {
        if (abs16(dx) < (int16_t)WVUI_CFG_SWIPE_THRESHOLD)
        {
            return;
        }
        e = (wvui_event_t)((dx > 0) ? WVUI_EVENT_SWIPE_RIGHT : WVUI_EVENT_SWIPE_LEFT);
    }
    else
    {
        if (abs16(dy) < (int16_t)WVUI_CFG_SWIPE_THRESHOLD)
        {
            return;
        }
        e = (wvui_event_t)((dy > 0) ? WVUI_EVENT_SWIPE_DOWN : WVUI_EVENT_SWIPE_UP);
    }

    (void)send_event(w, e);
    WVUI_ISTAT(s_stats.swipes++);
}

/* The release counts as a click only if the release point is still in the
 * holder's subtree (self included). Capture is produced via bubbling, so the
 * release hit may be a decorative widget: walk the parent chain back without
 * comparing objects or replaying callbacks. Sliding out to blank container
 * space is off the parent chain and counts as drift. */
static uint8_t still_on_holder(wvui_widget_t *hit, wvui_widget_t *holder)
{
    wvui_widget_t *p;

    for (p = hit; p != 0; p = p->parent)
    {
        if (p == holder)
        {
            return 1u;
        }
    }
    return 0u;
}

/* ---- Release ---- */
static void dispatch_released(void)
{
    wvui_widget_t *w = s_pressed;
    wvui_widget_t *hit;

    if (w == 0)
    {
        return;
    }

    (void)send_event(w, WVUI_EVENT_RELEASED);

    if (s_grab == 0)
    {
        hit = wvui_input_hit_test(s_indev.x, s_indev.y);
        if (still_on_holder(hit, w) != 0u)
        {
            (void)send_event(w, WVUI_EVENT_CLICKED);
        }
        else if (s_had_stay == 0u)
        {
            /* Only a quick slide with no STAY counts as a swipe */
            dispatch_swipe(w);
        }
    }

    s_pressed = 0;
    s_grab = 0;
}

/* ---- Public API ---- */
void wvui_input_reset(void)
{
    s_indev.x = 0;
    s_indev.y = 0;
    s_indev.pressed = 0u;
    s_pressed = 0;
    s_grab = 0;
    s_was_pressed = 0u;
    s_had_stay = 0u;
    s_drag_asked = 0u;
    s_press_x = 0;
    s_press_y = 0;
}

wvui_widget_t *wvui_input_pressed(void)
{
    return s_pressed;
}

wvui_widget_t *wvui_input_grab(void)
{
    return s_grab;
}

const wvui_input_stats_t *wvui_input_stats(void)
{
    return &s_stats;
}

void wvui_input_stats_reset(void)
{
    uint32_t i;
    uint8_t *p = (uint8_t *)&s_stats;

    for (i = 0u; i < (uint32_t)sizeof(s_stats); i++)
    {
        p[i] = 0u;
    }
}

void wvui_input_forget(wvui_widget_t *w)
{
    if (s_pressed == w)
    {
        /* Holder deleted: the press state is voided, no RELEASED is sent
         * (the object is already destructed). */
        s_pressed = 0;
        s_was_pressed = 0u;
        s_had_stay = 0u;
    }
    if (s_grab == w)
    {
        s_grab = 0;
    }
}

void wvui_input_feed(const wvui_indev_t *in)
{
    if (in == 0)
    {
        return;
    }

    s_indev = *in;

    if (s_indev.pressed != 0u)
    {
        if (s_was_pressed == 0u)
        {
            /* Press down edge: hit -> bubble capture. */
            wvui_widget_t *hit = wvui_input_hit_test(s_indev.x, s_indev.y);

            s_press_x = s_indev.x;
            s_press_y = s_indev.y;
            s_had_stay = 0u;
            s_drag_asked = 0u;
            s_grab = 0;
            dispatch_pressed(hit);
        }
        else if (s_pressed != 0)
        {
            /* Hold: ask DRAG_BEGIN first, then send STAY. */
            if ((s_drag_asked == 0u) && (over_drag_threshold() != 0u))
            {
                ask_drag_begin();
            }
            s_had_stay = 1u;
            (void)send_event(s_pressed, WVUI_EVENT_STAY);
        }
        s_was_pressed = 1u;
    }
    else
    {
        if (s_was_pressed != 0u)
        {
            dispatch_released();
        }
        s_was_pressed = 0u;
    }
}

#else /* !WVUI_CFG_ENABLE_INPUT: the whole input layer is compiled out */

/* Stubs: signatures match wvui_input.h, call sites need no #if; hit test
 * always returns NULL and stat counters stay zero. */

const wvui_input_stats_t *wvui_input_stats(void)
{
    static const wvui_input_stats_t s_zero;
    return &s_zero;
}

void wvui_input_stats_reset(void) {}
void wvui_input_reset(void) {}
void wvui_input_feed(const wvui_indev_t *in) { (void)in; }
wvui_widget_t *wvui_input_pressed(void) { return 0; }
wvui_widget_t *wvui_input_grab(void) { return 0; }
uint8_t wvui_indev_grab(wvui_widget_t *w) { (void)w; return 0u; }
void wvui_input_forget(wvui_widget_t *w) { (void)w; }
wvui_widget_t *wvui_input_hit_test(int16_t x, int16_t y) { (void)x; (void)y; return 0; }

#endif /* WVUI_CFG_ENABLE_INPUT */
