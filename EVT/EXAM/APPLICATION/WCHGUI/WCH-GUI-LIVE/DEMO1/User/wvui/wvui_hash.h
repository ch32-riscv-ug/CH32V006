#ifndef WVUI_HASH_H
#define WVUI_HASH_H

/**
 * @file wvui_hash.h
 * @brief Streaming hash: FNV-1a 32-bit, compares pixel streams pushed to the port frame by frame; never reads panel GRAM.
 * @note  Feed least significant byte first, independent of host byte order; the previous frame's hash can seed the next frame.
 */

#include <stdint.h>

/** FNV-1a 32-bit initial value and prime. */
#define WVUI_HASH_INIT  (2166136261u)
#define WVUI_HASH_PRIME (16777619u)

/**
 * @brief Feed one byte, chained.
 * @param h  Hash of the previous round; use WVUI_HASH_INIT for the first round
 * @return    New hash, ready to feed more
 */
static inline uint32_t wvui_hash_u8(uint32_t h, uint8_t b)
{
    return (h ^ (uint32_t)b) * WVUI_HASH_PRIME;
}

/**
 * @brief Feed one 16-bit value, chained, least significant byte first.
 * @param h  Hash of the previous round; use WVUI_HASH_INIT for the first round
 * @param v  16-bit value
 * @return    New hash
 */
static inline uint32_t wvui_hash_u16(uint32_t h, uint16_t v)
{
    h = wvui_hash_u8(h, (uint8_t)(v & 0xFFu));
    return wvui_hash_u8(h, (uint8_t)((v >> 8) & 0xFFu));
}

/**
 * @brief Feed one 32-bit value, chained, least significant byte first.
 * @param h  Hash of the previous round; use WVUI_HASH_INIT for the first round
 * @param v  32-bit value
 * @return    New hash
 */
static inline uint32_t wvui_hash_u32(uint32_t h, uint32_t v)
{
    h = wvui_hash_u8(h, (uint8_t)(v & 0xFFu));
    h = wvui_hash_u8(h, (uint8_t)((v >> 8) & 0xFFu));
    h = wvui_hash_u8(h, (uint8_t)((v >> 16) & 0xFFu));
    return wvui_hash_u8(h, (uint8_t)((v >> 24) & 0xFFu));
}

/**
 * @brief Feed an array of RGB565 pixels in panel push-stream order, two bytes per pixel, least significant byte first.
 * @param h      Hash of the previous round; use WVUI_HASH_INIT for the first round
 * @param px     Pixel array, stored contiguously row by row
 * @param count  Pixel count; 0 = feed nothing
 * @return        New hash
 */
static inline uint32_t wvui_hash_pixels(uint32_t h, const uint16_t *px, uint32_t count)
{
    uint32_t i;
    for (i = 0u; i < count; i++)
    {
        h = wvui_hash_u16(h, px[i]);
    }
    return h;
}

/**
 * @brief Feed a block of raw bytes.
 * @param h      Hash of the previous round; use WVUI_HASH_INIT for the first round
 * @param data   Source bytes; must not be NULL when count > 0
 * @param count  Byte count; 0 = feed nothing
 * @return        New hash
 */
static inline uint32_t wvui_hash_bytes(uint32_t h, const void *data, uint32_t count)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t i;
    for (i = 0u; i < count; i++)
    {
        h = wvui_hash_u8(h, p[i]);
    }
    return h;
}

/**
 * @brief Hash the script body, used to confirm both sides run the same script.
 * @param s  NUL-terminated string; passing 0 or an empty string returns WVUI_HASH_INIT
 * @return    Hash of the whole string, starting from WVUI_HASH_INIT
 * @note  Feed byte by byte up to NUL; the NUL itself is not hashed.
 */
static inline uint32_t wvui_hash_str(const char *s)
{
    uint32_t h = WVUI_HASH_INIT;
    while ((s != 0) && (*s != '\0'))
    {
        h = wvui_hash_u8(h, (uint8_t)*s);
        s++;
    }
    return h;
}

#endif /* WVUI_HASH_H */
