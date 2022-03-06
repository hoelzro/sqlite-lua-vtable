.load ./lua-vtable
.testcase tests/006-eponymous-only

select lua_create_module_from_file('tests/examples/eponymous_only.lua');
SELECT * FROM counter WHERE value > 5;
