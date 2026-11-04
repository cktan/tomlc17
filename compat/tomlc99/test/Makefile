CFLAGS := -O0 -g -fsanitize=address,undefined -std=c17 -Wmissing-declarations -Wall -Wextra -MMD -I..

EXEC = toml_cat unittest

all: $(EXEC)

# toml_cat.c is tomlc99's own program, kept verbatim. Building it
# against the compat layer checks that tomlc99 code compiles unchanged.
toml_cat: toml_cat.c ../toml.c
	$(CC) $(CFLAGS) -Wno-missing-declarations -o $@ toml_cat.c ../toml.c

unittest: unittest.c ../toml.c
	$(CC) $(CFLAGS) -o $@ unittest.c ../toml.c

test: all
	@echo
	@echo =========================
	@echo == tomlc99 compat test
	@echo =========================
	./unittest
	bash run.sh

-include $(EXEC:%=%.d)

clean:
	rm -f *.o *.d $(EXEC) stdex/*.out

distclean: clean

format:
	clang-format -i unittest.c


.PHONY: all clean distclean format test
