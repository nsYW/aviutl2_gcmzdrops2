--- GCMZDrops2 entrypoint module.
-- Handles drag_enter, drag_leave, and drop events by delegating to registered modules.
-- @module entrypoint

local M = {}

-- Local module storage (not accessible from global scope)
local modules = {}

--- Sort modules by priority (ascending order)
-- @local
local function sort_modules(a, b)
  return a.priority < b.priority
end

--- Register a module to the module list.
-- @param module_table table The module table (must have name field)
-- @param source string Source path of the module (file path or module origin, required)
-- @return boolean, string true on success, or false and error message on failure
-- @local
local function register_module(module_table, source)
  -- source is required
  if type(source) ~= "string" or source == "" then
    return false, "handler source path is required"
  end
  if type(module_table) ~= "table" then
    return false, "handler script must return a table"
  end
  -- name is required
  local name = module_table.name
  if type(name) ~= "string" or name == "" then
    return false, "handler must have a name field"
  end
  -- priority defaults to 1000 if not specified
  local priority = module_table.priority
  if type(priority) ~= "number" then
    priority = 1000
  end
  table.insert(modules, {
    name = name,
    priority = priority,
    module = module_table,
    source = source,
    active = true,
  })
  return true, nil
end

--- Load handler modules from a list of module info.
-- Called from C side with the list of module info in the script directory.
-- Loads all modules using require, registers them, and sorts by priority.
-- @param modinfo table Array of { name = "modname", path = "filepath" }
function M.load_handlers(modinfo)
  if type(modinfo) ~= "table" then
    return
  end
  for _, info in ipairs(modinfo) do
    local modname = info.name
    local modpath = info.path
    if type(modname) ~= "string" or modname == "" then
      debug_print("handler module name is invalid")
    elseif type(modpath) ~= "string" or modpath == "" then
      debug_print("handler source path is required: " .. modname)
    else
      local ok, result = pcall(require, modname)
      if not ok then
        debug_print("failed to load handler: " .. modname .. ": " .. tostring(result))
      else
        local registered, err = register_module(result, modpath)
        if not registered then
          debug_print(err .. ": " .. modname)
        end
      end
    end
  end
  table.sort(modules, sort_modules)
end

--- Add a handler module from a table.
-- Checks for name field and registers the module if valid.
-- @param module_table table The module table returned by the script (must have name field)
-- @param source string Source path of the module
-- @return boolean, string true on success, or false and error message on failure
-- @local
local function add_module(module_table, source)
  local ok, err = register_module(module_table, source)
  if not ok then
    return false, err
  end
  table.sort(modules, sort_modules)
  return true, nil
end

--- Add a handler module from a script string.
-- Loads and executes the script, then registers it as a module if valid.
-- @param script string The script content to execute
-- @param source string Source path indicating the caller (module path or empty)
-- @return boolean, string true on success, or false and error message on failure
function M.add_module_from_string(script, source)
  if type(source) ~= "string" or source == "" then
    return false, "handler source path is required"
  end
  local chunk, err = loadstring(script, source)
  if not chunk then
    return false, err
  end
  local ok, result = pcall(chunk)
  if not ok then
    return false, result
  end
  return add_module(result, source)
end

--- Add a handler module from a file.
-- Loads and executes the script file, then registers it as a module if valid.
-- @param filepath string Path to the script file
-- @return boolean, string true on success, or false and error message on failure
function M.add_module_from_file(filepath)
  if type(filepath) ~= "string" then
    return false, "invalid arguments"
  end
  local chunk, err = loadfile(filepath)
  if not chunk then
    return false, err
  end
  local ok, result = pcall(chunk)
  if not ok then
    return false, result
  end
  return add_module(result, filepath)
end

--- Get the number of registered modules.
-- Used for testing purposes.
-- @return number The number of registered modules
function M.get_module_count()
  return #modules
end

--- Get a module entry by index.
-- Used for testing purposes.
-- @param index number 1-based index
-- @return table|nil The module entry or nil if out of range
function M.get_module(index)
  return modules[index]
end

--- Enumerate all registered modules.
-- Calls the callback function for each registered handler module.
-- @param fn function Callback function(name, priority, source) called for each module
function M.enum_modules(fn)
  if type(fn) ~= "function" then
    return
  end
  for _, entry in ipairs(modules) do
    fn(entry.name, entry.priority, entry.source)
  end
end

--- Call drag_enter hook on all loaded modules in priority order.
-- Modules that return false from drag_enter are marked as inactive.
-- If a handler throws an error, it is caught and logged, and the handler is marked inactive.
-- @param files table File list with format { {filepath="...", mimetype="...", temporary=bool}, ... }
-- @param state table Key state with format { control=bool, shift=bool, alt=bool, ... }
-- @return table The files table (possibly modified by modules)
function M.drag_enter(files, state)
  -- Reset all module active flags to true
  for _, entry in ipairs(modules) do
    entry.active = true
  end

  -- Call drag_enter on each module
  for _, entry in ipairs(modules) do
    if entry.active and entry.module and entry.module.drag_enter then
      local ok, result = pcall(entry.module.drag_enter, files, state)
      if not ok then
        debug_print("error in " .. entry.name .. ".drag_enter: " .. tostring(result))
        entry.active = false
      elseif result == false then
        entry.active = false
      end
    end
  end

  return files
end

--- Call drag_leave hook on all active modules in priority order.
-- If a handler throws an error, it is caught and logged.
function M.drag_leave()
  for _, entry in ipairs(modules) do
    if entry.active and entry.module and entry.module.drag_leave then
      local ok, err = pcall(entry.module.drag_leave)
      if not ok then
        debug_print("error in " .. entry.name .. ".drag_leave: " .. tostring(err))
      end
    end
  end
end

--- Call drop hook on all active modules in priority order.
-- If a handler throws an error, it is caught and logged, but processing continues.
-- @param files table File list with format { {filepath="...", mimetype="...", temporary=bool}, ... }
-- @param state table Key state with format { control=bool, shift=bool, alt=bool, ... }
-- @return table The files table (possibly modified by modules)
function M.drop(files, state)
  for _, entry in ipairs(modules) do
    if entry.active and entry.module and entry.module.drop then
      local ok, err = pcall(entry.module.drop, files, state)
      if not ok then
        debug_print("error in " .. entry.name .. ".drop: " .. tostring(err))
      end
    end
  end

  return files
end

--- Convert EXO files to object format.
-- Calls the exo module's process_file_list function to convert AviUtl1 .exo files
-- to AviUtl2 .object files.
-- If an error occurs, it is caught and logged, and the original files are returned.
-- @param files table File list with format { {filepath="...", mimetype="...", temporary=bool}, ... }
-- @return table The files table (with .exo files converted to .object files)
function M.exo_convert(files)
  local ok, result = pcall(function()
    return require("exo").process_file_list(files)
  end)
  if not ok then
    debug_print("error in exo_convert: " .. tostring(result))
    return files
  end
  return result
end

return M
