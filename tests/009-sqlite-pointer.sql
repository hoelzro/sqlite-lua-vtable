.load ./lua-vtable

CREATE TABLE example (
    value INTEGER NOT NULL
);

INSERT INTO example SELECT value FROM generate_series(1, 10);

SELECT lua_create_module_from_file('tests/examples/odd_filter.lua');
CREATE VIRTUAL TABLE odd_example USING odd_filter(example);
SELECT * FROM odd_example;
