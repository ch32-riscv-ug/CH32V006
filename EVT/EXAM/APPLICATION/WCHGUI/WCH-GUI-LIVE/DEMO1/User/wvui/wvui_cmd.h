#ifndef WVUI_CMD_H
#define WVUI_CMD_H

/**
 * @file wvui_cmd.h
 * @brief Command channel: frame codec, OP space, dispatch and counters.
 * @note  set is idempotent and gets no reply; errors only count in errors, read and replied by query.
 */

#include <stdint.h>

#include "wvui_config.h"
#include "wvui_props.h"
#include "wvui_types.h"

/** Magic numbers: one per direction, scan forward to resync after packet loss. */
#define WVUI_CMD_SOF_DOWN (0x7Eu)  /**< Host to device */
#define WVUI_CMD_SOF_UP   (0x7Fu)  /**< Device to host */

/** Object-level OP: prop = op & 0x1F, 0x80-0xFF reserved. */
#define WVUI_CMD_OP_SET_BASE (0x00u)  /**< SET base: + prop(0..31) */
#define WVUI_CMD_OP_GET_BASE (0x20u)  /**< QUERY base: + prop(0..31) */

/**
 * @brief System-level OP: 0x40-0x7F, the top 2 bits define the domain; INPUT payload = {i16 x, i16 y, u8 pressed}.
 * @note  INPUT injection shares the same input path as a finger, identical frame by frame; drive taps with it, PRESSED is not writable.
 */
#define WVUI_CMD_OP_SYS_BASE   (0x40u)  /**< Start of the system-level domain */
#define WVUI_CMD_OP_STATS      (0x40u)  /**< QUERY: stat counters */
#define WVUI_CMD_OP_STATS_RESET (0x41u)  /**< SET: clear stats */
#define WVUI_CMD_OP_INPUT      (0x42u)  /**< SET: inject a touch sample, PRESSED not writable */
#define WVUI_CMD_OP_INFO       (0x43u)  /**< QUERY: configuration summary */
#define WVUI_CMD_OP_MAX        (0x44u)  /**< Upper bound of known OPs (inclusive) */

/** Dispatch return codes: for counting only, no error codes travel on the wire. */
#define WVUI_CMD_OK         (0)   /**< Success */
#define WVUI_CMD_ERR_ARG    (-1)  /**< Invalid payload length or value */
#define WVUI_CMD_ERR_NOOBJ  (-2)  /**< No object found for the id */
#define WVUI_CMD_ERR_UNSUP  (-3)  /**< Unsupported (including read-only properties) */
#define WVUI_CMD_ERR_NOMEM  (-4)  /**< Text pool full */

/** Command channel counters: accumulate across frames, cleared by wvui_cmd_stats_reset(). */
typedef struct
{
    uint32_t frames;     /**< Frames parsed out (including the first half of bad frames) */
    uint32_t dispatched; /**< Frames that entered dispatch */
    uint32_t queries;    /**< QUERY count among them */
    uint32_t sets;       /**< SET count among them */
    uint32_t errors;     /**< Times dispatch returned an error */
    uint32_t resyncs;    /**< Resyncs after bad frames */
    uint32_t garbage;    /**< Bytes discarded before the SOF lock */
    uint32_t dropped;    /**< Bytes discarded when the ring was full */
    uint32_t replies;    /**< Reply frames sent */
    uint32_t csum_errs;  /**< Frames failing CRC */
} wvui_cmd_stats_t;

/* ---- Dispatch (kernel side, always compiled) ---- */

/**
 * @brief Dispatch one frame: hand it to the object ops or the system ops by id / op.
 * @param id       Target object id
 * @param op       OP byte, see WVUI_CMD_OP_*
 * @param payload  Payload; text properties need not be NUL terminated, nor have any lifetime
 * @return         WVUI_CMD_OK or a WVUI_CMD_ERR_* code
 * @note   Any byte source and parser can call this directly; text properties are copied into the text pool and NUL terminated here.
 */
int wvui_cmd_dispatch(uint16_t id, uint8_t op, const uint8_t *payload, uint16_t len);

/**
 * @brief Encode one frame into out.
 * @param cap      Capacity of out, in bytes
 * @param sof      Direction magic: WVUI_CMD_SOF_DOWN or WVUI_CMD_SOF_UP
 * @param id       Object id, least significant byte first on the wire
 * @param op       OP byte
 * @param payload  Payload; must not be NULL when len > 0
 * @return         Frame length in bytes; 0 = insufficient capacity or invalid arguments
 * @note   Frame = [SOF 1B][id 2B][OP 1B][L 2B][V][CRC8 1B], L is given explicitly.
 * @note   CRC8 = poly 0x07 / init 0x00 / no reflection / no output XOR, covering from after the SOF to
 *         before the CRC; with WVUI_CFG_CMD_CRC8 off no CRC is appended, only magic and length are checked.
 */
uint16_t wvui_cmd_encode(uint8_t *out, uint16_t cap, uint8_t sof, uint16_t id, uint8_t op,
                        const uint8_t *payload, uint16_t len);

/**
 * @brief Copy bytes into the text pool and NUL terminate; text properties are always written to this pool.
 * @param bytes  Source bytes; need not be NUL terminated
 * @param len    Source byte count
 * @return       NUL-terminated string inside the pool; NULL when the pool is full
 * @note   When NULL comes back, the caller turns it into WVUI_CMD_ERR_NOMEM.
 */
const char *wvui_cmd_text_store(const uint8_t *bytes, uint16_t len);

/**
 * @brief Clear the text pool, leaving ring, parser state and counters untouched.
 */
void wvui_cmd_text_reset(void);

/* ---- Byte ring + TLV parser (no code generated when WVUI_CFG_ENABLE_CMD_CHANNEL == 0) ---- */
#if WVUI_CFG_ENABLE_CMD_CHANNEL

/**
 * @brief Write bytes into the ring buffer (UART ISR / file / script).
 * @param bytes  Source bytes
 * @param len    Byte count
 * @return       Bytes actually enqueued; the rest is counted in stats.dropped
 * @note   SPSC: only the producer writes tail, the consumer (wvui_cmd_poll) writes only head.
 * @note   Discard the remaining bytes when the ring is full, no blocking, no overwrite.
 */
uint16_t wvui_cmd_push(const uint8_t *bytes, uint16_t len);

/**
 * @brief Advance the parser state machine and dispatch complete frames (step 2 of wvui_frame, once per frame).
 * @return Frames dispatched in this pass
 * @note   Half-frame state persists across calls; a half frame is not an error.
 * @note   L is given explicitly, overrun means a bad frame; after a bad frame scan forward to resync.
 */
uint16_t wvui_cmd_poll(void);

/** Reply callback: the simulator writes to stdout, real hardware writes to UART. */
typedef void (*wvui_cmd_tx_fn)(const uint8_t *frame, uint16_t len);

/**
 * @brief Bind the reply channel.
 * @param tx  Reply callback; NULL = count only, send nothing (stats.replies still accumulates)
 */
void wvui_cmd_bind_tx(wvui_cmd_tx_fn tx);

#endif /* WVUI_CFG_ENABLE_CMD_CHANNEL */

/* ---- Reset and counters (available in both builds) ---- */

/**
 * @brief Clear the ring, parser state, text pool and counters.
 * @note  Call on scenario reset, idempotent.
 */
void wvui_cmd_reset(void);

/**
 * @brief Get the counters.
 * @return Address of the internal counters; contents cleared by wvui_cmd_stats_reset()
 */
const wvui_cmd_stats_t *wvui_cmd_stats(void);

/**
 * @brief Clear only the counters, leaving ring and parser state untouched.
 */
void wvui_cmd_stats_reset(void);

/**
 * @brief Get bytes stranded in the ring.
 * @return Bytes enqueued but not yet parsed; for debugging, always 0 in the stripped build
 */
uint16_t wvui_cmd_pending(void);

#endif /* WVUI_CMD_H */
