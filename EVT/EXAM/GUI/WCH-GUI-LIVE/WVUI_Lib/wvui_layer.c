/**
 * @file wvui_layer.c
 * @brief Top layer chain and modal slot.
 */

#include "wvui_layer.h"

#if WVUI_CFG_ENABLE_TOP_LAYER

static wvui_widget_t *s_layer; /* Top layer chain: head = bottom, tail = top */
static wvui_widget_t *s_modal;

wvui_widget_t *wvui_layer_first(void)
{
    return s_layer;
}

uint8_t wvui_layer_attach(wvui_widget_t *w)
{
    wvui_widget_t *p;

    if (w == 0)
    {
        return 0u;
    }
    /* A top layer object must not also be in the normal chain, otherwise it
       is drawn and hit-tested twice; detaching while not on the normal
       chain is a no-op. */
    (void)wvui_widget_detach(w);
    w->parent = 0;

    if (s_layer == 0)
    {
        s_layer = w;
    }
    else
    {
        p = s_layer;
        while (p->next != 0)
        {
            p = p->next;
        }
        p->next = w;
    }
    w->next = 0;
    wvui_widget_invalidate(w);
    return 1u;
}

void wvui_layer_detach(wvui_widget_t *w)
{
    wvui_widget_t *p;
    wvui_widget_t *prev = 0;

    if ((w == 0) || (s_layer == 0))
    {
        return;
    }
    for (p = s_layer; p != 0; p = p->next)
    {
        if (p == w)
        {
            if (prev == 0)
            {
                s_layer = p->next;
            }
            else
            {
                prev->next = p->next;
            }
            p->next = 0;
            if (s_modal == w)
            {
                s_modal = 0; /* Modal removed means slot cleared, otherwise hit testing locks onto an object off the chain */
            }
            wvui_widget_invalidate(w);
            return;
        }
        prev = p;
    }
}

void wvui_layer_unlink_widget(wvui_widget_t *w)
{
    if ((w == 0) || (s_layer == 0))
    {
        return;
    }
    wvui_layer_detach(w);
}

void wvui_layer_reset(void)
{
    wvui_widget_t *w = s_layer;

    while (w != 0)
    {
        wvui_widget_t *n = w->next;

        wvui_widget_invalidate(w); /* The area vacated by the overlay becomes dirty, flushed away by this frame */
        WVUI_W_CLEAR_CLASS(w);
        w->next = 0;
        w->parent = 0;
        w = n;
    }
    s_layer = 0;
    s_modal = 0;
}

uint8_t wvui_modal_open(wvui_widget_t *w)
{
    if (w == 0)
    {
        return 0u;
    }
    if ((s_modal != 0) && (s_modal != w))
    {
        /* Modal exclusivity: send MODAL_CLOSE to the old modal first, it
           closes itself and restores the widgets it covered. */
        wvui_event_t e = WVUI_EVENT_MODAL_CLOSE;

        if (WVUI_W_CLASS(s_modal) != 0)
        {
            if (WVUI_W_CLASS(s_modal)->event != 0)
            {
                (void)WVUI_W_CLASS(s_modal)->event(s_modal, e, 0);
            }
        }
    }
    s_modal = w;
    wvui_widget_invalidate(w);
    return 1u;
}

void wvui_modal_close(wvui_widget_t *w)
{
    if ((w == 0) || (s_modal != w))
    {
        return;
    }
    s_modal = 0;
    wvui_widget_invalidate(w);
}

wvui_widget_t *wvui_modal_get(void)
{
    return s_modal;
}

#else /* WVUI_CFG_ENABLE_TOP_LAYER == 0 */

/* With the switch cut, to-top degenerates to bring-to-front on the normal
   chain, and the modal API becomes a no-op. */
wvui_widget_t *wvui_layer_first(void)
{
    return 0;
}

uint8_t wvui_layer_attach(wvui_widget_t *w)
{
    if (w == 0)
    {
        return 0u;
    }
    wvui_widget_bring_to_front(w);
    return 1u;
}

void wvui_layer_detach(wvui_widget_t *w)
{
    (void)w;
}

void wvui_layer_unlink_widget(wvui_widget_t *w)
{
    (void)w;
}

void wvui_layer_reset(void)
{
}

uint8_t wvui_modal_open(wvui_widget_t *w)
{
    (void)w;
    return 0u;
}

void wvui_modal_close(wvui_widget_t *w)
{
    (void)w;
}

wvui_widget_t *wvui_modal_get(void)
{
    return 0;
}

#endif /* WVUI_CFG_ENABLE_TOP_LAYER */
