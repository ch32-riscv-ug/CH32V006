/**
 * @file wvui_cmd.c
 * @brief Command channel implementation: CRC8, text pool, frame encoding, dispatch and TLV parsing.
 * @note  Dispatch and encoding are always compiled; the byte ring + TLV parser exist only when the switch is on.
 */

#include "wvui_cmd.h"

#include "wvui_driver.h"
#include "wvui_input.h"
#include "wvui_widget.h"

/* ---- CRC-8/ATM: poly 0x07, init 0x00, covers from after the SOF to before the CRC ---- */
#if WVUI_CFG_CMD_CRC8
/* For chained use: the parser must reuse it while advancing byte by byte. */
static uint8_t crc8_step(uint8_t crc, uint8_t b)
{
    uint8_t i;

    crc ^= b;
    for (i = 0u; i < 8u; i++)
    {
        crc = (uint8_t)(((crc & 0x80u) != 0u) ? (uint8_t)((crc << 1) ^ 0x07u)
                                              : (uint8_t)(crc << 1));
    }
    return crc;
}

static uint8_t crc8_atm(const uint8_t *p, uint16_t len)
{
    uint8_t crc = 0u;
    uint16_t i;

    for (i = 0u; i < len; i++)
    {
        crc = crc8_step(crc, p[i]);
    }
    return crc;
}
#endif

/* ---- Text pool: bump allocation, cleared on scenario reset; the stripped build keeps empty stubs for both entry points ---- */
#if WVUI_CFG_ENABLE_CMD_CHANNEL
static char s_text_pool[WVUI_CFG_CMD_TEXT_POOL];
static uint16_t s_text_used;

void wvui_cmd_text_reset(void)
{
    s_text_used = 0u;
}

const char *wvui_cmd_text_store(const uint8_t *bytes, uint16_t len)
{
    char *dst;

    if ((uint32_t)s_text_used + (uint32_t)len + 1u > (uint32_t)WVUI_CFG_CMD_TEXT_POOL)
    {
        return 0; /* Pool full means failure, no ring reuse */
    }

    dst = &s_text_pool[s_text_used];
    if ((bytes != 0) && (len > 0u))
    {
        uint16_t i;
        for (i = 0u; i < len; i++)
        {
            dst[i] = (char)bytes[i];
        }
    }
    dst[len] = '\0'; /* label/btn only store the pointer, the NUL must be added */
    s_text_used = (uint16_t)(s_text_used + len + 1u);
    return dst;
}
#else
void wvui_cmd_text_reset(void)
{
    /* No text pool to clear (channel stripped) */
}

const char *wvui_cmd_text_store(const uint8_t *bytes, uint16_t len)
{
    (void)bytes;
    (void)len;
    return 0; /* No pool, always fails */
}
#endif /* WVUI_CFG_ENABLE_CMD_CHANNEL */

/* ---- Frame encoding ---- */
uint16_t wvui_cmd_encode(uint8_t *out, uint16_t cap, uint8_t sof, uint16_t id, uint8_t op,
                        const uint8_t *payload, uint16_t len)
{
    uint16_t total = (uint16_t)(6u + len
#if WVUI_CFG_CMD_CRC8
                                + 1u
#endif
    );
    uint16_t i;

    if ((out == 0) || (cap < total))
    {
        return 0u;
    }
    if ((len > 0u) && (payload == 0))
    {
        return 0u;
    }

    out[0] = sof;
    out[1] = (uint8_t)(id & 0xFFu);
    out[2] = (uint8_t)((id >> 8) & 0xFFu);
    out[3] = op;
    out[4] = (uint8_t)(len & 0xFFu);
    out[5] = (uint8_t)((len >> 8) & 0xFFu);
    for (i = 0u; i < len; i++)
    {
        out[6u + i] = payload[i];
    }
#if WVUI_CFG_CMD_CRC8
    out[6u + len] = crc8_atm(&out[1], (uint16_t)(5u + len));
#endif
    return total;
}

/* ---- Counters ---- */
static wvui_cmd_stats_t s_stats;

const wvui_cmd_stats_t *wvui_cmd_stats(void)
{
    return &s_stats;
}

void wvui_cmd_stats_reset(void)
{
    uint32_t i;
    uint8_t *p = (uint8_t *)&s_stats;

    for (i = 0u; i < (uint32_t)sizeof(s_stats); i++)
    {
        p[i] = 0u;
    }
}

/* ---- Reply ---- */
#if WVUI_CFG_ENABLE_CMD_CHANNEL
static wvui_cmd_tx_fn s_tx;

void wvui_cmd_bind_tx(wvui_cmd_tx_fn tx)
{
    s_tx = tx;
}
#endif

/* Kept in the stripped build too: external parsers can call it directly. */
static void reply(uint16_t id, uint8_t op, const uint8_t *payload, uint16_t len)
{
    uint8_t frame[6u + WVUI_CFG_CMD_MAX_PAYLOAD
#if WVUI_CFG_CMD_CRC8
                  + 1u
#endif
    ];
    uint16_t n = wvui_cmd_encode(frame, (uint16_t)sizeof(frame), WVUI_CMD_SOF_UP, id, op, payload,
                                len);

    if (n == 0u)
    {
        return;
    }
    s_stats.replies++;
#if WVUI_CFG_ENABLE_CMD_CHANNEL
    if (s_tx != 0)
    {
        s_tx(frame, n);
    }
#endif
}

/* ---- Dispatch: object level ---- */
static int set_object_prop(uint16_t id, uint8_t prop, const uint8_t *payload, uint16_t len)
{
    wvui_widget_t *w = wvui_widget_find_by_id(id);
    const uint8_t *arg = payload;
    uint16_t arg_len = len;

    if (w == 0)
    {
        return WVUI_CMD_ERR_NOOBJ;
    }
    if ((WVUI_W_CLASS(w) == 0) || (WVUI_W_CLASS(w)->set_prop == 0))
    {
        return WVUI_CMD_ERR_UNSUP;
    }

    if (prop == WVUI_PROP_TEXT)
    {
        /* Text: copy into the pool and add the NUL first, see wvui_props.h */
        const char *pooled = wvui_cmd_text_store(payload, len);
        if (pooled == 0)
        {
            return WVUI_CMD_ERR_NOMEM;
        }
        arg = (const uint8_t *)pooled;
    }

    return (int)WVUI_W_CLASS(w)->set_prop(w, prop, arg, arg_len);
}

static int query_object_prop(uint16_t id, uint8_t prop, uint8_t op)
{
    wvui_widget_t *w = wvui_widget_find_by_id(id);
    uint8_t out[WVUI_CFG_CMD_MAX_PAYLOAD];
    int16_t n;

    if (w == 0)
    {
        return WVUI_CMD_ERR_NOOBJ;
    }
    if ((WVUI_W_CLASS(w) == 0) || (WVUI_W_CLASS(w)->get_prop == 0))
    {
        return WVUI_CMD_ERR_UNSUP;
    }

    n = WVUI_W_CLASS(w)->get_prop(w, prop, out, (uint16_t)sizeof(out));
    if (n < 0)
    {
        return (int)n;
    }
    s_stats.queries++;
    /* Reply mirrors the downstream frame: same op and schema */
    reply(id, op, out, (uint16_t)n);
    return WVUI_CMD_OK;
}

/* ---- Dispatch: system level ---- */
static void put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static int sys_query_stats(void)
{
    const wvui_stats_t *st = wvui_stats();
    uint8_t out[44];

    /* 11 × u32 least significant byte first, field order matches the documentation */
    put_u32(out + 0u, st->frames);
    put_u32(out + 4u, st->rects);
    put_u32(out + 8u, st->set_window_calls);
    put_u32(out + 12u, st->flush_calls);
    put_u32(out + 16u, (uint32_t)st->bus_bytes);
    put_u32(out + 20u, st->dirty_rects);
    put_u32(out + 24u, st->dirty_area);
    put_u32(out + 28u, st->draw_calls);
    put_u32(out + 32u, (uint32_t)st->cpu_us);
    put_u32(out + 36u, (uint32_t)st->bus_us);
    put_u32(out + 40u, (uint32_t)st->wall_us);

    s_stats.queries++;
    reply(0u, WVUI_CMD_OP_STATS, out, (uint16_t)sizeof(out));
    return WVUI_CMD_OK;
}

static int sys_query_info(void)
{
    const wvui_info_t *in = wvui_info();
    uint8_t out[27];

    wvui_prop_write_u16(out + 0u, (uint16_t)in->w);
    wvui_prop_write_u16(out + 2u, (uint16_t)in->h);
    wvui_prop_write_u16(out + 4u, in->pfb_lines);
    wvui_prop_write_u16(out + 6u, in->chunk_rows);
    wvui_prop_write_u16(out + 8u, (uint16_t)wvui_widget_count());
    out[10] = (uint8_t)((in->double_buffer != 0u) ? 1u : 0u);
    put_u32(out + 11u, s_stats.frames);
    put_u32(out + 15u, s_stats.errors);
    put_u32(out + 19u, s_stats.resyncs);
    put_u32(out + 23u, s_stats.dropped);

    s_stats.queries++;
    reply(0u, WVUI_CMD_OP_INFO, out, (uint16_t)sizeof(out));
    return WVUI_CMD_OK;
}

static int sys_set_input(const uint8_t *payload, uint16_t len)
{
    wvui_indev_t in;

    if (len != 5u)
    {
        return WVUI_CMD_ERR_ARG;
    }
    in.x = wvui_prop_i16(payload);
    in.y = wvui_prop_i16(payload + 2);
    in.pressed = (uint8_t)((payload[4] != 0u) ? 1u : 0u);

    /* Same path as a finger: injected samples go through wvui_input_feed(), capture and events happen as usual. */
    wvui_input_feed(&in);
    s_stats.sets++;
    return WVUI_CMD_OK;
}

/* ---- Dispatch entry (kernel side, always compiled) ---- */
int wvui_cmd_dispatch(uint16_t id, uint8_t op, const uint8_t *payload, uint16_t len)
{
    int rc;

    if (op >= WVUI_CMD_OP_GET_BASE)
    {
        if (op < WVUI_CMD_OP_SYS_BASE)
        {
            /* Object-level QUERY: 0x20-0x3F */
            rc = query_object_prop(id, (uint8_t)(op - WVUI_CMD_OP_GET_BASE), op);
        }
        else
        {
            switch (op)
            {
            case WVUI_CMD_OP_STATS:
                rc = sys_query_stats();
                break;
            case WVUI_CMD_OP_STATS_RESET:
                wvui_stats_reset();
                s_stats.sets++;
                rc = WVUI_CMD_OK;
                break;
            case WVUI_CMD_OP_INPUT:
                rc = sys_set_input(payload, len);
                break;
            case WVUI_CMD_OP_INFO:
                rc = sys_query_info();
                break;
            default:
                rc = WVUI_CMD_ERR_UNSUP;
                break;
            }
        }
    }
    else
    {
        /* Object-level SET: 0x00-0x1F, SET gets no reply */
        rc = set_object_prop(id, (uint8_t)(op - WVUI_CMD_OP_SET_BASE), payload, len);
        if (rc == WVUI_CMD_OK)
        {
            s_stats.sets++;
        }
    }

    if (rc != WVUI_CMD_OK)
    {
        s_stats.errors++;
    }
    return rc;
}

/* ---- Byte ring + TLV parser state machine (no code generated when the switch is off) ---- */
#if WVUI_CFG_ENABLE_CMD_CHANNEL

static uint8_t s_ring[WVUI_CFG_CMD_RING];
static volatile uint16_t s_head; /* Written by the consumer (poll) */
static volatile uint16_t s_tail; /* Written by the producer (push, ISR side) */

#define CMD_RING_MASK ((uint16_t)(WVUI_CFG_CMD_RING - 1u))

uint16_t wvui_cmd_push(const uint8_t *bytes, uint16_t len)
{
    uint16_t i;

    if (bytes == 0)
    {
        return 0u;
    }
    for (i = 0u; i < len; i++)
    {
        uint16_t next = (uint16_t)((s_tail + 1u) & CMD_RING_MASK);
        if (next == s_head)
        {
            /* Ring full: discard the remaining bytes and count them, no overwrite, no blocking. */
            s_stats.dropped += (uint32_t)(len - i);
            break;
        }
        s_ring[s_tail] = bytes[i];
        s_tail = next;
    }
    return i;
}

uint16_t wvui_cmd_pending(void)
{
    return (uint16_t)((s_tail - s_head) & CMD_RING_MASK);
}

/* Bad frame = no way forward in the in-frame byte-stream state: clear half-frame state, count a resync, return to SOF scan.
 * resync / csum_errs drop whole frames but never bytes: ring bytes never vanish silently when diagnosing. */
typedef enum
{
    ST_SOF = 0,
    ST_T0,
    ST_T1,
    ST_OP,
    ST_L0,
    ST_L1,
    ST_V,
    ST_CRC
} cmd_state_t;

static cmd_state_t s_st;
static uint16_t s_id;
static uint16_t s_len;
static uint16_t s_got;
static uint8_t s_op;
static uint8_t s_payload[WVUI_CFG_CMD_MAX_PAYLOAD];
#if WVUI_CFG_CMD_CRC8
static uint8_t s_crc; /* CRC state (covers T/OP/L/V) */
#endif

static void parse_reset(void)
{
    s_st = ST_SOF;
    s_id = 0u;
    s_len = 0u;
    s_got = 0u;
    s_op = 0u;
#if WVUI_CFG_CMD_CRC8
    s_crc = 0u;
#endif
}

/* Bad frame handling: return to the SOF scan state and count one resync. */
static void frame_fail(void)
{
    s_stats.resyncs++;
    parse_reset();
}

/* Whole frame in hand: dispatch + count + reset; return 1 for parse_byte to report */
static uint8_t frame_complete(void)
{
    s_stats.frames++;
    (void)wvui_cmd_dispatch(s_id, s_op, s_payload, s_len);
    s_stats.dispatched++;
    parse_reset();
    return 1u;
}

/* 1 = this byte completed a frame and it was dispatched; non-magic bytes in the SOF scan state go to the discard branch. */
static uint8_t parse_byte(uint8_t b)
{
    switch (s_st)
    {
    case ST_SOF:
        if (b == WVUI_CMD_SOF_DOWN)
        {
            parse_reset(); /* Clear leftovers, including CRC state */
            s_st = ST_T0;
        }
        else
        {
            /* Resync: drop everything that is not SOF, including the reply-direction magic */
            s_stats.garbage++;
        }
        return 0u;

    case ST_T0:
        s_id = b;
#if WVUI_CFG_CMD_CRC8
        s_crc = crc8_step(s_crc, b);
#endif
        s_st = ST_T1;
        return 0u;

    case ST_T1:
        s_id = (uint16_t)(s_id | ((uint16_t)b << 8));
#if WVUI_CFG_CMD_CRC8
        s_crc = crc8_step(s_crc, b);
#endif
        s_st = ST_OP;
        return 0u;

    case ST_OP:
        s_op = b;
#if WVUI_CFG_CMD_CRC8
        s_crc = crc8_step(s_crc, b);
#endif
        s_st = ST_L0;
        return 0u;

    case ST_L0:
        s_len = b;
#if WVUI_CFG_CMD_CRC8
        s_crc = crc8_step(s_crc, b);
#endif
        s_st = ST_L1;
        return 0u;

    case ST_L1:
        s_len = (uint16_t)(s_len | ((uint16_t)b << 8));
#if WVUI_CFG_CMD_CRC8
        s_crc = crc8_step(s_crc, b);
#endif
        s_got = 0u;
        if (s_len > (uint16_t)WVUI_CFG_CMD_MAX_PAYLOAD)
        {
            /* Overrun = bad frame: resync immediately, no out-of-bounds write */
            frame_fail();
            return 0u;
        }
        if (s_len == 0u)
        {
#if WVUI_CFG_CMD_CRC8
            s_st = ST_CRC;
            return 0u;
#else
            return frame_complete();
#endif
        }
        s_st = ST_V;
        return 0u;

    case ST_V:
        s_payload[s_got] = b;
        s_got++;
#if WVUI_CFG_CMD_CRC8
        s_crc = crc8_step(s_crc, b);
#endif
        if (s_got >= s_len)
        {
#if WVUI_CFG_CMD_CRC8
            s_st = ST_CRC;
            return 0u;
#else
            return frame_complete();
#endif
        }
        return 0u;

#if WVUI_CFG_CMD_CRC8
    case ST_CRC:
        if (b != s_crc)
        {
            s_stats.csum_errs++;
            frame_fail(); /* Checksum failure drops the whole frame: SET is idempotent and safe to resend */
            return 0u;
        }
        return frame_complete();
#endif

    default:
        parse_reset();
        return 0u;
    }
}

uint16_t wvui_cmd_poll(void)
{
    uint16_t dispatched = 0u;

    while (s_head != s_tail)
    {
        uint8_t b = s_ring[s_head];
        s_head = (uint16_t)((s_head + 1u) & CMD_RING_MASK);
        if (parse_byte(b) != 0u)
        {
            dispatched++;
        }
    }
    return dispatched;
}

#else /* WVUI_CFG_ENABLE_CMD_CHANNEL == 0 */

uint16_t wvui_cmd_pending(void)
{
    return 0u;
}

#endif /* WVUI_CFG_ENABLE_CMD_CHANNEL */

/* ---- Whole-channel reset ---- */
void wvui_cmd_reset(void)
{
#if WVUI_CFG_ENABLE_CMD_CHANNEL
    s_head = 0u;
    s_tail = 0u;
    parse_reset();
#endif
    wvui_cmd_text_reset();
    wvui_cmd_stats_reset();
}
