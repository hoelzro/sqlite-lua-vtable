.load ./lua-vtable

select lua_create_module_from_file('tests/examples/eponymous.lua');
CREATE VIRTUAL TABLE counter USING counter10();
SELECT * FROM counter WHERE value > 5;
