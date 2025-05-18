# ----------------------------
# Makefile for Poplar VM Tests - Hexadecimal ASCII Format
# ----------------------------

CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -g

.PHONY: all clean run

all: gen_test bin2hex

gen_test: src/gen_test.c
	$(CC) $(CFLAGS) -o gen_test src/gen_test.c

bin2hex: src/bin2hex.c
	$(CC) $(CFLAGS) -o bin2hex src/bin2hex.c

run: gen_test
	./gen_test
	@echo "Generated test files in hexadecimal ASCII format"
	@echo "You can open these .ppx files in any text editor"

clean:
	rm -f gen_test bin2hex *.ppx

# Run a specific test with the VM
test_simple: gen_test
	./gen_test
	cd .. && $(MAKE) -C vm
	../vm/poplarvm simple_math.ppx

test_hello: gen_test
	./gen_test
	cd .. && $(MAKE) -C vm
	../vm/poplarvm hello_world.ppx

test_countdown: gen_test
	./gen_test
	cd .. && $(MAKE) -C vm
	../vm/poplarvm countdown.ppx

# View bytecode files (useful for debugging)
view_simple:
	@echo "Contents of simple_math.ppx:"
	@cat simple_math.ppx

view_hello:
	@echo "Contents of hello_world.ppx:"
	@cat hello_world.ppx

view_countdown:
	@echo "Contents of countdown.ppx:"
	@cat countdown.ppx

# Convert binary files to hex ASCII format
convert:
	@echo "Usage: ./bin2hex <input_binary_file> <output_hex_file>"
	@echo "Example: ./bin2hex binary.ppx hex_output.ppx"
