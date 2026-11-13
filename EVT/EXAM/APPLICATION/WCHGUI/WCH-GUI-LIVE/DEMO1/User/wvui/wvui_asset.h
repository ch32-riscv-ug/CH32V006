#ifndef WVUI_ASSET_H
#define WVUI_ASSET_H

/**
 * @file wvui_asset.h
 * @brief Asset resolver: 16-bit narrow IDs to wvui_blob_t, backend bound at runtime.
 * @note  Out-of-range read, short length, missing file, unbound backend - all fill 0xFF, never partial data.
 */

#include <stdint.h>
#include "wvui_config.h"
#include "wvui_types.h"

/** 16-bit narrow asset ID, bit layout: `[kind:2][class:3][index:11]`. */
typedef uint16_t wvui_asset_id_t;

/** ID field bit widths. */
#define WVUI_ASSET_INDEX_BITS 11u
#define WVUI_ASSET_CLASS_BITS 3u
#define WVUI_ASSET_KIND_BITS  2u

/** ID field shifts. */
#define WVUI_ASSET_INDEX_SHIFT 0u
#define WVUI_ASSET_CLASS_SHIFT (WVUI_ASSET_INDEX_BITS)                              /* 11 */
#define WVUI_ASSET_KIND_SHIFT  (WVUI_ASSET_INDEX_BITS + WVUI_ASSET_CLASS_BITS)       /* 14 */

/** ID field masks. */
#define WVUI_ASSET_INDEX_MASK 0x07FFu
#define WVUI_ASSET_CLASS_MASK 0x07u
#define WVUI_ASSET_KIND_MASK  0x03u

/** Index upper bound 2047; 2048 entries per (kind,class) pair. */
#define WVUI_ASSET_INDEX_MAX (WVUI_ASSET_INDEX_MASK)

/** kind: top-level asset classes. */
enum
{
    WVUI_ASSET_KIND_IMG = 0,   /**< Image */
    WVUI_ASSET_KIND_FONT = 1,  /**< Font */
    WVUI_ASSET_KIND_STR = 2,   /**< String */
    WVUI_ASSET_KIND_MAP = 3    /**< Map table (image map table / option group) */
};

/** class: interpreted per kind; for images the class is the compression class.
 *  An image class must equal the high nibble of the self-describing header's format byte. */
enum
{
    WVUI_ASSET_C_NONE = 0,           /**< No compression */
    WVUI_ASSET_C_RLE1 = 1,           /**< Plain RLE */
    WVUI_ASSET_C_RLE2 = 2,           /**< Improved RLE */
    WVUI_ASSET_C_QOI = 3,            /**< Plain QOI (no dictionary) */
    WVUI_ASSET_C_INDEXQOI = 4,       /**< Indexed QOI (V3 stream) */
    WVUI_ASSET_C_INDEXQOI_MASK = 5,  /**< Indexed QOI_MASK (A8 mask only) */
    WVUI_ASSET_C_OLED = 6,           /**< OLED dot matrix */
    WVUI_ASSET_C_RESERVED = 7        /**< Reserved */
};

/** Font class (two backends); encoded into the ID for tooling checks. */
enum
{
    WVUI_ASSET_FC_REAL = 0,     /**< True format (proportional metrics + ranges + sparse) */
    WVUI_ASSET_FC_IMGSTRIP = 1  /**< Bitmap glyph backend (contiguous image ID range + cells) */
};

/** ID packing and unpacking: the bit layout above is authoritative. */
#define WVUI_ASSET_ID(kind, cls, index)                                        \
    ((wvui_asset_id_t)((((uint32_t)(kind) & WVUI_ASSET_KIND_MASK) << WVUI_ASSET_KIND_SHIFT) | \
                      (((uint32_t)(cls) & WVUI_ASSET_CLASS_MASK) << WVUI_ASSET_CLASS_SHIFT) | \
                      ((uint32_t)(index) & WVUI_ASSET_INDEX_MASK)))

#define WVUI_ASSET_ID_KIND(id)  ((uint8_t)(((uint32_t)(id) >> WVUI_ASSET_KIND_SHIFT) & WVUI_ASSET_KIND_MASK))
#define WVUI_ASSET_ID_CLASS(id) ((uint8_t)(((uint32_t)(id) >> WVUI_ASSET_CLASS_SHIFT) & WVUI_ASSET_CLASS_MASK))
#define WVUI_ASSET_ID_INDEX(id) ((uint16_t)(((uint32_t)(id) >> WVUI_ASSET_INDEX_SHIFT) & WVUI_ASSET_INDEX_MASK))

#define WVUI_IMG(cls, index)  WVUI_ASSET_ID(WVUI_ASSET_KIND_IMG, (cls), (index))
#define WVUI_FONT(cls, index) WVUI_ASSET_ID(WVUI_ASSET_KIND_FONT, (cls), (index))

/** Self-describing header length: 6 bytes. */
#define WVUI_ASSET_HDR_SIZE 6u

/** Global asset format version (high nibble of hdr[0]); each stream carries its own self-identifying version, unknown versions take the diagnostics path. */
#define WVUI_ASSET_HDR_VERSION 1u

/** hdr[1] = [compress:4][pixel:4]. */
#define WVUI_ASSET_FMT_COMPRESS(fmt) ((uint8_t)(((fmt) >> 4) & 0x0Fu))
#define WVUI_ASSET_FMT_PIXEL(fmt)    ((uint8_t)((fmt) & 0x0Fu))

/** Pixel format: low nibble of the format code. */
enum
{
    WVUI_PIX_RGB565 = 0x0,    /**< 5/6/5 components */
    WVUI_PIX_A8 = 0xB,        /**< 8-bit alpha */
    WVUI_PIX_A4 = 0xC,        /**< 4-bit alpha */
    WVUI_PIX_A2 = 0xD,        /**< 2-bit alpha */
    WVUI_PIX_A1 = 0xE,        /**< 1-bit alpha */
    WVUI_PIX_OLED = 0xF       /**< 1-bit monochrome dot matrix */
};

/** Asset type byte: hdr[0] = [version:4][kind:4]. */
#define WVUI_ASSET_RES_TYPE(kind) ((uint8_t)(((WVUI_ASSET_HDR_VERSION & 0x0Fu) << 4) | ((uint8_t)(kind) & 0x0Fu)))
#define WVUI_ASSET_RES_KIND(rt)   ((uint8_t)((rt) & 0x0Fu))
#define WVUI_ASSET_RES_VERSION(rt) ((uint8_t)(((rt) >> 4) & 0x0Fu))

/**
 * @brief Decoded self-describing header.
 * @note  The 6 header bytes: `[res_type][format][w_hi][w_lo][h_hi][h_lo]`, sizes big-endian.
 */
typedef struct
{
    uint8_t res_type;  /**< Version in the high nibble, kind in the low nibble */
    uint8_t format;    /**< Compression class in the high nibble, pixel format in the low nibble */
    uint16_t w;        /**< Width, pixels */
    uint16_t h;        /**< Height, pixels */
} wvui_asset_hdr_t;

/**
 * @brief Decode the self-describing header, sizes restored as big-endian.
 * @param hdr  Points to 6 readable bytes
 * @param out  Decoded header; must not be NULL
 * @note  Pure decoding, no validation; use wvui_asset_hdr_check() to validate.
 */
void wvui_asset_hdr_decode(const uint8_t *hdr, wvui_asset_hdr_t *out);

/**
 * @brief Check that a self-describing header and an ID are self-consistent.
 * @param hdr  Points to 6 readable bytes
 * @param id   Asset ID
 * @param err  Failure reason output; may be NULL
 * @return     1 = header usable; 0 = not usable
 * @note  Checks that the version is known, the kind matches, and the image size is non-zero; the failure code goes through err.
 */
uint8_t wvui_asset_hdr_check(const uint8_t *hdr, wvui_asset_id_t id, uint8_t *err);

/**
 * @brief Opened asset handle.
 * @note  With a read-callback backend hdr points to an internal scratch and stays valid until the next open / bind; with an in-chip backend it stays valid forever.
 */
typedef struct
{
    const uint8_t *hdr;  /**< Self-describing header (6 B) */
    uint32_t size;       /**< Body byte count (header excluded) */
    uint32_t addr;       /**< Base address of the body in the storage */
} wvui_blob_t;

/** Media read result (diagnostic contract). */
typedef enum
{
    WVUI_ASSET_READ_OK = 0,       /**< Fully read */
    WVUI_ASSET_READ_PARTIAL = 1,  /**< Tail short, padded with 0xFF */
    WVUI_ASSET_READ_MISSING = 2   /**< Whole range missing, all 0xFF (erased state) */
} wvui_asset_read_t;

/** Media read callback; the kernel never touches the media, the real-hardware read callback is implemented by the port. */
typedef wvui_asset_read_t (*wvui_asset_read_fn)(void *ctx, uint32_t addr, uint8_t *dst, uint32_t len);

/** Asset table entry: ID to storage location; addr is the asset start (6-byte header included), size counts only the body. */
typedef struct
{
    wvui_asset_id_t id;    /**< Asset ID */
    uint32_t addr;        /**< Address of the asset start (header included) in the storage */
    uint32_t size;        /**< Body byte count, header excluded */
    const uint8_t *data;  /**< Direct pointer for the in-chip array backend; when non-NULL addr is unused */
} wvui_asset_entry_t;

/**
 * @brief Storage backend: table + read callback.
 * @note  The table must be sorted by id ascending; bind rejects unsorted tables, otherwise binary search silently misses.
 */
typedef struct
{
    const char *name;                /**< Backend name, for diagnostics only */
    const wvui_asset_entry_t *table;  /**< Asset table, sorted by id ascending */
    uint16_t count;                  /**< Number of entries */
    wvui_asset_read_fn read;          /**< Used when data is NULL; may be NULL for pure in-chip */
    void *ctx;                       /**< Callback's own context */
} wvui_asset_backend_t;

/**
 * @brief Bind the storage backend, swappable at runtime.
 * @param be  backend description; when count != 0 the table must be non-empty and sorted by id ascending
 * @return    1 = success; 0 = invalid arguments or table not sorted by id ascending
 * @note  On failure the previous binding is left untouched.
 * @note  One data path per backend: with a read callback no entry carries data,
 *        without one every entry must; a mixed table is rejected at bind.
 */
uint8_t wvui_asset_bind(const wvui_asset_backend_t *be);

/**
 * @brief Unbind the backend.
 * @note  Afterwards wvui_asset_open() fails across the board with reason WVUI_ASSET_ERR_NO_BACKEND.
 */
void wvui_asset_unbind(void);

/**
 * @brief Get the current backend.
 * @return The bound backend; NULL when unbound
 */
const wvui_asset_backend_t *wvui_asset_backend(void);

/** Failure reason of opening an asset (diagnostic contract). */
typedef enum
{
    WVUI_ASSET_ERR_NONE = 0,     /**< No failure */
    WVUI_ASSET_ERR_NO_BACKEND,   /**< No backend bound */
    WVUI_ASSET_ERR_NOT_FOUND,    /**< ID not in the table */
    WVUI_ASSET_ERR_TABLE,        /**< Table invalid (rejected at bind time) */
    WVUI_ASSET_ERR_READING,      /**< Media read incomplete (padded with 0xFF) */
    WVUI_ASSET_ERR_BAD_KIND,     /**< Header kind does not match the ID kind */
    WVUI_ASSET_ERR_BAD_VERSION,  /**< Global format version in the header unknown */
    WVUI_ASSET_ERR_BAD_SIZE,     /**< Size invalid (zero or over the limit) */
    WVUI_ASSET_ERR_UNSUPPORTED   /**< Compression class / pixel format unsupported */
} wvui_asset_err_t;

/**
 * @brief ASCII name of a failure reason.
 * @param err  failure code
 * @return     name string; a fixed placeholder for unknown codes
 */
const char *wvui_asset_err_name(uint8_t err);

/** Asset diagnostics statistics. */
typedef struct
{
    uint32_t opens;                /**< Number of open calls */
    uint32_t fails;                /**< How many of those failed */
    uint32_t direct_hits;          /**< In-chip direct pointer hits */
    uint32_t short_reads;          /**< Incomplete media reads */
    uint32_t not_found;            /**< Times the ID was not found in the table */
    uint8_t last_error;            /**< Reason of the most recent failure */
    wvui_asset_id_t last_error_id;  /**< ID of the most recent failure */
} wvui_asset_stats_t;

/**
 * @brief Get the diagnostics statistics.
 * @return Read-only pointer to the internal statistics; valid until reset
 */
const wvui_asset_stats_t *wvui_asset_stats(void);

/** Zero the diagnostics statistics; leaves the backend binding untouched. */
void wvui_asset_stats_reset(void);

/**
 * @brief Open an asset by ID, read and validate its header.
 * @param id   asset ID
 * @param out  output handle; non-NULL
 * @return     1 = success; 0 = failure
 * @note  On failure out is filled with the erased state (hdr = 6x0xFF, size = 0, addr = 0), so a drawing path that skips error checks still draws nothing.
 * @note  Failure reasons: see wvui_asset_stats().
 */
uint8_t wvui_asset_open(wvui_asset_id_t id, wvui_blob_t *out);

/**
 * @brief Read a span of the body, always fills len bytes.
 * @param b    Handle
 * @param off  Offset from the body start (b->addr)
 * @param dst  Destination buffer, at least len bytes
 * @param len  Byte count to read; 0 or dst NULL returns MISSING
 * @return     OK / PARTIAL / MISSING, see wvui_asset_read_t
 * @note  off >= size means all 0xFF; off + len past the tail means the valid part is read as-is and the tail is padded with 0xFF.
 */
wvui_asset_read_t wvui_asset_read(const wvui_blob_t *b, uint32_t off, uint8_t *dst, uint32_t len);

/**
 * @brief Convenience version reading just 1 byte.
 * @param b    Handle
 * @param off  Offset from the body start
 * @return     The byte value; 0xFF when off is out of range
 */
uint8_t wvui_asset_read_u8(const wvui_blob_t *b, uint32_t off);

/* Type contract: the ID is exactly 16 bits, all fields round-trip. */
WVUI_STATIC_ASSERT(WVUI_ASSET_INDEX_BITS + WVUI_ASSET_CLASS_BITS + WVUI_ASSET_KIND_BITS == 16,
                  "asset id must use exactly 16 bits");
WVUI_STATIC_ASSERT(sizeof(wvui_asset_id_t) == 2, "asset id is 16-bit");
WVUI_STATIC_ASSERT(WVUI_ASSET_ID(WVUI_ASSET_KIND_IMG, WVUI_ASSET_C_INDEXQOI, 46) ==
                  (wvui_asset_id_t)((4u << 11) | 46u),
                  "id layout is [kind:2][class:3][index:11]");
WVUI_STATIC_ASSERT(WVUI_ASSET_ID_KIND(WVUI_ASSET_ID(WVUI_ASSET_KIND_FONT, WVUI_ASSET_FC_REAL, 3)) == WVUI_ASSET_KIND_FONT,
                  "id kind round-trips");
WVUI_STATIC_ASSERT(WVUI_ASSET_ID_CLASS(WVUI_ASSET_ID(WVUI_ASSET_KIND_FONT, WVUI_ASSET_FC_REAL, 3)) == WVUI_ASSET_FC_REAL,
                  "id class round-trips");
WVUI_STATIC_ASSERT(WVUI_ASSET_ID_INDEX(WVUI_ASSET_ID(WVUI_ASSET_KIND_FONT, WVUI_ASSET_FC_REAL, 3)) == 3u,
                  "id index round-trips");
WVUI_STATIC_ASSERT(WVUI_ASSET_ID_INDEX(WVUI_ASSET_ID(0, 0, WVUI_ASSET_INDEX_MAX)) == WVUI_ASSET_INDEX_MAX,
                  "index field holds 11 bits");

#endif /* WVUI_ASSET_H */
