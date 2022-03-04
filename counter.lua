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
  return true
end

return vtable
