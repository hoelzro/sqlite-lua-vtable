.load ./lua-vtable

select lua_create_module_from_file('tests/examples/metatable_tester.lua');

-- __fromsqlite returns wrong type (string instead of table/userdata)
CREATE VIRTUAL TABLE test1 USING metatable_tester('
return {
  __fromsqlite = function(value)
    return "wrong type"
  end,
  __tosqlite = function(value)
    return value.data
  end,
}
');
SELECT trim(data) FROM test1;

-- __fromsqlite returns nil
CREATE VIRTUAL TABLE test2 USING metatable_tester('
return {
  __fromsqlite = function(value)
    return nil
  end,
  __tosqlite = function(value)
    return value.data
  end,
}
');
SELECT trim(data) FROM test2;

-- __tosqlite throws error
CREATE VIRTUAL TABLE test3 USING metatable_tester('
return {
  __fromsqlite = function(value)
    return { data = value }
  end,
  __tosqlite = function(value)
    error "tosqlite error"
  end,
}
');
SELECT trim(data) FROM test3;

-- Missing __fromsqlite field
CREATE VIRTUAL TABLE test4 USING metatable_tester('
return {
  __tosqlite = function(value)
    return value.data
  end,
}
');
SELECT trim(data) FROM test4;

-- __fromsqlite not a function
CREATE VIRTUAL TABLE test5 USING metatable_tester('
return {
  __fromsqlite = "not a function",
  __tosqlite = function(value)
    return value.data
  end,
}
');
SELECT trim(data) FROM test5;

-- Missing __tosqlite field
CREATE VIRTUAL TABLE test6 USING metatable_tester('
return {
  __fromsqlite = function(value)
    return { data = value }
  end,
}
');
SELECT trim(data) FROM test6;

-- __tosqlite not a function
CREATE VIRTUAL TABLE test7 USING metatable_tester('
return {
  __fromsqlite = function(value)
    return { data = value }
  end,
  __tosqlite = "not a function",
}
');
SELECT trim(data) FROM test7;

-- __tosqlite returns invalid type (function)
CREATE VIRTUAL TABLE test8 USING metatable_tester('
return {
  __fromsqlite = function(value)
    return { data = value }
  end,
  __tosqlite = function(value)
    return function() end
  end,
}
');
SELECT trim(data) FROM test8;

-- __fromsqlite propagates error
CREATE VIRTUAL TABLE test9 USING metatable_tester('
return {
  __fromsqlite = function(value)
    error "fromsqlite error"
  end,
  __tosqlite = function(value)
    return value.data
  end,
}
');
SELECT trim(data) FROM test9;

-- __tosqlite returns unsupported type (thread)
CREATE VIRTUAL TABLE test10 USING metatable_tester('
return {
  __fromsqlite = function(value)
    return { data = value }
  end,
  __tosqlite = function(value)
    return coroutine.create(function() end)
  end,
}
');
SELECT trim(data) FROM test10;

-- No metatable (table without metatable)
CREATE VIRTUAL TABLE test11 USING metatable_tester('
return {
  mode = "no_metatable",
}
');
SELECT trim(data) FROM test11;

-- Unmapped metatable (metatable not registered with subtype mapping)
CREATE VIRTUAL TABLE test12 USING metatable_tester('
return {
  mode = "unmapped",
  __tosqlite = function(value)
    return value.data
  end,
  __fromsqlite = function(value)
    return { data = value }
  end,
}
');
SELECT trim(data) FROM test12;
