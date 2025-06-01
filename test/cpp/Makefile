CFLAGS = -std=c17 -fpic -Wmissing-declarations -Wall -Wextra -MMD
EXEC = test1

ifdef DEBUG
    CFLAGS += -O0 -g
else
    CFLAGS += -O3 -DNDEBUG
endif

CXXFLAGS = $(subst -std=c17,-std=c++20,$(CFLAGS))

# Check if CXX is available
HAVE_CXX := $(shell command -v $(CXX) 2> /dev/null)

ifdef HAVE_CXX

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
	@echo "Skipped because C++ not available"

test: 
	@echo "Skipped because C++ not available"

endif

-include test1.d

clean:
	rm -f *.o *.d $(EXEC)

distclean: clean

format:
	clang-format -i *.[ch]

.PHONY: all clean distclean format test
