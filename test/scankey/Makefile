CFLAGS := -O0 -g -std=c17 -fpic -Wmissing-declarations -Wall -Wextra -MMD 

all: driver

driver: scankey.c
	$(CC) $(CFLAGS) -o $@ scankey.c

test: all
	bash run.sh

-include driver.d

clean:
	rm -f *.o *.d driver

distclean: clean

format:
	clang-format -i *.[ch]

.PHONY: all clean distclean format test
