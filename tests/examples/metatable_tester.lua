local function noop() end

local function parse_sqlite_string(str)
  assert(str:sub(1, 1) == "'" and str:sub(-1, -1) == "'")

  return str:sub(2, -2):gsub("''", "'")
end

local next_subtype_id = 1

local vtable = {
  name = 'metatable_tester',
  disconnect = noop,
  destroy = noop,
  filter = noop,
  close = noop,
}

function vtable.create(db, args)
  local metatable_code = parse_sqlite_string(assert(args[4]))

  db:declare_vtab 'CREATE TABLE _ (id INTEGER, data TEXT)'

  local chunk = assert(load(metatable_code))
  local metatable = assert(chunk())

  if metatable.mode ~= 'unmapped' then
    register_metatable_subtype_mapping(metatable, next_subtype_id)
    next_subtype_id = next_subtype_id + 1
  end

  return { metatable = metatable }
end

function vtable.connect(db, args)
  return vtable.create(db, args)
end

function vtable.best_index()
  return { constraint_usage = {} }
end

function vtable.open(vtab)
  return { n = 1, max = 1, metatable = vtab.metatable }
end

function vtable.eof(cursor)
  return cursor.n > cursor.max
end

function vtable.column(cursor, n)
  if n == 0 then
    return cursor.n
  else
    local data = { data = 'test_data_' .. cursor.n }

    if cursor.metatable.mode == 'no_metatable' then
      return data
    elseif cursor.metatable.mode == 'unmapped' then
      return setmetatable(data, cursor.metatable)
    else
      return setmetatable(data, cursor.metatable)
    end
  end
end

function vtable.next(cursor)
  cursor.n = cursor.n + 1
end

function vtable.rowid(cursor)
  return cursor.n
end

function vtable.find_function(vtab, argc, name)
  if name == 'trim' and argc == 1 then
    return function(value)
      -- XXX check metatable or something
      return value.data
    end
  elseif name == 'test_fn' and argc == 1 then
    return function(value)
      local mt = assert(getmetatable(value))

      return setmetatable({
        data = value.data:gsub('^test_', 'transformed_'),
      }, mt)
    end
  end
end

return vtable
