CFLAGS = -std=c17 -fpic -Wmissing-declarations -Wall -Wextra -MMD 
#EXEC = simple simplecpp
EXEC = simple 

ifdef DEBUG
    CFLAGS += -O0 -g
else
    CFLAGS += -O3 -DNDEBUG
endif

CXXFLAGS = $(subst -std=c17,-std=c++20,$(CFLAGS))

all: $(EXEC)

simple: simple.c
	$(CC) $(CFLAGS) -o $@ $@.c -L ../src -l tomlc17

simplecpp: simplecpp.cpp
	$(CXX) $(CXXFLAGS) -o $@ $@.cpp -L ../src -l tomlc17

-include simple.d simplecpp.d

test:

clean:
	rm -f *.o *.d *.a $(EXEC)

distclean: clean

format:
	clang-format -i *.[ch] *.cpp

.PHONY: all clean distclean format

