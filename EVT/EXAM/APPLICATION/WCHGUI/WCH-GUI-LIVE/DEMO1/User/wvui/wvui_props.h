#ifndef WVUI_PROPS_H
#define WVUI_PROPS_H

/**
 * @file wvui_props.h
 * @brief Object properties: write targets of object-level OPs on the command channel (id -> property number, semantics per class).
 * @note  WVUI_PROP_TEXT stores a pointer only, no copy: the source string must
 *        be NUL-terminated and outlive the widget.
 */

#include <stdint.h>

#include "wvui_config.h"
#include "wvui_types.h"

/** Return values of property read/write. */
#define WVUI_PROP_OK        (0)  /**< Success */
#define WVUI_PROP_ERR_ARG   (-1) /**< Invalid length or value */
#define WVUI_PROP_ERR_UNSUP (-2) /**< Property not supported by this class */
#define WVUI_PROP_ERR_RO    (-3) /**< Read-only property, not silently ignored */

/** Property numbers: low 5 bits of object-level OPs (0..31); writes and query ACK share the same encoding. */
#define WVUI_PROP_POS        (0u)  /**< int16 x, int16 y        (4 B) */
#define WVUI_PROP_SIZE       (1u)  /**< int16 w, int16 h        (4 B) */
#define WVUI_PROP_TEXT       (2u)  /**< raw byte string, no NUL */
#define WVUI_PROP_ALIGN      (3u)  /**< u8 */
#define WVUI_PROP_COLOR      (4u)  /**< u16 text color */
#define WVUI_PROP_OPA        (5u)  /**< u8 0..255 */
#define WVUI_PROP_ENABLED    (6u)  /**< u8; disabled state does not block gestures */
#define WVUI_PROP_PRESSED    (7u)  /**< u8, read-only: no direct writes */
#define WVUI_PROP_BG         (8u)  /**< u16, btn background */
#define WVUI_PROP_BG_PRESSED (9u)  /**< u16, pressed background */
#define WVUI_PROP_BORDER     (10u) /**< u16, btn border color */
#define WVUI_PROP_ASSET      (11u) /**< u16 asset ID */
#define WVUI_PROP_VALUE      (12u) /**< u8 0..255 progress */
#define WVUI_PROP_RADIUS     (13u) /**< u8 corner radius */
#define WVUI_PROP_KNOB       (14u) /**< u16 accent element color */
#define WVUI_PROP_CORNERS    (15u) /**< u8, 2 bits per corner */
#define WVUI_PROP_BORDER_W   (16u) /**< u8 border width / ring width */
#define WVUI_PROP_ANGLE_START (17u) /**< u16, 512-step scale */
#define WVUI_PROP_ANGLE_END   (18u) /**< u16 end angle */
#define WVUI_PROP_VALUE32     (19u) /**< int32 little-endian fixed point */
#define WVUI_PROP_COUNT      (20u)

/**
 * @brief Read one little-endian u16; explicit little-endian, fixed size, no struct cast.
 * @param p  At least 2 bytes; no NULL check, no length check
 * @return    The decoded 16-bit value
 */
static inline uint16_t wvui_prop_u16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8));
}

/**
 * @brief Read one little-endian i16, reinterpreted as two's complement.
 * @param p  At least 2 bytes
 * @return    The signed 16-bit value
 */
static inline int16_t wvui_prop_i16(const uint8_t *p)
{
    return (int16_t)wvui_prop_u16(p);
}

/**
 * @brief Write one little-endian u16.
 * @param p  At least 2 writable bytes
 * @param v  The 16-bit value to write
 */
static inline void wvui_prop_write_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

/**
 * @brief Write a C string into the buffer as raw bytes, truncate when cap is
 *        full, without a trailing NUL.
 * @param out  Destination buffer
 * @param cap  Writable byte count
 * @param s    Source string, must be NUL-terminated
 * @return      Bytes written (<= cap); 0 = empty string, cap = 0 or NULL args
 */
static inline uint16_t wvui_prop_write_text(uint8_t *out, uint16_t cap, const char *s)
{
    uint16_t n = 0u;

    if ((out == 0) || (s == 0))
    {
        return 0u;
    }
    while ((s[n] != '\0') && (n < cap))
    {
        out[n] = (uint8_t)s[n];
        n++;
    }
    return n;
}

#endif /* WVUI_PROPS_H */
