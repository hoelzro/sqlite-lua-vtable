.load ./lua-vtable
.testcase tests/007-drop-table

select lua_create_module_from_file('tests/examples/counter10.lua');
CREATE VIRTUAL TABLE counter USING counter10();
DROP TABLE counter;
