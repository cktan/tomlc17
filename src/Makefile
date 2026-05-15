HFILES := tomlc17.h
CFILES := tomlc17.c
OBJ := $(CFILES:.c=.o)

CFLAGS := -std=c17 -fpic -Wmissing-declarations -Wall -Wextra -MMD
LIB_VERSION := 1.0
LIB := libtomlc17.a
LIB_SHARED = libtomlc17.so.$(LIB_VERSION)
EXEC =

TOML_GIT_TAG := $(shell git describe --exact-match --tags HEAD 2>/dev/null | grep -E '^R[0-9]{8}$$')
TOML_GIT_HASH := $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)

ifdef DEBUG
    CFLAGS += -O0 -g -fsanitize=address,undefined
else
    CFLAGS += -O3 -DNDEBUG
endif

ifneq ($(TOML_GIT_TAG),)
    CFLAGS += -DTOML_GIT_TAG='"$(TOML_GIT_TAG)"'
endif
CFLAGS += -DTOML_GIT_HASH='"$(TOML_GIT_HASH)"'

all: $(EXEC) $(LIB) $(LIB_SHARED) 

*.o: $(HFILES)

libtomlc17.a: tomlc17.o
	ar -rcs $@ $^

$(LIB_SHARED): tomlc17.o
	$(CC) -shared -o $@ $^

-include $(OBJ:%.o=%.d) $(EXEC:%=%.d)

test: all

clean:
	rm -f *.o *.d *.a *.so.* $(EXEC)

distclean: clean

format:
	clang-format -i *.[ch] *.hpp


.PHONY: all clean distclean format

