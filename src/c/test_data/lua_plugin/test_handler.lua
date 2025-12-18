-- Test handler module for file loading test
return {
  name = "Test Handler",
  priority = 300,
  drag_enter = function(files, state) return true end,
  drop = function(files, state) end
}