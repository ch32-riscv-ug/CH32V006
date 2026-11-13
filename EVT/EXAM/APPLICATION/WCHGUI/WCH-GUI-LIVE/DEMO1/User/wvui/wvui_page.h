#ifndef WVUI_PAGE_H
#define WVUI_PAGE_H

/**
 * @file wvui_page.h
 * @brief Page system: descriptor array + active set; page switches run at frame start.
 * @note  When enabled, the top-level object chain belongs to pages: do not create top-level widgets manually outside pages.
 */

#include <stdint.h>

#include "wvui_config.h"
#include "wvui_widget.h"

typedef struct wvui_page_s wvui_page_t;

/**
 * @brief Page lifecycle callback prototype.
 * @param self  The page itself; keep application state in file-level statics
 * @note  on_enter runs after objects are created and geometry is set; it may mark dirty and modify the tree, a page switch inside it only sets pending.
 * @note  Objects and geometry are still readable in on_leave; on_tick runs once per frame, current page only, and callbacks must not spend noticeable time.
 */
typedef void (*wvui_page_fn)(const wvui_page_t *self);

/**
 * @brief Page descriptor.
 * @note  pinned = 1: built in the resident arena; switching only unlinks the chain and relinks on re-entry, no allocation.
 * @note  pinned = 0: bump-allocated from the temporary arena on entry, destroyed in place on leave with the cursor zeroed; parked page objects are not in the active set and are invisible to wvui_widget_find_by_id() and wvui_widget_count().
 */
struct wvui_page_s
{
    const wvui_widget_desc_t *descs; /**< Page object table, usually const and resident in flash */
    uint16_t count;                 /**< Table length, number of descs entries */
    uint8_t pinned;                 /**< 1 = resident page; 0 = stateless page */
    uint8_t flags;                  /**< Kernel-private: WVUI_PAGE_F_* bits */

    wvui_page_fn on_enter;           /**< Called after entry; 0 = none */
    wvui_page_fn on_leave;           /**< Called before leave; 0 = none */
    wvui_page_fn on_tick;            /**< Once per frame, current page only; 0 = none */

    wvui_widget_t *head; /**< Kernel-private: head of this page's object chain, saved here while parked */

    struct wvui_page_s *parked_next; /**< Kernel-private: next entry in the parked registration chain; non-NULL means listed */
};

/** flags bits (kernel-private). */
#define WVUI_PAGE_F_BUILD_FAILED (0x01u) /**< Set after a failed page build, no retry */
#define WVUI_PAGE_F_PARKED       (0x02u) /**< On the parked registration chain */

/**
 * @brief Switch to a page without stack push.
 * @param p  Target page; only sets pending, applied by wvui_page_apply() at next frame start, invalid argument is ignored
 */
void wvui_page_go(wvui_page_t *p);

/**
 * @brief Push onto the stack and enter the target page.
 * @param p  Target page; descs must not be NULL when count > 0
 * @return 1 = pending set; 0 = invalid argument or stack full (pending not set then, to avoid a half-switched state at frame start)
 */
uint8_t wvui_page_push(wvui_page_t *p);

/**
 * @brief Go back to the previous page; for a stateless page, back is a rebuild not a restore.
 * @return 1 = pending set; 0 = only the current page on the stack, nowhere to go
 */
uint8_t wvui_page_back(void);

/**
 * @brief Scene reset: clear the page stack and build the first page immediately.
 * @param p  First page; descs must not be NULL when count > 0; skips pending and does not mark the whole screen dirty, the caller pushes its own screen right after
 * @return 1 = built; 0 = invalid argument (no current page in that case)
 */
uint8_t wvui_page_reset(wvui_page_t *p);

/**
 * @brief Apply pending page switches at the frame boundary.
 * @return 1 = a page switch actually happened this frame; 0 = no pending switch
 */
uint8_t wvui_page_apply(void);

/**
 * @brief Leave the current page and clear the page stack, without marking the whole screen dirty.
 * @note  Kernel-private, used by scene reset; the caller pushes its own full screen right after, orphan widgets are cleared there.
 */
void wvui_page_leave(void);

/**
 * @brief Call on_tick of the current page.
 * @note  wvui_frame() calls it once after wvui_page_apply().
 */
void wvui_page_tick(void);

/**
 * @brief Get the current page.
 * @return Current page; NULL = wvui_page_reset() never called
 */
const wvui_page_t *wvui_page_current(void);

/**
 * @brief Get the page with a pending switch.
 * @return Non-NULL = it will be switched to at frame start; NULL = no pending switch
 */
const wvui_page_t *wvui_page_pending(void);

/**
 * @brief Get the page stack depth.
 * @return 1 = only the current page; 0 = no current page yet
 */
uint16_t wvui_page_depth(void);

/**
 * @brief Get the page layer opacity multiplier.
 * @return 255 = unchanged (no switch or fade-in finished); < 255 = fading in
 * @note  The top layer chain survives across pages and does not fade; always returns 255 when WVUI_CFG_PAGE_FADE_FRAMES = 0.
 */
uint8_t wvui_page_opa(void);

/**
 * @brief Enable or disable page-switch fade-in.
 * @param on  0 = hard cut; 1 = fade in per WVUI_CFG_PAGE_FADE_FRAMES (default)
 * @note  If disabled while fading in, opacity is restored to 255 immediately and the whole screen is marked dirty once.
 */
void wvui_page_set_fade(uint8_t on);

/**
 * @brief Check whether page-switch fade-in is enabled.
 * @return 1 = fade-in; 0 = hard cut
 */
uint8_t wvui_page_fade_enabled(void);

/**
 * @brief Unlink a widget from a parked page object chain, including the chain-head case.
 * @param w  Widget to unlink
 * @return 1 = unlinked; 0 = not on any parked chain; must be called on widget deletion, missing it leaves a dangling pointer
 */
uint8_t wvui_page_unlink_widget(wvui_widget_t *w);

/**
 * @brief Stop all anim nodes on a page chain, recursive child widgets included.
 * @param head  Page chain head; NULL = no action
 * @note  Cold path, O(child widgets x chain length); called when leaving a resident page, invisible animations must not keep running.
 */
void wvui_page_stop_anims(wvui_widget_t *head);

#endif /* WVUI_PAGE_H */
