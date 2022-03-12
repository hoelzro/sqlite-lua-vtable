.PHONY: default test

CFLAGS+=-fPIC
LIBS+=-llua -lsqlite3

default: lua-vtable.so

%.so: %.o
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^

test:
	for sql in tests/*.sql; do                                        \
		sqlite3 :memory: ".read $$sql"                          ; \
		diff -q testcase-out.txt tests/$$(basename $$sql .sql).output ; \
	done

clean:
	rm -f *.o *.so
