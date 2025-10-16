CFLAGS = -std=c17 -fpic -Wmissing-declarations -Wall -Wextra -MMD
EXEC = test1

ifdef DEBUG
    CFLAGS += -O0 -g
else
    CFLAGS += -O3 -DNDEBUG
endif

CXXFLAGS = $(subst -std=c17,-std=c++20,$(CFLAGS))

# Test for C++20 support
CXX20_SUPPORT := $(shell echo "int main(){}" | $(CXX) -std=c++20 -x c++ -c - -o /dev/null 2>/dev/null && echo yes || echo no)

ifeq ($(CXX20_SUPPORT),yes)

all: $(EXEC)

test1: test1.cpp
	$(CXX) $(CXXFLAGS) -o $@ $@.cpp -L../../src -ltomlc17

test: all
	@echo
	@echo =========================
	@echo == cpp test
	@echo =========================
	./test1

else

all:
	@echo "Skipped because C++20 not available"

test: 
	@echo "Skipped because C++20 not available"

endif

-include test1.d

clean:
	rm -f *.o *.d $(EXEC)

distclean: clean

format:
	clang-format -i *.cpp

.PHONY: all clean distclean format test
