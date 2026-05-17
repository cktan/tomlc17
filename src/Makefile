HFILES := tomlc17.h
CFILES := tomlc17.c
OBJ := $(CFILES:.c=.o)

# set up LIB_SHARED
LIB_VERSION := 1.0
LIB := libtomlc17.a
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    LIB_SHARED =
else ifeq ($(OS), Windows_NT)
    LIB_SHARED =
else
    LIB_SHARED = libtomlc17.so.$(LIB_VERSION)
endif
EXEC =

# set up CFLAGS / CXXFLAGS
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

