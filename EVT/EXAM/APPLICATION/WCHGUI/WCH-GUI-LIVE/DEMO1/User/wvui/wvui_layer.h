#ifndef WVUI_LAYER_H
#define WVUI_LAYER_H

/**
 * @file wvui_layer.h
 * @brief Top layer chain and modal slot: entry point for cross-page overlays and modals.
 * @note  The top layer chain is drawn after the normal layer, z order is on
 *        top; hit testing checks the top layer first, and only the modal
 *        when one is open.
 * @note  Top layer objects belong to no page and survive page switches, so
 *        they must be allocated in persistent memory.
 */

#include "wvui_widget.h"

/**
 * @brief Append w to the end of the top layer chain (topmost), detaching it from its previous list first.
 * @param w  Widget to add
 * @return 1 = added; 0 = invalid argument; degenerates to bring_to_front()
 *         when WVUI_CFG_ENABLE_TOP_LAYER == 0
 */
uint8_t wvui_layer_attach(wvui_widget_t *w);

/**
 * @brief Remove w from the top layer chain; w stays valid and may be added again.
 * @param w  Widget to remove
 * @note  If it is the current modal, the modal slot is cleared as well.
 */
void wvui_layer_detach(wvui_widget_t *w);

/**
 * @brief Get the head of the top layer chain.
 * @return The bottommost top layer object; NULL = chain is empty
 */
wvui_widget_t *wvui_layer_first(void);

/**
 * @brief Clear the top layer chain and modal slot when an object is deleted.
 * @param w  Widget to clear
 */
void wvui_layer_unlink_widget(wvui_widget_t *w);

/**
 * @brief Empty the top layer chain and modal slot without deleting objects;
 *        the caller deletes them or they are reclaimed with the temporary
 *        arena reset.
 */
void wvui_layer_reset(void);

/**
 * @brief Open a modal: from then on hit testing is restricted to the top layer.
 * @param w  Modal object; opening a new one first sends WVUI_EVENT_MODAL_CLOSE
 *           to the old modal
 * @return 1 = opened; 0 = invalid argument, or a no-op when
 *         WVUI_CFG_ENABLE_TOP_LAYER == 0
 */
uint8_t wvui_modal_open(wvui_widget_t *w);

/**
 * @brief Close the modal.
 * @param w  Modal object; no action if it is not the current modal
 */
void wvui_modal_close(wvui_widget_t *w);

/**
 * @brief Get the current modal.
 * @return The modal object; NULL = no modal
 */
wvui_widget_t *wvui_modal_get(void);

#endif /* WVUI_LAYER_H */
