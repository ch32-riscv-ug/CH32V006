#ifndef WVUI_WIDGET_H
#define WVUI_WIDGET_H

/**
 * @file wvui_widget.h
 * @brief Widget base class, class descriptor and object tree: create, geometry, hit test, dirty marking and deletion.
 * @note  An object = control header + the runtime block right after it, one arena allocation.
 */

#include <stdint.h>
#include <stddef.h> /* layout assertion for offsetof */
#include "wvui_config.h"
#include "wvui_types.h"
#include "wvui_arena.h"
#include "wvui_mem.h"        /* memory modes and allocator backends */
#include "wvui_input_port.h" /* wvui_indev_t */
#include "wvui_props.h"      /* property numbers and encoding */

/** Class flags (16-bit, remaining bits reserved); split into structural and behaviour bits. */
#define WVUI_CLASS_CHILD_OWNER (0x0001u) /**< Structural bit: first runtime field is the child list */
#define WVUI_CLASS_RESERVED2   (0x0002u) /**< Reserved */
#define WVUI_CLASS_RESERVED4   (0x0004u) /**< Reserved */

typedef struct wvui_widget_s wvui_widget_t;

/**
 * @brief Class descriptor: the callback set of one class and its rt_size.
 * @note  rt_size drives the arena allocation of the runtime block; not read when the object is held statically.
 * @note  A new behaviour bit must take a fresh bit; never occupy a structural bit.
 */
typedef struct
{
    uint16_t rt_size;  /**< Runtime block size in bytes, the arena allocation basis */

    /** Pure rendering, called once per slice; must not mark dirty, change the tree or depend on call count. */
    void (*draw)(wvui_widget_t *w);

    /** Event callback; non-zero return = consumed. Must not delete self or an ancestor: still dereferenced after return. */
    uint8_t (*event)(wvui_widget_t *w, wvui_event_t e, wvui_indev_t *d);

    /** Called once at create to fill runtime defaults from desc; geometry and id are already set, must not mark dirty or change the tree. */
    void (*init)(wvui_widget_t *w);

    /** Geometry change hook; the kernel already invalidates layout-related caches, the widget only invalidates its own. */
    void (*set_pos)(wvui_widget_t *w, int16_t x, int16_t y);

    /** Own cleanup before detach (close modals created by self); must not call wvui_widget_delete(): deleting self recurses infinitely. */
    void (*on_delete)(wvui_widget_t *w);

    /** Property write; must mark dirty when a visible property changes, return WVUI_PROP_OK / WVUI_PROP_ERR_*.
     *  in is a little-endian payload whose length is given by len; WVUI_PROP_TEXT must be NUL-terminated and outlive the widget. */
    int8_t (*set_prop)(wvui_widget_t *w, uint8_t prop, const uint8_t *in, uint16_t len);

    /** Property read, payload shares the write schema: write the value into out (capacity cap), return the written byte count or a negative error code. */
    int16_t (*get_prop)(wvui_widget_t *w, uint8_t prop, uint8_t *out, uint16_t cap);

    uint16_t flags;    /**< Flag combination, see WVUI_CLASS_* */
} wvui_class_t;

/**
 * @brief Widget descriptor: only 6 fields of geometry and identity.
 * @note  const, resident in flash, no RAM cost; array order is the z-order; type is the
 *        compile-time class table index WVUI_TYPE_* (entries trimmed by WVUI_CFG_WIDGET_*),
 *        color / text / font / value live in the runtime block.
 */
typedef struct
{
    uint16_t type; /**< Class table index WVUI_TYPE_*; 0 = invalid */
    uint16_t id;   /**< Logical ID for the command channel; 0 = unused */
    int16_t x;     /**< Top-left x in screen coordinates */
    int16_t y;     /**< Top-left y in screen coordinates */
    int16_t w;     /**< Width in pixels; <= 0 = not drawn */
    int16_t h;     /**< Height in pixels; <= 0 = not drawn */
} wvui_widget_desc_t;

/**
 * @brief Widget control header: geometry, identity, flags and two list pointers.
 * @note  Holds no desc pointer: geometry and id are copied in; page rebuilds use the descriptor array itself.
 */
struct wvui_widget_s
{
    struct wvui_widget_s *next;   /**< Z-order list of the same layer; NULL on the last node */
    struct wvui_widget_s *parent; /**< Parent container; NULL for top-level widgets */
#if !WVUI_CFG_WIDGET_COMPACT_TYPE
    const wvui_class_t *cls;      /**< Class descriptor, resident in flash */
#endif
    int16_t x, y, w, h;          /**< Absolute geometry x/y/width/height in screen coordinates */
    uint16_t id;                 /**< Logical ID; 0 = excluded from lookup */
#if WVUI_CFG_ENABLE_INPUT
    /** Touch hit-area expansion in pixels; negative shrinks, 0 = no expansion. Read only by the input layer. */
    int8_t pad_l, pad_t, pad_r, pad_b;
#endif
    uint8_t flags;               /**< WVUI_WFLAG_* bit combination */
#if WVUI_CFG_WIDGET_COMPACT_TYPE
    /** Compact mode: a class number replaces the class pointer; must be the last struct field,
     *  otherwise int16 alignment pads it larger; type is uint8, so WVUI_CFG_CLASS_MAX must not
     *  exceed 256 (larger values are truncated). */
    uint8_t type;                /**< WVUI_TYPE_*; 0 = deleted */
#endif
};

/** Widget flags (stored in alignment padding, no extra RAM). */
#define WVUI_WFLAG_HIDDEN 0x01u /**< Subtree not drawn, not hit */

/* Layout assertion: turning off input must shrink the control header (not true on 64-bit hosts, so limited to 32-bit targets). */
#if !WVUI_CFG_ENABLE_INPUT && !WVUI_CFG_WIDGET_COMPACT_TYPE && (UINTPTR_MAX == 0xFFFFFFFFu)
WVUI_STATIC_ASSERT(sizeof(wvui_widget_t) == 24,
                  "WVUI_CFG_ENABLE_INPUT=0 must shrink wvui_widget_t to 24 B on 32-bit");
#endif

/* Layout assertion: compact mode saves one pointer per object; the class table index is one byte, hence a capacity bound. */
#if WVUI_CFG_WIDGET_COMPACT_TYPE
WVUI_STATIC_ASSERT(WVUI_CFG_CLASS_MAX <= 256,
                  "WVUI_CFG_WIDGET_COMPACT_TYPE stores the class type in a uint8_t");
#if (UINTPTR_MAX == 0xFFFFFFFFu)
WVUI_STATIC_ASSERT(sizeof(wvui_widget_t) == (WVUI_CFG_ENABLE_INPUT ? 24u : 20u),
                  "WVUI_CFG_WIDGET_COMPACT_TYPE must shrink wvui_widget_t by 4 B on 32-bit");
#endif
#endif

/**
 * @brief Get the start address of the runtime block (writable); the block follows the control header.
 * @param w  widget
 * @return   pointer to the runtime block
 */
static inline void *wvui_widget_rt(wvui_widget_t *w)
{
    return (void *)((uint8_t *)w + sizeof(wvui_widget_t));
}

/**
 * @brief Get the start address of the runtime block (read-only).
 * @param w  widget
 * @return   read-only pointer to the runtime block
 */
static inline const void *wvui_widget_rt_c(const wvui_widget_t *w)
{
    return (const void *)((const uint8_t *)w + sizeof(wvui_widget_t));
}

/**
 * @brief Container convention: classes with WVUI_CLASS_CHILD_OWNER must have the child list head as their first runtime field.
 * @note  The kernel walks the child list from children_head and recurses.
 */
typedef struct
{
    wvui_widget_t *children_head; /**< Child list head; NULL when empty */
} wvui_child_owner_t;

/**
 * @brief Get the pointer to the container's child list head (address of the first runtime field); writing it changes the list.
 * @param w  container widget
 * @return   pointer to children_head
 */
static inline wvui_widget_t **wvui_widget_children(wvui_widget_t *w)
{
    return (wvui_widget_t **)wvui_widget_rt(w);
}

/* Kernel private: the class table itself, accessible only from wvui_widget.c and wvui_widgets.c; apps use wvui_class_get().
 * With WVUI_CFG_ENABLE_CLASS_BIND = 0 the table is const (in flash) and bind becomes a stub; change declaration and definition in pairs. */
#if WVUI_CFG_ENABLE_CLASS_BIND
extern const wvui_class_t *wvui_class_table[WVUI_CFG_CLASS_MAX];
#else
extern const wvui_class_t *const wvui_class_table[WVUI_CFG_CLASS_MAX];
#endif

/**
 * @brief Register a class at runtime (for custom widgets; a later bind overwrites the earlier slot).
 * @param type  class table index; 0 and out-of-range are always rejected
 * @param cls   class descriptor; NULL is always rejected
 * @return      1 = registered; 0 = invalid argument or WVUI_CFG_ENABLE_CLASS_BIND disabled
 * @note  type 0 cannot be bound: compact mode records deleted as type = 0; built-in widgets
 *        need no call; with WVUI_CFG_ENABLE_CLASS_BIND = 0 this is a stub.
 */
uint8_t wvui_class_bind(uint16_t type, const wvui_class_t *cls);

/**
 * @brief Get a class descriptor.
 * @param type  class table index WVUI_TYPE_*
 * @return      class descriptor; NULL when out of range or the slot is empty
 */
const wvui_class_t *wvui_class_get(uint16_t type);

/**
 * @brief Number of slots holding a registered class.
 * @return  Tail back-scan plus one for the last non-empty slot; 0 when all empty
 */
uint16_t wvui_class_bound_count(void);

/**
 * @brief Look up the class table directly by type (internal fast path, no bounds check).
 * @param type  class table index; the caller guarantees < WVUI_CFG_CLASS_MAX
 * @return      class descriptor; slot 0 is always NULL (= deleted)
 * @note  type is written only by wvui_widget_create*(), already bounds-checked at write time.
 */
static inline const wvui_class_t *wvui_class_at(uint16_t type)
{
    return wvui_class_table[type];
}

/**
 * @brief Unified access point for class ownership: get class / test class / mark deleted.
 * @note  Compact mode records deleted as type = 0, so bind rejects type 0; test macros
 *        null-check by themselves, the get macro does not, w must be non-NULL.
 */
#if WVUI_CFG_WIDGET_COMPACT_TYPE
#define WVUI_W_CLASS(w) (wvui_class_at((w)->type))
/* w may be the literal 0: short-circuit evaluation guarantees no dereference when w == 0. */
#define WVUI_W_IS_TYPE(w, c) \
    (((w) != 0) && (wvui_class_at(((const wvui_widget_t *)(w))->type) == (c)))
#define WVUI_W_CLEAR_CLASS(w) do { (w)->type = 0u; } while (0)
#else
#define WVUI_W_CLASS(w) ((w)->cls)
#define WVUI_W_IS_TYPE(w, c) \
    (((w) != 0) && (((const wvui_widget_t *)(w))->cls == (c)))
#define WVUI_W_CLEAR_CLASS(w) do { (w)->cls = 0; } while (0)
#endif

/**
 * @brief Bind the object memory areas (main path of the ARENA mode).
 * @param persistent  persistent area; may be NULL
 * @param transient   transient area; its cursor resets to zero when a page is left
 * @note  One entry for all three modes: STATIC / HEAP make it a no-op; call after wvui_init() and before creating widgets.
 */
void wvui_widget_bind_arena(wvui_arena_t *persistent, wvui_arena_t *transient);

/**
 * @brief Create a widget from a descriptor, appended to the tail of the top-level list.
 * @param desc        descriptor, const; the type must be in the class table
 * @param persistent  non-zero = allocate from the persistent area, 0 = from the transient area
 * @return            the widget; NULL = type trimmed or out of memory
 * @note  Fixed assembly order: fields → runtime zeroed → init → append to tail → mark dirty; on failure
 *        no out-of-bounds write happens, failure counts stay in the memory backend.
 */
wvui_widget_t *wvui_widget_create(const wvui_widget_desc_t *desc, uint8_t persistent);

/**
 * @brief Create a widget from unpacked arguments, appended to the tail of the top-level list.
 * @param type,id     class table index WVUI_TYPE_*; logical ID, 0 = excluded from lookup
 * @param x,y,w,h     top-left corner and size in screen coordinates / pixels
 * @param persistent  non-zero = allocate from the persistent area, 0 = from the transient area
 * @return            the widget; NULL = type trimmed or out of memory
 * @note  STATIC mode has no allocatable memory: this entry asserts failure and returns NULL.
 */
wvui_widget_t *wvui_widget_create_raw(uint16_t type, uint16_t id, int16_t x, int16_t y,
                                    int16_t w, int16_t h, uint8_t persistent);

/**
 * @brief Storage declaration for static objects: control header + runtime laid out contiguously, zero allocation.
 * @note  RT_T must be the wvui_*_rt_t from the widget's own header (basis of the layout assertion); the HEAP
 *        mode does not provide this macro: deletion in that mode calls WVUI_MEM_FREE().
 */
#define WVUI_WIDGET_STORAGE(name, RT_T)                                                 \
    struct wvui_wstor_##name                                                            \
    {                                                                                  \
        wvui_widget_t hdr;                                                              \
        RT_T rt;                                                                       \
    };                                                                                 \
    WVUI_STATIC_ASSERT(offsetof(struct wvui_wstor_##name, rt) == sizeof(wvui_widget_t),    \
                      "WVUI_WIDGET_STORAGE: runtime block must start right after the control header"); \
    struct wvui_wstor_##name name

/**
 * @brief Bind a descriptor onto static storage (zero allocation).
 * @param storage  storage start address, declared by WVUI_WIDGET_STORAGE
 * @param bytes    storage size in bytes; assertion fails when smaller than control header + rt_size
 * @param desc     descriptor, const
 * @return         the widget; NULL = invalid argument, storage too small or type trimmed
 * @note  HEAP mode always returns NULL: static memory cannot be handed to free.
 */
wvui_widget_t *wvui_widget_init_static(wvui_widget_t *storage, uint32_t bytes,
                                     const wvui_widget_desc_t *desc);

/**
 * @brief Macro shell of WVUI_WIDGET_STORAGE: hands sizeof(storage) to the runtime for a capacity check.
 * @param desc_ptr  const wvui_widget_desc_t *
 * @note  Assertion fails and NULL is returned when storage is too small, nothing is written.
 */
#define WVUI_WIDGET_STATIC_BIND(storage, desc_ptr) \
    wvui_widget_init_static(&(storage).hdr, (uint32_t)sizeof(storage), (desc_ptr))

/**
 * @brief Delete a widget and its descendants (single delete entry).
 * @param w  widget; NULL is a no-op
 * @note  Fixed sequence: on_delete → post-order recursion over the child list → dirty-mark erase → detach → release references → clear fields.
 * @note  ARENA mode does not return memory (waits for the cursor reset), HEAP mode releases at once; w is invalid when this returns.
 */
void wvui_widget_delete(wvui_widget_t *w);

/**
 * @brief Release kernel-side references to a widget.
 * @param w  the widget about to be destroyed
 * @note  Slots: pressed state, gesture taker, anim node, top-level list and modals; a new slot must be added here.
 */
void wvui_kernel_forget_widget(wvui_widget_t *w);

/**
 * @brief Get the head of the top-level list (the bottom-most).
 * @return  the first top-level widget; NULL = no widget
 */
wvui_widget_t *wvui_widget_first(void);

/**
 * @brief Get the total widget count.
 * @return  the count; grows and shrinks with create and delete
 */
uint32_t wvui_widget_count(void);

/**
 * @brief Find a widget by logical ID.
 * @param id  logical ID
 * @return    the first matching widget; NULL = not found
 * @note  Walks the top-level list only, does not recurse into children.
 */
wvui_widget_t *wvui_widget_find_by_id(uint16_t id);

/**
 * @brief Detach all top-level widgets.
 * @note  Last step of the transient-area reset sequence: ARENA mode only clears markers, HEAP mode deletes one by one; the top-level
 *        list is emptied together so no layer survives across scenes.
 */
void wvui_widget_reset_top(void);

/**
 * @brief Detach the whole top-level list and hand it to the caller (docked-page parking).
 * @return  the list head after detach; NULL = there was no widget
 * @note  Detaches and clears the count, no dirty mark and no cls change (decided by the page layer); invisible and unhittable until re-attached.
 */
wvui_widget_t *wvui_widget_detach_top(void);

/**
 * @brief Re-attach a parked list at the list tail (array order = z-order) and restore the count by the list length.
 * @param head  the head of the parked list; NULL is a no-op
 * @note  Requires the current top-level list to be empty (guaranteed by the page layer), otherwise the two pages' lists interfere.
 */
void wvui_widget_attach_top(wvui_widget_t *head);

/**
 * @brief Get the transient area (kernel private): NULL in STATIC / HEAP modes.
 * @return  the transient arena; wvui_page.c uses it to reset the cursor when a page is left
 */
wvui_arena_t *wvui_widget_transient_arena(void);

/**
 * @brief Change the widget position.
 * @param w  widget; NULL is a no-op
 * @param x  new top-left x in screen coordinates
 * @param y  new top-left y in screen coordinates
 * @note  Returns directly without dirty marking when the position is unchanged; when changed, erase the old position first, then mark the new one.
 * @note  Calls back cls->set_pos(); child widgets cache no hit results and need no invalidation.
 */
void wvui_widget_set_pos(wvui_widget_t *w, int16_t x, int16_t y);

/**
 * @brief Set the touch hit-area expansion.
 * @param w        widget; NULL is a no-op
 * @param l,t,r,b  expansion of the four edges in pixels; negative shrinks
 * @note  Affects hit test only, not visuals, so no dirty mark.
 * @note  No-op when WVUI_CFG_ENABLE_INPUT == 0 (fields removed).
 */
void wvui_widget_set_hit_pad(wvui_widget_t *w, int8_t l, int8_t t, int8_t r, int8_t b);

/**
 * @brief Show or hide a widget: hidden = the whole subtree is not drawn and not hit.
 * @param w       widget; NULL is a no-op
 * @param hidden  non-zero = hide, 0 = show
 * @note  No dirty mark when the state is unchanged; when changed, mark old and new once each.
 */
void wvui_widget_set_hidden(wvui_widget_t *w, uint8_t hidden);

/**
 * @brief Query the hidden flag.
 * @param w  widget; NULL counts as not hidden
 * @return   1 = hidden, 0 = shown
 */
uint8_t wvui_widget_is_hidden(const wvui_widget_t *w);

/**
 * @brief Move a widget to the end of the active set (top-most).
 * @param w  top-level widget; no-op when NULL or not in the active set
 * @note  Moves only within the active set (docked pages and other containers' children are never touched); marks the whole area dirty on success.
 */
void wvui_widget_bring_to_front(wvui_widget_t *w);

/**
 * @brief Detach a single top-level object from the list (keeps cls / id / geometry, can be re-attached).
 * @param w  widget; NULL is a no-op
 * @return   1 = detached; 0 = not in the active set
 * @note  A detached object is on no list: not drawn, not hit; this function does not mark dirty, the caller erases the old pixels.
 */
uint8_t wvui_widget_detach(wvui_widget_t *w);

/**
 * @brief Re-add an object to the end of the active set (normal layer), the inverse of wvui_widget_detach().
 * @param w  widget; NULL is a no-op
 * @note  Idempotent: does nothing when already in the active set; marks dirty once automatically after re-adding.
 */
void wvui_widget_attach_root(wvui_widget_t *w);

/**
 * @brief Append a widget to the tail of a container's child list.
 * @param parent  the container; rejected outright when not a container (no WVUI_CLASS_CHILD_OWNER)
 * @param child   the widget; first detached from its previous list and parent
 * @note  Children use absolute coordinates, joining the child list changes no geometry; after joining the kernel marks child dirty, the caller need not.
 */
void wvui_widget_append_child(wvui_widget_t *parent, wvui_widget_t *child);

/**
 * @brief Get the widget rectangle (hit area excluded).
 * @param w  widget
 * @return   bounding rectangle in screen coordinates, both ends included
 */
wvui_rect_t wvui_widget_rect(const wvui_widget_t *w);

/**
 * @brief Get the hit rectangle (hit area included).
 * @param w  widget
 * @return   screen coordinates, both ends included; equals wvui_widget_rect() without an input layer
 */
wvui_rect_t wvui_widget_hit_rect(const wvui_widget_t *w);

/**
 * @brief Point hit test, using the hit rectangle.
 * @param w  widget
 * @param x  screen coordinate
 * @param y  screen coordinate
 * @return   1 = hit, 0 = missed
 */
uint8_t wvui_widget_hit_test_point(const wvui_widget_t *w, int16_t x, int16_t y);

/**
 * @brief Mark the widget itself dirty.
 * @param w  widget; no-op when NULL or width/height <= 0
 * @note  Covers its own rectangle only; parent-chain clipping is done by the driver.
 */
void wvui_widget_invalidate(wvui_widget_t *w);

/**
 * @brief Mark a sub-rectangle of the widget dirty (intersected with the widget rectangle first).
 * @param w  widget; no-op when NULL or width/height <= 0
 * @param r  sub-rectangle in screen coordinates, both ends included
 * @note  For line-by-line dirty marking: the widget pushes only the changed line, out-of-frame parts are not handed to the driver.
 * @note  An empty rectangle (x0 > x1) produces no dirty region.
 */
void wvui_widget_invalidate_rect(wvui_widget_t *w, wvui_rect_t r);

/* Type convention (fixed at compile time): descriptor is 6 two-byte fields; structural and behaviour bits use different bits. */
WVUI_STATIC_ASSERT(sizeof(wvui_widget_desc_t) == 12, "descriptor is {type,id,x,y,w,h}");
WVUI_STATIC_ASSERT(WVUI_CLASS_CHILD_OWNER != WVUI_CLASS_RESERVED2, "structural/behaviour bits are distinct");

#endif /* WVUI_WIDGET_H */
