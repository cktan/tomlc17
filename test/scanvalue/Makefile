CFLAGS := -O0 -g -fsanitize=address,undefined -std=c17 -Wmissing-declarations -Wall -Wextra -MMD 

all: driver

driver: scanvalue.c
	$(CC) $(CFLAGS) -o $@ scanvalue.c -lm

test: all
	bash run.sh

-include driver.d

clean:
	rm -f *.o *.d driver

distclean: clean

format:
	clang-format -i *.[ch]

.PHONY: all clean distclean format test
