.load ./lua-vtable

select lua_create_module_from_file('tests/examples/sparse_constraint_usage.lua');
CREATE VIRTUAL TABLE sparse USING sparse_constraint_usage();
-- Two constraints so best_index gets nConstraint=2 and returns a sparse table
SELECT * FROM sparse WHERE value > 'a' AND value < 'z';
