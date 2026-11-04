prefix ?= /usr/local
# remove trailing /
override prefix := $(prefix:%/=%)

HFILES := toml.h
CFILES := toml.c
OBJ := $(CFILES:.c=.o)

# set up LIB_SHARED
LIB_VERSION := 1.0
LIB := libtoml.a
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    LIB_SHARED =
else ifeq ($(OS), Windows_NT)
    LIB_SHARED =
else
    LIB_SHARED = libtoml.so.$(LIB_VERSION)
endif

CFLAGS := -std=c17 -Wmissing-declarations -Wall -Wextra -MMD

ifdef DEBUG
    CFLAGS += -O0 -g -fsanitize=address,undefined
else
    CFLAGS += -O3 -DNDEBUG
endif
# Add -fpic only for non-Windows platforms
ifneq ($(OS), Windows_NT)
    CFLAGS += -fpic
endif


all: $(LIB) $(LIB_SHARED)

*.o: $(HFILES)

libtoml.a: toml.o
	ar -rcs $@ $^

$(LIB_SHARED): toml.o
	$(CC) -shared -o $@ $^

-include $(OBJ:%.o=%.d)

install: all
	install -d $(DESTDIR)${prefix}/include
	install -d $(DESTDIR)${prefix}/lib
	install -m 0644 toml.h $(DESTDIR)${prefix}/include/
	install -m 0644 $(LIB) $(DESTDIR)${prefix}/lib/
ifneq ($(LIB_SHARED),)
	install -m 0644 $(LIB_SHARED) $(DESTDIR)${prefix}/lib/
endif

test: all
	$(MAKE) -C test test

clean:
	rm -f *.o *.d *.a *.so.*
	$(MAKE) -C test clean

distclean: clean

# toml.h is tomlc99's header, kept verbatim.
format:
	clang-format -i toml.c
	$(MAKE) -C test format


.PHONY: all install test clean distclean format
