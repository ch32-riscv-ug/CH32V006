/**
 * @file wvui_widgets.c
 * @brief Built-in widget class table, index = WVUI_TYPE_*.
 * @note  Each entry is wrapped by WVUI_CFG_WIDGET_<NAME>: a trimmed widget produces no symbol reference.
 */

#include "wvui_widgets.h"

#include "widgets/arc/wvui_arc.h"
#include "widgets/barchart/wvui_barchart.h"
#include "widgets/box/wvui_box.h"
#include "widgets/btn/wvui_btn.h"
#include "widgets/chart/wvui_chart.h"
#include "widgets/checkbox/wvui_checkbox.h"
#include "widgets/dropdown/wvui_dropdown.h"
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
#include "widgets/monoimg/wvui_monoimg.h"
#include "widgets/progress/wvui_progress.h"
#include "widgets/roller/wvui_roller.h"
#include "widgets/scroll_panel/wvui_scroll_panel.h"
#include "widgets/segbar/wvui_segbar.h"
#include "widgets/segdisp/wvui_segdisp.h"
#include "widgets/slider/wvui_slider.h"
#include "widgets/slideshow/wvui_slideshow.h"
#include "widgets/stepper/wvui_stepper.h"
#include "widgets/toast/wvui_toast.h"
#include "widgets/toggle/wvui_toggle.h"

/* With CLASS_BIND=1 the table stays in RAM (wvui_class_bind writes it); with =0 const is added, into flash. */
#if WVUI_CFG_ENABLE_CLASS_BIND
const wvui_class_t *wvui_class_table[WVUI_CFG_CLASS_MAX] = {
#else
const wvui_class_t *const wvui_class_table[WVUI_CFG_CLASS_MAX] = {
#endif
#if WVUI_CFG_WIDGET_LABEL
    [WVUI_TYPE_LABEL] = &wvui_label_class,
#endif
#if WVUI_CFG_WIDGET_BTN
    [WVUI_TYPE_BTN] = &wvui_btn_class,
#endif
#if WVUI_CFG_WIDGET_SCROLL_PANEL
    [WVUI_TYPE_SCROLL_PANEL] = &wvui_scroll_panel_class,
#endif
#if WVUI_CFG_WIDGET_IMG
    [WVUI_TYPE_IMG] = &wvui_img_class,
#endif
#if WVUI_CFG_WIDGET_LINE
    [WVUI_TYPE_LINE] = &wvui_line_class,
#endif
#if WVUI_CFG_WIDGET_PROGRESS
    [WVUI_TYPE_PROGRESS] = &wvui_progress_class,
#endif
#if WVUI_CFG_WIDGET_TOGGLE
    [WVUI_TYPE_TOGGLE] = &wvui_toggle_class,
#endif
#if WVUI_CFG_WIDGET_CHECKBOX
    [WVUI_TYPE_CHECKBOX] = &wvui_checkbox_class,
#endif
#if WVUI_CFG_WIDGET_INDICATOR
    [WVUI_TYPE_INDICATOR] = &wvui_indicator_class,
#endif
#if WVUI_CFG_WIDGET_BOX
    [WVUI_TYPE_BOX] = &wvui_box_class,
#endif
#if WVUI_CFG_WIDGET_ARC
    [WVUI_TYPE_ARC] = &wvui_arc_class,
#endif
#if WVUI_CFG_WIDGET_SLIDER
    [WVUI_TYPE_SLIDER] = &wvui_slider_class,
#endif
#if WVUI_CFG_WIDGET_GAUGE
    [WVUI_TYPE_GAUGE] = &wvui_gauge_class,
#endif
#if WVUI_CFG_WIDGET_CHART
    [WVUI_TYPE_CHART] = &wvui_chart_class,
#endif
#if WVUI_CFG_WIDGET_STEPPER
    [WVUI_TYPE_STEPPER] = &wvui_stepper_class,
#endif
#if WVUI_CFG_WIDGET_LIST
    [WVUI_TYPE_LIST] = &wvui_list_class,
#endif
#if WVUI_CFG_WIDGET_ROLLER
    [WVUI_TYPE_ROLLER] = &wvui_roller_class,
#endif
#if WVUI_CFG_WIDGET_TOAST
    [WVUI_TYPE_TOAST] = &wvui_toast_class,
#endif
#if WVUI_CFG_WIDGET_MSGBOX
    [WVUI_TYPE_MSGBOX] = &wvui_msgbox_class,
#endif
#if WVUI_CFG_WIDGET_DROPDOWN
    [WVUI_TYPE_DROPDOWN] = &wvui_dropdown_class,
#endif
#if WVUI_CFG_WIDGET_SEGDISP
    [WVUI_TYPE_SEGDISP] = &wvui_segdisp_class,
#endif
#if WVUI_CFG_WIDGET_MARQUEE
    [WVUI_TYPE_MARQUEE] = &wvui_marquee_class,
#endif
#if WVUI_CFG_WIDGET_LABEL_EX
    [WVUI_TYPE_LABEL_EX] = &wvui_label_ex_class,
#endif
#if WVUI_CFG_WIDGET_GROUP
    [WVUI_TYPE_GROUP] = &wvui_group_class,
#endif
#if WVUI_CFG_WIDGET_SLIDESHOW
    [WVUI_TYPE_SLIDESHOW] = &wvui_slideshow_class,
#endif
#if WVUI_CFG_WIDGET_BARCHART
    [WVUI_TYPE_BARCHART] = &wvui_barchart_class,
#endif
#if WVUI_CFG_WIDGET_SEGBAR
    [WVUI_TYPE_SEGBAR] = &wvui_segbar_class,
#endif
#if WVUI_CFG_WIDGET_MONOIMG
    [WVUI_TYPE_MONOIMG] = &wvui_monoimg_class,
#endif
};

/* ---- Compatibility entry ---- */
/* The class table is filled at compile time; this function binds nothing and only returns the non-empty slot count (see wvui_widgets.h). */
uint16_t wvui_widgets_register_builtin(void)
{
    return wvui_class_bound_count();
}
