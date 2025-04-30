CFLAGS = -std=c17 -fpic -Wmissing-declarations -Wall -Wextra -MMD 
EXEC = simple

ifdef DEBUG
    CFLAGS += -O0 -g
else
    CFLAGS += -O3 -DNDEBUG
endif

all: simple

simple: simple.c
	$(CC) $(CFLAGS) -o $@ $@.c -L ../src -l tomlc17

clean:
	rm -f *.o *.d *.a $(EXEC)

distclean: clean

format:
	clang-format -i *.[ch]

.PHONY: all clean distclean format

