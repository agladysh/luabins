local ensure_equals = function(msg, actual, expected)
  if actual ~= expected then
    error(
        msg..": actual `"..tostring(actual)
        .."` expected `"..tostring(expected).."'"
      )
  end
end

local function deepequals(lhs, rhs)
  if type(lhs) ~= "table" or type(rhs) ~= "table" then
    return lhs == rhs
  end

  local checked_keys = {}

  for k, v in pairs(lhs) do
    checked_keys[k] = true
    if not deepequals(v, rhs[k]) then
      return false
    end
  end

  for k, v in pairs(rhs) do
    if not checked_keys[k] then
      return false -- extra key
    end
  end

  return true
end

local nargs = function(...)
  return select("#", ...), ...
end

local eat_true = function(t, ...)
  assert(t, ...)
  return ...
end

-- Most basic tests
local luabins_local = require 'luabins'
assert(luabins_local == luabins)

assert(type(luabins.save) == "function")
assert(type(luabins.load) == "function")

local check_fn_ok = function(eq, ...)
  local expected = { nargs(...) }
  local saved = assert(luabins.save(...))

  assert(type(saved) == "string")

  print("saved (truncated to 70 chars)")
  print(
      (saved:gsub(
          "[^0-9A-Za-z_%-]",
          function(char) return ("%%%02X"):format(char:byte()) end
        ):sub(1, 70))
    )

  local loaded = { nargs(eat_true(luabins.load(saved))) }

  ensure_equals("num arguments match", expected[1], loaded[1])
  for i = 2, expected[1] do
    assert(eq(expected[i], loaded[i]))
  end
end

local check_ok = function(...)
  print("check_ok")
  return check_fn_ok(deepequals, ...)
end

local check_fail_save = function(msg, ...)
  print("check_fail_save")
  local res, err = luabins.save(...)
  ensure_equals("result", res, nil)
  ensure_equals("error message", err, msg)
end

local check_fail_load = function(msg, v)
  print("check_fail_load")
  local res, err = luabins.load(v)
  ensure_equals("result", res, nil)
  ensure_equals("error message", err, msg)
end

check_fail_load("corrupt data", "")
check_fail_load("corrupt data", "bad data")

check_ok(nil)
check_ok(true)
check_ok(false)
check_ok(42)
check_ok(math.pi)
check_ok(1/0)
check_ok(-1/0)

-- This is the way to detect NaN
check_fn_ok(function(lhs, rhs) return lhs ~= lhs and rhs ~= rhs end, 0/0)

check_ok("")
check_ok("Luabins")

check_ok("Embedded\0Zero")

check_ok(("longstring"):rep(1024000))

check_fail_save("can't save: unsupported type detected", function() end)
check_fail_save(
    "can't save: unsupported type detected",
    coroutine.create(function() end)
  )
check_fail_save("can't save: unsupported type detected", newproxy())

check_ok({ })
check_ok({ 1 })
check_ok({ a = 1 })
check_ok({ a = 1, 2, [42] = true })
check_ok({ { } })
check_ok({ a = {}, b = { c = 7 } })

check_ok(nil, nil)
check_ok(nil, false, true, 42, "Embedded\0Zero", { { [{3}] = 54 } })
check_ok({ a = {}, b = { c = 7 } }, nil, { { } }, 42)

check_fail_save(
    "can't save: unsupported type detected",
    { { function() end } }
  )

check_fail_save(
    "can't save: unsupported type detected",
    nil, false, true, 42, "Embedded\0Zero", function() end,
    { { [{3}] = 54 } }
  )

local t = {}; t[1] = t
check_fail_save("can't save: nesting is too deep", t)

-- TODO: Extensive Save Autocollapse testing
-- TODO: Load Mutation testing
-- TODO: Load Truncation testing
-- TODO: Look for existing sets of related test cases

print("OK")
