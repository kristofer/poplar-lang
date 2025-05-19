# Poplar2 VM: SOM Virtual Machine for Agon Light 2

## Overview

Poplar2 is a Simple Object Machine (SOM) virtual machine implementation for the Agon Light 2 board. This design follows the core principles of CSOM, a C implementation of the SOM language, while being optimized for the eZ80 architecture and memory constraints of the Agon Light 2 platform.

## System Architecture

The Poplar2 VM architecture consists of these key components:

1. **Memory Management**
   - Object heap management
   - Garbage collection
   - Object layout optimized for eZ80's 24-bit address space

2. **Object Model**
   - Classes and objects
   - Method invocation
   - Primitive operations

3. **Bytecode Interpreter**
   - Stack-based execution
   - Optimized bytecode set
   - Dispatch mechanism

4. **Parser/Compiler**
   - SOM language parser
   - Bytecode generation

5. **Standard Library**
   - Core Smalltalk classes
   - Agon-specific primitives (for I/O, graphics, etc.)

## Memory Layout

The eZ80 processor in the Agon Light 2 has a 24-bit address space, giving us access to 16MB of addressable memory, though the physical SRAM is limited to 512KB. We'll design our memory layout to efficiently use this available memory:

```
0x000000 - 0x00FFFF: Interpreter code and static data
0x010000 - 0x01FFFF: Bytecode storage
0x020000 - 0x07FFFF: Object heap (384KB)
```

## Object Representation

Objects in Poplar2 will be represented with a compact format optimized for our limited memory:

```c
typedef struct {
    uint32_t class_pointer;  // Pointer to class object (24-bit address)
    uint8_t  hash;           // Object hash
    uint8_t  flags;          // Object flags (GC mark, etc.)
    uint16_t size;           // Size of fields array
    value_t  fields[];       // Variable-sized array of fields
} object_t;
```

Where `value_t` is a tagged union that can represent:
- Integer values (16-bit, tagged to distinguish from pointers)
- Object pointers (24-bit)
- Special constants (nil, true, false)

```c
typedef union {
    uint32_t bits;  // Raw bits
    struct {
        uint32_t tag : 2;        // Tag bits (00 = INT, 01 = OBJ, 10 = SPECIAL)
        uint32_t value : 30;     // Value or pointer
    };
} value_t;
```

## Class Structure

Classes are themselves objects with additional metadata:

```c
typedef struct {
    object_t  object;           // Base object header
    value_t   name;             // Symbol object with class name
    value_t   super_class;      // Pointer to superclass
    value_t   instance_fields;  // Array of field names
    value_t   instance_methods; // Dictionary of methods
    value_t   class_methods;    // Dictionary of class methods
} class_t;
```

## Method Structure

Methods will be represented as:

```c
typedef struct {
    object_t  object;          // Base object header
    value_t   signature;       // Method signature (Symbol)
    value_t   holder;          // Class that holds this method
    uint16_t  num_arguments;   // Number of arguments
    uint16_t  num_locals;      // Number of local variables
    uint16_t  bytecode_count;  // Number of bytecodes
    uint8_t   bytecodes[];     // Array of bytecodes
} method_t;
```

## Bytecode Instruction Set

Poplar2 will use a compact bytecode format optimized for our stack-based VM:

```
0x00-0x0F: Stack operations (push, pop, dup, etc.)
0x10-0x1F: Local variable access (load, store)
0x20-0x2F: Field access (load, store)
0x30-0x3F: Method invocation
0x40-0x4F: Control flow (jumps, conditionals)
0x50-0x5F: Primitive operations
0x60-0x7F: Constants and literals
```

## Interpreter Main Loop

The main interpreter loop will be:

```c
void interpret_method(method_t* method, frame_t* frame) {
    uint8_t* bytecodes = method->bytecodes;
    uint16_t pc = 0;
    
    while (pc < method->bytecode_count) {
        uint8_t bytecode = bytecodes[pc++];
        
        switch (bytecode) {
            case PUSH_LOCAL:
                // Push local to stack
                break;
            case STORE_LOCAL:
                // Store stack top to local
                break;
            case SEND_MESSAGE:
                // Send message to receiver
                break;
            // ... other bytecode handlers
        }
    }
}
```

## Garbage Collection

For the Agon Light 2's limited memory, we'll implement a simple but efficient mark-sweep garbage collector:

```c
void collect_garbage() {
    // Mark phase: trace objects from roots
    mark_roots();
    
    // Sweep phase: reclaim unmarked objects
    sweep_heap();
}
```

## Agon-Specific Integration

We'll add special primitives to interact with Agon Light 2 hardware:

1. **VDP Access** - For graphics operations via the ESP32 co-processor
2. **File I/O** - For SD card access
3. **Keyboard Input** - For PS/2 keyboard handling
4. **Sound Generation** - For audio output

## Implementation Plan

1. **Phase 1: Core VM**
   - Basic object model implementation
   - Bytecode interpreter
   - Minimal GC

2. **Phase 2: Parser/Compiler**
   - SOM syntax parser
   - Bytecode generation

3. **Phase 3: Standard Library**
   - Core Smalltalk classes
   - Basic primitives

4. **Phase 4: Agon Integration**
   - Agon-specific primitives
   - Graphics and I/O support

5. **Phase 5: Optimization**
   - Performance tuning
   - Memory usage optimization

## Compilation

The VM will be compiled using the agondev C compiler, targeting the eZ80 architecture of the Agon Light 2.

```
gcc -march=ez80 -O2 -I./include -o poplar2 src/*.c
```

## Project Structure

```
/poplar2
  /src
    vm.c           - Main VM implementation
    object.c       - Object model
    interpreter.c  - Bytecode interpreter
    gc.c           - Garbage collector
    parser.c       - SOM language parser
    compiler.c     - Bytecode compiler
    primitives.c   - Primitive operations
    agon.c         - Agon-specific code
  /include
    vm.h           - VM declarations
    object.h       - Object model declarations
    interpreter.h  - Interpreter declarations
    gc.h           - GC declarations
    parser.h       - Parser declarations
    compiler.h     - Compiler declarations
    primitives.h   - Primitives declarations
    agon.h         - Agon-specific declarations
  /lib
    /som           - Standard library in SOM
  /examples        - Example SOM programs
  Makefile         - Build configuration
```

This design provides a solid foundation for implementing a SOM-compatible VM on the Agon Light 2 platform. The implementation balances the constraints of the 8-bit system with the requirements of a modern bytecode interpreter.
