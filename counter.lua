local vtable = {
  name = 'counter',
}

local counter_cursor_methods = {}

function counter_cursor_methods:close()
end

function counter_cursor_methods:rowid()
end

function counter_cursor_methods:column(n)
end

function counter_cursor_methods:next()
end

function counter_cursor_methods:eof()
end

function counter_cursor_methods:filter(index_num, index_str, args)
end

local counter_vtable_methods = {}

function counter_vtable_methods:disconnect()
end

function counter_vtable_methods:destroy()
end

function counter_vtable_methods:open()
end

function counter_vtable_methods.update(args, rowid)
end

function counter_vtable_methods:begin()
end

function counter_vtable_methods:sync()
end

function counter_vtable_methods:commit()
end

function counter_vtable_methods:rollback()
end

function counter_vtable_methods:rename(new_name)
end

function counter_vtable_methods:find_function(args, name)
end

-- XXX args.module_name, args.database_name, args.table_name
function vtable.create(db, args)
  db:declare_vtab 'CREATE TABLE _ (value INTEGER NOT NULL)'

  return setmetatable({}, {__index = counter_vtable_methods})
end

vtable.connect = vtable.create

function vtable.disconnect(vtab)
end

function vtable.best_index(vtab, info)
  return {
    constraint_usage = {},
  }
end

function vtable.open(vtab)
  return {
    n = 1,
    max = 10,
  }
end

function vtable.filter(cursor, index_num, index_str, args)
  print 'in filter'
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

function vtable.close(cursor)
  print 'in close'
end

function vtable.rowid(cursor)
  return cursor.n
end

function vtable.update(vtab, args)
  return args[2]
end

function vtable.begin()
end

function vtable.commit()
end

function vtable.rollback()
end

function vtable.sync()
end

function vtable.find_function(vtab, argc, name)
  print(string.format('in find_function (argc = %d, name = %s)', argc, name))
  return function(lhs, rhs)
    local op
    if lhs == 'odd' then
      op = 1
    elseif lhs == 'even' then
      op = 0
    end
    return (rhs % 2) == op
  end
end

return vtable
