/**
 * @file wvui_scroll_panel.c
 * @brief Single-axis scroll panel implementation.
 * @note  No transform: scrolling moves children one by one, coordinates stay absolute; thumb y
 *        is converted with a Q16 reciprocal.
 */

#include "wvui_scroll_panel.h"

#include "wvui_render.h"

#define BAR_W_DEF (2)
#define BAR_MIN_H (8)
#define BAR_COLOR WVUI_RGB565(120, 130, 140)
#define BAR_TRACK WVUI_RGB565(40, 46, 52)

static wvui_scroll_panel_rt_t *rt_of(wvui_widget_t *w)
{
    return (wvui_scroll_panel_rt_t *)wvui_widget_rt(w);
}

/* ---- Scrollbar idle fade: touch / idle wait / linear fade-out ---- */
/* touch resets the timer and re-links the node; the wait period never repaints, the fade period repaints every frame. */
/* When FADE_FRAMES == 0 this section and both call sites are compiled out. */
#if WVUI_CFG_BAR_FADE_FRAMES > 0
static uint8_t panel_bar_step(wvui_anim_t *a)
{
    wvui_widget_t *w = a->owner;
    wvui_scroll_panel_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_scroll_panel_class))
    {
        return 0u; /* type guard */
    }
    rt = rt_of(w);

    if (rt->bar_idle < (uint16_t)WVUI_CFG_BAR_FADE_DELAY)
    {
        rt->bar_idle++; /* wait period: no repaint */
        return 1u;
    }

    wvui_widget_invalidate(w); /* fading: redraw every frame */
    return wvui_ramp_step(&rt->bar_ramp);
}

/* Call on any activity: idempotent, restarts the timer on every call.
 * Do not return early just because the node is linked; new activity during the wait period must reset the timer too. */
static void panel_bar_touch(wvui_widget_t *w)
{
    wvui_scroll_panel_rt_t *rt = rt_of(w);

    if (rt->bar_h <= 0)
    {
        return; /* not scrollable: no scrollbar */
    }
    if (rt->bar_ramp.pos != 0u)
    {
        wvui_widget_invalidate(w); /* lit up again mid-fade: repaint this frame */
    }
    wvui_ramp_start(&rt->bar_ramp, WVUI_RAMP_INC(WVUI_CFG_BAR_FADE_FRAMES),
                   WVUI_CFG_BAR_FADE_FRAMES);
    rt->bar_idle = 0u;
    /* Permanent type: stays unstarted without the animation tier, pos holds at 0 (always lit). */
    wvui_anim_start_permanent(&rt->an_bar, w, panel_bar_step);
}

static void panel_bar_stop(wvui_widget_t *w)
{
    wvui_anim_stop(&rt_of(w)->an_bar);
}
#else
/* Fade compiled out: call sites are empty expressions, zero code. */
#define panel_bar_touch(w) ((void)(w))
#define panel_bar_stop(w)  ((void)(w))
#endif /* WVUI_CFG_BAR_FADE_FRAMES */

/* Content height = height of the children's bounding box relative to the viewport top.
 * Child y is the visual coordinate (pos already subtracted); adding pos back restores the design coordinate before the union. */
static int16_t measure_content(const wvui_scroll_panel_rt_t *rt, int16_t view_y0)
{
    const wvui_widget_t *c;
    int32_t bottom = 0;

    for (c = rt->children_head; c != 0; c = c->next)
    {
        int32_t b = (int32_t)c->y + (int32_t)c->h;
        if (b > bottom)
        {
            bottom = b;
        }
    }
    bottom += rt->sc.pos; /* restore the design coordinate */
    if (bottom <= (int32_t)view_y0)
    {
        return 0;
    }
    return (int16_t)(bottom - (int32_t)view_y0);
}

/* pos -> thumb y: one multiply plus shift, zero division. */
static void panel_update_bar(wvui_widget_t *w)
{
    wvui_scroll_panel_rt_t *rt = rt_of(w);
    uint32_t y;

    if (rt->bar_h <= 0)
    {
        rt->bar_y = 0;
        return;
    }
    if (rt->sc.pos <= 0)
    {
        rt->bar_y = 0;
        return;
    }
    if (rt->sc.pos >= rt->sc.max)
    {
        rt->bar_y = rt->bar_y_max;
        return;
    }

    /* (track − h) ≤ 240 => product ≤ 240<<16 ≈ 15.7M, no int32 overflow. */
    y = ((uint32_t)rt->sc.pos * rt->bar_q16) >> 16;
    if (y > (uint32_t)rt->bar_y_max)
    {
        y = (uint32_t)rt->bar_y_max;
    }
    rt->bar_y = (int16_t)y;
}

/* Scrollbar geometry: the only division point in this widget, computed once on refresh.
 * Q16 truncation puts middling positions at most 1 pixel low; pos = 0 and pos ≥ max are exact. */
static void panel_measure_bar(wvui_widget_t *w)
{
    wvui_scroll_panel_rt_t *rt = rt_of(w);
    int32_t track = (int32_t)w->h;
    int32_t span;
    int32_t h;

    rt->bar_h = 0;
    rt->bar_y = 0;
    rt->bar_y_max = 0;
    rt->bar_q16 = 0u;

    /* Both divisors are non-zero: content_h > 0 and max > 0 to reach here. */
    if ((track <= 0) || (rt->content_h <= 0) || (rt->sc.max <= 0))
    {
        return;
    }

    h = (track * track) / (int32_t)rt->content_h; /* thumb length ∝ viewport/content */
    if (h < BAR_MIN_H)
    {
        h = BAR_MIN_H;
    }
    if (h > track)
    {
        h = track;
    }

    span = track - h;
    rt->bar_h = (int16_t)h;
    rt->bar_y_max = (int16_t)span;
    rt->bar_q16 = ((uint32_t)span << 16) / (uint32_t)rt->sc.max;

    panel_update_bar(w);
}

/* Moves children, dirties the container, refreshes scrollbar geometry; no velocity tracking here.
 * The follow path takes the delta via take_delta in the caller; taking it again on the inertia path breaks convergence. */
static void panel_commit(wvui_widget_t *w, int32_t delta)
{
    wvui_scroll_panel_rt_t *rt = rt_of(w);
    wvui_widget_t *c;

    if (delta != 0)
    {
        for (c = rt->children_head; c != 0; c = c->next)
        {
            wvui_widget_set_pos(c, c->x, (int16_t)((int32_t)c->y - delta));
        }
        wvui_widget_invalidate(w);
        panel_bar_touch(w);       /* position moved: reset the fade timer */
    }
    panel_update_bar(w);
}

/* Inertia/rebound step: 0 = physically at rest, the engine removes the node; owner is this panel. */
static uint8_t panel_step(wvui_anim_t *a)
{
    wvui_widget_t *w = a->owner;
    wvui_scroll_panel_rt_t *rt;
    int32_t before;

    if (!WVUI_W_IS_TYPE(w, &wvui_scroll_panel_class))
    {
        return 0u; /* type guard */
    }
    rt = rt_of(w);
    before = rt->sc.pos;

    if (wvui_scroll_step(&rt->sc) == 0u)
    {
        return 0u;
    }
    panel_commit(w, rt->sc.pos - before);
    return 1u;
}

/* Start the anim node: idempotent, only refreshes the callback when already running. */
static void panel_start_anim(wvui_widget_t *w)
{
    wvui_anim_start(&rt_of(w)->an, w, panel_step);
}

static void panel_stop_anim(wvui_widget_t *w)
{
    wvui_anim_stop(&rt_of(w)->an);
}

static void panel_init(wvui_widget_t *w)
{
    wvui_scroll_panel_rt_t *rt = rt_of(w);

    rt->children_head = 0;
    wvui_scroll_init(&rt->sc, 0);
    wvui_anim_init(&rt->an);
#if WVUI_CFG_BAR_FADE_FRAMES > 0
    wvui_anim_init(&rt->an_bar);
    /* pos = 0 => opa = 255: the scrollbar is fully lit before any interaction, timing starts at the first activity. */
    wvui_ramp_start(&rt->bar_ramp, WVUI_RAMP_INC(WVUI_CFG_BAR_FADE_FRAMES),
                   WVUI_CFG_BAR_FADE_FRAMES);
    rt->bar_idle = 0u;
#endif
    rt->content_h = 0;
    rt->bar_w = BAR_W_DEF;
    rt->bar_show = 1u;
    rt->bar_h = 0;
    rt->bar_y = 0;
    rt->bar_y_max = 0;
    rt->bar_q16 = 0u;
}

/* Explicitly stop both anim nodes; core reclamation by owner is the fallback path. */
static void panel_on_delete(wvui_widget_t *w)
{
    panel_stop_anim(w);
    panel_bar_stop(w);
}

void wvui_scroll_panel_refresh(wvui_widget_t *w)
{
    wvui_scroll_panel_rt_t *rt;

    if (!WVUI_W_IS_TYPE(w, &wvui_scroll_panel_class))
    {
        return;
    }
    rt = rt_of(w);
    rt->content_h = measure_content(rt, w->y);
    wvui_scroll_set_range(&rt->sc, (int32_t)rt->content_h - (int32_t)w->h);
    panel_measure_bar(w);
}

static uint8_t panel_event(wvui_widget_t *w, wvui_event_t e, wvui_indev_t *d)
{
    wvui_scroll_panel_rt_t *rt = rt_of(w);

    switch (e)
    {
    case WVUI_EVENT_HIT_TEST:
        /* Everything inside the viewport is a hit (the core recurses into children first), 1 = do not veto; no transparent clipping. */
        return 1u;

    case WVUI_EVENT_PRESSED:
        panel_stop_anim(w); /* press stops inertia/rebound at once */
        panel_bar_touch(w); /* light up and restart the timer */
        wvui_scroll_press(&rt->sc, (int16_t)((d != 0) ? d->y : 0));
        return 1u; /* captured: blank areas are draggable */

    case WVUI_EVENT_DRAG_BEGIN:
        /* Take over only when the content is scrollable, otherwise yield to another ancestor (e.g. an outer scrollable panel). */
        if (rt->sc.max > 0)
        {
            panel_stop_anim(w);
            panel_bar_touch(w);
            wvui_scroll_press(&rt->sc, (int16_t)((d != 0) ? d->y : 0));
            return 1u;
        }
        return 0u;

    case WVUI_EVENT_STAY:
        if (d == 0)
        {
            return 1u;
        }
        if (wvui_scroll_stay(&rt->sc, (int16_t)d->y) != 0u)
        {
            /* Follow path: the delta is taken by take_delta, which also records this frame's
             * movement as velocity, i.e. the initial velocity of the release inertia. */
            panel_commit(w, wvui_scroll_take_delta(&rt->sc));
        }
        return 1u;

    case WVUI_EVENT_RELEASED:
        /* Release: pos untouched, only decides whether an anim node starts (wvui_frame step 4 takes the first step). */
        if (wvui_scroll_release(&rt->sc) != 0u)
        {
            panel_start_anim(w);
        }
        else
        {
            panel_update_bar(w);
        }
        return 1u;

    case WVUI_EVENT_SWIPE_UP:
    case WVUI_EVENT_SWIPE_DOWN:
        /* Flick initial velocity: this frame records velocity only and commits no movement, in step with the release inertia. */
        panel_bar_touch(w);
        if ((d != 0) && (wvui_scroll_swipe(&rt->sc, (int16_t)d->y) != 0u))
        {
            panel_start_anim(w);
        }
        return 1u;

    case WVUI_EVENT_CLICKED:
        return 1u; /* click on blank area: intercepted by the panel */

    default:
        return 0u;
    }
}

static void panel_draw(wvui_widget_t *w)
{
    wvui_scroll_panel_rt_t *rt = rt_of(w);
    wvui_rect_t r = wvui_widget_rect(w);
    wvui_rect_t bar;
    uint8_t opa = 255u;

    if ((rt->bar_show == 0u) || (rt->bar_h <= 0) || (rt->bar_w <= 0))
    {
        return; /* not scrollable: no scrollbar to draw */
    }

#if WVUI_CFG_BAR_FADE_FRAMES > 0
    opa = (uint8_t)(255u - rt->bar_ramp.pos);
    if (opa == 0u)
    {
        return; /* fully faded out: skip drawing */
    }
#endif

    /* Track + thumb as two fill calls, geometry taken from the cached bar_y / bar_h, no division.
     * With opa = 255 blend is pixel-identical to fill_rect. */
    bar.x0 = (int16_t)(r.x1 - rt->bar_w + 1);
    bar.y0 = r.y0;
    bar.x1 = r.x1;
    bar.y1 = r.y1;
    wvui_render_fill_rect_blend(bar, BAR_TRACK, opa);

    bar.x0 = (int16_t)(bar.x0 + 1);
    bar.y0 = (int16_t)(r.y0 + rt->bar_y);
    bar.y1 = (int16_t)(bar.y0 + rt->bar_h - 1);
    wvui_render_fill_rect_blend(bar, BAR_COLOR, opa);
}

const wvui_class_t wvui_scroll_panel_class = {
    .rt_size = (uint16_t)sizeof(wvui_scroll_panel_rt_t),
    .draw = panel_draw,
    .event = panel_event,
    .init = panel_init,
    .set_pos = 0,
    .on_delete = panel_on_delete,
    /* Property access not open for now: scrolling is driven by gestures and wvui_scroll_panel_set_scroll(). */
    .set_prop = 0,
    .get_prop = 0,
    /* Structural bit: the first runtime field is children_head */
    .flags = WVUI_CLASS_CHILD_OWNER,
};

void wvui_scroll_panel_set_scroll(wvui_widget_t *w, int32_t pos)
{
    wvui_scroll_panel_rt_t *rt;
    int32_t before;

    if (!WVUI_W_IS_TYPE(w, &wvui_scroll_panel_class))
    {
        return;
    }
    rt = rt_of(w);
    /* Programmatic positioning lands at once: stop inertia/rebound first, otherwise the next frame pulls it back onto a converging path. */
    panel_stop_anim(w);
    before = rt->sc.pos;
    if (wvui_scroll_set(&rt->sc, pos) != 0u)
    {
        /* Movement computed as a before/after difference, not take_delta: programmatic positioning leaves no inertia velocity. */
        panel_commit(w, rt->sc.pos - before);
    }
}

int32_t wvui_scroll_panel_scroll(const wvui_widget_t *w)
{
    if (!WVUI_W_IS_TYPE(w, &wvui_scroll_panel_class))
    {
        return 0;
    }
    return ((const wvui_scroll_panel_rt_t *)wvui_widget_rt_c(w))->sc.pos;
}
