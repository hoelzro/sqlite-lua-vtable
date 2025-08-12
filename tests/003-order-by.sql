.load ./lua-vtable

select lua_create_module_from_file('tests/examples/counter10.lua');
CREATE VIRTUAL TABLE counter USING counter10();
SELECT * FROM counter ORDER BY value DESC;
