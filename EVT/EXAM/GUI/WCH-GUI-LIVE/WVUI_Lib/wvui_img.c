/**
 * @file wvui_img.c
 * @brief Image format dispatch and decoding.
 * @note  Failures are not silent: record stats and return 0; sizes use int32
 *        intermediates; the decode context uses zero malloc.
 */

#include "wvui_render.h"

#include "wvui_slice.h"

/* Indexed ops have 6 bits only, hence the palette/dictionary cap of 64. */
#define IMG_PAL_MAX 64u
#define IMG_PAL_ENTRY_BYTES 3u /* ARGB8565 3 B; RGB565 uses 2 B */

/* V3 header length and version marker for indexed QOI. */
#define IMG_QOI_HDR_LEN 14u
#define IMG_QOI_V3 0x0Fu

/* Union of the two indexed QOI switches: both share the stream buffer, row
 * offset tables and the 6-bit index, so compile out on the union; with both off
 * the code and static RAM vanish together. */
#if (WVUI_CFG_ENABLE_INDEXQOI == 1) || (WVUI_CFG_ENABLE_INDEXQOI_MASK == 1)
#define QOI_ANY 1
#else
#define QOI_ANY 0
#endif

static wvui_img_stats_t s_stats;

#if QOI_ANY
/* Palette / dictionary / 4-byte random-read scratch for the indexed QOI family;
 * no RAM cost when the switches are off. */
static uint8_t s_pal[IMG_PAL_MAX * IMG_PAL_ENTRY_BYTES];
static uint8_t s_dict[IMG_PAL_MAX];
static uint8_t s_scratch[4];
#endif

/* ---- Small helpers ---- */

const wvui_img_stats_t *wvui_img_stats(void)
{
    return &s_stats;
}

void wvui_img_stats_reset(void)
{
    s_stats.draws = 0u;
    s_stats.drawn = 0u;
    s_stats.pixels = 0u;
    s_stats.unsupported = 0u;
    s_stats.bad_header = 0u;
    s_stats.bad_stream = 0u;
    s_stats.short_reads = 0u;
    s_stats.last_error = WVUI_IMG_ERR_NONE;
#if QOI_ANY
    {
        uint32_t i;

        for (i = 0u; i < (uint32_t)sizeof(s_pal); i++) { s_pal[i] = 0u; }
        for (i = 0u; i < (uint32_t)sizeof(s_dict); i++) { s_dict[i] = 0u; }
    }
#endif
}

const char *wvui_img_err_name(uint8_t err)
{
    switch (err)
    {
    case WVUI_IMG_ERR_NONE: return "none";
    case WVUI_IMG_ERR_NO_BLOB: return "no-blob";
    case WVUI_IMG_ERR_HDR: return "bad-header";
    case WVUI_IMG_ERR_UNSUPPORTED: return "unsupported";
    case WVUI_IMG_ERR_STREAM: return "bad-stream";
    case WVUI_IMG_ERR_READ: return "media-read";
    default: return "?";
    }
}

static uint8_t fail(uint8_t err)
{
    s_stats.last_error = err;
    switch (err)
    {
    case WVUI_IMG_ERR_UNSUPPORTED: s_stats.unsupported++; break;
    case WVUI_IMG_ERR_HDR: s_stats.bad_header++; break;
    case WVUI_IMG_ERR_STREAM: s_stats.bad_stream++; break;
    case WVUI_IMG_ERR_READ: s_stats.short_reads++; break;
    default: break;
    }
    return 0u;
}

/* Alpha scaled by opacity: >>8 instead of /255; zero overhead at opa == 255. */
static uint8_t mul_opa(uint8_t a, uint8_t opa)
{
    if ((a == 0u) || (opa == 0u))
    {
        return 0u;
    }
    if (opa == 255u)
    {
        return a;
    }
    return (uint8_t)(((uint32_t)a * (uint32_t)opa) >> 8);
}

static uint16_t be16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

#if QOI_ANY
/* Big-endian 32-bit: shared by the u32 row index section of both indexed
 * decoders. */
static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}
#endif

/* ---- Format queries ---- */

uint8_t wvui_img_format_supported(uint8_t format)
{
    uint8_t comp = WVUI_ASSET_FMT_COMPRESS(format);
    uint8_t pix = WVUI_ASSET_FMT_PIXEL(format);

    switch (comp)
    {
    case WVUI_ASSET_C_NONE:
        switch (pix)
        {
        case WVUI_PIX_RGB565:
        case WVUI_PIX_A8:
        case WVUI_PIX_A4:
        case WVUI_PIX_A2:
        case WVUI_PIX_A1:
            return 1u;
        default:
            return 0u;
        }

    case WVUI_ASSET_C_INDEXQOI:
#if (WVUI_CFG_ENABLE_INDEXQOI == 1)
        return ((pix == WVUI_PIX_RGB565) || (pix == WVUI_PIX_ARGB8565)) ? 1u : 0u;
#else
        return 0u;
#endif

    case WVUI_ASSET_C_INDEXQOI_MASK:
#if (WVUI_CFG_ENABLE_INDEXQOI_MASK == 1)
        return (pix == WVUI_PIX_A8) ? 1u : 0u;
#else
        return 0u;
#endif

    default:
        /* RLE1/RLE2/QOI/OLED and reserved classes have no decoder: return 0 explicitly,
 * the caller reports it. */
        return 0u;
    }
}

uint8_t wvui_img_format_is_alpha(uint8_t format)
{
    uint8_t comp = WVUI_ASSET_FMT_COMPRESS(format);
    uint8_t pix = WVUI_ASSET_FMT_PIXEL(format);

    if (comp == WVUI_ASSET_C_INDEXQOI_MASK)
    {
        return (uint8_t)((pix == WVUI_PIX_A8) ? 1u : 0u);
    }
    if (comp != WVUI_ASSET_C_NONE)
    {
        return 0u;
    }
    return (uint8_t)(((pix == WVUI_PIX_A8) || (pix == WVUI_PIX_A4) || (pix == WVUI_PIX_A2) ||
                      (pix == WVUI_PIX_A1)) ? 1u : 0u);
}

uint8_t wvui_img_format_pixel_bytes(uint8_t format)
{
    uint8_t comp = WVUI_ASSET_FMT_COMPRESS(format);
    uint8_t pix = WVUI_ASSET_FMT_PIXEL(format);

    if ((comp == WVUI_ASSET_C_NONE) || (comp == WVUI_ASSET_C_INDEXQOI))
    {
        if (pix == WVUI_PIX_RGB565) { return 2u; }
    }
    return 0u; /* Pure alpha bitmaps have no full pixels */
}

void wvui_img_size(const wvui_blob_t *blob, int16_t *w, int16_t *h)
{
    wvui_asset_hdr_t hd;

    if ((w == 0) || (h == 0))
    {
        return;
    }
    *w = 0;
    *h = 0;
    if ((blob == 0) || (blob->hdr == 0) || (blob->size == 0u))
    {
        return;
    }
    wvui_asset_hdr_decode(blob->hdr, &hd);
    if ((WVUI_ASSET_RES_KIND(hd.res_type) != WVUI_ASSET_KIND_IMG) ||
        (WVUI_ASSET_RES_VERSION(hd.res_type) != WVUI_ASSET_HDR_VERSION))
    {
        return;
    }
    if ((hd.w > 32767u) || (hd.h > 32767u))
    {
        return; /* Beyond coordinate range: treat as invalid */
    }
    *w = (int16_t)hd.w;
    *h = (int16_t)hd.h;
}

/* ---- Geometry ---- */
/* Visible source rows/cols = image rect intersected with the current slice;
 * outside draw the slice is the full screen. */
typedef struct
{
    int16_t w;
    int16_t h;
    int16_t x; /* Top-left target (screen coords) */
    int16_t y;
    int16_t col0; /* Visible source columns (endpoints included) */
    int16_t col1;
    int16_t row0; /* Visible source rows (endpoints included) */
    int16_t row1;
    uint8_t visible;
} img_geo_t;

static void geo_from(const wvui_asset_hdr_t *hd, int16_t x, int16_t y, img_geo_t *g)
{
    const wvui_slice_t *sl = wvui_draw_slice();
    int32_t x1 = (int32_t)x + (int32_t)hd->w - 1;
    int32_t y1 = (int32_t)y + (int32_t)hd->h - 1;
    int32_t c0 = ((int32_t)x > (int32_t)sl->x0) ? (int32_t)x : (int32_t)sl->x0;
    int32_t c1 = (x1 < (int32_t)sl->x1) ? x1 : (int32_t)sl->x1;
    int32_t r0 = ((int32_t)y > (int32_t)sl->y0) ? (int32_t)y : (int32_t)sl->y0;
    int32_t r1 = (y1 < (int32_t)sl->y1) ? y1 : (int32_t)sl->y1;

    g->w = (int16_t)hd->w;
    g->h = (int16_t)hd->h;
    g->x = x;
    g->y = y;
    g->visible = 0u;
    if ((c0 > c1) || (r0 > r1))
    {
        return; /* Image and slice do not intersect: draw nothing, not an error */
    }
    g->col0 = (int16_t)(c0 - (int32_t)x);
    g->col1 = (int16_t)(c1 - (int32_t)x);
    g->row0 = (int16_t)(r0 - (int32_t)y);
    g->row1 = (int16_t)(r1 - (int32_t)y);
    g->visible = 1u;
}

static void put_pixel(const img_geo_t *g, int16_t col, int16_t row, wvui_color_t c, uint8_t a)
{
    if (a == 0u)
    {
        return;
    }
    wvui_render_pixel((int16_t)(g->x + col), (int16_t)(g->y + row), c, a);
    s_stats.pixels++;
}

/* ---- Uncompressed RGB565 / ARGB8565 ---- */

static uint8_t draw_raw(const wvui_blob_t *b, const img_geo_t *g, uint8_t px_bytes, uint8_t opa)
{
    uint32_t stride = (uint32_t)(uint16_t)g->w * (uint32_t)px_bytes;
    /* How many pixels per read: pick one of two compile-time constants, avoiding a
 * runtime division library call. */
    uint16_t per = (px_bytes == 2u) ? (uint16_t)(WVUI_CFG_IMG_STREAM_CHUNK / 2u)
                                    : (uint16_t)(WVUI_CFG_IMG_STREAM_CHUNK / 3u);
    int16_t row;

    if (per == 0u)
    {
        per = 1u;
    }

    for (row = g->row0; row <= g->row1; row++)
    {
        uint32_t rowbase = (uint32_t)(uint16_t)row * stride;
        int16_t col = g->col0;

        while (col <= g->col1)
        {
            uint8_t buf[WVUI_CFG_IMG_STREAM_CHUNK];
            uint16_t n = (uint16_t)(g->col1 - col + 1);
            uint16_t i;
            uint32_t off;

            if (n > per)
            {
                n = per;
            }
            off = rowbase + ((uint32_t)(uint16_t)col * (uint32_t)px_bytes);
            if (wvui_asset_read(b, off, buf, (uint32_t)n * (uint32_t)px_bytes) != WVUI_ASSET_READ_OK)
            {
                return WVUI_IMG_ERR_READ;
            }

            for (i = 0u; i < n; i++)
            {
                if (px_bytes == 2u)
                {
                    wvui_color_t c = be16(&buf[(uint32_t)i * 2u]);
                    put_pixel(g, (int16_t)(col + (int16_t)i), row, c, opa);
                }
                else
                {
                    const uint8_t *p = &buf[(uint32_t)i * 3u];
                    uint8_t a = mul_opa(p[0], opa);

                    put_pixel(g, (int16_t)(col + (int16_t)i), row, be16(p + 1), a);
                }
            }
            col = (int16_t)(col + (int16_t)n);
        }
    }
    return WVUI_IMG_ERR_NONE;
}

/* ---- Uncompressed pure alpha bitmaps A8/A4/A2/A1 ---- */
/* Row stride (w * bpp + 7) >> 3; MSB first; bpp in {1,2,4,8} so >>3 suffices. */

static uint8_t alpha_scale(uint8_t raw, uint8_t bpp)
{
    if (bpp == 8u) { return raw; }
    if (bpp == 4u) { return (uint8_t)(raw * 17u); }
    if (bpp == 2u) { return (uint8_t)(raw * 85u); }
    return (raw != 0u) ? 255u : 0u;
}

static uint8_t draw_alpha_fmt(const wvui_blob_t *b, const img_geo_t *g, uint8_t bpp,
                              wvui_color_t fg, uint8_t opa)
{
    uint32_t stride = (((uint32_t)(uint16_t)g->w * (uint32_t)bpp) + 7u) >> 3u;
    uint8_t mask = (uint8_t)((1u << bpp) - 1u);
    int16_t row;

    for (row = g->row0; row <= g->row1; row++)
    {
        uint32_t rowbase = (uint32_t)(uint16_t)row * stride;
        uint32_t byte_last = ((uint32_t)(uint16_t)g->col1 * (uint32_t)bpp + (uint32_t)bpp - 1u) >> 3u;
        int16_t col = g->col0;

        /* Compute the chunk start in pixels, not fixed bytes: the start pixel may not be
         * byte aligned; bpp divides 8, so one pixel never crosses a byte. */
        while (col <= g->col1)
        {
            uint8_t buf[WVUI_CFG_IMG_STREAM_CHUNK];
            uint32_t byte_cur = ((uint32_t)(uint16_t)col * (uint32_t)bpp) >> 3u;
            uint32_t remain = (byte_last - byte_cur) + 1u;
            uint32_t take = (remain < (uint32_t)sizeof(buf)) ? remain : (uint32_t)sizeof(buf);

            if (wvui_asset_read(b, rowbase + byte_cur, buf, take) != WVUI_ASSET_READ_OK)
            {
                return WVUI_IMG_ERR_READ;
            }

            while (col <= g->col1)
            {
                uint32_t bit = ((uint32_t)(uint16_t)col * (uint32_t)bpp);
                uint32_t byte_index = (bit >> 3u) - byte_cur;
                uint8_t shift;
                uint8_t raw;

                if (byte_index >= take)
                {
                    break;
                }
                shift = (uint8_t)(8u - bpp - (uint8_t)(bit & 7u));
                raw = (uint8_t)((buf[byte_index] >> shift) & mask);
                put_pixel(g, col, row, fg, mul_opa(alpha_scale(raw, bpp), opa));
                col++;
            }
        }
    }
    return WVUI_IMG_ERR_NONE;
}

/* ---- Sequential read buffer: indexed streams decode byte by byte (compiled
 * with QOI_ANY only) ---- */
/* One read fetches WVUI_CFG_IMG_STREAM_CHUNK bytes, the callback is not called
 * per byte. */
#if QOI_ANY
typedef struct
{
    const wvui_blob_t *b;
    uint32_t next;  /* Next stream offset to read (relative to the payload start) */
    uint32_t limit; /* Hard limit (= payload length), guards against a corrupted index table
 * jumping around */
    uint16_t len;
    uint16_t pos;
    uint8_t eof;
    uint8_t buf[WVUI_CFG_IMG_STREAM_CHUNK];
} img_stream_t;

static void st_seek(img_stream_t *s, const wvui_blob_t *b, uint32_t off, uint32_t limit)
{
    s->b = b;
    s->next = off;
    s->limit = limit;
    s->len = 0u;
    s->pos = 0u;
    s->eof = 0u;
}

static uint8_t st_u8(img_stream_t *s)
{
    if (s->pos >= s->len)
    {
        uint32_t n;

        if (s->next >= s->limit)
        {
            s->eof = 1u;
            return 0xFFu;
        }
        n = s->limit - s->next;
        if (n > (uint32_t)sizeof(s->buf))
        {
            n = (uint32_t)sizeof(s->buf);
        }
        if (wvui_asset_read(s->b, s->next, s->buf, n) != WVUI_ASSET_READ_OK)
        {
            s->eof = 1u;
            return 0xFFu;
        }
        s->next += n;
        s->len = (uint16_t)n;
        s->pos = 0u;
    }
    return s->buf[s->pos++];
}

/* Random access read of 1 / 2 / 4 bytes (for row index tables); returns 1 =
 * read complete. */
static uint8_t rd(const wvui_blob_t *b, uint32_t off, uint32_t n)
{
    return (uint8_t)(wvui_asset_read(b, off, s_scratch, n) == WVUI_ASSET_READ_OK);
}
#endif /* QOI_ANY */

/* Indexed QOI (V3) payload layout (relative to the payload start): [0]=0x0F
 * version, V2/V1 rejected; [1..2] width, [3..4] height (big-endian, must match
 * the 6-byte header), [5..6] m16, [7..12] reserved, [13] palette count <= 64;
 * [14..] u16 table m16*2 B + u32 table (h-m16)*4 B + palette + QOI stream
 * (per-row encoding, offsets not monotonic). */

#if (WVUI_CFG_ENABLE_INDEXQOI == 1)
typedef struct
{
    uint8_t px_bytes;  /* Palette/pixel byte count: 2 = RGB565, 3 = ARGB8565 */
    uint16_t m16;
    uint8_t pal_count;
    uint32_t tab16_off;
    uint32_t tab32_off;
    uint32_t pal_off;
    uint32_t qoi_off; /* Stream start = pal_off + palette byte count */
    uint32_t limit;   /* Payload length (hard limit) */
} qoi_hdr_t;

static uint8_t qoi_parse(const wvui_blob_t *b, const img_geo_t *g, uint8_t px_bytes, qoi_hdr_t *q)
{
    uint8_t hdr[IMG_QOI_HDR_LEN];
    uint32_t pal_bytes;

    if (wvui_asset_read(b, 0u, hdr, IMG_QOI_HDR_LEN) != WVUI_ASSET_READ_OK)
    {
        return WVUI_IMG_ERR_READ;
    }
    if (hdr[0] != IMG_QOI_V3)
    {
        return WVUI_IMG_ERR_STREAM; /* V3 only: V2/V1 report an error */
    }
    if ((be16(hdr + 1) != (uint16_t)g->w) || (be16(hdr + 3) != (uint16_t)g->h))
    {
        return WVUI_IMG_ERR_STREAM; /* Stream sizes disagree with the 6-byte header: corrupted */
    }

    q->px_bytes = px_bytes;
    q->m16 = be16(hdr + 5);
    q->pal_count = hdr[13];
    if ((q->m16 > (uint16_t)g->h) || (q->pal_count > IMG_PAL_MAX))
    {
        return WVUI_IMG_ERR_STREAM;
    }

    q->tab16_off = IMG_QOI_HDR_LEN;
    q->tab32_off = q->tab16_off + ((uint32_t)q->m16 << 1);
    q->pal_off = q->tab32_off + (((uint32_t)(uint16_t)g->h - (uint32_t)q->m16) << 2);
    pal_bytes = (uint32_t)q->pal_count * (uint32_t)px_bytes;
    q->qoi_off = q->pal_off + pal_bytes;
    q->limit = b->size;
    if (q->qoi_off > q->limit)
    {
        return WVUI_IMG_ERR_STREAM;
    }
    if (pal_bytes != 0u)
    {
        if (wvui_asset_read(b, q->pal_off, s_pal, pal_bytes) != WVUI_ASSET_READ_OK)
        {
            return WVUI_IMG_ERR_READ;
        }
    }
    return WVUI_IMG_ERR_NONE;
}

/* Indexed QOI_MASK (A8 mask) payload: flag byte b1..b0 quantization bits
 * 00/01/10/11 -> 8/7/6/5 bit, b7..b2 must be 0; then m and the u32 entry count
 * (must == h - m), u16 table m*2 B + u32 table (h-m)*4 B, dictionary entry count
 * n (<= 64) with n*1 B dictionary, finally the pixel stream (row offsets
 * relative to the dictionary end, the first byte of each row has no tag). */

/* Row offset read shared by both indexed decoders (u16/u32 sectioned tables). */
#endif /* WVUI_CFG_ENABLE_INDEXQOI (only covers qoi_hdr_t + qoi_parse above) */

#if QOI_ANY
static uint8_t row_index_read(const wvui_blob_t *b, uint32_t tab16_off, uint32_t tab32_off,
                              uint16_t m16, int16_t row, uint32_t *out)
{
    if ((uint16_t)row < m16)
    {
        if (!rd(b, tab16_off + ((uint32_t)(uint16_t)row << 1), 2u))
        {
            return WVUI_IMG_ERR_READ;
        }
        *out = be16(s_scratch);
        return WVUI_IMG_ERR_NONE;
    }
    if (!rd(b, tab32_off + (((uint32_t)((uint16_t)row - m16)) << 2), 4u))
    {
        return WVUI_IMG_ERR_READ;
    }
    *out = be32(s_scratch);
    return WVUI_IMG_ERR_NONE;
}
#endif /* QOI_ANY */

#if (WVUI_CFG_ENABLE_INDEXQOI == 1)
static uint8_t draw_qoi(const wvui_blob_t *b, const img_geo_t *g, uint8_t opa)
{
    const uint8_t px_bytes = wvui_img_format_pixel_bytes(b->hdr[1]);
    qoi_hdr_t q;
    img_stream_t st;
    uint8_t err;
    int16_t row;

    if (px_bytes == 0u)
    {
        return WVUI_IMG_ERR_UNSUPPORTED;
    }
    err = qoi_parse(b, g, px_bytes, &q);
    if (err != WVUI_IMG_ERR_NONE)
    {
        return err;
    }

    for (row = g->row0; row <= g->row1; row++)
    {
        uint32_t row_off = 0u;
        int16_t cur_x = 0;
        uint8_t r = 0u;
        uint8_t gg = 0u;
        uint8_t bb = 0u;
        uint16_t px = 0u;
        uint8_t cur_alpha = 255u;
        uint8_t final_alpha = mul_opa(cur_alpha, opa);

        err = row_index_read(b, q.tab16_off, q.tab32_off, q.m16, row, &row_off);
        if (err != WVUI_IMG_ERR_NONE)
        {
            return err;
        }
        if (q.qoi_off + row_off >= q.limit)
        {
            return WVUI_IMG_ERR_STREAM; /* Corrupted index table: row offset beyond the payload */
        }
        st_seek(&st, b, q.qoi_off + row_off, q.limit);

        /* Decode only to the visible right edge: each row is self-contained, bytes
 * after the edge do not belong to it. */
        while (cur_x <= g->col1)
        {
            uint8_t flag = st_u8(&st);

            if (flag == 0xFFu)
            {
                if (px_bytes == 3u)
                {
                    cur_alpha = st_u8(&st);
                    final_alpha = mul_opa(cur_alpha, opa);
                }
                px = (uint16_t)(((uint16_t)st_u8(&st) << 8) | (uint16_t)st_u8(&st));
                r = (uint8_t)(px >> 11) & 0x1Fu;
                gg = (uint8_t)((px >> 5) & 0x3Fu);
                bb = (uint8_t)(px & 0x1Fu);
            }
            else if (flag == 0xFEu)
            {
                /* New RGB565, alpha unchanged */
                px = (uint16_t)(((uint16_t)st_u8(&st) << 8) | (uint16_t)st_u8(&st));
                r = (uint8_t)(px >> 11) & 0x1Fu;
                gg = (uint8_t)((px >> 5) & 0x3Fu);
                bb = (uint8_t)(px & 0x1Fu);
            }
            else if ((flag & 0xC0u) == 0x40u)
            {
                r = (uint8_t)((r + ((flag >> 4) & 0x03u) - 2u) & 0x1Fu);
                gg = (uint8_t)((gg + ((flag >> 2) & 0x03u) - 2u) & 0x3Fu);
                bb = (uint8_t)((bb + (flag & 0x03u) - 2u) & 0x1Fu);
                px = (uint16_t)(((uint16_t)r << 11) | ((uint16_t)gg << 5) | (uint16_t)bb);
            }
            else if ((flag & 0xC0u) == 0x80u)
            {
                int16_t vg = (int16_t)(flag & 0x3Fu) - 32;
                uint8_t nb = st_u8(&st);

                r = (uint8_t)((r + vg - 8 + (int16_t)((nb >> 4) & 0x0Fu)) & 0x1Fu);
                gg = (uint8_t)((gg + vg) & 0x3Fu);
                bb = (uint8_t)((bb + vg - 8 + (int16_t)(nb & 0x0Fu)) & 0x1Fu);
                px = (uint16_t)(((uint16_t)r << 11) | ((uint16_t)gg << 5) | (uint16_t)bb);
            }
            else if (flag < 0x40u)
            {
                const uint8_t *pe;

                if (flag >= q.pal_count)
                {
                    return WVUI_IMG_ERR_STREAM; /* Beyond the entry count = corrupted stream */
                }
                pe = &s_pal[(uint32_t)flag * (uint32_t)px_bytes];
                if (px_bytes == 3u)
                {
                    cur_alpha = pe[0];
                    final_alpha = mul_opa(cur_alpha, opa);
                    px = be16(pe + 1);
                }
                else
                {
                    px = be16(pe);
                }
                r = (uint8_t)(px >> 11) & 0x1Fu;
                gg = (uint8_t)((px >> 5) & 0x3Fu);
                bb = (uint8_t)(px & 0x1Fu);
            }
            else
            {
                /* RUN: repeat the current pixel, never across rows */
                uint8_t run = (uint8_t)((flag & 0x3Fu) + 1u);

                while ((run-- != 0u) && (cur_x <= g->col1))
                {
                    put_pixel(g, cur_x, row, px, final_alpha);
                    cur_x++;
                }
                continue;
            }

            put_pixel(g, cur_x, row, px, final_alpha);
            cur_x++;
        }

        if (st.eof != 0u)
        {
            return WVUI_IMG_ERR_READ; /* Row stream cut short: the asset is truncated */
        }
    }
    return WVUI_IMG_ERR_NONE;
}
#endif /* WVUI_CFG_ENABLE_INDEXQOI */

#if (WVUI_CFG_ENABLE_INDEXQOI_MASK == 1)
static uint8_t draw_qoi_mask(const wvui_blob_t *b, const img_geo_t *g, wvui_color_t fg, uint8_t opa)
{
    uint8_t head[5];
    uint8_t dict_n;
    img_stream_t st;
    uint8_t q;
    uint8_t shift;
    uint16_t m16;
    uint16_t u32_count;
    uint32_t tab16_off = 5u;
    uint32_t tab32_off;
    uint32_t dict_n_off;
    uint32_t base_off;
    int16_t row;

    if (wvui_asset_read(b, 0u, head, sizeof(head)) != WVUI_ASSET_READ_OK)
    {
        return WVUI_IMG_ERR_READ;
    }
    if ((head[0] & 0xFCu) != 0u)
    {
        return WVUI_IMG_ERR_STREAM; /* Reserved bits not 0: corrupted stream */
    }
    q = (uint8_t)(8u - (head[0] & 0x03u));
    shift = (uint8_t)(8u - q);
    m16 = be16(head + 1);
    u32_count = be16(head + 3);
    if ((m16 > (uint16_t)g->h) || ((uint32_t)u32_count != ((uint32_t)(uint16_t)g->h - (uint32_t)m16)))
    {
        return WVUI_IMG_ERR_STREAM; /* The two table sections must sum to exactly h rows */
    }

    tab32_off = tab16_off + ((uint32_t)m16 << 1);
    dict_n_off = tab32_off + (((uint32_t)(uint16_t)g->h - (uint32_t)m16) << 2);
    if (dict_n_off >= b->size)
    {
        return WVUI_IMG_ERR_STREAM;
    }
    if (wvui_asset_read(b, dict_n_off, s_scratch, 1u) != WVUI_ASSET_READ_OK)
    {
        return WVUI_IMG_ERR_READ;
    }
    dict_n = s_scratch[0];
    if (dict_n > IMG_PAL_MAX)
    {
        return WVUI_IMG_ERR_STREAM;
    }
    base_off = dict_n_off + 1u + (uint32_t)dict_n;
    if (base_off > b->size)
    {
        return WVUI_IMG_ERR_STREAM;
    }
    if ((dict_n != 0u) &&
        (wvui_asset_read(b, dict_n_off + 1u, s_dict, (uint32_t)dict_n) != WVUI_ASSET_READ_OK))
    {
        return WVUI_IMG_ERR_READ;
    }

    for (row = g->row0; row <= g->row1; row++)
    {
        uint32_t row_off = 0u;
        int16_t cur_x = 0;
        uint8_t prev;
        uint8_t err = row_index_read(b, tab16_off, tab32_off, m16, row, &row_off);

        if (err != WVUI_IMG_ERR_NONE)
        {
            return err;
        }
        if ((row_off >= b->size) || (base_off + row_off >= b->size))
        {
            return WVUI_IMG_ERR_STREAM;
        }
        st_seek(&st, b, base_off + row_off, b->size);

        prev = st_u8(&st); /* First byte of a row = first pixel value, no tag */
        put_pixel(g, cur_x, row, fg, mul_opa((uint8_t)((prev << shift) | (prev >> (q - shift))), opa));
        cur_x++;

        while (cur_x <= g->col1)
        {
            uint8_t code = st_u8(&st);

            if (code == 0xFFu)
            {
                prev = st_u8(&st);
                put_pixel(g, cur_x, row, fg,
                          mul_opa((uint8_t)((prev << shift) | (prev >> (q - shift))), opa));
                cur_x++;
            }
            else
            {
                switch (code >> 6)
                {
                case 0: /* INDEX: look up the static dictionary */
                    if ((code & 0x3Fu) >= dict_n)
                    {
                        return WVUI_IMG_ERR_STREAM;
                    }
                    prev = s_dict[code & 0x3Fu];
                    put_pixel(g, cur_x, row, fg,
                              mul_opa((uint8_t)((prev << shift) | (prev >> (q - shift))), opa));
                    cur_x++;
                    break;

                case 1: /* DIFF: two pixels in a row, 3 bit each [-4,+3] */
                    prev = (uint8_t)(prev + (int16_t)((code >> 3) & 0x07u) - 4);
                    put_pixel(g, cur_x, row, fg,
                              mul_opa((uint8_t)((prev << shift) | (prev >> (q - shift))), opa));
                    cur_x++;
                    if (cur_x <= g->col1)
                    {
                        prev = (uint8_t)(prev + (int16_t)(code & 0x07u) - 4);
                        put_pixel(g, cur_x, row, fg,
                                  mul_opa((uint8_t)((prev << shift) | (prev >> (q - shift))), opa));
                        cur_x++;
                    }
                    break;

                case 2: /* DELTA: one pixel, 6 bit [-32,+31] */
                    prev = (uint8_t)(prev + (int16_t)(code & 0x3Fu) - 32);
                    put_pixel(g, cur_x, row, fg,
                              mul_opa((uint8_t)((prev << shift) | (prev >> (q - shift))), opa));
                    cur_x++;
                    break;

                default: /* RUN: repeat prev count+1 times; prev==0 skips the whole run */
                {
                    uint8_t cnt = (uint8_t)((code & 0x3Fu) + 1u);

                    if (prev == 0u)
                    {
                        cur_x = (int16_t)(cur_x + (int16_t)cnt);
                    }
                    else
                    {
                        uint8_t a = mul_opa((uint8_t)((prev << shift) | (prev >> (q - shift))), opa);

                        while ((cnt-- != 0u) && (cur_x <= g->col1))
                        {
                            put_pixel(g, cur_x, row, fg, a);
                            cur_x++;
                        }
                    }
                    break;
                }
                }
            }
        }

        if (st.eof != 0u)
        {
            return WVUI_IMG_ERR_READ;
        }
    }
    return WVUI_IMG_ERR_NONE;
}
#endif /* WVUI_CFG_ENABLE_INDEXQOI_MASK */

/* Compile-out probe: external linkage prevents inlining, the build checks by
 * symbol name; must live in the same #if block as the decoder. */
#if (WVUI_CFG_ENABLE_INDEXQOI == 1)
const uint32_t wvui_img_qoi_v3_compiled = 1u;
#endif
#if (WVUI_CFG_ENABLE_INDEXQOI_MASK == 1)
const uint32_t wvui_img_qoi_mask_compiled = 1u;
#endif

/* ---- Render entry point ---- */

uint8_t wvui_img_render_auto(const wvui_blob_t *blob, int16_t x, int16_t y,
                            wvui_color_t fg_color, uint8_t opa)
{
    wvui_asset_hdr_t hd;
    img_geo_t g;
    uint8_t comp;
    uint8_t pix;
    uint8_t err;

    s_stats.draws++;

    if ((blob == 0) || (blob->hdr == 0) || (blob->size == 0u))
    {
        return fail(WVUI_IMG_ERR_NO_BLOB);
    }
    wvui_asset_hdr_decode(blob->hdr, &hd);
    if ((WVUI_ASSET_RES_KIND(hd.res_type) != WVUI_ASSET_KIND_IMG) ||
        (WVUI_ASSET_RES_VERSION(hd.res_type) != WVUI_ASSET_HDR_VERSION) ||
        (hd.w == 0u) || (hd.h == 0u) || (hd.w > 32767u) || (hd.h > 32767u))
    {
        return fail(WVUI_IMG_ERR_HDR);
    }
    if (!wvui_img_format_supported(hd.format))
    {
        return fail(WVUI_IMG_ERR_UNSUPPORTED);
    }
    if (opa == 0u)
    {
        return 1u; /* Fully transparent: draw nothing, not an error */
    }

    geo_from(&hd, x, y, &g);
    if (g.visible == 0u)
    {
        return 1u; /* Image outside this slice: no decoding needed */
    }

    comp = WVUI_ASSET_FMT_COMPRESS(hd.format);
    pix = WVUI_ASSET_FMT_PIXEL(hd.format);

    if (comp == WVUI_ASSET_C_NONE)
    {
        if ((pix == WVUI_PIX_RGB565))
        {
            err = draw_raw(blob, &g, wvui_img_format_pixel_bytes(hd.format), opa);
        }
        else
        {
            err = draw_alpha_fmt(blob, &g, (uint8_t)((pix == WVUI_PIX_A8) ? 8u :
                                                     (pix == WVUI_PIX_A4) ? 4u :
                                                     (pix == WVUI_PIX_A2) ? 2u : 1u),
                                 fg_color, opa);
        }
    }
    else if (comp == WVUI_ASSET_C_INDEXQOI)
    {
#if (WVUI_CFG_ENABLE_INDEXQOI == 1)
        err = draw_qoi(blob, &g, opa);
#else
        /* The switch is off, this branch is unreachable (the support matrix already
 * returned 0); kept so no branch stays empty. */
        err = WVUI_IMG_ERR_UNSUPPORTED;
#endif
    }
    else
    {
#if (WVUI_CFG_ENABLE_INDEXQOI_MASK == 1)
        err = draw_qoi_mask(blob, &g, fg_color, opa);
#else
        err = WVUI_IMG_ERR_UNSUPPORTED;
#endif
    }

    if (err != WVUI_IMG_ERR_NONE)
    {
        return fail(err);
    }
    s_stats.drawn++;
    s_stats.last_error = WVUI_IMG_ERR_NONE;
    return 1u;
}
