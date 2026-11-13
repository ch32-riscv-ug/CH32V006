/**
 * @file wvui_asset.c
 * @brief Asset resolver implementation: self-describing header, erased state semantics and diagnostics counts.
 * @note  On failure fills 0xFF and records into stats, so callers can draw without checking errors.
 */

#include "wvui_asset.h"

#include <string.h>

/* ---- Module state ---- */
static const wvui_asset_backend_t *s_be;
static const wvui_asset_entry_t *s_last;

static uint8_t s_hdr_scratch[WVUI_ASSET_HDR_SIZE];
static const uint8_t s_erased_hdr[WVUI_ASSET_HDR_SIZE] = { 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu };
static const uint8_t s_erased_byte = 0xFFu;

static wvui_asset_stats_t s_stats;

/* ---- Small helpers ---- */

static void fill_ff(uint8_t *dst, uint32_t len)
{
    if ((dst == 0) || (len == 0u))
    {
        return;
    }
    (void)memset(dst, 0xFF, (size_t)len);
}

static void blob_set_erased(wvui_blob_t *out)
{
    out->hdr = s_erased_hdr;
    out->size = 0u;
    out->addr = 0u;
}

static uint8_t fail(wvui_blob_t *out, wvui_asset_id_t id, uint8_t err)
{
    s_stats.fails++;
    s_stats.last_error = err;
    s_stats.last_error_id = id;
    blob_set_erased(out);
    return 0u;
}

/* Table sorted by id ascending, so binary search; midpoint via >>1, no division. */
static const wvui_asset_entry_t *table_find(wvui_asset_id_t id)
{
    uint16_t lo = 0u;
    uint16_t hi = s_be->count;

    while (lo < hi)
    {
        uint16_t mid = (uint16_t)(lo + (uint16_t)((uint16_t)(hi - lo) >> 1));
        wvui_asset_id_t cur = s_be->table[mid].id;

        if (cur == id)
        {
            return &s_be->table[mid];
        }
        if (cur < id)
        {
            lo = (uint16_t)(mid + 1u);
        }
        else
        {
            hi = mid;
        }
    }
    return 0;
}

/* ---- Self-describing header ---- */

void wvui_asset_hdr_decode(const uint8_t *hdr, wvui_asset_hdr_t *out)
{
    out->res_type = hdr[0];
    out->format = hdr[1];
    /* sizes big-endian: high byte first */
    out->w = (uint16_t)(((uint16_t)hdr[2] << 8) | (uint16_t)hdr[3]);
    out->h = (uint16_t)(((uint16_t)hdr[4] << 8) | (uint16_t)hdr[5]);
}

uint8_t wvui_asset_hdr_check(const uint8_t *hdr, wvui_asset_id_t id, uint8_t *err)
{
    uint8_t kind;

    if (hdr == 0)
    {
        if (err != 0) { *err = WVUI_ASSET_ERR_BAD_SIZE; }
        return 0u;
    }

    /* Outer version (two layers in total): unknown means BAD_VERSION */
    if (WVUI_ASSET_RES_VERSION(hdr[0]) != WVUI_ASSET_HDR_VERSION)
    {
        if (err != 0) { *err = WVUI_ASSET_ERR_BAD_VERSION; }
        return 0u;
    }

    /* header kind must match the ID kind */
    kind = WVUI_ASSET_RES_KIND(hdr[0]);
    if (kind != WVUI_ASSET_ID_KIND(id))
    {
        if (err != 0) { *err = WVUI_ASSET_ERR_BAD_KIND; }
        return 0u;
    }

    /* size is only meaningful for images: other kinds do not read these fields, nor require non-zero */
    if (kind == WVUI_ASSET_KIND_IMG)
    {
        uint16_t w = (uint16_t)(((uint16_t)hdr[2] << 8) | (uint16_t)hdr[3]);
        uint16_t h = (uint16_t)(((uint16_t)hdr[4] << 8) | (uint16_t)hdr[5]);

        if ((w == 0u) || (h == 0u))
        {
            if (err != 0) { *err = WVUI_ASSET_ERR_BAD_SIZE; }
            return 0u;
        }
    }

    if (err != 0) { *err = WVUI_ASSET_ERR_NONE; }
    return 1u;
}

/* ---- Diagnostics ---- */

const char *wvui_asset_err_name(uint8_t err)
{
    switch (err)
    {
    case WVUI_ASSET_ERR_NONE: return "none";
    case WVUI_ASSET_ERR_NO_BACKEND: return "no-backend";
    case WVUI_ASSET_ERR_NOT_FOUND: return "not-found";
    case WVUI_ASSET_ERR_TABLE: return "bad-table";
    case WVUI_ASSET_ERR_READING: return "media-read";
    case WVUI_ASSET_ERR_BAD_KIND: return "bad-kind";
    case WVUI_ASSET_ERR_BAD_VERSION: return "bad-version";
    case WVUI_ASSET_ERR_BAD_SIZE: return "bad-size";
    case WVUI_ASSET_ERR_UNSUPPORTED: return "unsupported";
    default: return "?";
    }
}

const wvui_asset_stats_t *wvui_asset_stats(void)
{
    return &s_stats;
}

void wvui_asset_stats_reset(void)
{
    (void)memset(&s_stats, 0, sizeof(s_stats));
}

/* ---- Binding ---- */

/* Table must be sorted by id ascending: precondition of binary search, checked once at bind time */
static uint8_t table_sorted(const wvui_asset_entry_t *table, uint16_t count)
{
    uint16_t i;

    if (count <= 1u)
    {
        return 1u;
    }
    if (table == 0)
    {
        return 0u;
    }
    for (i = 1u; i < count; i++)
    {
        if (table[i - 1u].id >= table[i].id)
        {
            return 0u;
        }
    }
    return 1u;
}

uint8_t wvui_asset_bind(const wvui_asset_backend_t *be)
{
    if (be == 0)
    {
        return 0u;
    }
    if ((be->count != 0u) && (be->table == 0))
    {
        return 0u;
    }
    if (!table_sorted(be->table, be->count))
    {
        return 0u;
    }
    /* Every entry needs a data path: its own direct pointer, or a backend read callback */
    if (be->read == 0)
    {
        uint16_t i;

        for (i = 0u; i < be->count; i++)
        {
            if (be->table[i].data == 0)
            {
                return 0u;
            }
        }
    }
    else
    {
        /* One data path per backend; body reads go through the last opened entry. */
        uint16_t i;

        for (i = 0u; i < be->count; i++)
        {
            if (be->table[i].data != 0)
            {
                return 0u;
            }
        }
    }

    s_be = be;
    s_last = 0; /* old blobs are invalidated at once */
    return 1u;
}

void wvui_asset_unbind(void)
{
    s_be = 0;
    s_last = 0;
}

const wvui_asset_backend_t *wvui_asset_backend(void)
{
    return s_be;
}

/* ---- open ---- */

uint8_t wvui_asset_open(wvui_asset_id_t id, wvui_blob_t *out)
{
    const wvui_asset_entry_t *e;
    const uint8_t *hdr;
    uint8_t err = WVUI_ASSET_ERR_NONE;

    s_stats.opens++;
    if (out == 0)
    {
        s_stats.fails++;
        s_stats.last_error = WVUI_ASSET_ERR_TABLE;
        s_stats.last_error_id = id;
        return 0u;
    }
    blob_set_erased(out);
    s_last = 0;

    if (s_be == 0)
    {
        return fail(out, id, WVUI_ASSET_ERR_NO_BACKEND);
    }

    e = table_find(id);
    if (e == 0)
    {
        s_stats.not_found++;
        return fail(out, id, WVUI_ASSET_ERR_NOT_FOUND);
    }

    /* Fetch the 6-byte header: in-chip arrays via direct pointer (zero copy), the rest via the backend callback */
    if (e->data != 0)
    {
        s_stats.direct_hits++;
        hdr = e->data;
    }
    else
    {
        if (s_be->read == 0)
        {
            return fail(out, id, WVUI_ASSET_ERR_TABLE);
        }
        if (s_be->read(s_be->ctx, e->addr, s_hdr_scratch, WVUI_ASSET_HDR_SIZE) != WVUI_ASSET_READ_OK)
        {
            /* Media missing or read incomplete: return the erased state, count into short_reads */
            s_stats.short_reads++;
            return fail(out, id, WVUI_ASSET_ERR_READING);
        }
        hdr = s_hdr_scratch;
    }

    if (e->size == 0u)
    {
        return fail(out, id, WVUI_ASSET_ERR_BAD_SIZE);
    }
    if (!wvui_asset_hdr_check(hdr, id, &err))
    {
        return fail(out, id, err);
    }

    s_last = e;
    out->hdr = hdr;
    out->size = e->size;
    /* addr is the body base: asset start + header length */
    out->addr = e->addr + WVUI_ASSET_HDR_SIZE;
    s_stats.last_error = WVUI_ASSET_ERR_NONE;
    return 1u;
}

/* ---- Reading the body (erased state semantics live here) ---- */

wvui_asset_read_t wvui_asset_read(const wvui_blob_t *b, uint32_t off, uint8_t *dst, uint32_t len)
{
    uint32_t avail;
    uint32_t n;

    if ((dst == 0) || (len == 0u))
    {
        return WVUI_ASSET_READ_MISSING;
    }
    if ((b == 0) || (b->size == 0u) || (off >= b->size))
    {
        fill_ff(dst, len);
        return WVUI_ASSET_READ_MISSING;
    }

    avail = b->size - off; /* > 0: off >= size ruled out earlier */
    n = (len < avail) ? len : avail;

    if ((s_last != 0) && (s_last->data != 0) && (b->hdr == s_last->data))
    {
        /* In-chip array: straight memcpy, zero extra cost */
        (void)memcpy(dst, b->hdr + WVUI_ASSET_HDR_SIZE + off, (size_t)n);
    }
    else if ((s_be != 0) && (s_be->read != 0))
    {
        if (s_be->read(s_be->ctx, b->addr + off, dst, n) != WVUI_ASSET_READ_OK)
        {
            /* Media incomplete: pad with 0xFF and record in stats */
            s_stats.short_reads++;
            fill_ff(dst, len);
            return WVUI_ASSET_READ_MISSING;
        }
    }
    else
    {
        fill_ff(dst, len);
        return WVUI_ASSET_READ_MISSING;
    }

    if (n < len)
    {
        fill_ff(dst + n, len - n);
        return WVUI_ASSET_READ_PARTIAL;
    }
    return WVUI_ASSET_READ_OK;
}

uint8_t wvui_asset_read_u8(const wvui_blob_t *b, uint32_t off)
{
    uint8_t v = s_erased_byte;

    (void)wvui_asset_read(b, off, &v, 1u);
    return v;
}
