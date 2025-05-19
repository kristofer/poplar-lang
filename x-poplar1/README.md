# Poplar Language
Poplar is a very simple programming language designed for the Agon platform. It consists of:

1. A small compiler that translates Poplar source code (.pplr) into human-readable hexadecimal ASCII bytecode files (.ppx)
2. A virtual machine (VM) that loads and executes the bytecode files (.ppx) on the Agon

## Project Structure

- `/vm` - The Poplar virtual machine implementation
- `/test` - Test programs and utilities for testing the VM

## Data Types

The Poplar language supports the following data types:

- **Char**: A single byte
- **String**: An array of bytes with a 16-bit length (n:i16 bytes)
- **Int**: 16-bit integer (int-16)
- **Ptr**: 24-bit pointer (int-24) used for addressing memory

## VM Architecture

The virtual machine uses the following internal data structures:

- **vmStack**: Main stack for operations
- **vmTemp**: Temporary stack used for frame operations (256 bytes)
- **vmHeap**: Dynamic memory allocation area
- **outputBuffer** (outb): Buffer where code can place bytes to send to standard output (puts())
- **inChar**: Storage for the latest keyboard input character (inchar())

## Virtual Machine

The Poplar VM is implemented in C and designed to run efficiently on the Agon platform. See the [VM documentation](vm/README.md) for detailed information on building and using the VM.

## Instruction Set

The following bytecode instructions are supported by the virtual machine (shown with their hexadecimal opcode values):

### Stack Operations
- `PUSHN (n: i16)` [00]: Push a literal number onto the stack.

### Binary Operations
- `ADD` [01]: Pop two numbers off of the stack, and push their sum.
- `SUB` [02]: Pop two numbers off of the stack. Subtract the first from the second, and push the result.
- `MUL` [04]: Pop two numbers off of the stack, and push their product.
- `DIV` [05]: Pop two numbers off of the stack. Divide the second by the first, and push the result.
- `MOD` [06]: Pop two numbers off of the stack. Mod the second by the first and push the result.
- `SIGN` [07]: Pop a number off of the stack. If it is greater or equal to zero, push 1, otherwise push -1.

### Memory Management
- `ALLOCATE` [08]: Pop int off of the stack, and return a pointer to that number of free bytes on the heap.
- `FREE` [09]: Pop a pointer off of the stack. Pop number off of the stack, and free that many bytes at pointer location in memory.
- `STORE (size: i24)` [0C]: Pop a pointer off of the stack. Then, pop size bytes off of the stack. Store these bytes in reverse order at the memory pointer.
- `LOAD (size: i24)` [0D]: Pop a pointer off of the stack, and push size onto stack. Then, push size number of consecutive memory bytes onto the stack.

### Control Flow
- `BEGIN_WHILE` [0A]: Start a while loop. For each iteration, pop a number off of the stack. If the number is not zero, run the loop.
- `END_WHILE` [0B]: Mark the end of a while loop.
- `CALL (fn: i16)` [0E]: Call a user defined function by its compiler assigned ID.

### Stack Frame Operations
- `LOAD_FRAME_PTR` [0F]: Load the frame pointer of the current stack frame, which is always less than or equal to the stack pointer. Variables are stored relative to the frame pointer for each function.
- `MAKE_STACK_FRAME (arg_size: i8, local_scope_size: i8)` [10]: Create a new stack frame for function calls.
- `DROP_STACK_FRAME (return_size: i8, local_scope_size: i8)` [11]: Remove a stack frame after function execution completes.
- `POPSTR` [12]: Pop a pointer off of the stack. Then, pop size bytes off of the stack. Print to standard output the size number of bytes starting at the pointer location in memory.

## Usage

To build the VM:
```
cd vm
make
```

To run a Poplar bytecode program:
```
cd vm
./poplarvm path/to/program.ppx
```

## Bytecode Format

Poplar bytecode (.ppx) files use a human-readable hexadecimal ASCII format:

- Each byte is represented as two hexadecimal characters (e.g., `00` for PUSHN opcode)
- Whitespace and line breaks are ignored and can be used for readability
- Comments can be added by starting a line with `#`

### Example Bytecode
```
# Simple math program: Calculate 1+2*3
# PUSHN 1
00 0100

# PUSHN 2
00 0200

# PUSHN 3
00 0300

# MUL (2*3)
04

# ADD (1+(2*3))
01
```

This format makes it easy to create, inspect, and modify bytecode files with any text editor.

## Test Programs

The `/test` directory contains utilities for generating test bytecode programs that can be run with the VM:
```
cd test
make run     # Generate test programs
make test_simple    # Run the simple math test
make test_hello     # Run the hello world test
make test_countdown # Run the countdown test
```
