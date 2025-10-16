CFLAGS = -std=c17 -fpic -Wmissing-declarations -Wall -Wextra -MMD 
EXEC = simple simplecpp repro

ifdef DEBUG
    CFLAGS += -O0 -g
else
    CFLAGS += -O3 -DNDEBUG
endif

CXXFLAGS = $(subst -std=c17,-std=c++20,$(CFLAGS))

# Test if C++ is available
HAVE_CXX := $(shell command -v $(CXX) 2> /dev/null)

# Test for C++20 support
CXX20_SUPPORT := $(shell echo "int main(){}" | $(CXX) -std=c++20 -x c++ -c - -o /dev/null 2>/dev/null && echo yes || echo no)



all: $(EXEC)

simple: simple.c ../src/libtomlc17.a
	$(CC) $(CFLAGS) -o $@ $@.c -L../src -ltomlc17

repro: repro.c ../src/libtomlc17.a
	$(CC) $(CFLAGS) -o $@ $@.c -L../src -ltomlc17


simplecpp: simplecpp.cpp ../src/libtomlc17.a
ifndef HAVE_CXX
	@echo "INFO: skipping $@ because no $(CXX) compiler found"
else ifeq ($(CXX20_SUPPORT),no)
	@echo "INFO: skipping $@ because $(CXX) does not support C++20"
else
	$(CXX) $(CXXFLAGS) -o $@ $@.cpp -L ../src -l tomlc17
endif

-include simple.d simplecpp.d

test: all
	./simple > /dev/null
	[ ! -x ./simplecpp ] || ./simplecpp > /dev/null

clean:
	rm -f *.o *.d *.a $(EXEC)

distclean: clean

format:
	clang-format -i *.[ch] *.cpp

.PHONY: all clean distclean format

