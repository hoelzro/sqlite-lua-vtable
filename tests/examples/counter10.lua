local function noop() end

local vtable = {
  name = 'counter10',

  disconnect = noop,
  destroy = noop,
  filter = noop,
  close = noop,
}

function vtable.create(db, args)
  db:declare_vtab 'CREATE TABLE _ (value INTEGER NOT NULL)'

  return {}
end

function vtable.connect(db, args)
  db:declare_vtab 'CREATE TABLE _ (value INTEGER NOT NULL)'

  return {}
end

function vtable.best_index()
  return {
    constraint_usage = {},
  }
end

function vtable.open()
  return {
    n = 1,
    max = 10,
  }
end

function vtable.eof(cursor)
  return cursor.n > cursor.max
end

function vtable.column(cursor, n)
  return cursor.n
end

function vtable.next(cursor)
  cursor.n = cursor.n + 1
end

function vtable.rowid(cursor)
  return cursor.n
end

function vtable.find_function(vtab, nargs, name)
  if name == 'match' and nargs == 2 then
    return function(a, b)
      return b % a == 0
    end
  end
end

return vtable
