/**
 * @file wvui_draw.c
 * @brief Draw primitive implementation (contracts in wvui_draw.h): int32 fixed-point, bound magnitudes before multiplying.
 * @note  Division happens only before the per-pixel loops (once per primitive); check bounds before multiplying, that order must not be swapped.
 */

#include "wvui_draw.h"

#include "wvui_render.h"

/* Coverage rule: a 1-pixel-wide analytic ramp shared by all four primitives; outer − inner error ≤ 2/255. */

/* Integer form of (a*b)/255: (x + 128) * 257 >> 16, no division, rounded to nearest. */
static uint32_t mul255(uint32_t a, uint32_t b)
{
    return (((a * b) + 128u) * 257u) >> 16;
}

/* Coverage × widget opacity → alpha. Container opacity is multiplied once more by wvui_render_pixel;
 * doing it here as well would fade twice. */
static void cov_pixel(int16_t x, int16_t y, wvui_color_t c, uint32_t cov, uint8_t opa)
{
    uint32_t a = mul255(cov, opa);

    if (a == 0u)
    {
        return;
    }
    if (a > 255u)
    {
        a = 255u;
    }
    wvui_render_pixel(x, y, c, (uint8_t)a);
}

uint32_t wvui_isqrt32(uint32_t v)
{
    uint32_t res = 0u;
    uint32_t bit = 1u << 30;

    while (bit > v)
    {
        bit >>= 2;
    }
    while (bit != 0u)
    {
        if (v >= res + bit)
        {
            v -= res + bit;
            res = (res >> 1) + bit;
        }
        else
        {
            res >>= 1;
        }
        bit >>= 2;
    }
    return res;
}

/* ---- Trigonometry ---- */
/* 64-entry Q15 sine table with linear interpolation (512-step scale): entry = round(sin(2π·i/64)·32767),
 * error ≤ about 40/32768. Entry 65 = entry 1: interpolation at a = 511 need not wrap around. */
static const int16_t s_sin64[65] = {
         0,   3212,   6393,   9512,  12539,  15446,  18204,  20787,
     23170,  25329,  27245,  28898,  30273,  31356,  32137,  32609,
     32767,  32609,  32137,  31356,  30273,  28898,  27245,  25329,
     23170,  20787,  18204,  15446,  12539,   9512,   6393,   3212,
         0,  -3212,  -6393,  -9512, -12539, -15446, -18204, -20787,
    -23170, -25329, -27245, -28898, -30273, -31356, -32137, -32609,
    -32767, -32609, -32137, -31356, -30273, -28898, -27245, -25329,
    -23170, -20787, -18204, -15446, -12539,  -9512,  -6393,  -3212,
         0,
};

int16_t wvui_sin512(uint16_t a)
{
    uint32_t i = (uint32_t)(a & 0x1FFu); /* 512-step scale: modulo is just a mask */
    uint32_t idx = i >> 3;               /* one table slot per 8 steps */
    uint32_t frac = i & 7u;
    int32_t v0 = s_sin64[idx];
    int32_t v1 = s_sin64[idx + 1u];

    /* Linear interpolation: v0 + (v1 − v0)·frac/8 (>>3, zero division)*/
    return (int16_t)(v0 + (((v1 - v0) * (int32_t)frac) >> 3));
}

int16_t wvui_cos512(uint16_t a)
{
    return wvui_sin512((uint16_t)((a + 128u) & 0x1FFu)); /* cos = sin + 90° */
}

/* ---- Parameter-space clipping ---- */
/* Liang–Barsky: u is 0..4096 fixed-point, p<0 tightens the lower bound and p>0 the upper bound; intermediates
 * fit in int32. Returns 0 = the whole line lies outside the window. */
static uint8_t seg_clip_win(int32_t *px0, int32_t *py0, int32_t *px1, int32_t *py1,
                            int32_t wx0, int32_t wy0, int32_t wx1, int32_t wy1)
{
    int32_t x0 = *px0;
    int32_t y0 = *py0;
    int32_t x1 = *px1;
    int32_t y1 = *py1;
    int32_t dx = x1 - x0;
    int32_t dy = y1 - y0;
    int32_t u0 = 0;    /* start parameter (12 fractional-bit fixed-point) */
    int32_t u1 = 4096; /* end parameter (12 fractional-bit fixed-point) */
    int32_t p[4];
    int32_t q[4];
    uint8_t i;

    p[0] = -dx;
    q[0] = x0 - wx0; /* x ≥ wx0 */
    p[1] = dx;
    q[1] = wx1 - x0; /* x ≤ wx1 */
    p[2] = -dy;
    q[2] = y0 - wy0; /* y ≥ wy0 */
    p[3] = dy;
    q[3] = wy1 - y0; /* y ≤ wy1 */

    for (i = 0u; i < 4u; i++)
    {
        if (p[i] == 0)
        {
            if (q[i] < 0)
            {
                return 0u; /* parallel to this edge and outside */
            }
            continue;
        }

        {
            int32_t r = (q[i] << 12) / p[i]; /* cold path: at most 4 divisions per call */

            if (p[i] < 0)
            {
                if (r > u1)
                {
                    return 0u;
                }
                if (r > u0)
                {
                    u0 = r;
                }
            }
            else
            {
                if (r < u0)
                {
                    return 0u;
                }
                if (r < u1)
                {
                    u1 = r;
                }
            }
        }
    }

    /* End-point recomputation: widen u by 32/4096 then clamp back to [0,4096] to compensate integer-division
     * truncation, so the visible region does not lose a segment; only widening, never shrinking, so coverage
     * is never underestimated. */
    if (u0 > 0)
    {
        u0 -= 32;
        if (u0 < 0)
        {
            u0 = 0;
        }
        *px0 = x0 + (dx * u0) / 4096;
        *py0 = y0 + (dy * u0) / 4096;
    }
    if (u1 < 4096)
    {
        u1 += 32;
        if (u1 > 4096)
        {
            u1 = 4096;
        }
        *px1 = x0 + (dx * u1) / 4096;
        *py1 = y0 + (dy * u1) / 4096;
    }
    return 1u;
}

/* ---- Anti-aliased line segment (single-pass capsule coverage) ---- */
/* Q8 fixed-point: df = d·256, r_fp = w·128. Coverage = 1-pixel ramp of the distance d from the center to
 * the segment: d ≤ (w-1)/2 → 255, d ≥ (w+1)/2 → 0. Perpendicular distance on the body, end-point distance
 * in the caps, no double-blending at the seams. */
void wvui_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t width,
                   wvui_line_cap_t cap, wvui_color_t color, uint8_t opa)
{
    int32_t ax0;
    int32_t ay0;
    int32_t ax1;
    int32_t ay1;
    int32_t dx;
    int32_t dy;
    int32_t len2;
    int32_t r_fp;
    int32_t thr_in_cr;
    int32_t thr_out_cr;
    int32_t thr_in;
    int32_t thr_out;
    int32_t ext;
    uint32_t len;
    uint32_t inv_len;
    int16_t minx;
    int16_t miny;
    int16_t maxx;
    int16_t maxy;
    int16_t px;
    int16_t py;
    wvui_rect_t clip;

    if ((width == 0u) || (opa == 0u))
    {
        return;
    }

    clip = wvui_render_clip();
    if (wvui_rect_is_empty(&clip) != 0u)
    {
        return; /* not in a chunk / empty clip: write no pixel */
    }

    ext = (int32_t)width / 2 + 2; /* half width + AA margin */
    ax0 = x0;
    ay0 = y0;
    ax1 = x1;
    ay1 = y1;

    /* Window = clip rect ± ext; skip clipping when both end points are inside the window. */
    {
        int32_t wx0 = (int32_t)clip.x0 - ext;
        int32_t wy0 = (int32_t)clip.y0 - ext;
        int32_t wx1 = (int32_t)clip.x1 + ext;
        int32_t wy1 = (int32_t)clip.y1 + ext;

        if ((ax0 < wx0) || (ax0 > wx1) || (ay0 < wy0) || (ay0 > wy1) ||
            (ax1 < wx0) || (ax1 > wx1) || (ay1 < wy0) || (ay1 > wy1))
        {
            if (seg_clip_win(&ax0, &ay0, &ax1, &ay1, wx0, wy0, wx1, wy1) == 0u)
            {
                return;
            }
        }
    }

    dx = ax1 - ax0;
    dy = ay1 - ay0;
    len2 = dx * dx + dy * dy;
    len = wvui_isqrt32((uint32_t)len2);
    /* Round the reciprocal instead of truncating: truncation steepens the AA ramp and leaves faint
     * shadows on adjacent rows. */
    inv_len = (len != 0u) ? (((1u << 16) + (len >> 1)) / len) : 0u;

    r_fp = (int32_t)width * 128;
    /* Body thresholds: |cross| ≥ ceil((w+1)/2·len) → 0, ≤ floor((w-1)/2·len) → 255, the middle uses the
     * reciprocal ramp; thr_out_cr is also the overflow bound (diff < len, product < 65536 + len/2). */
    thr_in_cr = (((int32_t)width - 1) * (int32_t)len) >> 1;
    thr_out_cr = ((((int32_t)width + 1) * (int32_t)len) + 1) >> 1;
    thr_in = (int32_t)(width - 1u) * (int32_t)(width - 1u);
    thr_out = (int32_t)(width + 1u) * (int32_t)(width + 1u);

    minx = (int16_t)(((ax0 < ax1) ? ax0 : ax1) - ext);
    maxx = (int16_t)(((ax0 > ax1) ? ax0 : ax1) + ext);
    miny = (int16_t)(((ay0 < ay1) ? ay0 : ay1) - ext);
    maxy = (int16_t)(((ay0 > ay1) ? ay0 : ay1) + ext);

    if (minx < clip.x0) { minx = clip.x0; }
    if (maxx > clip.x1) { maxx = clip.x1; }
    if (miny < clip.y0) { miny = clip.y0; }
    if (maxy > clip.y1) { maxy = clip.y1; }
    if ((minx > maxx) || (miny > maxy))
    {
        return;
    }

    for (py = miny; py <= maxy; py++)
    {
        for (px = minx; px <= maxx; px++)
        {
            int32_t ex = (int32_t)px - ax0;
            int32_t ey = (int32_t)py - ay0;
            int32_t dot = ex * dx + ey * dy;
            int32_t cov = 0;

            /* Strict inequalities for the cap test: dot == 0 or len2 means the pixel center belongs to the
             * body (perpendicular distance 0, coverage 255); using <= / >= instead would shorten a
             * 1-pixel line by 1 pixel. */
            if ((len2 == 0) || (dot < 0) || (dot > len2))
            {
                /* Cap region (includes the whole circle of a zero-length segment). Butt cap: empty beyond the cut plane. */
                if (cap == WVUI_CAP_ROUND)
                {
                    int32_t far_end = (dot > len2) ? 1 : 0;
                    int32_t ddx = far_end ? ((int32_t)px - ax1) : ex;
                    int32_t ddy = far_end ? ((int32_t)py - ay1) : ey;
                    int32_t d2 = ddx * ddx + ddy * ddy;
                    int32_t q4 = 4 * d2;

                    if (q4 <= thr_in)
                    {
                        cov = 255;
                    }
                    else if (q4 < thr_out)
                    {
                        /* Square root only inside the AA band: d2 << 16 < 2^30, no overflow. */
                        uint32_t df = wvui_isqrt32((uint32_t)d2 << 16);

                        cov = (int32_t)((((uint32_t)(r_fp + 128)) - df) * 255u >> 8);
                    }
                }
            }
            else
            {
                int32_t cr = ex * dy - ey * dx;

                if (cr < 0)
                {
                    cr = -cr;
                }
                if (cr >= thr_out_cr)
                {
                    continue; /* outside the band: coverage 0 (also the overflow check) */
                }
                if (cr <= thr_in_cr)
                {
                    cov = 255; /* inside the band: coverage 255 */
                }
                else
                {
                    uint32_t q8 = ((uint32_t)(thr_out_cr - cr) * inv_len) >> 8;

                    cov = (int32_t)((q8 * 255u) >> 8);
                }
            }

            if (cov > 0)
            {
                cov_pixel(px, py, color, (uint32_t)cov, opa);
            }
        }
    }
}

/* ---- Rounded rectangle ---- */
/* Coverage = 1-pixel ramp of the distance d from the pixel center to the inset rectangle
 * [X0+r, X1-r]×[Y0+r, Y1-r]; r=0 is square, r=min(w,h)/2 gives a capsule or circle. Internals use
 * 1/16 pixel units throughout; squared difference sums stay < 1.3e8 (within int32). */

/* Coverage (0..255) of the pixel [px,px+1]×[py,py+1] for the distance-≤-r region of the inset rectangle:
 * cov = clamp(0.5 + (r − d), 0, 1); ramp in 1/8-pixel fixed-point (isqrt(d²·64) = 8·d),
 * error ≤ 2/255, one isqrt inside the band. */
static uint8_t rr_px_cov(int32_t bx0, int32_t by0, int32_t bx1, int32_t by1,
                         int32_t r16, int16_t px, int16_t py)
{
    int32_t cx = ((int32_t)px * 16) + 8; /* pixel center (1/16 units) */
    int32_t cy = ((int32_t)py * 16) + 8;
    int32_t nx;
    int32_t ny;
    int32_t d2;
    int32_t ro = r16 + 8; /* center distance ≥ ro ⇒ 0 (outer edge of the ramp) */
    int32_t ri = r16 - 8; /* center distance ≤ ri ⇒ 255 (inner edge of the ramp) */
    uint32_t dq;
    int32_t t;

    /* Separation from the center to the box (0 when overlapping) */
    if (bx1 <= cx) { nx = cx - bx1; }
    else if (bx0 >= cx) { nx = bx0 - cx; }
    else { nx = 0; }

    if (by1 <= cy) { ny = cy - by1; }
    else if (by0 >= cy) { ny = by0 - cy; }
    else { ny = 0; }

    if ((nx == 0) && (ny == 0))
    {
        return 255; /* center inside the box: straight edges align with the pixel grid, no partial coverage */
    }

    d2 = (nx * nx) + (ny * ny);
    if (d2 >= (ro * ro))
    {
        return 0;
    }
    if ((ri > 0) && (d2 <= (ri * ri)))
    {
        return 255;
    }

    /* Band (about 1 pixel wide): t = 64·(ro − d) ∈ (0,1024), coverage = t·255/1024. */
    dq = wvui_isqrt32((uint32_t)d2 * 64u); /* = floor(8·d) */
    t = (ro * 64) - ((int32_t)dq * 8);    /* 64·(ro − d) ∈ (0, 1024) */
    if (t <= 0)
    {
        return 0;
    }
    if (t >= 1024)
    {
        return 255;
    }
    return (uint8_t)((((uint32_t)t * 255u) + 512u) >> 10);
}

/* ---- Box (independent corner styles) ---- */
/* Coverage rules match the rounded rectangle (256 levels, no division, no floating point).
 * ROUND: the inset rectangle shrinks to a point, reusing the circle coverage of rr_px_cov.
 * CHAMFER: half-plane test against the 45° cut line x_c + y_c = c. */

/* Coverage (0..255) of a pixel against a rectangle with independent corner styles. bx/by are closed
 * 1/16-unit intervals [x0*16, (x1+1)*16]; r16 is the corner radius and ch16 the chamfer cut-line constant
 * (both in 1/16 units). */
static uint8_t box_px_cov(int32_t bx0, int32_t by0, int32_t bx1, int32_t by1,
                          const uint8_t *corners, int32_t r16, int32_t ch16,
                          int16_t px, int16_t py)
{
    int32_t lo_x = (int32_t)px * 16;
    int32_t hi_x = lo_x + 16;
    int32_t lo_y = (int32_t)py * 16;
    int32_t hi_y = lo_y + 16;
    uint8_t left;
    uint8_t right;
    uint8_t top;
    uint8_t bottom;
    uint8_t style;
    int32_t cx;
    int32_t cy;

    /* Whole cell outside the given rectangle scores 0: box_px_cov runs once for the outer loop and once
     * for the inner loop, and the inner call relies on this test to zero pixels inside the outer shape
     * but outside the inner one; otherwise a 1-pixel border would go missing. */
    if ((lo_x < bx0) || (hi_x > bx1) || (lo_y < by0) || (hi_y > by1))
    {
        return 0;
    }

    if (r16 <= 0)
    {
        return 255; /* all corners square: pixels inside the rectangle score 255 */
    }

    left = (uint8_t)(lo_x < bx0 + r16);
    right = (uint8_t)(hi_x > bx1 - r16);
    top = (uint8_t)(lo_y < by0 + r16);
    bottom = (uint8_t)(hi_y > by1 - r16);

    if (((left == 0u) && (right == 0u)) || ((top == 0u) && (bottom == 0u)))
    {
        return 255; /* in the middle band: more than r from every corner */
    }

    if (left != 0u) { cx = bx0 + r16; } else { cx = bx1 - r16; }
    if (top != 0u) { cy = by0 + r16; } else { cy = by1 - r16; }

    if ((left != 0u) && (top != 0u)) { style = corners[WVUI_BOX_LT]; }
    else if ((right != 0u) && (top != 0u)) { style = corners[WVUI_BOX_RT]; }
    else if ((left != 0u) && (bottom != 0u)) { style = corners[WVUI_BOX_LB]; }
    else { style = corners[WVUI_BOX_RB]; }

    if (style == WVUI_BOX_SQUARE)
    {
        return 255;
    }
    if (style == WVUI_BOX_ROUND)
    {
        /* Inset rectangle shrinks to a point, i.e. a circle: reuse the circle coverage of rr_px_cov */
        return rr_px_cov(cx, cy, cx, cy, r16, px, py);
    }

    /* Chamfer: coverage set by the distance to the cut line x_c + y_c = ch16, normal (1,1)/√2,
     * Q16 factor 2885 (>>8). */
    {
        int32_t pc_x = (int32_t)px * 16;
        int32_t pc_y = (int32_t)py * 16;
        int32_t ax = (left != 0u) ? (pc_x - bx0) : (bx1 - pc_x - 16);
        int32_t ay = (top != 0u) ? (pc_y - by0) : (by1 - pc_y - 16);
        int32_t uc = ax + 8; /* pixel center (relative to the corner) */
        int32_t vc = ay + 8;
        int32_t s = (uc + vc) - ch16;
        int32_t cov;

        if (s <= -12)
        {
            return 0; /* center is already more than half a pixel beyond the cut line */
        }
        if (s >= 12)
        {
            return 255;
        }
        cov = 128 + ((s * 2885) >> 8);
        if (cov <= 0)
        {
            return 0;
        }
        if (cov >= 255)
        {
            return 255;
        }
        return (uint8_t)cov;
    }
}

/* ---- Constant-255 column ranges for one row ---- */
/* Tests derived from the return conditions of box_px_cov: row disjoint from the rectangle → the whole
 * row is 0; r16 ≤ 0 or row outside the corner bands → a constant-255 span. Ranges come straight from
 * rx0 + rr / rx1 − rr; SQUARE corner bands extend their constant-255 span to the visible edge. top and
 * bottom never both hold (2r ≤ h). */
static uint8_t box_row_255(int32_t rx0, int32_t rx1, int32_t ry0, int32_t ry1, int32_t rr,
                           const uint8_t *corners, int32_t lo_y, int32_t hi_y,
                           int16_t ax0, int16_t ax1, int16_t *m0, int16_t *m1)
{
    int32_t by0 = ry0 * 16;
    int32_t by1 = (ry1 + 1) * 16;
    int32_t r16 = rr * 16;
    /* c0/c1 = visible area ∩ this loop's rectangle: without the intersection the inner pass would score
     * 255 on edges inside the outer rectangle and the border would go missing. */
    int16_t c0;
    int16_t c1;
    int16_t a0;
    int16_t a1;
    uint8_t top;

    c0 = (int16_t)((ax0 > rx0) ? ax0 : rx0);
    c1 = (int16_t)((ax1 < rx1) ? ax1 : rx1);

    *m0 = (int16_t)1; /* empty range = [1, 0] */
    *m1 = (int16_t)0;

    if ((lo_y < by0) || (hi_y > by1))
    {
        return 0u; /* row disjoint from the rectangle: this loop is 0 all along the row */
    }
    if ((r16 <= 0) || ((lo_y >= by0 + r16) && (hi_y <= by1 - r16)))
    {
        *m0 = c0; /* entire row 255 (zero radius, or row outside both corner bands) */
        *m1 = c1;
        return 1u;
    }

    top = (uint8_t)((lo_y < by0 + r16) ? 1u : 0u);

    a0 = (int16_t)(rx0 + rr);
    a1 = (int16_t)(rx1 - rr);
    if (a0 < c0) { a0 = c0; }
    if (a1 > c1) { a1 = c1; }

    if (top != 0u)
    {
        if (corners[WVUI_BOX_LT] == WVUI_BOX_SQUARE) { a0 = c0; }
        if (corners[WVUI_BOX_RT] == WVUI_BOX_SQUARE) { a1 = c1; }
    }
    else
    {
        if (corners[WVUI_BOX_LB] == WVUI_BOX_SQUARE) { a0 = c0; }
        if (corners[WVUI_BOX_RB] == WVUI_BOX_SQUARE) { a1 = c1; }
    }

    *m0 = a0;
    *m1 = a1;
    return 1u;
}

void wvui_draw_box(wvui_rect_t rect, const uint8_t corners[4], uint8_t r, uint8_t bw,
                  wvui_color_t fill, uint8_t fill_opa,
                  wvui_color_t border, uint8_t border_opa)
{
    int32_t w;
    int32_t h;
    int32_t rr;
    int32_t rmax;
    int32_t rin;
    int32_t ch_out;
    int32_t ch_in;
    uint8_t has_fill;
    uint8_t has_border;
    uint8_t has_inner;
    wvui_rect_t inner;
    wvui_rect_t full;
    wvui_rect_t area;
    wvui_rect_t clip;
    int16_t px;
    int16_t py;

    if ((corners == 0) || (wvui_rect_is_empty(&rect) != 0u))
    {
        return;
    }

    w = (int32_t)wvui_rect_w(&rect);
    h = (int32_t)wvui_rect_h(&rect);
    rr = (int32_t)r;
    rmax = ((w < h) ? w : h) / 2;
    if (rr > rmax)
    {
        rr = rmax;
    }
    ch_out = rr * 16;

    /* Inner shape: the outer shape inset by bw. The inner chamfer constant is r − 0.5·bw (exact value
     * r − (2 − √2)·bw), so the slanted edges are < 6% thicker or thinner than the straight ones. */
    if (bw > 0u)
    {
        rin = (rr > (int32_t)bw) ? (rr - (int32_t)bw) : 0;
        ch_in = (rr * 16) - ((int32_t)bw * 8);
        if (ch_in < 0)
        {
            ch_in = 0;
        }
        inner.x0 = (int16_t)(rect.x0 + bw);
        inner.y0 = (int16_t)(rect.y0 + bw);
        inner.x1 = (int16_t)(rect.x1 - bw);
        inner.y1 = (int16_t)(rect.y1 - bw);
        has_inner = (uint8_t)((inner.x0 <= inner.x1) && (inner.y0 <= inner.y1));
    }
    else
    {
        rin = rr;
        ch_in = ch_out;
        inner = rect;
        has_inner = 1u;
    }

    has_fill = (uint8_t)((fill_opa != 0u) && (has_inner != 0u));
    has_border = (uint8_t)((border_opa != 0u) && (bw > 0u));
    if ((has_fill == 0u) && (has_border == 0u))
    {
        return;
    }

    clip = wvui_render_clip();
    full = rect;
    area = wvui_rect_clip(full, clip);
    if (wvui_rect_is_empty(&area) != 0u)
    {
        return;
    }

    for (py = area.y0; py <= area.y1; py++)
    {
        /* Per-row precomputation: box_row_255() yields the constant-255 column span of this row, handed to
         * wvui_render_fill_rect_blend as a whole; the left/right corner bands still go through box_px_cov,
         * AA pixels are bit-identical. */
        int32_t lo_y = (int32_t)py * 16;
        int32_t hi_y = lo_y + 16;
        int16_t om0;
        int16_t om1;
        int16_t im0 = (int16_t)1;
        int16_t im1 = (int16_t)0;
        uint8_t i_in = 0u;
        int16_t sx0 = (int16_t)1;
        int16_t sx1 = (int16_t)0;

        if (box_row_255(full.x0, full.x1, full.y0, full.y1, rr, corners,
                        lo_y, hi_y, area.x0, area.x1, &om0, &om1) == 0u)
        {
            continue; /* outer is 0 for the whole row: draw neither */
        }

        if (has_border != 0u)
        {
            i_in = box_row_255(inner.x0, inner.x1, inner.y0, inner.y1, rin, corners,
                               lo_y, hi_y, area.x0, area.x1, &im0, &im1);
        }

        /* --- Span constant 255 in both loops: draw the whole span at once (pure fill) --- */
        if (has_fill != 0u)
        {
            int16_t a0 = om0;
            int16_t a1 = om1;

            if (bw > 0u)
            {
                if (i_in == 0u)
                {
                    a0 = (int16_t)1; /* inner has no range on this row ⇒ the whole row is opaque border band */
                    a1 = (int16_t)0;
                }
                else
                {
                    if (im0 > a0) { a0 = im0; }
                    if (im1 < a1) { a1 = im1; }
                }
            }
            if (a0 <= a1)
            {
                wvui_rect_t span;

                span.x0 = a0;
                span.y0 = py;
                span.x1 = a1;
                span.y1 = py;
                wvui_render_fill_rect_blend(span, fill, fill_opa);
                sx0 = a0;
                sx1 = a1;
            }
        }

        for (px = area.x0; px <= area.x1; px++)
        {
            uint32_t outer;
            uint32_t in_cov = 0u;

            if ((px >= sx0) && (px <= sx1))
            {
                continue; /* span above already drew this pixel */
            }

            if ((px >= om0) && (px <= om1))
            {
                outer = 255u; /* box_row_255 guarantees the same value as box_px_cov */
            }
            else
            {
                outer = (uint32_t)box_px_cov(
                    (int32_t)full.x0 * 16, (int32_t)full.y0 * 16,
                    ((int32_t)full.x1 + 1) * 16, ((int32_t)full.y1 + 1) * 16,
                    corners, (int32_t)rr * 16, ch_out, px, py);
            }

            if (outer == 0u)
            {
                continue;
            }
            if (has_inner != 0u)
            {
                /* Clamping the constant-255 span to outer keeps the value unchanged; with bw == 0 both
                 * loops share the same parameters, so the second call must return the same value. */
                if ((bw == 0u) || ((i_in != 0u) && (px >= im0) && (px <= im1)))
                {
                    in_cov = outer;
                }
                else
                {
                    in_cov = (uint32_t)box_px_cov(
                        (int32_t)inner.x0 * 16, (int32_t)inner.y0 * 16,
                        ((int32_t)inner.x1 + 1) * 16, ((int32_t)inner.y1 + 1) * 16,
                        corners, rin * 16, ch_in, px, py);
                    if (in_cov > outer)
                    {
                        in_cov = outer;
                    }
                }
            }

            if ((in_cov > 0u) && (has_fill != 0u))
            {
                cov_pixel(px, py, fill, in_cov, fill_opa);
            }
            if ((outer > in_cov) && (has_border != 0u))
            {
                cov_pixel(px, py, border, outer - in_cov, border_opa);
            }
        }
    }
}

/* ---- Arc / ring sector ---- */
/* Two tests, neither needs atan2: radial uses d² compared with (r∓1)² plus the Q16 reciprocal
 * inv_4r = 65536/(4r) (a 1-pixel band is 4r in d²); angular uses cross products of the two boundary
 * rays, >>7 gives the Q8 perpendicular distance (dir is a Q15 unit vector). */

/* Signed Q8 perpendicular distance from the ray (Q15 direction) to the point (dx,dy): F = dir_x·dy − dir_y·dx > 0
 * means the point is on the clockwise side of the ray (y down, angle growing); within a half circle that
 * means a larger angle. |dir| = 2^15, >>15 normalizes and then >>7 gives Q8. */
static int32_t ray_side_q8(int32_t dx, int32_t dy, int16_t dir_x, int16_t dir_y)
{
    int32_t cr = ((int32_t)dir_x * dy) - ((int32_t)dir_y * dx);

    return cr >> 7;
}

/* Cross-product side → coverage (Q8 ramp, same rule as the line primitive). invert = 1 takes the other side. */
static uint8_t side_cov(int32_t side_q8, uint8_t invert)
{
    if (invert != 0u)
    {
        side_q8 = -side_q8;
    }
    if (side_q8 >= 128)
    {
        return 255;
    }
    if (side_q8 <= -128)
    {
        return 0;
    }
    return (uint8_t)((((uint32_t)(side_q8 + 128)) * 255u) >> 8);
}

/* Radial coverage: d2 = squared distance to the center, r = radius of this edge, inv_4r = 65536/(4r); inner = 1 for the inner edge. */
static uint8_t radial_cov(uint32_t d2, uint32_t r, uint32_t inv_4r, uint8_t inner)
{
    uint32_t rp = (r + 1u) * (r + 1u);
    uint32_t rm = (r > 0u) ? ((r - 1u) * (r - 1u)) : 0u;

    if (inner != 0u)
    {
        if (d2 >= rp)
        {
            return 255; /* fully outside the inner edge (inside the ring) */
        }
        if (d2 <= rm)
        {
            return 0;
        }
        return (uint8_t)((((d2 - rm) * inv_4r) >> 8));
    }
    if (d2 <= rm)
    {
        return 255;
    }
    if (d2 >= rp)
    {
        return 0;
    }
    return (uint8_t)((((rp - d2) * inv_4r) >> 8));
}

void wvui_draw_arc(int16_t cx, int16_t cy, uint16_t r_out, uint16_t r_in,
                  uint16_t a0, uint16_t a1, wvui_color_t color, uint8_t opa)
{
    uint32_t inv_out;
    uint32_t inv_in = 0u;
    uint32_t sweep;
    int16_t minx;
    int16_t miny;
    int16_t maxx;
    int16_t maxy;
    int16_t px;
    int16_t py;
    wvui_rect_t clip;

    if ((opa == 0u) || (r_out == 0u) || (r_in >= r_out))
    {
        return;
    }

    inv_out = ((1u << 16) + (2u * r_out)) / (4u * r_out); /* one division per call */
    if (r_in > 0u)
    {
        inv_in = ((1u << 16) + (2u * r_in)) / (4u * r_in);
    }

    clip = wvui_render_clip();
    if (wvui_rect_is_empty(&clip) != 0u)
    {
        return;
    }

    minx = (int16_t)((int32_t)cx - (int32_t)r_out - 1);
    maxx = (int16_t)((int32_t)cx + (int32_t)r_out + 1);
    miny = (int16_t)((int32_t)cy - (int32_t)r_out - 1);
    maxy = (int16_t)((int32_t)cy + (int32_t)r_out + 1);
    if (minx < clip.x0) { minx = clip.x0; }
    if (maxx > clip.x1) { maxx = clip.x1; }
    if (miny < clip.y0) { miny = clip.y0; }
    if (maxy > clip.y1) { maxy = clip.y1; }
    if ((minx > maxx) || (miny > maxy))
    {
        return;
    }

    sweep = (uint32_t)((a1 - a0) & 0x1FFu);
    if (sweep == 0u)
    {
        sweep = 512u; /* a1 == a0 means a full circle */
    }

    for (py = miny; py <= maxy; py++)
    {
        for (px = minx; px <= maxx; px++)
        {
            int32_t dx = (int32_t)px - (int32_t)cx;
            int32_t dy = (int32_t)py - (int32_t)cy;
            uint32_t d2 = (uint32_t)((dx * dx) + (dy * dy));
            uint32_t cov = 0u;
            uint32_t cov_r;

            if (d2 > (uint32_t)((r_out + 1u) * (r_out + 1u)))
            {
                continue; /* outside the full circle (AA margin included) */
            }

            cov_r = radial_cov(d2, r_out, inv_out, 0u);
            if (cov_r == 0u)
            {
                continue;
            }
            if (r_in > 0u)
            {
                uint32_t ci = radial_cov(d2, r_in, inv_in, 1u);

                cov_r = (cov_r * (ci + (ci >> 7))) >> 8;
                if (cov_r == 0u)
                {
                    continue;
                }
            }

            if (sweep >= 512u)
            {
                /* Full circle: skip the angular test to avoid the two seams. */
                cov = cov_r;
            }
            else if (d2 == 0u)
            {
                /* Center pixel: both boundary rays pass through the center, coverage stays 255 */
                cov = cov_r;
            }
            else
            {
                /* Cross products are reliable only within a half circle: with sweep > 180°, compute the
                 * complement arc [a1, a0+512] and invert. */
                uint32_t wide = (sweep > 256u) ? 1u : 0u;
                uint16_t s0 = (wide != 0u) ? a1 : a0;
                uint16_t s1 = (wide != 0u) ? a0 : a1;
                uint32_t cov_a = (uint32_t)side_cov(
                    ray_side_q8(dx, dy, wvui_cos512(s0), wvui_sin512(s0)), 0u); /* after the start ray */

                if (cov_a > 0u)
                {
                    uint32_t ce = (uint32_t)side_cov(
                        ray_side_q8(dx, dy, wvui_cos512(s1), wvui_sin512(s1)), 1u);

                    cov_a = (cov_a * (ce + (ce >> 7))) >> 8;
                }
                if (wide != 0u)
                {
                    cov_a = 255u - cov_a; /* what was just computed is the complement arc */
                }
                cov = (cov_r * (cov_a + (cov_a >> 7))) >> 8;
            }

            if (cov > 0u)
            {
                cov_pixel(px, py, color, cov, opa);
            }
        }
    }
}

/* ---- Column ranges for one row ---- */
/* Lift the rr_px_cov tests to row level: always-0 uses the closest distance and always-255 the tightest
 * maximum distance; ranges are computed by wvui_isqrt32 plus integer division, no searching. */
typedef struct
{
    uint8_t row_ok; /* 0 = the row is always 0 (caller skips the row) */
    int16_t m0;     /* constant-255 column range (closed); m0 > m1 = empty */
    int16_t m1;
    int32_t zl;     /* px <= zl or px >= zr ⇒ always 0 */
    int32_t zr;
} rr_row_t;

/* Floor/ceil a/16: C integer division truncates toward zero for negatives, and being off by one cell
 * misclassifies edge columns. */
static int32_t div16_floor(int32_t a)
{
    int32_t q = a / 16;

    if (((a % 16) != 0) && (a < 0))
    {
        q--;
    }
    return q;
}

static int32_t div16_ceil(int32_t a)
{
    int32_t q = a / 16;

    if (((a % 16) != 0) && (a > 0))
    {
        q++;
    }
    return q;
}

/* Inputs same as rr_px_cov: bx0/by0/bx1/by1 form the inset rectangle (1/16 units), r16 is the radius,
 * lo_y is the top edge of the row (fixed row height 16), ax0/ax1 the visible column range; computing
 * from the clipped rectangle shifts everything.
 * Tests: coverage 0 ⟺ center distance ≥ ro = r + 0.5; 255 ⟺ center distance ≤ ri = r − 0.5. Both ends
 * keep a 1-column margin; the 255/0 ranges are sufficient conditions: judging too little only costs
 * extra work, judging too much corrupts pixels. */
static rr_row_t rr_row_of(int32_t bx0, int32_t bx1, int32_t by0, int32_t by1,
                          int32_t r16, int32_t lo_y,
                          int16_t ax0, int16_t ax1)
{
    rr_row_t o;
    int32_t cy = lo_y + 8; /* y of the row's pixel centers (1/16 units; same for the whole row) */
    int32_t nyc;           /* y distance from the row center to the box */
    int32_t ro = r16 + 8;
    int32_t ri = r16 - 8;
    int32_t m0;
    int32_t m1;

    o.row_ok = 0u;
    o.m0 = (int16_t)1;
    o.m1 = (int16_t)0;
    o.zl = (int32_t)ax0 - 1;
    o.zr = (int32_t)ax1 + 1;

    if (ri < 0)
    {
        ri = 0; /* r == 0: the coverage-255 region becomes the interior of the rectangle (center distance ≤ 0) */
    }

    if (cy < by0) { nyc = by0 - cy; }
    else if (cy > by1) { nyc = cy - by1; }
    else { nyc = 0; }

    /* Whole row always 0: row center is ≥ r + 0.5 from the box */
    if ((nyc * nyc) >= (ro * ro))
    {
        return o;
    }
    o.row_ok = 1u;

    /* --- Always-0 columns: |center x separation| ≥ ceil(sqrt(ro² − nyc²)) --- */
    {
        int32_t rem = (ro * ro) - (nyc * nyc);
        int32_t nl;

        if (nyc == 0)
        {
            nl = ro;
        }
        else
        {
            nl = (int32_t)wvui_isqrt32((uint32_t)rem);
            if ((nl * nl) < rem)
            {
                nl++; /* ceil sqrt */
            }
        }
        o.zl = div16_floor(bx0 - nl - 8) - 1; /* center on the left, separation ≥ nl */
        o.zr = div16_ceil(bx1 + nl - 8) + 1;  /* center on the right */
        if (o.zl < (int32_t)ax0 - 1) { o.zl = (int32_t)ax0 - 1; }
        if (o.zr > (int32_t)ax1 + 1) { o.zr = (int32_t)ax1 + 1; }
    }

    /* --- Always-255 columns: center distance ≤ ri --- */
    if ((nyc * nyc) <= (ri * ri))
    {
        int32_t rem = (ri * ri) - (nyc * nyc);
        int32_t fl = (int32_t)wvui_isqrt32((uint32_t)rem); /* floor sqrt */

        /* center_x ∈ [bx0 − fl, bx1 + fl] ⇔ px ∈ [ceil(..), floor(..)] */
        m0 = div16_ceil(bx0 - fl - 8) + 1;
        m1 = div16_floor(bx1 + fl - 8) - 1;
        if (m0 > m1) { m0 = (int32_t)1; m1 = (int32_t)0; }
    }
    else
    {
        m0 = (int32_t)1;
        m1 = (int32_t)0;
    }

    if (m0 < (int32_t)ax0) { m0 = (int32_t)ax0; }
    if (m1 > (int32_t)ax1) { m1 = (int32_t)ax1; }
    if (m0 < o.zl + 1) { m0 = o.zl + 1; } /* intersect with the always-0 range */
    if (m1 > o.zr - 1) { m1 = o.zr - 1; }
    if (m0 > m1) { m0 = (int32_t)1; m1 = (int32_t)0; }
    o.m0 = (int16_t)m0;
    o.m1 = (int16_t)m1;
    return o;
}


void wvui_draw_round_rect(wvui_rect_t rect, uint8_t r, uint8_t bw,
                         wvui_color_t fill, uint8_t fill_opa,
                         wvui_color_t border, uint8_t border_opa)
{
    int32_t w;
    int32_t h;
    int32_t rr;
    int32_t rmax;
    int32_t rin;
    int32_t bx0;
    int32_t by0;
    int32_t bx1;
    int32_t by1;
    int32_t ix0;
    int32_t iy0;
    int32_t ix1;
    int32_t iy1;
    int32_t rr16;  /* outer radius (1/16 pixel units) */
    int32_t rin16; /* inner radius (1/16 pixel units) */
    uint8_t has_fill;
    uint8_t has_border;
    uint8_t has_inner;
    wvui_rect_t full;
    wvui_rect_t area;
    wvui_rect_t clip;
    int16_t px;
    int16_t py;

    if (wvui_rect_is_empty(&rect) != 0u)
    {
        return;
    }

    w = (int32_t)wvui_rect_w(&rect);
    h = (int32_t)wvui_rect_h(&rect);

    rr = (int32_t)r;
    rmax = ((w < h) ? w : h) / 2;
    if (rr > rmax)
    {
        rr = rmax; /* past half size the inset rectangle collapses to a point/segment, i.e. capsule/circle */
    }

    /* Inner shape (fill region): the rectangle inset by bw more, radius r - bw. */
    if (bw > 0u)
    {
        rin = (rr > (int32_t)bw) ? (rr - (int32_t)bw) : 0;
        has_inner = (uint8_t)(((int32_t)rect.x0 + (int32_t)bw <= (int32_t)rect.x1 - (int32_t)bw) &&
                              ((int32_t)rect.y0 + (int32_t)bw <= (int32_t)rect.y1 - (int32_t)bw));
    }
    else
    {
        rin = rr;
        has_inner = 1u;
    }

    has_fill = (uint8_t)((fill_opa != 0u) && (has_inner != 0u));
    has_border = (uint8_t)((border_opa != 0u) && (bw > 0u));
    if ((has_fill == 0u) && (has_border == 0u))
    {
        return;
    }

    /* Outer inset rectangle (1/16 units; in continuous coordinates the rectangle's right/bottom edges are x1+1 / y1+1) */
    bx0 = ((int32_t)rect.x0 + rr) * 16;
    by0 = ((int32_t)rect.y0 + rr) * 16;
    bx1 = ((int32_t)rect.x1 + 1 - rr) * 16;
    by1 = ((int32_t)rect.y1 + 1 - rr) * 16;
    rr16 = rr * 16;

    ix0 = ((int32_t)rect.x0 + (int32_t)bw + rin) * 16;
    iy0 = ((int32_t)rect.y0 + (int32_t)bw + rin) * 16;
    ix1 = ((int32_t)rect.x1 + 1 - (int32_t)bw - rin) * 16;
    iy1 = ((int32_t)rect.y1 + 1 - (int32_t)bw - rin) * 16;
    rin16 = rin * 16;

    clip = wvui_render_clip();
    full = rect;                       /* geometric reference: coverage is computed from the unclipped original rectangle */
    area = wvui_rect_clip(full, clip);  /* visit only the visible pixels */
    if (wvui_rect_is_empty(&area) != 0u)
    {
        return;
    }

    for (py = area.y0; py <= area.y1; py++)
    {
        /* Per-row precomputation: one set of column ranges per outer/inner loop computed up front, leaving
         * only the AA band to the per-pixel loop; the 255-scored columns match rr_px_cov (see rr_row_of),
         * pixels are bit-identical. */
        const int32_t lo_y = (int32_t)py * 16;
        rr_row_t ro;
        rr_row_t ri;
        int16_t sx0 = (int16_t)1; /* pure fill span (both loops always 255); empty = [1, 0] */
        int16_t sx1 = (int16_t)0;

        ro = rr_row_of(bx0, bx1, by0, by1, rr * 16, lo_y,
                       area.x0, area.x1);
        if (ro.row_ok == 0u)
        {
            continue; /* whole row always 0 */
        }

        ri.row_ok = 0u;
        ri.m0 = (int16_t)1;
        ri.m1 = (int16_t)0;
        ri.zl = ro.zl;
        ri.zr = ro.zr;
        if ((has_border != 0u) && (has_inner != 0u))
        {
            ri = rr_row_of(ix0, ix1, iy0, iy1, rin * 16, lo_y,
                           area.x0, area.x1);
        }

        /* --- Span constant 255 in both loops: draw the whole span at once (pure fill) --- */
        if (has_fill != 0u)
        {
            int16_t a0 = ro.m0;
            int16_t a1 = ro.m1;

            if (has_border != 0u)
            {
                if ((ri.row_ok == 0u) || (ri.m0 > ri.m1))
                {
                    a0 = (int16_t)1;
                    a1 = (int16_t)0;
                }
                else
                {
                    if (ri.m0 > a0) { a0 = ri.m0; }
                    if (ri.m1 < a1) { a1 = ri.m1; }
                }
            }
            if (a0 <= a1)
            {
                wvui_rect_t span;

                span.x0 = a0;
                span.y0 = py;
                span.x1 = a1;
                span.y1 = py;
                wvui_render_fill_rect_blend(span, fill, fill_opa);
                sx0 = a0;
                sx1 = a1;
            }
        }

        for (px = area.x0; px <= area.x1; px++)
        {
            uint32_t outer;
            uint32_t icov = 0u;

            if ((px >= sx0) && (px <= sx1))
            {
                continue; /* span above already drew this pixel */
            }

            if ((px >= ro.m0) && (px <= ro.m1))
            {
                outer = 255u; /* guaranteed by the tight tests of rr_row_of to match rr_px_cov */
            }
            else if ((px <= ro.zl) || (px >= ro.zr))
            {
                continue; /* outer always 0 (near² >= r²) */
            }
            else
            {
                outer = (uint32_t)rr_px_cov(bx0, by0, bx1, by1, rr16, px, py);
            }

            if (outer == 0u)
            {
                continue;
            }
            /* The inner shape ⊆ the outer shape, so icov ≤ outer always holds; with has_border == 0 the
             * fill covers the whole outer shape (agreed semantics, changing it changes pixels). */
            if (has_inner != 0u)
            {
                if (has_border == 0u)
                {
                    icov = outer;
                }
                else if ((ri.row_ok != 0u) && (px >= ri.m0) && (px <= ri.m1))
                {
                    icov = outer;
                }
                else if ((ri.row_ok == 0u) || (px <= ri.zl) || (px >= ri.zr))
                {
                    icov = 0u; /* inner always 0 */
                }
                else
                {
                    icov = (uint32_t)rr_px_cov(ix0, iy0, ix1, iy1, rin16, px, py);
                    if (icov > outer)
                    {
                        icov = outer;
                    }
                }
            }

            /* Single-pass draw: border = outer − inner, never stacked with the fill on the same pixel. */
            if ((icov > 0u) && (has_fill != 0u))
            {
                cov_pixel(px, py, fill, icov, fill_opa);
            }
            if ((outer > icov) && (has_border != 0u))
            {
                cov_pixel(px, py, border, outer - icov, border_opa);
            }
        }
    }
}
