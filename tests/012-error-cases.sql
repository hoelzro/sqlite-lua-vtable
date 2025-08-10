.load ./lua-vtable

select lua_create_module_from_file('tests/examples/error_tester.lua');

-- __fromsqlite returns wrong type (string instead of table/userdata)
CREATE VIRTUAL TABLE test1 USING error_tester('return {__fromsqlite = function(value) return "wrong type" end, __tosqlite = function(value) return value.data end}');
SELECT trim(data) FROM test1;

-- __fromsqlite returns nil
CREATE VIRTUAL TABLE test2 USING error_tester('return {__fromsqlite = function(value) return nil end, __tosqlite = function(value) return value.data end}');
SELECT trim(data) FROM test2;

-- __tosqlite throws error
CREATE VIRTUAL TABLE test3 USING error_tester('return {__fromsqlite = function(value) return {data = value} end, __tosqlite = function(value) error "tosqlite error" end}');
SELECT trim(data) FROM test3;

-- Missing __fromsqlite field
CREATE VIRTUAL TABLE test4 USING error_tester('return {__tosqlite = function(value) return value.data end}');
SELECT trim(data) FROM test4;

-- __fromsqlite not a function
CREATE VIRTUAL TABLE test5 USING error_tester('return {__fromsqlite = "not a function", __tosqlite = function(value) return value.data end}');
SELECT trim(data) FROM test5;

-- Missing __tosqlite field
CREATE VIRTUAL TABLE test6 USING error_tester('return {__fromsqlite = function(value) return {data = value} end}');
SELECT trim(data) FROM test6;

-- __tosqlite not a function
CREATE VIRTUAL TABLE test7 USING error_tester('return {__fromsqlite = function(value) return {data = value} end, __tosqlite = "not a function"}');
SELECT trim(data) FROM test7;

-- __tosqlite returns invalid type (function)
CREATE VIRTUAL TABLE test8 USING error_tester('return {__fromsqlite = function(value) return {data = value} end, __tosqlite = function(value) return function() end end}');
SELECT trim(data) FROM test8;

-- __fromsqlite propagates error
CREATE VIRTUAL TABLE test9 USING error_tester('return {__fromsqlite = function(value) error "fromsqlite error" end, __tosqlite = function(value) return value.data end}');
SELECT trim(data) FROM test9;
