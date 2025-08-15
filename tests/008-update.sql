.load ./lua-vtable

select lua_create_module_from_file('tests/examples/update.lua');
CREATE VIRTUAL TABLE t USING updater;
INSERT INTO t VALUES (1), (1), (2), (3), (5), (8), (13);
SELECT * FROM t;
UPDATE t SET value = value + 1;
SELECT * FROM t;
DELETE FROM t WHERE value % 2 = 0;
SELECT * FROM t;
