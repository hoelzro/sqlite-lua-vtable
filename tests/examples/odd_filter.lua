local sqlite = require 'lsqlite3'

local vtable = {
  name = 'odd_filter',

  disconnect = function() end,
  destroy = function() end,
}

function vtable.connect(db, args)
  local target_table = args[4]

  db:declare_vtab 'CREATE TABLE _ (value INTEGER NOT NULL)'

  return {
    db = db,
    target = target_table,
  }
end

vtable.create = vtable.connect

function vtable.best_index()
  return {
    constraint_usage = {},
  }
end

function vtable.open(vtab)
  -- XXX error check
  local stmt = vtab.db:prepare(string.format('SELECT rowid, value FROM %s WHERE value %% 2 <> 1', vtab.target))
  return {
    stmt = stmt,
    last_status = sqlite.ROW,
  }
end

function vtable.close(cursor)
  cursor.stmt:finalize()
end

function vtable.filter(cursor)
  return vtable.next(cursor)
end

function vtable.eof(cursor)
  return cursor.last_status ~= sqlite.ROW and cursor.last_status ~= sqlite.BUSY
end

function vtable.column(cursor, n)
  return cursor.stmt:get_value(n + 1)
end

function vtable.next(cursor)
  local status = cursor.stmt:step()
  cursor.last_status = status
  if status == sqlite.ERROR or status == sqlite.MISUSE then
    return nil, 'error stepping target table' -- XXX better error
  end
end

function vtable.rowid(cursor)
  local v = cursor.stmt:get_value(0)
  return v
end

return vtable
