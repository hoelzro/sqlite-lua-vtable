.PHONY: default

CFLAGS+=-fPIC

default: lua-vtable.so

%.so: %.o
	$(CC) $(CFLAGS) -shared -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f *.o *.so
