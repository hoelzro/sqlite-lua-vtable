local function noop() end

local vtable = {
  name = 'sparse_constraint_usage',

  disconnect = noop,
  destroy = noop,
  close = noop,
}

function vtable.create(db, args)
  db:declare_vtab 'CREATE TABLE _ (value TEXT NOT NULL)'

  return {}
end

vtable.connect = vtable.create

function vtable.best_index(vtab, index_info)
  -- When called with 2+ constraints, return a constraint_usage table with
  -- an explicit nil hole at index 1 and a real entry at index 2 that
  -- requests the second constraint's value as argv[1].  This exercises the
  -- nil-skip path in pop_index_info — if the nil entry isn't skipped
  -- properly, the argv_index on the second entry won't be read, and filter
  -- won't receive the constraint value.
  local n = #index_info.constraints
  if n >= 2 then
    return {
      constraint_usage = table.pack(nil, { argv_index = 1 }),
    }
  end

  return {
    constraint_usage = {},
  }
end

function vtable.open()
  return {}
end

function vtable.filter(cursor, idx_num, idx_str, argv)
  -- Report whether filter received the constraint value.  If the sparse
  -- constraint_usage was handled correctly, argv[1] will be present.
  if argv[1] then
    cursor.result = 'constraint_value_received'
  else
    cursor.result = 'constraint_value_missing'
  end
  cursor.done = false
end

function vtable.eof(cursor)
  return cursor.done
end

function vtable.column(cursor, n)
  return cursor.result
end

function vtable.next(cursor)
  cursor.done = true
end

function vtable.rowid(cursor)
  return 1
end

return vtable
