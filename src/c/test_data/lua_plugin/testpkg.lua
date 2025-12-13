-- Test plugin module (single file style: testpkg.lua)
-- This module is used to test setup_plugin_loading with single .lua files

local M = {}

M.name = "testpkg"
M.version = "1.0.0"
M.loaded = true

function M.get_info()
  return {
    name = M.name,
    version = M.version,
  }
end

return M
