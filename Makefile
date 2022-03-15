.PHONY: default test

CFLAGS+=-fPIC
LIBS+=-llua -lsqlite3

default: lua-vtable.so

%.so: %.o
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^

test:
	for sql in tests/*.sql; do                                                        \
		sqlite3 :memory: ".read $$sql" || exit 1                                ; \
		diff -q testcase-out.txt tests/$$(basename $$sql .sql).output || exit 1 ; \
	done

clean:
	rm -f *.o *.so
