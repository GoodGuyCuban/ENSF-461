.PHONY: all
all: memsym.out

memsym.out: memsym.c
	gcc -g -Wall -o $@ $^

clean:
	rm -f memsym.out

test:
	cd test_basic1 && python3 test_basic1.py