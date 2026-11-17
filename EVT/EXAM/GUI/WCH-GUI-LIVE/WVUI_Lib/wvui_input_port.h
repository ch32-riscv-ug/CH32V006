#ifndef WVUI_INPUT_PORT_H
#define WVUI_INPUT_PORT_H

/**
 * @file wvui_input_port.h
 * @brief Input port: the port only reports {x, y, pressed}; state machine and policy stay in the framework.
 * @note  Single-key devices use this path too: the port maps the key to a
 *        touch sample at one point.
 */

#include <stdint.h>
#include <stddef.h>
#include "wvui_config.h"

/** Touch/pointer sample: the kernel hands a wvui_indev_t * to widgets in every event_cb. */
typedef struct
{
    int16_t x;       /**< Screen coordinate, end-point inclusive [0, width-1] */
    int16_t y;       /**< Screen coordinate, end-point inclusive [0, height-1] */
    uint8_t pressed; /**< 1 = pressed, 0 = released; only these two values */
} wvui_indev_t;

/** Input port: one per board. */
typedef struct
{
    /**
     * @brief Read the current touch state and fill the three fields of out.
     * @param out  Output; all three fields must be written
     * @note  The kernel calls it once per frame: return immediately, never
     *        call back into the GUI.
     * @note  The port keeps no state, no debouncing, no threshold check,
     *        and does not synthesize STAY.
     */
    void (*read)(wvui_indev_t *out);
} wvui_input_port_t;

WVUI_STATIC_ASSERT(sizeof(wvui_indev_t) == 6, "wvui_indev_t must be 6 bytes: int16 x + int16 y + uint8 pressed");
WVUI_STATIC_ASSERT(offsetof(wvui_input_port_t, read) == 0, "wvui_input_port_t has a single member today");

#endif /* WVUI_INPUT_PORT_H */
