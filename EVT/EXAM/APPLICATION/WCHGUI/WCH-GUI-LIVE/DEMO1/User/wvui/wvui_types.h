#ifndef WVUI_TYPES_H
#define WVUI_TYPES_H

/**
 * @file wvui_types.h
 * @brief Basic types: end-point inclusive rectangles, RGB565 color, event codes.
 * @note  Conventions are frozen, add-only; depends only on wvui_config.h
 *        and <stdint.h>.
 */

#include <stdint.h>
#include "wvui_config.h"

/**
 * @brief End-point inclusive rectangle: x0/y0 and x1/y1 both belong to it.
 * @note  Coordinates are int16; scrolling position and offsets use int32,
 *        int16 would truncate at the upper limit.
 */
typedef struct
{
    int16_t x0;  /**< Left edge, inclusive */
    int16_t y0;  /**< Top edge, inclusive */
    int16_t x1;  /**< Right edge, inclusive; x0 > x1 means empty */
    int16_t y1;  /**< Bottom edge, inclusive; y0 > y1 means empty */
} wvui_rect_t;

/** @brief Get the width of an end-point inclusive rectangle. @return x1 - x0 + 1; <= 0 when empty */
static inline int16_t wvui_rect_w(const wvui_rect_t *r) { return (int16_t)(r->x1 - r->x0 + 1); }
/** @brief Get the height of an end-point inclusive rectangle. @return y1 - y0 + 1; <= 0 when empty */
static inline int16_t wvui_rect_h(const wvui_rect_t *r) { return (int16_t)(r->y1 - r->y0 + 1); }
/** @brief Test for an empty rectangle. @return Non-0 = empty (x0 > x1 or y0 > y1) */
static inline uint8_t wvui_rect_is_empty(const wvui_rect_t *r) { return (uint8_t)((r->x0 > r->x1) || (r->y0 > r->y1)); }

/**
 * @brief Get the rectangle area, end points included.
 * @return Pixel count = width * height; an empty rectangle returns 0
 */
static inline uint32_t wvui_rect_area(wvui_rect_t r)
{
    if ((r.x0 > r.x1) || (r.y0 > r.y1)) { return 0u; }
    return (uint32_t)(r.x1 - r.x0 + 1) * (uint32_t)(r.y1 - r.y0 + 1);
}

/**
 * @brief Test whether inner lies fully inside outer, end points included.
 * @return Non-0 = fully contained
 */
static inline uint8_t wvui_rect_contains(wvui_rect_t outer, wvui_rect_t inner)
{
    return (uint8_t)((inner.x0 >= outer.x0) && (inner.x1 <= outer.x1) &&
                     (inner.y0 >= outer.y0) && (inner.y1 <= outer.y1));
}

/**
 * @brief Test whether two rectangles intersect; touching edges count as intersecting.
 * @return Non-0 = intersecting
 */
static inline uint8_t wvui_rect_intersect(wvui_rect_t a, wvui_rect_t b)
{
    return (uint8_t)((a.x0 <= b.x1) && (b.x0 <= a.x1) && (a.y0 <= b.y1) && (b.y0 <= a.y1));
}

/**
 * @brief Union of two rectangles: the smallest rectangle covering both.
 * @return The union; no empty check, coordinates of empty rectangles
 *         still take part in min/max
 */
static inline wvui_rect_t wvui_rect_union(wvui_rect_t a, wvui_rect_t b)
{
    wvui_rect_t u;
    u.x0 = (a.x0 < b.x0) ? a.x0 : b.x0;
    u.y0 = (a.y0 < b.y0) ? a.y0 : b.y0;
    u.x1 = (a.x1 > b.x1) ? a.x1 : b.x1;
    u.y1 = (a.y1 > b.y1) ? a.y1 : b.y1;
    return u;
}

/**
 * @brief Intersection of two rectangles.
 * @return The intersection; an empty rectangle (x0 > x1 or y0 > y1) when
 *         they do not overlap
 */
static inline wvui_rect_t wvui_rect_clip(wvui_rect_t a, wvui_rect_t b)
{
    wvui_rect_t c;
    c.x0 = (a.x0 > b.x0) ? a.x0 : b.x0;
    c.y0 = (a.y0 > b.y0) ? a.y0 : b.y0;
    c.x1 = (a.x1 < b.x1) ? a.x1 : b.x1;
    c.y1 = (a.y1 < b.y1) ? a.y1 : b.y1;
    return c;
}

/** @brief Device-native 16-bit RGB565; color depth is switched only here; full-screen flush size = width * height * 2. */
typedef uint16_t wvui_color_t;

/** Pack 8-bit components into RGB565; inputs may be runtime values, only the high bits of each are kept. */
#define WVUI_RGB565(r, g, b)                                    \
    ((wvui_color_t)((((uint16_t)(r) & 0xF8u) << 8) |            \
                   (((uint16_t)(g) & 0xFCu) << 3) |            \
                   (((uint16_t)(b) & 0xF8u) >> 3)))

/**
 * @brief Event codes: segmented encoding, a single event_cb receives all segments.
 * @note  0x00..0x0F touch/gesture/query, 0x40..0x7F widgets and custom;
 *        custom codes must be < 0x80.
 */
typedef uint16_t wvui_event_t;

enum
{
    WVUI_EVENT_PRESSED = 0x00,   /**< Pressed */
    WVUI_EVENT_RELEASED = 0x01,  /**< Released */
    WVUI_EVENT_CLICKED = 0x02,   /**< Clicked: released on the same object */
    WVUI_EVENT_STAY = 0x03,      /**< Held down, synthesized by the kernel */
    WVUI_EVENT_VALUE_CHG = 0x04, /**< Value changed */
    WVUI_EVENT_SCROLLED = 0x05,  /**< Forced to scroll externally */
    WVUI_EVENT_SWIPE_LEFT = 0x06,
    WVUI_EVENT_SWIPE_RIGHT = 0x07,
    WVUI_EVENT_SWIPE_UP = 0x08,
    WVUI_EVENT_SWIPE_DOWN = 0x09,

    /** Kernel query segment: sent only to composite containers, leaf widgets never see it. */
    WVUI_EVENT_HIT_TEST = 0x0A,   /**< Returning 0 = skip this subtree */
    WVUI_EVENT_DRAG_BEGIN = 0x0B, /**< Returning non-0 = take over the gesture */
    WVUI_EVENT_MODAL_CLOSE = 0x0C, /**< Modal closing, collapse yourself upon receipt */

    WVUI_EVENT_USER_BASE = 0x40
};

/**
 * @brief Type conventions: fixed at compile time; including this file fails
 *        the build if they are broken.
 * @note  Assert messages are ASCII only: under non-UTF-8 locales they would
 *        print as octal escapes and become unreadable in the field.
 */
WVUI_STATIC_ASSERT(sizeof(wvui_event_t) == 2, "wvui_event_t must be uint16");
WVUI_STATIC_ASSERT(sizeof(wvui_color_t) == 2, "wvui_color_t is native 16-bit RGB565");
WVUI_STATIC_ASSERT(sizeof(wvui_rect_t) == 8, "wvui_rect_t is 4x int16, end-points inclusive");
WVUI_STATIC_ASSERT(WVUI_EVENT_MODAL_CLOSE <= 0x0F, "touch/query codes must stay in 0x00..0x0F");
WVUI_STATIC_ASSERT(WVUI_EVENT_USER_BASE >= 0x30, "user codes must not collide with the freed 0x10..0x2F range");
WVUI_STATIC_ASSERT(WVUI_EVENT_USER_BASE == 0x40, "WVUI_EVENT_USER_BASE must be 0x40");
WVUI_STATIC_ASSERT(WVUI_EVENT_USER_BASE + 0x3F == 0x7F, "user events must end at 0x7F (0x80 is the release flag)");

#endif /* WVUI_TYPES_H */
