-- Test plugin module (directory style: testpkg2/init.lua)
-- This module is used to test setup_plugin_loading with directory/init.lua pattern

local M = {}

M.name = "testpkg2"
M.version = "2.0.0"
M.loaded = true

function M.get_info()
  return {
    name = M.name,
    version = M.version,
  }
end

return M
