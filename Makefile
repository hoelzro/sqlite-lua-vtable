.PHONY: default test

CFLAGS+=-fPIC -Wunused -Werror
LIBS+=-llua -lsqlite3

default: lua-vtable.so

%.so: %.o
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^

test:
	for sql in tests/*.sql; do \
		sqlite3 :memory: < $$sql > testcase-out.txt || exit 1                ; \
		diff -q testcase-out.txt tests/$$(basename $$sql .sql).output || exit 1 ; \
	done; \
	rm -f testcase-out.txt

format:
	clang-format -i lua-vtable.c
	stylua counter.lua tests/examples/*.lua

clean:
	rm -f *.o *.so testcase-out.txt
