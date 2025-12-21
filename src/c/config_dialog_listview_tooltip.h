#pragma once

#include <ovbase.h>

struct config_dialog_listview_tooltip;

/**
 * @brief Create tooltip manager for listview items
 *
 * Shows full text in tooltip when listview cell text is truncated.
 *
 * @param parent Parent window handle (HWND)
 * @param listview Listview control handle (HWND)
 * @param err [out] Error information on failure
 * @return Tooltip manager instance on success, NULL on failure
 */
struct config_dialog_listview_tooltip *
config_dialog_listview_tooltip_create(void *parent, void *listview, struct ov_error *const err);

/**
 * @brief Destroy tooltip manager and free resources
 *
 * @param ttpp Pointer to tooltip manager instance
 */
void config_dialog_listview_tooltip_destroy(struct config_dialog_listview_tooltip **ttpp);
