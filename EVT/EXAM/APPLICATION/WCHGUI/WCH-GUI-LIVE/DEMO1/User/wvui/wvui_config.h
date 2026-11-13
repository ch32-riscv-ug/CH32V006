#ifndef WVUI_CONFIG_H
#define WVUI_CONFIG_H

/**
 * @file wvui_config.h
 * @brief Compile-time configuration: feature switches, capacity caps and value checks; invalid values always #error.
 */

#include <stdint.h>

#if !defined(__cplusplus) && (!defined(__STDC_VERSION__) || (__STDC_VERSION__ < 201112L))
#error "WVUI requires C11 (_Static_assert). Use -std=c11 or newer."
#endif

/** Compile-time assertion: `_Static_assert` on C11, `static_assert` on C++. */
#if defined(__cplusplus)
#define WVUI_STATIC_ASSERT(expr, msg) static_assert((expr), msg)
#else
#define WVUI_STATIC_ASSERT(expr, msg) _Static_assert((expr), msg)
#endif

/** Runtime assertion: no-op by default. Redefinable; cold paths only, never assert on hot paths or ISRs. */
#ifndef WVUI_ASSERT
#define WVUI_ASSERT(expr) ((void)0)
#endif

/* wvui_conf.h mechanism: included automatically when found on the include path; list only the
 * switches to change, everything else keeps its default.
 * A switch must be set in exactly one place, wvui_conf.h or -D; defining the same macro in
 * both is a redefinition with no priority.
 * -D WVUI_CONF_INCLUDE changes the file name; with WVUI_CONF_SKIP=1 or a compiler without
 * __has_include, auto-discovery is off and only -D / WVUI_CONF_INCLUDE works. */
#if !defined(WVUI_CONF_SKIP)
#if defined(WVUI_CONF_INCLUDE)
#include WVUI_CONF_INCLUDE
#elif defined(__has_include)
#if __has_include("wvui_conf.h")
#include "wvui_conf.h"
#endif
#endif
#endif /* !WVUI_CONF_SKIP */

/* ---- Switches: input and pages ---- */

/**
 * Input layer (touch state machine, hit testing, gestures); default 1, values 0/1.
 * @note 0 = compile the whole layer out: wvui_frame() takes no samples and no input stats,
 *       wvui_driver_bind_input() becomes a stub, and the screen updates only through
 *       setters.
 */
#ifndef WVUI_CFG_ENABLE_INPUT
#define WVUI_CFG_ENABLE_INPUT 1
#endif

/**
 * Page system (wvui_page_go/push/back, pinned pages, page fade-in); default 1, values 0/1.
 * @note 0 = compile the whole system out and the widget tree is always a single page:
 *       apps call wvui_build()/wvui_widget_create() directly, and wvui_transient_reset()
 *       drops the whole top-level chain with unchanged semantics.
 */
#ifndef WVUI_CFG_ENABLE_PAGES
#define WVUI_CFG_ENABLE_PAGES 1
#endif

/* ---- Memory mode: widget object source ---- */

/** Single page: objects declared statically with WVUI_WIDGET_STORAGE + WVUI_WIDGET_STATIC_BIND. */
#define WVUI_MEM_STATIC (0)
/** Two-area bump arena: leaving a page resets the temp-area cursor, no per-object free. */
#define WVUI_MEM_ARENA (1)
/** Real allocation and free, for apps with repeated add/remove (RAM >= 64 KB). */
#define WVUI_MEM_HEAP (2)

/**
 * Widget object source, fixed at compile time with three tiers; default WVUI_MEM_ARENA,
 * any other value #error.
 * @note STATIC tier allocates nothing and requires WVUI_CFG_ENABLE_PAGES=0, else #error;
 *       use the ARENA tier when pages are needed.
 * @note All three tiers expose the same API and differ only in whether they can allocate
 *       and whether delete reclaims objects. Unselected tiers are compiled out entirely.
 */
#ifndef WVUI_CFG_MEM_MODE
#define WVUI_CFG_MEM_MODE WVUI_MEM_ARENA
#endif

/**
 * HEAP-tier allocator: default 0 = stdlib malloc/free; 1 = FreeRTOS pvPortMalloc/vPortFree.
 * @note Taking 1 requires #include "FreeRTOS.h" plus its include path. Use a custom
 *       allocator with -D WVUI_MEM_ALLOC=my_alloc -D WVUI_MEM_FREE=my_free (must come in
 *       pairs, and this switch is then ignored).
 * @note Only meaningful when WVUI_CFG_MEM_MODE == WVUI_MEM_HEAP.
 */
#ifndef WVUI_CFG_MEM_HEAP_FREERTOS
#define WVUI_CFG_MEM_HEAP_FREERTOS 0
#endif

/**
 * Class table writability (needed by custom widgets); default 1, values 0/1.
 * @note 1 = wvui_class_table[] stays in RAM (4 B per slot) and wvui_class_bind() can
 *       override any slot.
 * @note 0 = the table goes const into flash and wvui_class_bind() always returns 0, so the
 *       class is fixed at compile time; pure built-in firmware may turn this off.
 */
#ifndef WVUI_CFG_ENABLE_CLASS_BIND
#define WVUI_CFG_ENABLE_CLASS_BIND 1
#endif

/**
 * Whether the control header stores a class pointer or a type number; default 0, values 0/1.
 * @note 0 = const wvui_class_t *cls, dispatch is one indirect call and the control header is
 *       larger; 1 = uint8_t type, saves 4 B per instance at the cost of one extra class-
 *       table lookup per dispatch.
 * @note With 1 it is best to also set WVUI_CFG_ENABLE_CLASS_BIND=0 (const table); type
 *       casts go through wvui_class_at(type), and destroyed widgets still read type == 0.
 */
#ifndef WVUI_CFG_WIDGET_COMPACT_TYPE
#define WVUI_CFG_WIDGET_COMPACT_TYPE 0
#endif

/* ---- Switches: log, layers, animation and command channel ---- */

/**
 * Log: WVUI_LOG(...) goes through printf or is a no-op; default 1, values 0/1.
 * @note 0 = the entire printf/snprintf chain stays out of the binary. Only WVUI_LOG call
 *       sites are affected; the app calling printf itself still pulls that chain in.
 */
#ifndef WVUI_CFG_ENABLE_LOG
#define WVUI_CFG_ENABLE_LOG 1
#endif

#if WVUI_CFG_ENABLE_LOG
#define WVUI_LOG(...) printf(__VA_ARGS__)
#else
#define WVUI_LOG(...) ((void)0)
#endif

/**
 * Top-level chain + modal: popups on top and touch interception while modal; default 1,
 * values 0/1.
 * @note 0 = compile the whole thing out: wvui_layer_attach() degrades to
 *       wvui_widget_bring_to_front() (raise to top, not a separate layer), the modal trio
 *       becomes stubs, and popups lose the on-top and no-passthrough guarantees.
 */
#ifndef WVUI_CFG_ENABLE_TOP_LAYER
#define WVUI_CFG_ENABLE_TOP_LAYER 1
#endif

/**
 * Layered input: whether hit testing descends into composite containers; default 1,
 * values 0/1.
 * @note 0 = no descent into containers and the DRAG_BEGIN takeover query is dropped too,
 *       so nested buttons receive no events. Flat UIs save the recursion cost in ROM and
 *       call stack.
 */
#ifndef WVUI_CFG_ENABLE_NESTED_INPUT
#define WVUI_CFG_ENABLE_NESTED_INPUT 1
#endif

/**
 * Performance counters: per-frame counter fields and their increments; default 1,
 * values 0/1.
 * @note 0 = remove these fields from the runtime structs to save RAM, at the cost of
 *       losing per-frame statistics.
 */
#ifndef WVUI_CFG_ENABLE_RENDER_STATS
#define WVUI_CFG_ENABLE_RENDER_STATS 1
#endif

/**
 * Animation engine master switch: tweens, scroll inertia, blink, carousel, fade-in;
 * default 1, values 0/1.
 * @note 0 = transitional animations (toggle, progress, checkbox entry, roller snap,
 *       inertia/rubber-band, page fade-in) run synchronously to completion in
 *       wvui_anim_start(), and the state is final right away.
 * @note With 0, persistent animations (indicator, marquee, slideshow, toast lifetime,
 *       fade timers) never start and stay at the widget's initial value; toast does not
 *       expire on its own and must be dismissed with an explicit wvui_toast_hide().
 */
#ifndef WVUI_CFG_ENABLE_ANIM
#define WVUI_CFG_ENABLE_ANIM 1
#endif

/**
 * Command channel: TLV parsing and dispatch; default 1, values 0/1.
 * @note 0 = removed entirely and the ROM cost is zero; CMD_RING / CMD_MAX_PAYLOAD /
 *       CMD_TEXT_POOL / CMD_CRC8 are voided along with it.
 */
#ifndef WVUI_CFG_ENABLE_CMD_CHANNEL
#define WVUI_CFG_ENABLE_CMD_CHANNEL 1
#endif

/**
 * CRC8 check on the TLV frame tail; default 1, values 0/1.
 * @note 0 = no CRC is checked and the frame format is [SOF][T:2][OP:1][L:2][V:L]; only
 *       effective when CMD_CHANNEL is on.
 */
#ifndef WVUI_CFG_CMD_CRC8
#define WVUI_CFG_CMD_CRC8 1
#endif

/* ---- Command channel capacity ---- */

/**
 * Command channel byte-ring capacity in bytes; default 128, must be a power of two >= 8,
 * else #error.
 * @note Wrap-around is done with a bitwise AND; a ring smaller than the largest frame
 *       payload cannot hold a complete frame.
 */
#ifndef WVUI_CFG_CMD_RING
#define WVUI_CFG_CMD_RING 128
#endif

/**
 * Max payload per frame in bytes; default 64, range 4..512, out of range #error.
 * @note This is the parser's static buffer, and partial frames stay in it too. The L field
 *       is 2 bytes wide, so frames beyond this value are judged bad and resynchronized,
 *       never written out of bounds.
 */
#ifndef WVUI_CFG_CMD_MAX_PAYLOAD
#define WVUI_CFG_CMD_MAX_PAYLOAD 64
#endif

/**
 * Command-layer text pool in bytes, the backing store for text properties (WVUI_PROP_TEXT);
 * default 256, range >= 8.
 * @note label/btn store a pointer only and never copy, so protocol text must live inside
 *       this pool. A full pool reports an error and does not recycle; it clears on scene
 *       reset.
 */
#ifndef WVUI_CFG_CMD_TEXT_POOL
#define WVUI_CFG_CMD_TEXT_POOL 256
#endif

/* ---- Resource decoding and debug ---- */

/**
 * Indexed QOI decoding (RGB565 / ARGB8565: row jump table + static palette); default 1,
 * values 0/1.
 * @note 0 = the decoder and palette are compiled out, the format is reported as
 *       unsupported, and such images do not render.
 */
#ifndef WVUI_CFG_ENABLE_INDEXQOI
#define WVUI_CFG_ENABLE_INDEXQOI 1
#endif

/**
 * Indexed QOI_MASK (A8 mask compression) decoding: row jump table + dictionary +
 * quantized alpha; default 1, values 0/1.
 * @note 0 = the decoder and dictionary are compiled out and the format is reported as
 *       unsupported.
 */
#ifndef WVUI_CFG_ENABLE_INDEXQOI_MASK
#define WVUI_CFG_ENABLE_INDEXQOI_MASK 1
#endif

/**
 * Dirty-rectangle visualization overlay (debug only); default 0, values 0/1.
 * @note 1 = outline the dirty area every frame, costing extra CPU and altering the image;
 *       keep 0 in production firmware.
 */
#ifndef WVUI_CFG_DEBUG_DIRTY_RECT
#define WVUI_CFG_DEBUG_DIRTY_RECT 0
#endif

/**
 * Performance debug switch: force every top-level widget dirty each frame; default 0,
 * values 0/1.
 * @note 1 is for performance debugging only and the refresh volume grows noticeably; keep
 *       0 in production firmware.
 */
#ifndef WVUI_CFG_DEBUG_PERF_STRESS
#define WVUI_CFG_DEBUG_PERF_STRESS 0
#endif

/* ---- Ports and dirty rectangles ---- */

/**
 * Port-side pixel-stream hash: every flushed pixel feeds an FNV-1a chain; default 1,
 * values 0/1.
 * @note 1 is for frame-by-frame comparison between simulator and real hardware and costs
 *       two extra multiplications per pixel; 0 = compiled out entirely, zero cost in
 *       production.
 */
#ifndef WVUI_CFG_ENABLE_PORT_HASH
#define WVUI_CFG_ENABLE_PORT_HASH 1
#endif

/**
 * Dirty-rectangle strategy tier, fixed at compile time; default 2, any other value #error.
 * 0 = full (one dirty rectangle for the whole screen, cheapest CPU, heaviest bus),
 * 1 = bbox (merge into one bounding box), 2 = multi (multiple rectangles merged by cost).
 * All three tiers share the same refresh semantics and differ only in the rectangle split.
 */
#ifndef WVUI_CFG_DIRTY_STRATEGY
#define WVUI_CFG_DIRTY_STRATEGY 2
#endif

/**
 * Dirty-rectangle slot count for the multi tier; default 10, range 1..255, out of range
 * #error.
 * @note 8 B per slot; when the slots run out, new dirty areas merge into existing slots.
 *       Meaningful only in the multi tier.
 */
#ifndef WVUI_CFG_DIRTY_MAX_NUM
#define WVUI_CFG_DIRTY_MAX_NUM 10
#endif

/**
 * Cost-based merge constant: how many equivalent pixels one saved transaction is worth;
 * default 0, values >= 0.
 * @note 0 = derived by wvui_init from the time model (per-pixel cost = CPU rendering + bus
 *       transfer, Q16 fixed point); when the time model is absent (cpu/bus both 0),
 *       WVUI_CFG_DIRTY_FIXED_COST_PX_FALLBACK applies.
 * @note > 0 = an explicit override; pick a fixed value calibrated against the port
 *       bandwidth and CPU speed.
 */
#ifndef WVUI_CFG_DIRTY_FIXED_COST_PX
#define WVUI_CFG_DIRTY_FIXED_COST_PX 0
#endif

/**
 * Time-model defaults folded in by wvui_init(): a zero field in wvui_cfg_t / bus_bytes_per_ms
 * = 0 in wvui_port_t falls back to these, so the app config can omit the timing numbers once
 * a project supplies calibrated constants at build time. Defaults 0 = not modeled, which
 * reproduces the previous "0 = not modeled" behavior exactly.
 */
#ifndef WVUI_CFG_CPU_PX_PER_MS
#define WVUI_CFG_CPU_PX_PER_MS 0
#endif

#ifndef WVUI_CFG_FIXED_US_PER_RECT
#define WVUI_CFG_FIXED_US_PER_RECT 0
#endif

#ifndef WVUI_CFG_BUS_BYTES_PER_MS
#define WVUI_CFG_BUS_BYTES_PER_MS 0
#endif

/**
 * Fallback when no time model exists, in equivalent pixels (one transaction ≈ 125 pixels
 * of rendering). Default 125, must be >= 1; 0 triggers #error (it would mean no merge).
 */
#ifndef WVUI_CFG_DIRTY_FIXED_COST_PX_FALLBACK
#define WVUI_CFG_DIRTY_FIXED_COST_PX_FALLBACK 125
#endif

/**
 * Cap on merge attempts per frame; default 16, range 0..255 (0 = no merge), out of range
 * #error.
 * @note Merge is an O(N^2) global fusion truncated at this cap, keeping the dirty-marking
 *       path bounded.
 */
#ifndef WVUI_CFG_DIRTY_MERGE_MAX
#define WVUI_CFG_DIRTY_MERGE_MAX 16
#endif

/* ---- Class table and memory constraints ---- */

/**
 * Class table slot count (type -> wvui_class_t*); default 32, range >= 1.
 * @note Constraint type < WVUI_CFG_CLASS_MAX: built-in type numbers already reach 16, and
 *       when the capacity is short wvui_widget_create() returns NULL with no other hint
 *       (wvui_widgets.h has a compile-time guard). 4 B per slot; type numbers are frozen,
 *       and the capacity is unaffected by widget trimming.
 */
#ifndef WVUI_CFG_CLASS_MAX
#define WVUI_CFG_CLASS_MAX 32
#endif

/**
 * GUI RAM budget in bytes; default 0 = no compile-time assertion.
 * @note Non-zero: WVUI_BUDGET_ASSERT() in wvui_arena.h feeds the sum of the static area +
 *       temp area + PFB + table entries + queues and this value into _Static_assert, and
 *       exceeding the limit fails the build.
 */
#ifndef WVUI_CFG_RAM_BUDGET
#define WVUI_CFG_RAM_BUDGET 0
#endif

/* ---- Input and animation parameters (constants, not trim switches) ---- */

/**
 * Drag takeover threshold in pixels: past this displacement after a press, the kernel
 * walks the ancestor chain once asking WVUI_EVENT_DRAG_BEGIN. Default 8, range >= 1;
 * 0 triggers #error (every MOVE would count as a drag). Independent of
 * WVUI_CFG_SWIPE_THRESHOLD.
 */
#ifndef WVUI_CFG_DRAG_THRESHOLD
#define WVUI_CFG_DRAG_THRESHOLD 8
#endif

/**
 * Swipe detection threshold in pixels: a fast pass without STAY and with displacement
 * >= this generates SWIPE_*. Default 30, range >= 1; 0 triggers #error.
 */
#ifndef WVUI_CFG_SWIPE_THRESHOLD
#define WVUI_CFG_SWIPE_THRESHOLD 30
#endif

/**
 * Rubber-band limit when dragged out of bounds, in pixels; default 40, range 0..1000,
 * out of range #error.
 * @note 0 = dragging beyond the edge is not allowed, so there is no rubber band.
 */
#ifndef WVUI_CFG_SCROLL_OVERSCROLL
#define WVUI_CFG_SCROLL_OVERSCROLL 40
#endif

/**
 * Inertia stop threshold in pixels/frame; default 2, range >= 1, 0 triggers #error
 * (inertia would never stop).
 */
#ifndef WVUI_CFG_SCROLL_MIN_VEL
#define WVUI_CFG_SCROLL_MIN_VEL 2
#endif

/**
 * Per-frame step of the progress value transition (0..255 scale); default 16, range
 * 1..255, out of range #error.
 * @note Each frame moves display toward value by at most this many units, and the
 *       transition ends on arrival; a larger value transitions faster, independent of
 *       the frame rate.
 */
#ifndef WVUI_CFG_PROGRESS_STEP
#define WVUI_CFG_PROGRESS_STEP 16
#endif

/**
 * Toast lifetime in frames; default 90, range 2..60000, out of range #error.
 * @note Every animation in this file counts frames, not milliseconds; with
 *       WVUI_CFG_ENABLE_ANIM=0, toast does not expire on its own.
 */
#ifndef WVUI_CFG_TOAST_FRAMES
#define WVUI_CFG_TOAST_FRAMES 90
#endif

/**
 * Toast fade-out frames; default 20, range 0..WVUI_CFG_TOAST_FRAMES (0 = no fade),
 * out of range #error.
 */
#ifndef WVUI_CFG_TOAST_FADE
#define WVUI_CFG_TOAST_FADE 20
#endif

/**
 * Slideshow hold frames per image; default 90, range 1..255, out of range #error.
 * @note The 255 cap comes from a u8 runtime field; for longer holds, call
 *       wvui_slideshow_next() from the frame loop.
 */
#ifndef WVUI_CFG_SLIDESHOW_HOLD
#define WVUI_CFG_SLIDESHOW_HOLD 90
#endif

/**
 * Slideshow transition frames (old image out, new image in); default 12, range 0..255
 * (0 = hard cut), out of range #error.
 */
#ifndef WVUI_CFG_SLIDESHOW_SLIDE
#define WVUI_CFG_SLIDESHOW_SLIDE 12
#endif

/**
 * Page fade-in frames; default 8, range 0..60000 (0 = hard cut, the page layer runs no
 * ramp), out of range #error.
 * @note During the fade-in the full screen is marked dirty each frame, so reserve this
 *       for sparse events like page switches.
 */
#ifndef WVUI_CFG_PAGE_FADE_FRAMES
#define WVUI_CFG_PAGE_FADE_FRAMES 8
#endif

/**
 * Scrollbar fade-out delay in frames: wait this long after the last scroll before fading.
 * Default 60, range 0..60000 (0 = start on release), out of range #error; during the
 * delay only the timer runs, nothing is redrawn.
 */
#ifndef WVUI_CFG_BAR_FADE_DELAY
#define WVUI_CFG_BAR_FADE_DELAY 60
#endif

/**
 * Scrollbar fade-out frames; default 12, range 0..60000 (0 = always visible),
 * out of range #error.
 * @note 0 = the fade is compiled out entirely (call sites become empty expressions);
 *       with ANIM=0 the timer never starts, so the bar stays visible as well.
 */
#ifndef WVUI_CFG_BAR_FADE_FRAMES
#define WVUI_CFG_BAR_FADE_FRAMES 12
#endif

/* ---- PFB size (line count fixed at compile time) ---- */

/**
 * PFB line capacity in lines; default 8, must be >= 1, 0 triggers #error.
 * @note Memory = lines x screen width x 2 B, doubled again for double buffering.
 * @note The line count must be a multiple of ALIGN_Y: screen width and ALIGN_Y are port
 *       runtime facts and are not validated in this file.
 */
#ifndef WVUI_CFG_PFB_LINES
#define WVUI_CFG_PFB_LINES 8
#endif

/**
 * Double-buffering switch; default 1, values 0/1.
 * @note The actual effective condition is port->async != 0: blocking ports fall back to a
 *       full-size PFB automatically to avoid doubling the flush count.
 */
#ifndef WVUI_CFG_PFB_DOUBLE_BUFFER
#define WVUI_CFG_PFB_DOUBLE_BUFFER 1
#endif

/* ---- Resource layer ---- */

/**
 * Streaming decode read buffer in bytes, on the stack, one each in wvui_img.c and glyph
 * decoding; default 64, range 8..256, out of range #error.
 * @note raw images and alpha bitmaps are read in chunks by visible columns, and the
 *       indexed QOI family prefetches by this size. Larger = fewer callbacks, smaller =
 *       less stack (one RGB565 line at 240 px wide is 480 B).
 */
#ifndef WVUI_CFG_IMG_STREAM_CHUNK
#define WVUI_CFG_IMG_STREAM_CHUNK 64
#endif

/**
 * Line-count cap for marking text dirty line by line, in lines; default 32, range >= 1,
 * 0 triggers #error.
 * @note Text beyond the cap takes the whole-block dirty path with no unbounded walk;
 *       this also caps the per-line comparison loop of label.
 */
#ifndef WVUI_CFG_TEXT_LINE_MAX
#define WVUI_CFG_TEXT_LINE_MAX 32
#endif

/* ---- Page system ---- */

/**
 * Page stack depth in pages; default 4, range 1..32, out of range #error.
 * @note The stack stores page pointers, RAM = depth x sizeof(void*); depth 1 = only the
 *       current page, no back.
 */
#ifndef WVUI_CFG_PAGE_STACK
#define WVUI_CFG_PAGE_STACK 4
#endif

/* Widget trimming switches: built-in widgets are provided by the compile-time class table
 * (wvui_class_table[]) and are never registered at runtime. A trimmed widget stays out of
 * the class table, its .c is not pulled out of the static library, and code, static tables
 * and draw/event handlers vanish together. Type numbers are a frozen enum: trimming one
 * does not shift the others, and wvui_widget_create() on a trimmed type returns NULL.
 * Three ways to trim: -D WVUI_CFG_WIDGET_XXX=0; -D WVUI_WIDGETS=label,btn,... (the CMake side
 * translates it into setting all the remaining ones to 0, see cmake/wvui_switches.cmake);
 * or edit the defaults of the lines below directly.
 * Any non-zero value enables the widget (#if semantics). These switches get no 0/1 checks
 * and never #error. */

/** label single-line text. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_LABEL
#define WVUI_CFG_WIDGET_LABEL 1
#endif
/** btn button. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_BTN
#define WVUI_CFG_WIDGET_BTN 1
#endif
/** scroll_panel single-axis scrollable container. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_SCROLL_PANEL
#define WVUI_CFG_WIDGET_SCROLL_PANEL 1
#endif
/** img (streamed resource read). Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_IMG
#define WVUI_CFG_WIDGET_IMG 1
#endif
/** line anti-aliased segment / divider. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_LINE
#define WVUI_CFG_WIDGET_LINE 1
#endif
/** progress bar. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_PROGRESS
#define WVUI_CFG_WIDGET_PROGRESS 1
#endif
/** toggle switch, with animation. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_TOGGLE
#define WVUI_CFG_WIDGET_TOGGLE 1
#endif
/** checkbox with entry animation. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_CHECKBOX
#define WVUI_CFG_WIDGET_CHECKBOX 1
#endif
/** indicator status dot. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_INDICATOR
#define WVUI_CFG_WIDGET_INDICATOR 1
#endif
/** box container with individually styled corners. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_BOX
#define WVUI_CFG_WIDGET_BOX 1
#endif
/** arc / ring sector drawing. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_ARC
#define WVUI_CFG_WIDGET_ARC 1
#endif

/** slider slider, drag based. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_SLIDER
#define WVUI_CFG_WIDGET_SLIDER 1
#endif
/** gauge meter, two arcs + pointer. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_GAUGE
#define WVUI_CFG_WIDGET_GAUGE 1
#endif
/** chart line chart, one sample per pixel. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_CHART
#define WVUI_CFG_WIDGET_CHART 1
#endif
/** stepper stepper, fixed-point int32. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_STEPPER
#define WVUI_CFG_WIDGET_STEPPER 1
#endif
/** list text list with self-drawn rows. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_LIST
#define WVUI_CFG_WIDGET_LIST 1
#endif
/** roller wheel selector, snaps on release. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_ROLLER
#define WVUI_CFG_WIDGET_ROLLER 1
#endif

/** toast floating note, non-modal. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_TOAST
#define WVUI_CFG_WIDGET_TOAST 1
#endif
/** msgbox modal dialog. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_MSGBOX
#define WVUI_CFG_WIDGET_MSGBOX 1
#endif
/** dropdown dropdown selector, collapses when tapped outside. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_DROPDOWN
#define WVUI_CFG_WIDGET_DROPDOWN 1
#endif
/** segdisp seven-segment display. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_SEGDISP
#define WVUI_CFG_WIDGET_SEGDISP 1
#endif

/** marquee scrolling ticker. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_MARQUEE
#define WVUI_CFG_WIDGET_MARQUEE 1
#endif
/** label_ex word-wrap label. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_LABEL_EX
#define WVUI_CFG_WIDGET_LABEL_EX 1
#endif
/** group container. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_GROUP
#define WVUI_CFG_WIDGET_GROUP 1
#endif
/** slideshow auto carousel. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_SLIDESHOW
#define WVUI_CFG_WIDGET_SLIDESHOW 1
#endif

/** barchart bar chart. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_BARCHART
#define WVUI_CFG_WIDGET_BARCHART 1
#endif
/** segbar segment bar / infinite scroll window. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_SEGBAR
#define WVUI_CFG_WIDGET_SEGBAR 1
#endif
/** monoimg 1bpp monochrome bitmap. Default 1; 0 = not in class table. */
#ifndef WVUI_CFG_WIDGET_MONOIMG
#define WVUI_CFG_WIDGET_MONOIMG 1
#endif

/* Value validation: values outside the asserted ranges in this section always #error */
#if (WVUI_CFG_ENABLE_INPUT != 0) && (WVUI_CFG_ENABLE_INPUT != 1)
#error "WVUI_CFG_ENABLE_INPUT must be 0 or 1"
#endif
#if (WVUI_CFG_ENABLE_PAGES != 0) && (WVUI_CFG_ENABLE_PAGES != 1)
#error "WVUI_CFG_ENABLE_PAGES must be 0 or 1"
#endif
#if (WVUI_CFG_ENABLE_LOG != 0) && (WVUI_CFG_ENABLE_LOG != 1)
#error "WVUI_CFG_ENABLE_LOG must be 0 or 1"
#endif
#if (WVUI_CFG_ENABLE_TOP_LAYER != 0) && (WVUI_CFG_ENABLE_TOP_LAYER != 1)
#error "WVUI_CFG_ENABLE_TOP_LAYER must be 0 or 1"
#endif
#if (WVUI_CFG_ENABLE_NESTED_INPUT != 0) && (WVUI_CFG_ENABLE_NESTED_INPUT != 1)
#error "WVUI_CFG_ENABLE_NESTED_INPUT must be 0 or 1"
#endif
#if (WVUI_CFG_ENABLE_RENDER_STATS != 0) && (WVUI_CFG_ENABLE_RENDER_STATS != 1)
#error "WVUI_CFG_ENABLE_RENDER_STATS must be 0 or 1"
#endif
#if (WVUI_CFG_ENABLE_CMD_CHANNEL != 0) && (WVUI_CFG_ENABLE_CMD_CHANNEL != 1)
#error "WVUI_CFG_ENABLE_CMD_CHANNEL must be 0 or 1"
#endif
#if (WVUI_CFG_CMD_CRC8 != 0) && (WVUI_CFG_CMD_CRC8 != 1)
#error "WVUI_CFG_CMD_CRC8 must be 0 or 1"
#endif
#if (WVUI_CFG_CMD_RING < 8) || ((WVUI_CFG_CMD_RING & (WVUI_CFG_CMD_RING - 1)) != 0)
#error "WVUI_CFG_CMD_RING must be a power of two >= 8 (wrap by mask, no M0 division)"
#endif
#if (WVUI_CFG_CMD_MAX_PAYLOAD < 4) || (WVUI_CFG_CMD_MAX_PAYLOAD > 512)
#error "WVUI_CFG_CMD_MAX_PAYLOAD must be in 4..512 (static parser buffer)"
#endif
#if (WVUI_CFG_CMD_TEXT_POOL < 8)
#error "WVUI_CFG_CMD_TEXT_POOL must be >= 8"
#endif
#if (WVUI_CFG_ENABLE_INDEXQOI != 0) && (WVUI_CFG_ENABLE_INDEXQOI != 1)
#error "WVUI_CFG_ENABLE_INDEXQOI must be 0 or 1"
#endif
#if (WVUI_CFG_ENABLE_INDEXQOI_MASK != 0) && (WVUI_CFG_ENABLE_INDEXQOI_MASK != 1)
#error "WVUI_CFG_ENABLE_INDEXQOI_MASK must be 0 or 1"
#endif
#if (WVUI_CFG_DEBUG_DIRTY_RECT != 0) && (WVUI_CFG_DEBUG_DIRTY_RECT != 1)
#error "WVUI_CFG_DEBUG_DIRTY_RECT must be 0 or 1"
#endif
#if (WVUI_CFG_DEBUG_PERF_STRESS != 0) && (WVUI_CFG_DEBUG_PERF_STRESS != 1)
#error "WVUI_CFG_DEBUG_PERF_STRESS must be 0 or 1"
#endif
#if (WVUI_CFG_ENABLE_PORT_HASH != 0) && (WVUI_CFG_ENABLE_PORT_HASH != 1)
#error "WVUI_CFG_ENABLE_PORT_HASH must be 0 or 1"
#endif

/* Thresholds must be positive: 0 makes every MOVE count as a drag. */
#if (WVUI_CFG_DRAG_THRESHOLD < 1)
#error "WVUI_CFG_DRAG_THRESHOLD must be >= 1 (0 makes every MOVE count as a drag)"
#endif
#if (WVUI_CFG_SWIPE_THRESHOLD < 1)
#error "WVUI_CFG_SWIPE_THRESHOLD must be >= 1"
#endif
#if (WVUI_CFG_SCROLL_OVERSCROLL < 0) || (WVUI_CFG_SCROLL_OVERSCROLL > 1000)
#error "WVUI_CFG_SCROLL_OVERSCROLL must be in 0..1000 (pixels)"
#endif
#if (WVUI_CFG_SCROLL_MIN_VEL < 1)
#error "WVUI_CFG_SCROLL_MIN_VEL must be >= 1 (0 would never stop the inertia)"
#endif
#if (WVUI_CFG_PROGRESS_STEP < 1) || (WVUI_CFG_PROGRESS_STEP > 255)
#error "WVUI_CFG_PROGRESS_STEP must be in 1..255 (0 would never reach the target)"
#endif
#if (WVUI_CFG_TOAST_FRAMES < 2) || (WVUI_CFG_TOAST_FRAMES > 60000)
#error "WVUI_CFG_TOAST_FRAMES must be in 2..60000 (frames)"
#endif
#if (WVUI_CFG_TOAST_FADE < 0) || (WVUI_CFG_TOAST_FADE > WVUI_CFG_TOAST_FRAMES)
#error "WVUI_CFG_TOAST_FADE must be in 0..WVUI_CFG_TOAST_FRAMES (frames)"
#endif
#if (WVUI_CFG_SLIDESHOW_HOLD < 1) || (WVUI_CFG_SLIDESHOW_HOLD > 255)
#error "WVUI_CFG_SLIDESHOW_HOLD must be in 1..255 (frames; u8 runtime field)"
#endif
#if (WVUI_CFG_SLIDESHOW_SLIDE > 255)
#error "WVUI_CFG_SLIDESHOW_SLIDE must be <= 255 (0 = hard cut)"
#endif

/* Frame-count caps (a ramp keeps its remaining frames in a u16); 0 is allowed for
 * PAGE/BAR_FADE_FRAMES. */
#if (WVUI_CFG_PAGE_FADE_FRAMES > 60000)
#error "WVUI_CFG_PAGE_FADE_FRAMES must be <= 60000 (0 = hard cut)"
#endif
#if (WVUI_CFG_BAR_FADE_DELAY > 60000)
#error "WVUI_CFG_BAR_FADE_DELAY must be <= 60000 (frames)"
#endif
#if (WVUI_CFG_BAR_FADE_FRAMES > 60000)
#error "WVUI_CFG_BAR_FADE_FRAMES must be <= 60000 (0 = bar always visible)"
#endif

#if (WVUI_CFG_PFB_LINES < 1)
#error "WVUI_CFG_PFB_LINES must be >= 1"
#endif

#if (WVUI_CFG_PFB_DOUBLE_BUFFER != 0) && (WVUI_CFG_PFB_DOUBLE_BUFFER != 1)
#error "WVUI_CFG_PFB_DOUBLE_BUFFER must be 0 or 1"
#endif

/* Streamed read-buffer bounds 8..256: the buffer lives on the stack, whose capacity is
 * limited at runtime. */
#if (WVUI_CFG_IMG_STREAM_CHUNK < 8) || (WVUI_CFG_IMG_STREAM_CHUNK > 256)
#error "WVUI_CFG_IMG_STREAM_CHUNK must be in 8..256 (stack buffer for streamed decode)"
#endif

#if (WVUI_CFG_TEXT_LINE_MAX < 1)
#error "WVUI_CFG_TEXT_LINE_MAX must be >= 1"
#endif

/* Page stack: at least 1 (1 = only the current page), capped at 32 (static array). */
#if (WVUI_CFG_PAGE_STACK < 1) || (WVUI_CFG_PAGE_STACK > 32)
#error "WVUI_CFG_PAGE_STACK must be in 1..32 (static array of page pointers)"
#endif

/* Memory mode: values outside the three tiers error here; the STATIC tier and the page
 * system are mutually exclusive. */
#if (WVUI_CFG_MEM_MODE != WVUI_MEM_STATIC) && (WVUI_CFG_MEM_MODE != WVUI_MEM_ARENA) && \
    (WVUI_CFG_MEM_MODE != WVUI_MEM_HEAP)
#error "WVUI_CFG_MEM_MODE must be WVUI_MEM_STATIC (0), WVUI_MEM_ARENA (1) or WVUI_MEM_HEAP (2)"
#endif
#if (WVUI_CFG_MEM_MODE == WVUI_MEM_STATIC) && (WVUI_CFG_ENABLE_PAGES != 0)
#error "WVUI_CFG_MEM_MODE=0 (static: caller-declared objects) requires WVUI_CFG_ENABLE_PAGES=0"
#endif
#if (WVUI_CFG_MEM_HEAP_FREERTOS != 0) && (WVUI_CFG_MEM_HEAP_FREERTOS != 1)
#error "WVUI_CFG_MEM_HEAP_FREERTOS must be 0 (stdlib) or 1 (pvPortMalloc/vPortFree)"
#endif
#if (WVUI_CFG_ENABLE_CLASS_BIND != 0) && (WVUI_CFG_ENABLE_CLASS_BIND != 1)
#error "WVUI_CFG_ENABLE_CLASS_BIND must be 0 (const table) or 1 (writable table)"
#endif
#if (WVUI_CFG_WIDGET_COMPACT_TYPE != 0) && (WVUI_CFG_WIDGET_COMPACT_TYPE != 1)
#error "WVUI_CFG_WIDGET_COMPACT_TYPE must be 0 (class pointer) or 1 (type byte)"
#endif

#if (WVUI_CFG_DIRTY_STRATEGY < 0) || (WVUI_CFG_DIRTY_STRATEGY > 2)
#error "WVUI_CFG_DIRTY_STRATEGY must be 0 (full), 1 (bbox) or 2 (multi)"
#endif
#if (WVUI_CFG_DIRTY_MAX_NUM < 1) || (WVUI_CFG_DIRTY_MAX_NUM > 255)
#error "WVUI_CFG_DIRTY_MAX_NUM must be in 1..255 (the slot count lives in one byte)"
#endif
#if (WVUI_CFG_DIRTY_MERGE_MAX > 255)
#error "WVUI_CFG_DIRTY_MERGE_MAX must be <= 255"
#endif
#if (WVUI_CFG_DIRTY_FIXED_COST_PX_FALLBACK < 1)
#error "WVUI_CFG_DIRTY_FIXED_COST_PX_FALLBACK must be >= 1 (0 would mean never merge)"
#endif
#if (WVUI_CFG_CLASS_MAX < 1)
#error "WVUI_CFG_CLASS_MAX must be >= 1"
#endif

/* Widget-side convention: every widget header declares WVUI_<NAME>_USE_*, and its default
 * points to the matching WVUI_CFG_* master switch here. */

#endif /* WVUI_CONFIG_H */
