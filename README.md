# foundational implementation of the Poplar2 VM for the Agon Light 2

1. **Design Document**: A detailed specification for the Poplar2 VM, including memory layout, object representation, and execution model.

2. **Core VM Components**:
   - Tagged value system for efficient representation of different data types
   - Object model with classes, methods, and message passing
   - Stack-based bytecode interpreter
   - Mark-sweep garbage collector
   - Primitive operations including Agon-specific graphics functions

3. **Implementation Files**:
   - `vm.h/vm.c`: Core VM definitions and main execution loop
   - `object.h/object.c`: Object model implementation
   - `interpreter.h/interpreter.c`: Bytecode interpreter
   - `gc.h/gc.c`: Garbage collector
   - `value.c`: Value type handling
   - `Makefile`: Build configuration

4. **Sample SOM Program**: A simple Hello World example that demonstrates basic syntax and features.

This implementation is designed to efficiently run on the Agon Light 2 board with its eZ80 processor and limited memory. The VM uses a compact object representation and memory layout optimized for the 24-bit address space of the eZ80.

## Next Steps

To complete this project and get it running on your Agon Light 2 board, you might want to consider:

1. **Parser Implementation**: Add a full SOM language parser to load `.som` files directly.

2. **Agon-Specific Integration**: Implement the platform-specific primitives for graphics, input, and file operations using the Agon's APIs.

3. **Cross-Compilation Setup**: Configure the build system to properly cross-compile for the eZ80 architecture.

4. **Standard Library**: Develop a standard library of SOM classes that provide useful functionality.

5. **Testing Framework**: Create tests to verify the VM's functionality.

This implementation is a solid foundation to build upon, adapting the concepts from CSOM to create a VM that is optimized for the constraints of the Agon Light 2 platform.
