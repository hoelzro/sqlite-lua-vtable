.load ./lua-vtable

select lua_create_module_from_file('tests/examples/metatable_tester.lua');

-- Basic successful case: serialize and deserialize simple data
CREATE VIRTUAL TABLE simple_test USING metatable_tester('
return {
  __fromsqlite = function(value)
    return { data = value, type = "simple" }
  end,
  __tosqlite = function(value)
    return value.data
  end,
}
');
SELECT trim(data) FROM simple_test;
