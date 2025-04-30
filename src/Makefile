HFILES = tomlc17.h
CFILES = tomlc17.c
OBJ = $(CFILES:.c=.o)

CFLAGS = -std=c17 -fpic -Wmissing-declarations -Wall -Wextra -MMD 
LIB_VERSION = 1.0
LIB = libtomlc17.a
LIB_SHARED = libtomlc17.so.$(LIB_VERSION)
EXEC = scankey scanvalue parser 

ifdef DEBUG
    CFLAGS += -O0 -g
else
    CFLAGS += -O3 -DNDEBUG
endif

all: $(EXEC) $(LIB) $(LIB_SHARED) 

*.o: $(HFILES)

libtomlc17.a: tomlc17.o
	ar -rcs $@ $^

$(LIB_SHARED): tomlc17.o
	$(CC) -shared -o $@ $^

-include $(OBJ:%.o=%.d) $(EXEC:%=%.d)

clean:
	rm -f *.o *.d *.a *.so.* $(EXEC)

distclean: clean

format:
	clang-format -i *.[ch]


.PHONY: all clean distclean format

