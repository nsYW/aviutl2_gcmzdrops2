#pragma once

#include <ovbase.h>

struct gcmz_file_list;
struct gcmz_drop;

/**
 * @brief Data object extraction callback
 *
 * @param dataobj IDataObject pointer
 * @param dest File list to populate
 * @param userdata User data passed to the function
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
typedef bool (*gcmz_drop_dataobj_extract_fn)(void *dataobj,
                                             struct gcmz_file_list *dest,
                                             void *userdata,
                                             struct ov_error *const err);

/**
 * @brief Temporary file cleanup callback
 *
 * @param path File path to clean up
 * @param userdata User data passed to the function
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
typedef bool (*gcmz_drop_cleanup_temp_file_fn)(wchar_t const *const path, void *userdata, struct ov_error *const err);

/**
 * @brief File management callback
 *
 * @param source_file Source file path to process
 * @param final_file [out] Final file path (caller must OV_ARRAY_DESTROY)
 * @param userdata User data passed to the function
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
typedef bool (*gcmz_drop_file_manage_fn)(wchar_t const *source_file,
                                         wchar_t **final_file,
                                         void *userdata,
                                         struct ov_error *const err);

/**
 * @brief EXO conversion callback
 *
 * Called to convert files to EXO format via Lua scripts.
 *
 * @param file_list File list to convert (can be modified)
 * @param userdata User data passed to the function
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
typedef bool (*gcmz_drop_exo_convert_fn)(struct gcmz_file_list *file_list, void *userdata, struct ov_error *const err);

/**
 * @brief Drag enter callback
 *
 * Called when drag enters the target window.
 *
 * @param file_list File list being dragged (can be modified)
 * @param key_state Key state flags (MK_CONTROL, MK_SHIFT, etc.)
 * @param modifier_keys Additional modifier keys
 * @param from_api true if called from external API
 * @param userdata User data passed to the function
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
typedef bool (*gcmz_drop_drag_enter_fn)(struct gcmz_file_list *file_list,
                                        uint32_t key_state,
                                        uint32_t modifier_keys,
                                        bool from_api,
                                        void *userdata,
                                        struct ov_error *const err);

/**
 * @brief Drop callback
 *
 * Called when files are dropped on the target window.
 *
 * @param file_list File list being dropped (can be modified)
 * @param key_state Key state flags (MK_CONTROL, MK_SHIFT, etc.)
 * @param modifier_keys Additional modifier keys
 * @param from_api true if called from external API
 * @param userdata User data passed to the function
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
typedef bool (*gcmz_drop_drop_fn)(struct gcmz_file_list *file_list,
                                  uint32_t key_state,
                                  uint32_t modifier_keys,
                                  bool from_api,
                                  void *userdata,
                                  struct ov_error *const err);

/**
 * @brief Drag leave callback
 *
 * Called when drag leaves the target window.
 *
 * @param userdata User data passed to the function
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
typedef bool (*gcmz_drop_drag_leave_fn)(void *userdata, struct ov_error *const err);

/**
 * @brief Options for drop context creation
 */
struct gcmz_drop_options {
  gcmz_drop_dataobj_extract_fn extract;   ///< Required: Data object extraction function
  gcmz_drop_cleanup_temp_file_fn cleanup; ///< Required: Temporary file cleanup function
  gcmz_drop_file_manage_fn file_manage;   ///< Optional: File management function
  gcmz_drop_exo_convert_fn exo_convert;   ///< Optional: EXO conversion callback
  gcmz_drop_drag_enter_fn drag_enter;     ///< Optional: Drag enter callback
  gcmz_drop_drop_fn drop;                 ///< Optional: Drop callback
  gcmz_drop_drag_leave_fn drag_leave;     ///< Optional: Drag leave callback
  void *userdata;                         ///< User data passed to all callbacks
};

/**
 * @brief Create and initialize drop context
 *
 * @param options Drop options (required)
 * @param err [out] Error information on failure
 * @return Drop context pointer on success, NULL on failure
 */
struct gcmz_drop *gcmz_drop_create(struct gcmz_drop_options const *const options, struct ov_error *const err);

/**
 * @brief Register a window for drop target functionality
 *
 * @param ctx Drop context
 * @param window Window handle to register
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
bool gcmz_drop_register_window(struct gcmz_drop *const ctx, void *const window, struct ov_error *const err);

/**
 * @brief Destroy drop context and free memory
 *
 * @param ctx Drop context pointer
 */
void gcmz_drop_destroy(struct gcmz_drop **const ctx);

/**
 * @brief Create IDataObject from file list with CF_HDROP format
 *
 * Creates a new IDataObject containing file paths in CF_HDROP format.
 * The returned object must be released by the caller using IDataObject_Release.
 *
 * @param file_list File list to include in the data object
 * @param x X coordinate to store in DROPFILES structure
 * @param y Y coordinate to store in DROPFILES structure
 * @param err [out] Error information on failure
 * @return IDataObject pointer (as void*) on success, NULL on failure
 */
void *gcmz_drop_create_file_list_dataobj(struct gcmz_file_list const *const file_list,
                                         int const x,
                                         int const y,
                                         struct ov_error *const err);

/**
 * @brief Simple callback for after file processing
 *
 * @param file_list Processed file list (temporary, valid only during callback)
 * @param userdata User data passed via completion_userdata parameter
 */
typedef void (*gcmz_drop_simulate_callback)(struct gcmz_file_list const *file_list, void *userdata);

/**
 * @brief Process files with Lua hooks
 *
 * Performs file processing and calls Lua handlers,
 * then invokes callback with processed file list.
 *
 * Processing steps:
 * 1. Performs EXO conversion if enabled
 * 2. Calls Lua handlers (drag_enter, drop) in sequence
 * 3. Applies file management (copying, etc.)
 * 4. Calls completion callback with processed file list
 *
 * @param ctx Drop context
 * @param file_list File list to process
 * @param use_exo_converter Whether to enable EXO conversion
 * @param completion_callback Callback receiving processed files (required, must not be NULL)
 * @param completion_userdata User data passed to completion callback
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
bool gcmz_drop_simulate_drop(struct gcmz_drop *const ctx,
                             struct gcmz_file_list *file_list,
                             bool const use_exo_converter,
                             gcmz_drop_simulate_callback const completion_callback,
                             void *const completion_userdata,
                             struct ov_error *const err);
