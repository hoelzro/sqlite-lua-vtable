.PHONY: default test

CFLAGS+=-fPIC -Wunused -Werror
LIBS+=-llua -lsqlite3

default: lua-vtable.so

%.so: %.o
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^

test:
	./run-tests.sh

format:
	clang-format -i lua-vtable.c
	stylua counter.lua tests/examples/*.lua

clean:
	rm -f *.o *.so
