# Makefile for Poplar2 language

CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99 -I.

# Default target
all: test_value

# Object files
OBJS = value.o test_value.o

# Test targets
test_value: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Object file compilation rules
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencies
value.o: value.c value.h
test_value.o: test_value.c value.h

# Clean target
clean:
	rm -f *.o test_value

# Run tests
test: test_value
	./test_value

.PHONY: all clean test