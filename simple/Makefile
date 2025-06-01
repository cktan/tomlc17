CFLAGS = -std=c17 -fpic -Wmissing-declarations -Wall -Wextra -MMD 
EXEC = simple simplecpp

ifdef DEBUG
    CFLAGS += -O0 -g
else
    CFLAGS += -O3 -DNDEBUG
endif

CXXFLAGS = $(subst -std=c17,-std=c++20,$(CFLAGS))

# Check if CXX is available
HAVE_CXX := $(shell command -v $(CXX) 2> /dev/null)


all: $(EXEC)

simple: simple.c
	$(CC) $(CFLAGS) -o $@ $@.c -L../src -ltomlc17

simplecpp: simplecpp.cpp
ifndef HAVE_CXX
	@echo "INFO: skipping $@ because no $(CXX) compiler found"
else
	$(CXX) $(CXXFLAGS) -o $@ $@.cpp -L ../src -l tomlc17
endif

-include simple.d simplecpp.d

test:

clean:
	rm -f *.o *.d *.a $(EXEC)

distclean: clean

format:
	clang-format -i *.[ch] *.cpp

.PHONY: all clean distclean format

