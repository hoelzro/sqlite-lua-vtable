.load ./lua-vtable
.load ./test-helper

select lua_create_module_from_file('tests/examples/metatable_tester.lua');

CREATE VIRTUAL TABLE simple_test USING metatable_tester('
return {
  __fromsqlite = function(value)
    return { data = value }
  end,
  __tosqlite = function(value)
    return value.data
  end,
}
');

-- verify that we're getting the structured data out of the virtual table
SELECT data FROM simple_test;

-- …and that its subtype is correct.
SELECT subtype(data) FROM simple_test;

-- verify that we can pass the structured data into a function overridden by the
-- virtual table, and that we get the structured data out
SELECT test_fn(data) FROM simple_test;

-- …and that the subtype of *that* is also correct.
SELECT subtype(test_fn(data)) FROM simple_test;
