CC = gcc

CPPFLAGS = -Iinclude

# ---------------------------------
# Build mode
# Usage:
#   make json_parser
#   make json_parser MODE=debug
# ---------------------------------

MODE ?= release

ifeq ($(MODE),debug)
	CFLAGS = -Wall -Wextra -std=c11 -march=native -g -O0
else
	CFLAGS = -Wall -Wextra -std=c11 -march=native -O3 -DNDEBUG
endif

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

.PHONY: all clean rebuild asm \
	json_parser radix_sort vm_stack

# ---------------------------------
# Default target
# ---------------------------------

all: json_parser

# ---------------------------------
# Examples
# ---------------------------------

json_parser: $(OBJ) examples/json_parser/json_parser.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(OBJ) examples/json_parser/json_parser.c -o $@

radix_sort: $(OBJ) examples/radix_sort.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(OBJ) examples/radix_sort.c -o $@


src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# ---------------------------------
# Assembly
# Usage:
#   make asm FILE=hashmap
# ---------------------------------

asm:
	$(CC) $(CPPFLAGS) $(CFLAGS) -S src/$(FILE).c -o $(FILE).s


clean:
	rm -f $(OBJ) json_parser radix_sort vm_stack *.s


rebuild: clean all