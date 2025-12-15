#pragma once

#include <ovbase.h>

struct gcmz_window_list;

/**
 * @brief Create window list
 *
 * Creates a new window list container for tracking window handles.
 *
 * @param err [out] Error information
 * @return Pointer to new window list on success, NULL on failure
 */
NODISCARD struct gcmz_window_list *gcmz_window_list_create(struct ov_error *const err);

/**
 * @brief Destroy window list and free memory
 *
 * @param wl Pointer to window list pointer
 */
void gcmz_window_list_destroy(struct gcmz_window_list **const wl);

/**
 * @brief Update window list with new window handles
 *
 * Updates the internal list with new window handles.
 * Returns whether the list contents changed (windows added/removed/reordered).
 *
 * @param wl Window list to update
 * @param windows Array of window handles
 * @param num_windows Number of windows in array
 * @param err [out] Error information
 * @return ov_true if list changed, ov_false if unchanged, ov_indeterminate on error
 */
NODISCARD ov_tribool gcmz_window_list_update(struct gcmz_window_list *const wl,
                                             void *const *const windows,
                                             size_t const num_windows,
                                             struct ov_error *const err);
