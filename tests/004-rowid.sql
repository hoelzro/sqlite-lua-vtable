.load ./lua-vtable
.testcase tests/004-rowid

select lua_create_module_from_file('tests/examples/counter10.lua');
CREATE VIRTUAL TABLE counter USING counter10();
SELECT * FROM counter WHERE rowid > 5;
