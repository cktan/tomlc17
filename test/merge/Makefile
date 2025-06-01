CFLAGS := -O0 -g -std=c17 -fpic -Wmissing-declarations -Wall -Wextra -MMD

EXEC = test1

all: $(EXEC)

test1: test1.c
	$(CC) $(CFLAGS) -o $@ $@.c

test: all
	@echo
	@echo =========================
	@echo == merge test
	@echo =========================
	./test1

-include test1.d

clean:
	rm -f *.o *.d $(EXEC)

distclean: clean

format:
	clang-format -i *.[ch]

.PHONY: all clean distclean format test
