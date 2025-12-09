#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct gcmzdrops;
struct gcmz_lua_context;
struct ov_error;
struct aviutl2_host_app_table;

/**
 * @brief Create and initialize gcmzdrops context (called from InitializePlugin)
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
 * @brief Register plugin with AviUtl2 host (called from RegisterPlugin)
 */
void gcmzdrops_register(struct gcmzdrops *const ctx, struct aviutl2_host_app_table *const host);
