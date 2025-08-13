local function noop() end

local vtable = {
  name = 'updater',

  disconnect = noop,
  destroy = noop,
  close = noop,
}

function vtable.create(db, args)
  db:declare_vtab 'CREATE TABLE _ (value INTEGER NOT NULL)'

  return {}
end

vtable.connect = vtable.create

function vtable.best_index()
  return {
    constraint_usage = {},
  }
end

function vtable.open(vtab)
  return {
    vtab = vtab,
  }
end

function vtable.filter(cursor)
  cursor.n = 0
  vtable.next(cursor)
end

function vtable.eof(cursor)
  return cursor.n > #cursor.vtab
end

local TOMBSTONE = {}

function vtable.column(cursor, n)
  assert(cursor.vtab[cursor.n] ~= TOMBSTONE)
  return cursor.vtab[cursor.n]
end

function vtable.next(cursor)
  cursor.n = cursor.n + 1
  while cursor.n <= #cursor.vtab and cursor.vtab[cursor.n] == TOMBSTONE do
    cursor.n = cursor.n + 1
  end
end

function vtable.rowid(cursor)
  return cursor.n
end

function vtable.update(vtab, args)
  if #args == 1 then
    -- DELETE
    vtab[args[1]] = TOMBSTONE
  elseif args[1] ~= nil then
    -- UPDATE
    assert(args[1] == args[2])

    vtab[args[1]] = args[3]
    return args[1]
  else
    -- INSERT
    assert(args[2] == nil)

    vtab[#vtab + 1] = args[3]
    return #vtab
  end
end

return vtable
