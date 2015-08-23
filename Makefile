CFLAGS = -std=c11 -Werror -Wall -Wextra

HEADERS = $(wildcard *.h)
OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))

all: libkiss.a

libkiss.a: $(OBJECTS)
	ar cr $@ $^

%.o: %.c $(HEADERS)

clean:
	rm *.o *.a

.PHONY: all clean