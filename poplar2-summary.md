# Poplar2 VM: Implementation Summary

## Overview

Poplar2 is a Smalltalk-like virtual machine implementation based on the Simple Object Machine (SOM), designed to run on the Agon Light 2 board. This implementation provides a minimal yet functional VM that can execute SOM bytecode with an object-oriented approach optimized for the eZ80 architecture.

## Key Components

### 1. Core VM Architecture
- **Tagged Value System**: Efficient representation of objects, integers, and special values using tag bits.
- **Object Model**: Smalltalk-inspired object hierarchy with classes, methods, and message passing.
- **Memory Management**: Custom heap allocation with mark-sweep garbage collection.
- **Bytecode Interpreter**: Stack-based interpreter for executing compiled SOM methods.

### 2. Object Representation
- **Compact Object Headers**: Minimized memory overhead with a small object header (class pointer, hash, flags, size).
- **Class Structure**: Classes themselves are objects with fields for superclass, methods, and instance information.
- **Method Structure**: Methods contain bytecode, argument count, and local variable information.

### 3. Execution Model
- **Stack-Based**: Uses an operand stack for method execution.
- **Message Passing**: Dynamic method lookup and invocation.
- **Frames**: Call stack represented as linked frames for each method activation.
- **Primitives**: Optimized native implementations of common operations.

### 4. Agon Integration
- **VDP Integration**: Primitives for drawing graphics using the ESP32 co-processor.
- **Memory Optimization**: Designed to operate efficiently within the 512KB RAM constraints.
- **I/O Handling**: Support for keyboard input and file operations.

## Implementation Files

1. **vm.h/vm.c**: Core VM definitions and main entry point.
2. **object.h/object.c**: Object model and basic operations.
3. **interpreter.h/interpreter.c**: Bytecode interpreter implementation.
4. **gc.h/gc.c**: Garbage collector implementation.
5. **value.c**: Value type conversion and handling.

## Memory Layout

The VM uses a carefully designed memory layout to maximize the available memory on the Agon Light 2:

```
0x000000 - 0x00FFFF: Interpreter code and static data
0x010000 - 0x01FFFF: Bytecode storage
0x020000 - 0x07FFFF: Object heap (384KB)
```

## Compilation & Building

The implementation can be compiled in two ways:
1. Standard C compilation for development and testing on a host machine.
2. Cross-compilation for the eZ80 processor in the Agon Light 2 using the appropriate toolchain.

The included Makefile provides targets for both compilation modes.

## Future Enhancements

1. **Full Parser**: Adding a SOM language parser to load `.som` files directly.
2. **Optimized Bytecode**: Specialized bytecodes for common operations.
3. **JIT Compilation**: Potential just-in-time compilation for hot methods.
4. **Extended Library**: More extensive standard library with Agon-specific features.
5. **Improved GC**: More sophisticated garbage collection algorithms.

## Conclusion

Poplar2 provides a solid foundation for running SOM language code on the Agon Light 2 board, enabling object-oriented programming in a constrained 8-bit environment while maintaining the flexibility and expressiveness of the Smalltalk programming model.
