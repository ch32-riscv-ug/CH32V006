#ifndef WVUI_PORT_H
#define WVUI_PORT_H

/**
 * @file wvui_port.h
 * @brief Display port: the kernel pushes pixels to the screen only through it.
 * @note  The port only sets the window and streams pixels, it never composes
 *        pixels or parses image formats; never call back into the GUI from
 *        the callbacks.
 */

#include <stdint.h>
#include <stddef.h>
#include "wvui_config.h"

/**
 * Display port.
 * @note Callback order is fixed: set_window -> flush_begin -> flush x N ->
 *       flush_wait -> flush_end.
 * @note Capability fields are runtime values, changing screens does not
 *       require a rebuild; callbacks carry no ctx, give each instance its
 *       own set of functions.
 */
typedef struct
{
    /**
     * @brief Stage 1: set the rectangular window of this transaction.
     * @param x0,y0  Top-left corner, screen coordinates, inclusive
     * @param x1,y1  Bottom-right corner, screen coordinates, inclusive;
     *               width = x1 - x0 + 1
     * @note  Coordinates are already clipped to the screen and aligned by
     *       the kernel; called once per transaction, never reset between
     *       flushes.
     */
    void (*set_window)(int16_t x0, int16_t y0, int16_t x1, int16_t y1);

    /**
     * @brief Stage 2: start one pixel transfer.
     * @note  Called once per transaction; asynchronous ports latch the
     *        window origin here and start the DMA.
     */
    void (*flush_begin)(void);

    /**
     * @brief Stage 3: deliver one block of pixels.
     * @param pixels  Start address of the block, device-native colors in a
     *                contiguous run
     * @param count   Number of pixels; byte size = count * bytes per pixel
     * @note  Bytes per pixel are computed by the port from the color depth
     *        and byte order; the kernel has no color-depth branches.
     * @note  Called 1..N times per transaction, N = the number of blocks the
     *        rectangle splits into by PFB line capacity; pixels is read-only.
     */
    void (*flush)(const void *pixels, uint32_t count);

    /**
     * @brief Stage 4: wait for in-flight transfers to finish.
     * @note  Always called at least once at the end of a transaction; also
     *        called before reusing a PFB, so possibly several times per
     *        transaction.
     * @note  Semantics: wait until all in-flight transfers end; reusing a
     *        PFB without waiting rewrites video memory the DMA is reading.
     */
    void (*flush_wait)(void);

    /**
     * @brief Stage 5: finish one transaction.
     */
    void (*flush_end)(void);

    /**
     * Runtime capabilities: switches, alignment granularity and bus bandwidth.
     * @note paged != 0 additionally requires align_y > 1 and
     *       PFB line capacity % align_y == 0.
     */
    uint8_t async;             /**< 0 = block until sent; 1 = return at once, transfer in background */
    uint8_t align_x;           /**< X alignment granularity, power of 2; 1 = unaligned; screen width must be a multiple */
    uint8_t align_y;           /**< Y alignment granularity, power of 2; 1 = unaligned; screen height must be a multiple */
    uint8_t paged;             /**< 1 = paged addressing, block boundaries must fall on full pages */
    uint32_t bus_bytes_per_ms; /**< Bus bandwidth in bytes/ms; only involved in stats and frame budget checks */
} wvui_port_t;

WVUI_STATIC_ASSERT(offsetof(wvui_port_t, async) == 5 * sizeof(((wvui_port_t *)0)->flush_begin),
                  "the five stage callbacks must come first");
WVUI_STATIC_ASSERT(offsetof(wvui_port_t, bus_bytes_per_ms) == offsetof(wvui_port_t, async) + 4,
                  "four consecutive uint8 capabilities, then bus_bytes_per_ms");

#endif /* WVUI_PORT_H */
