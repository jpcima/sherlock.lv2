CC ?= gcc
C_FLAGS ?= -I../ -Wall -Wextra -Wpedantic $(shell pkg-config --cflags lv2) \
					 -fprofile-arcs -ftest-coverage

all: ser_atom_test

ser_atom_test: ser_atom_test.c ../ser_atom.lv2/ser_atom.h
	$(CC) -std=c11 -g -o $@ $< $(C_FLAGS)
	valgrind ./$@
	gcov $<

clean:
	rm -f ser_atom_test *.gcov *.gc*
