#include <lauxlib.h>
#include <lua.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  (void)hinstDLL;
  (void)fdwReason;
  (void)lpvReserved;
  return TRUE;
}

static int testmod_hello(lua_State *L) {
  lua_pushstring(L, "Hello from testmod C module!");
  return 1;
}

static int testmod_get_info(lua_State *L) {
  lua_newtable(L);
  lua_pushstring(L, "testmod");
  lua_setfield(L, -2, "name");
  lua_pushstring(L, "1.0.0");
  lua_setfield(L, -2, "version");
  return 1;
}

// Entry point for require("testmod")
__declspec(dllexport) int luaopen_testmod(lua_State *L);
__declspec(dllexport) int luaopen_testmod(lua_State *L) {
  lua_newtable(L);
  lua_pushcfunction(L, testmod_hello);
  lua_setfield(L, -2, "hello");
  lua_pushcfunction(L, testmod_get_info);
  lua_setfield(L, -2, "get_info");
  lua_pushstring(L, "testmod");
  lua_setfield(L, -2, "name");
  lua_pushstring(L, "1.0.0");
  lua_setfield(L, -2, "version");
  lua_pushboolean(L, 1);
  lua_setfield(L, -2, "loaded");
  return 1;
}

// Submodule entry point for require("testmod.sub")
// This is the standard Lua C module submodule mechanism
static int testmod_sub_greet(lua_State *L) {
  lua_pushstring(L, "Hello from testmod.sub submodule!");
  return 1;
}

__declspec(dllexport) int luaopen_testmod_sub(lua_State *L);
__declspec(dllexport) int luaopen_testmod_sub(lua_State *L) {
  lua_newtable(L);
  lua_pushcfunction(L, testmod_sub_greet);
  lua_setfield(L, -2, "greet");
  lua_pushstring(L, "testmod.sub");
  lua_setfield(L, -2, "name");
  lua_pushboolean(L, 1);
  lua_setfield(L, -2, "loaded");
  return 1;
}
