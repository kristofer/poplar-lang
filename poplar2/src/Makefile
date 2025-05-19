# Makefile for Poplar2 language

CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99 -I.

# Default target
all: test_value poplar2

# Object files for test_value
TEST_OBJS = value.o test_value.o

# Object files for main VM
VM_OBJS = value.o object.o vm.o interpreter.o gc.o som_parser.o

# Test targets
test_value: $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJS)

# Main VM target
poplar2: $(VM_OBJS)
	$(CC) $(CFLAGS) -o $@ $(VM_OBJS)

# Object file compilation rules
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencies
value.o: value.c value.h
test_value.o: test_value.c value.h
object.o: object.c object.h value.h vm.h
vm.o: vm.c vm.h value.h object.h interpreter.h gc.h som_parser.h
interpreter.o: interpreter.c interpreter.h vm.h value.h object.h
gc.o: gc.c gc.h vm.h value.h object.h
som_parser.o: som_parser.c som_parser.h vm.h value.h object.h

# Clean target
clean:
	rm -f *.o test_value poplar2

# Run tests
test: test_value
	./test_value

# Run SOM example
run_hello: poplar2
	./poplar2 ../examples/Hello.som

.PHONY: all clean test run_hello