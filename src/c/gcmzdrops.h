#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct gcmzdrops;
struct gcmz_lua_context;
struct ov_error;
struct aviutl2_host_app_table;

/**
 * @brief Create and initialize gcmzdrops context
 *
 * @param ctx [out] Pointer to store the created context
 * @param lua_ctx Lua context created by gcmz_lua_create (ownership NOT transferred)
 * @param version AviUtl ExEdit2 version number
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
bool gcmzdrops_create(struct gcmzdrops **const ctx,
                      struct gcmz_lua_context *const lua_ctx,
                      uint32_t const version,
                      struct ov_error *const err);

/**
 * @brief Destroy gcmzdrops context
 *
 * @param ctx [in,out] Pointer to context to destroy, will be set to NULL
 */
void gcmzdrops_destroy(struct gcmzdrops **const ctx);

/**
 * @brief Register plugin with AviUtl2 host
 */
void gcmzdrops_register(struct gcmzdrops *const ctx, struct aviutl2_host_app_table *const host);

/**
 * @brief Show configuration dialog
 *
 * @param ctx Plugin context
 * @param hwnd Parent window handle
 * @param dll_hinst DLL instance handle
 */
void gcmzdrops_show_config_dialog(struct gcmzdrops *const ctx, void *const hwnd, void *const dll_hinst);

/**
 * @brief Handle project load event
 *
 * @param ctx Plugin context
 * @param project_path Project file path (can be NULL)
 */
void gcmzdrops_on_project_load(struct gcmzdrops *const ctx, wchar_t const *const project_path);

/**
 * @brief Paste from clipboard
 *
 * @param ctx Plugin context
 */
void gcmzdrops_paste_from_clipboard(struct gcmzdrops *const ctx);
