.PHONY: default

CFLAGS+=-fPIC
LIBS+=-llua

default: lua-vtable.so

%.so: %.o
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f *.o *.so
