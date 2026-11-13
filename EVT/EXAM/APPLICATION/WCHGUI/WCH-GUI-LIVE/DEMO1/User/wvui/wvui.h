#ifndef WVUI_H
#define WVUI_H

/**
 * @file wvui.h
 * @brief Master header: include all public headers at once.
 * @note  Set each switch in exactly one place, wvui_conf.h or -D, otherwise
 *        macro redefinition warnings appear; kernel .c files must not
 *        include this file.
 */

/* Include first: the other declarations depend on WVUI_CFG_* values.
 * wvui_config.h pulls in wvui_conf.h on its path and fills defaults; builds
 * may also inject -D WVUI_CFG_X=N. */
#include "wvui_config.h"

#define WVUI_VERSION "0.1.0"

/** Basic types and kernel. */
#include "wvui_types.h"       /* rectangles / colors / event codes */
#include "wvui_mem.h"         /* memory modes and stats */
#include "wvui_arena.h"       /* static object memory (arena) */
#include "wvui_driver.h"      /* init, frames and stats */
#include "wvui_dirty.h"       /* dirty rectangles, invalidate entry */
#include "wvui_render.h"      /* slice render context */
#include "wvui_widget.h"      /* widget base class and class table */
#include "wvui_widgets.h"     /* WVUI_TYPE_* and class tables */
#include "wvui_slice.h"       /* current slice query */
#include "wvui_input.h"       /* input state machine */
#include "wvui_input_port.h"  /* touch/key drivers implement it */
#include "wvui_layer.h"       /* top layer chain and modality */
#include "wvui_page.h"        /* page system */
#include "wvui_props.h"       /* property numbers (command addressing) */
#include "wvui_cmd.h"         /* command channel (serial) */
#include "wvui_anim.h"        /* animation engine */
#include "wvui_scroll.h"      /* scroll physics */

/** Drawing, fonts and assets. */
#include "wvui_draw.h"        /* draw methods (AA/corners) */
#include "wvui_font.h"        /* glyphs and metrics */
#include "wvui_asset.h"       /* asset ID -> blob */
#include "wvui_hash.h"        /* pixel stream hash */

/* Built-in widget names match the directory names under widgets/.
 * Widgets cut by WVUI_WIDGETS remain includable (only declarations and type
 * numbers left), so the full list is safe. */
#include "widgets/arc/wvui_arc.h"
#include "widgets/barchart/wvui_barchart.h"
#include "widgets/box/wvui_box.h"
#include "widgets/btn/wvui_btn.h"
#include "widgets/chart/wvui_chart.h"
#include "widgets/checkbox/wvui_checkbox.h"
#include "widgets/dropdown/wvui_dropdown.h"
#include "widgets/monoimg/wvui_monoimg.h"
#include "widgets/gauge/wvui_gauge.h"
#include "widgets/group/wvui_group.h"
#include "widgets/img/wvui_img.h"
#include "widgets/indicator/wvui_indicator.h"
#include "widgets/label/wvui_label.h"
#include "widgets/label_ex/wvui_label_ex.h"
#include "widgets/line/wvui_line.h"
#include "widgets/list/wvui_list.h"
#include "widgets/marquee/wvui_marquee.h"
#include "widgets/msgbox/wvui_msgbox.h"
#include "widgets/progress/wvui_progress.h"
#include "widgets/roller/wvui_roller.h"
#include "widgets/scroll_panel/wvui_scroll_panel.h"
#include "widgets/segdisp/wvui_segdisp.h"
#include "widgets/slider/wvui_slider.h"
#include "widgets/slideshow/wvui_slideshow.h"
#include "widgets/stepper/wvui_stepper.h"
#include "widgets/toast/wvui_toast.h"
#include "widgets/toggle/wvui_toggle.h"

#endif /* WVUI_H */
