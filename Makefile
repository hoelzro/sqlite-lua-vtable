.PHONY: default test

CFLAGS+=-fPIC -Wunused -Werror
LIBS+=-llua -lsqlite3

default: lua-vtable.so

%.so: %.o
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^

test: test-helper.so
	./run-tests.sh

format:
	clang-format -i lua-vtable.c test-helper.c
	stylua counter.lua format-inline-lua.lua tests/examples/*.lua
	./format-inline-lua.lua tests/012-error-cases.sql
	./format-inline-lua.lua tests/013-metatable-happy-path.sql

clean:
	rm -f *.o *.so
