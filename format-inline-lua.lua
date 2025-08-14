#!/usr/bin/env lua

local function read_file(path)
  local file = assert(io.open(path, 'r'))
  local content = file:read '*a'
  file:close()
  return content
end

local function write_file(path, content)
  local file = assert(io.open(path, 'w'))
  file:write(content)
  file:close()
end

local function write_temp_file(content)
  local temp_file = os.tmpname()
  write_file(temp_file, content)
  return setmetatable({ path = temp_file }, {
    __close = function(self)
      os.remove(self.path)
    end,
  })
end

local function run_stylua(lua_code)
  local temp_file <close> = write_temp_file(lua_code)
  assert(os.execute('stylua --config-path .stylua.toml --quote-style ForceDouble ' .. temp_file.path))

  local formatted = read_file(temp_file.path)

  return formatted:gsub('\n$', '') -- Remove trailing newline
end

local function extract_and_format_lua(sql_content)
  local result = sql_content

  local pattern = "(CREATE VIRTUAL TABLE %w+ USING metatable_tester%('%s*)(.-)(%s*'%)%;)"

  result = result:gsub(pattern, function(prefix, lua_code, suffix)
    local formatted_lua = run_stylua(lua_code)
    return prefix .. formatted_lua .. suffix
  end)

  return result
end

local function main()
  local filename = arg[1]
  if not filename then
    print 'Usage: lua format_inline_lua.lua <sql_file>'
    os.exit(1)
  end

  local sql_content = read_file(filename)
  local formatted_content = extract_and_format_lua(sql_content)
  write_file(filename, formatted_content)
end

main()
