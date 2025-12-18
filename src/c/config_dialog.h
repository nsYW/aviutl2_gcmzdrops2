#pragma once

#include <ovbase.h>

struct gcmz_config;

/**
 * @brief Callback function for enumerating handlers
 *
 * @param name Handler module name
 * @param priority Handler priority (higher = earlier processing)
 * @param source Source path of the handler script
 * @param userdata User-provided context pointer
 * @return true to continue enumeration, false to stop
 */
typedef bool (*gcmz_config_dialog_handler_enum_fn)(char const *name, int priority, char const *source, void *userdata);

/**
 * @brief Callback function to enumerate handlers via injection
 *
 * @param callback_context Context pointer for the callback implementation itself
 * @param fn Callback function to call for each handler
 * @param userdata User-provided context pointer passed to fn
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
typedef bool (*gcmz_config_dialog_enum_handlers_fn)(void *callback_context,
                                                    gcmz_config_dialog_handler_enum_fn fn,
                                                    void *userdata,
                                                    struct ov_error *err);

/**
 * @brief Show configuration dialog
 *
 * Displays a modal dialog for editing application configuration.
 * Changes are saved to GCMZDrops.json when the user clicks OK.
 *
 * @param config Configuration object to use for the dialog
 * @param enum_handlers Function to enumerate handlers (can be NULL)
 * @param enum_handlers_context Context pointer passed as first argument to enum_handlers (can be NULL)
 * @param parent_window Parent window handle for dialog positioning (can be NULL)
 * @param external_api_running Whether the external API is currently running
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
bool gcmz_config_dialog_show(struct gcmz_config *config,
                             gcmz_config_dialog_enum_handlers_fn enum_handlers,
                             void *enum_handlers_context,
                             void *parent_window,
                             bool const external_api_running,
                             struct ov_error *const err);
