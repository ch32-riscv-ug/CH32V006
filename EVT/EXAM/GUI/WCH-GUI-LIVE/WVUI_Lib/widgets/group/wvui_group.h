#ifndef WVUI_GROUP_H
#define WVUI_GROUP_H

/**
 * @file wvui_group.h
 * @brief Group container: keeps child widgets as one group and shows/hides the whole subtree together.
 * @note  Children use screen-absolute coordinates; the group adds no offset, move each child with set_pos.
 */

#include "wvui_widget.h"

typedef struct
{
    wvui_widget_t *children_head;  /**< Child chain head; must be the first field of this struct */
    wvui_color_t bg;               /**< Background color; 0 = no background */
    wvui_color_t border;           /**< Border color; 0 = no border */
    uint8_t bg_on;                /**< 1 = fill even when the bg value is 0 */
    uint8_t border_w;             /**< Border width in pixels; 0 = no border */
    uint8_t radius;               /**< Corner radius in pixels; 0 = square corners */
    uint8_t opa;                  /**< Opacity 0..255, default 255 */
} wvui_group_rt_t;

extern const wvui_class_t wvui_group_class;

/**
 * @brief Attach a child widget to the group.
 * @param child  Child widget; NULL is ignored
 * @note  Equivalent to wvui_widget_append_child() with one extra type check.
 */
void wvui_group_add(wvui_widget_t *g, wvui_widget_t *child);

/**
 * @brief Show or hide the whole subtree.
 * @param hidden  1 = hide, 0 = show
 * @note  Hidden children are neither drawn nor hit-tested.
 * @note  The state lives only on the group, regardless of how many children are hidden.
 */
void wvui_group_set_hidden(wvui_widget_t *g, uint8_t hidden);

/**
 * @brief Read back the group's hidden flag.
 * @return 1 = hidden, 0 = visible; also returns 0 on a type mismatch
 */
uint8_t wvui_group_is_hidden(const wvui_widget_t *g);

/**
 * @brief Set the group's background and border colors.
 * @param bg      Background color; 0 = no background
 * @param border  Border color; 0 = no border
 * @note  Both default to 0, in which case the group only groups children and toggles visibility.
 */
void wvui_group_set_colors(wvui_widget_t *g, wvui_color_t bg, wvui_color_t border);

/**
 * @brief Enable drawing a background even when the bg value is 0.
 * @param visible  1 = draw even when bg is 0 (pure black); 0 = off, default 0
 * @note  0 also means "no background", so a pure black background needs this switch.
 */
void wvui_group_set_bg_visible(wvui_widget_t *g, uint8_t visible);

/**
 * @brief Set the border width and corner radius.
 * @param border_w  Border width in pixels; 0 = no border
 * @param radius    Corner radius in pixels; 0 = square corners
 * @note  Radius above min(w,h)/2 is clamped by the draw layer.
 */
void wvui_group_set_border(wvui_widget_t *g, uint8_t border_w, uint8_t radius);

/**
 * @brief Set the group's opacity.
 * @param opa  0..255, default 255; 0 = neither background nor border is drawn
 * @note  Affects only the group's own background and border, not how children are drawn.
 */
void wvui_group_set_opa(wvui_widget_t *g, uint8_t opa);

/**
 * @brief Count the group's direct children.
 * @return Number of direct children; one level only, no grandchildren
 */
uint16_t wvui_group_child_count(const wvui_widget_t *g);

#endif /* WVUI_GROUP_H */
