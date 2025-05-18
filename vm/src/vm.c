
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Define types for Agon
typedef uint8_t byte;
typedef int16_t i16;
typedef uint16_t u16;
// Use pre-defined uint24_t from stdint.h
#define DARWIN
//#define AGON
#ifdef DARWIN
typedef uint64_t PTR;
typedef uint64_t uint24_t;
#endif
#ifdef AGON
typedef uint24_t PTR;
#endif

// Define VM constants
#define STACK_SIZE 1024
#define TEMP_STACK_SIZE 256
#define HEAP_SIZE 4096
#define OUTPUT_BUFFER_SIZE 256

// Bytecode opcodes
// typedef enum {
//     OP_PUSHN,        // Push a number onto the stack
//     OP_ADD,          // Add two numbers
//     OP_SUB,          // Subtract
//     OP_MUL,          // Multiply
//     OP_DIV,          // Divide
//     OP_MOD,          // Modulo
//     OP_SIGN,         // Sign of a number
//     OP_ALLOCATE,     // Allocate memory on heap
//     OP_FREE,         // Free memory on heap
//     OP_BEGIN_WHILE,  // Begin a while loop
//     OP_END_WHILE,    // End a while loop
//     OP_STORE,        // Store bytes in memory
//     OP_LOAD,         // Load bytes from memory
//     OP_CALL,         // Call a function
//     OP_LOAD_FRAME_PTR, // Load the frame pointer
//     OP_MAKE_STACK_FRAME, // Create a stack frame
//     OP_DROP_STACK_FRAME,  // Drop a stack frame
//     OP_POPSTR        // Output memory contents to stdout
// } Opcode;

// Bytecode opcodes
typedef enum {
    OP_PUSHN = 0,        // Push a number onto the stack
    OP_ADD = 1,          // Add two numbers
    OP_SUB = 2,          // Subtract
    OP_MUL = 3,          // Multiply
    OP_DIV = 4,          // Divide
    OP_MOD = 5,          // Modulo
    OP_SIGN = 6,         // Sign of a number
    OP_ALLOCATE = 7,     // Allocate memory on heap
    OP_FREE = 8,         // Free memory on heap
    OP_BEGIN_WHILE = 9,  // Begin a while loop
    OP_END_WHILE = 10,    // End a while loop
    OP_STORE = 11,        // Store bytes in memory
    OP_LOAD = 12,         // Load bytes from memory
    OP_CALL = 13,         // Call a function
    OP_LOAD_FRAME_PTR = 14, // Load the frame pointer
    OP_MAKE_STACK_FRAME = 15, // Create a stack frame
    OP_DROP_STACK_FRAME = 16,  // Drop a stack frame
    OP_POPSTR = 17        // Output memory contents to stdout
} Opcode;

// VM state
typedef struct {
    // Memory areas
    i16 stack[STACK_SIZE];
    int stackPtr;

    i16 tempStack[TEMP_STACK_SIZE];
    int tempStackPtr;

    byte heap[HEAP_SIZE];
    PTR heapPtr;

    byte outputBuffer[OUTPUT_BUFFER_SIZE];
    int outputBufferPtr;

    byte inChar;  // Latest keyboard input

    // Program data
    byte* program;
    size_t programSize;
    size_t programCounter;

    // Call stack for function calls
    int framePtr;
    int* callStack;
    int callStackPtr;
    int callStackSize;

    // While loop tracking
    int* whileStack;
    int whileStackPtr;
    int whileStackSize;
} VM;

// Function prototypes
VM* vm_create();
void vm_destroy(VM* vm);
bool vm_load_program(VM* vm, const char* filename);
bool vm_run(VM* vm);
void vm_dump_state(VM* vm);

// Stack operations
void vm_stack_push(VM* vm, i16 value);
i16 vm_stack_pop(VM* vm);
void vm_temp_stack_push(VM* vm, i16 value);
i16 vm_temp_stack_pop(VM* vm);

// Heap operations
PTR vm_heap_allocate(VM* vm, size_t size);
void vm_heap_free(VM* vm, PTR ptr, size_t size);

// Instruction implementations
void vm_execute_instruction(VM* vm);
void vm_pushn(VM* vm, i16 n);
void vm_add(VM* vm);
void vm_subtract(VM* vm);
void vm_multiply(VM* vm);
void vm_divide(VM* vm);
void vm_modulo(VM* vm);
void vm_sign(VM* vm);
void vm_allocate(VM* vm);
void vm_free(VM* vm);
void vm_begin_while(VM* vm);
void vm_end_while(VM* vm);
void vm_store(VM* vm, uint24_t size);
void vm_load(VM* vm, uint24_t size);
void vm_call(VM* vm, i16 fnId);
void vm_load_frame_ptr(VM* vm);
void vm_make_stack_frame(VM* vm, byte argSize, byte localScopeSize);
void vm_drop_stack_frame(VM* vm, byte returnSize, byte localScopeSize);

// Helper functions
void vm_flush_output(VM* vm);
void vm_popstr(VM* vm);
void vm_print_stack(VM* vm, int max_elements, bool print_reverse, int format, const char* message);

// Create a new VM instance
VM* vm_create() {
    VM* vm = (VM*)malloc(sizeof(VM));
    if (!vm) return NULL;

    // Initialize the VM state
    vm->stackPtr = 0;
    vm->tempStackPtr = 0;
    vm->heapPtr = 0;
    vm->outputBufferPtr = 0;
    vm->inChar = 0;

    vm->program = NULL;
    vm->programSize = 0;
    vm->programCounter = 0;

    vm->framePtr = 0;

    // Call stack setup
    vm->callStackSize = 64;  // Initial size, can grow
    vm->callStack = (int*)malloc(vm->callStackSize * sizeof(int));
    vm->callStackPtr = 0;

    // While stack setup
    vm->whileStackSize = 32;  // Initial size, can grow
    vm->whileStack = (int*)malloc(vm->whileStackSize * sizeof(int));
    vm->whileStackPtr = 0;

    return vm;
}

// Free VM resources
void vm_destroy(VM* vm) {
    if (vm) {
        if (vm->program) free(vm->program);
        if (vm->callStack) free(vm->callStack);
        if (vm->whileStack) free(vm->whileStack);
        free(vm);
    }
}

// Load program from a .ppx file (hexadecimal ASCII format)
bool vm_load_program(VM* vm, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        return false;
    }

    // Initial allocation - we'll resize as needed
    size_t programCapacity = 1024;
    vm->program = (byte*)malloc(programCapacity);
    if (!vm->program) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return false;
    }

    // Parse the file
    char line[1024];
    size_t programSize = 0;

    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        // Parse hex values
        char* ptr = line;
        while (*ptr) {
            // Skip whitespace
            while (*ptr && isspace(*ptr)) ptr++;
            if (!*ptr) break;

            // Read 2 hex digits
            char hexByte[3] = {0};
            if (isxdigit(ptr[0]) && isxdigit(ptr[1])) {
                hexByte[0] = ptr[0];
                hexByte[1] = ptr[1];

                // Convert hex to byte
                byte value;
                sscanf(hexByte, "%hhx", &value);

                // Ensure capacity
                if (programSize >= programCapacity) {
                    programCapacity *= 2;
                    byte* newProgram = (byte*)realloc(vm->program, programCapacity);
                    if (!newProgram) {
                        fprintf(stderr, "Error: Memory reallocation failed\n");
                        free(vm->program);
                        fclose(file);
                        return false;
                    }
                    vm->program = newProgram;
                }

                // Store the byte
                vm->program[programSize++] = value;

                // Move to next hex byte
                ptr += 2;
            } else {
                // Skip non-hex characters
                ptr++;
            }
        }
    }

    fclose(file);

    if (programSize == 0) {
        fprintf(stderr, "Warning: No bytecode found in file\n");
    }

    // Resize to actual size
    if (programSize < programCapacity) {
        byte* newProgram = (byte*)realloc(vm->program, programSize);
        if (newProgram || programSize == 0) {
            vm->program = newProgram;
        }
        // If realloc fails, we just keep the existing allocation
    }

    vm->programSize = programSize;
    vm->programCounter = 0;

    return true;
}

// Run the loaded program
bool vm_run(VM* vm) {
    if (!vm->program) {
        fprintf(stderr, "Error: No program loaded\n");
        return false;
    }

    // Reset VM state
    vm->stackPtr = 0;
    vm->tempStackPtr = 0;
    vm->outputBufferPtr = 0;
    vm->programCounter = 0;
    vm->framePtr = 0;
    vm->callStackPtr = 0;
    vm->whileStackPtr = 0;

    // Execute until end of program
    while (vm->programCounter < vm->programSize) {
        vm_execute_instruction(vm);
    }

    vm_print_stack(vm, 0, true, 2, "Stack - Bottom to top:");
    // Flush any remaining output
    vm_flush_output(vm);

    return true;
}

// Execute the current instruction
void vm_execute_instruction(VM* vm) {
    if (vm->programCounter >= vm->programSize) return;

    byte opcode = vm->program[vm->programCounter++];

    printf("%02x ", opcode);
    switch (opcode) {
        case OP_PUSHN: {
            // Read 16-bit value (little endian)
            i16 value = vm->program[vm->programCounter] |
                       (vm->program[vm->programCounter + 1] << 8);
            vm->programCounter += 2;
            vm_pushn(vm, value);
            break;
        }
        case OP_ADD:
            vm_add(vm);
            break;
        case OP_SUB:
            vm_subtract(vm);
            break;
        case OP_MUL:
            vm_multiply(vm);
            break;
        case OP_DIV:
            vm_divide(vm);
            break;
        case OP_MOD:
            vm_modulo(vm);
            break;
        case OP_SIGN:
            vm_sign(vm);
            break;
        case OP_ALLOCATE:
            vm_allocate(vm);
            break;
        case OP_FREE:
            vm_free(vm);
            break;
        case OP_BEGIN_WHILE:
            vm_begin_while(vm);
            break;
        case OP_END_WHILE:
            vm_end_while(vm);
            break;
        case OP_STORE: {
            // Read 24-bit value (little endian)
            uint24_t size = vm->program[vm->programCounter] |
                           (vm->program[vm->programCounter + 1] << 8) |
                           (vm->program[vm->programCounter + 2] << 16);
            vm->programCounter += 3;
            vm_store(vm, size);
            break;
        }
        case OP_LOAD: {
            // Read 24-bit value (little endian)
            uint24_t size = vm->program[vm->programCounter] |
                           (vm->program[vm->programCounter + 1] << 8) |
                           (vm->program[vm->programCounter + 2] << 16);
            vm->programCounter += 3;
            vm_load(vm, size);
            break;
        }
        case OP_CALL: {
            // Read 16-bit function ID
            i16 fnId = vm->program[vm->programCounter] |
                      (vm->program[vm->programCounter + 1] << 8);
            vm->programCounter += 2;
            vm_call(vm, fnId);
            break;
        }
        case OP_LOAD_FRAME_PTR:
            vm_load_frame_ptr(vm);
            break;
        case OP_MAKE_STACK_FRAME: {
            byte argSize = vm->program[vm->programCounter++];
            byte localScopeSize = vm->program[vm->programCounter++];
            vm_make_stack_frame(vm, argSize, localScopeSize);
            break;
        }
        case OP_DROP_STACK_FRAME: {
            byte returnSize = vm->program[vm->programCounter++];
            byte localScopeSize = vm->program[vm->programCounter++];
            vm_drop_stack_frame(vm, returnSize, localScopeSize);
            break;
        }
        case OP_POPSTR: {
            vm_popstr(vm);
            break;
        }
        default:
            fprintf(stderr, "Error: Unknown opcode %d at position %zu\n",
                    opcode, vm->programCounter - 1);
            // Skip this instruction and try to continue
            break;
    }
}

// Stack operations
void vm_stack_push(VM* vm, i16 value) {
    if (vm->stackPtr >= STACK_SIZE) {
        fprintf(stderr, "Error: Stack overflow\n");
        exit(1);
    }
    vm->stack[vm->stackPtr++] = value;
}

i16 vm_stack_pop(VM* vm) {
    if (vm->stackPtr <= 0) {
        fprintf(stderr, "Error: Stack underflow\n");
        exit(1);
    }
    return vm->stack[--vm->stackPtr];
}

void vm_temp_stack_push(VM* vm, i16 value) {
    if (vm->tempStackPtr >= TEMP_STACK_SIZE) {
        fprintf(stderr, "Error: Temp stack overflow\n");
        exit(1);
    }
    vm->tempStack[vm->tempStackPtr++] = value;
}

i16 vm_temp_stack_pop(VM* vm) {
    if (vm->tempStackPtr <= 0) {
        fprintf(stderr, "Error: Temp stack underflow\n");
        exit(1);
    }
    return vm->tempStack[--vm->tempStackPtr];
}

// Heap operations
PTR vm_heap_allocate(VM* vm, size_t size) {
    if (vm->heapPtr + size > HEAP_SIZE) {
        fprintf(stderr, "Error: Out of heap memory\n");
        exit(1);
    }

    PTR ptr = vm->heapPtr;
    vm->heapPtr += size;

    // Initialize allocated memory to zero
    memset(vm->heap + ptr, 0, size);

    return ptr;
}

void vm_heap_free(VM* vm, PTR ptr, size_t size) {
    // Simple memory management - we don't actually free memory in this implementation
    // A real implementation would need a proper memory manager
    // This is just a placeholder
    (void)vm;
    (void)ptr;
    (void)size;
}

// Instruction implementations
void vm_pushn(VM* vm, i16 n) {
    vm_stack_push(vm, n);
}

void vm_add(VM* vm) {
    i16 b = vm_stack_pop(vm);
    i16 a = vm_stack_pop(vm);
    vm_stack_push(vm, a + b);
}

void vm_subtract(VM* vm) {
    i16 b = vm_stack_pop(vm);
    i16 a = vm_stack_pop(vm);
    vm_stack_push(vm, a - b);
}

void vm_multiply(VM* vm) {
    i16 b = vm_stack_pop(vm);
    i16 a = vm_stack_pop(vm);
    vm_stack_push(vm, a * b);
}

void vm_divide(VM* vm) {
    i16 b = vm_stack_pop(vm);
    if (b == 0) {
        fprintf(stderr, "Error: Division by zero\n");
        exit(1);
    }
    i16 a = vm_stack_pop(vm);
    vm_stack_push(vm, a / b);
}

void vm_modulo(VM* vm) {
    i16 b = vm_stack_pop(vm);
    if (b == 0) {
        fprintf(stderr, "Error: Modulo by zero\n");
        exit(1);
    }
    i16 a = vm_stack_pop(vm);
    vm_stack_push(vm, a % b);
}

void vm_sign(VM* vm) {
    i16 a = vm_stack_pop(vm);
    vm_stack_push(vm, a >= 0 ? 1 : -1);
}

void vm_allocate(VM* vm) {
    i16 size = vm_stack_pop(vm);
    if (size <= 0) {
        fprintf(stderr, "Error: Attempting to allocate non-positive size\n");
        exit(1);
    }

    PTR ptr = vm_heap_allocate(vm, (size_t)size);
    vm_stack_push(vm, (i16)ptr);  // Push the pointer as an i16
}

void vm_free(VM* vm) {
    PTR ptr = vm_stack_pop(vm);
    i16 size = vm_stack_pop(vm);
    if (size <= 0) {
        fprintf(stderr, "Error: Attempting to free non-positive size\n");
        exit(1);
    }

    vm_heap_free(vm, ptr, (size_t)size);
}

void vm_begin_while(VM* vm) {
    // Save the current position for potential jump back
    if (vm->whileStackPtr >= vm->whileStackSize) {
        // Grow the while stack if needed
        vm->whileStackSize *= 2;
        vm->whileStack = (int*)realloc(vm->whileStack, vm->whileStackSize * sizeof(int));
        if (!vm->whileStack) {
            fprintf(stderr, "Error: Memory allocation failed for while stack\n");
            exit(1);
        }
    }

    vm->whileStack[vm->whileStackPtr++] = vm->programCounter;

    // Evaluate the condition
    i16 condition = vm_stack_pop(vm);

    // If condition is false, skip to the end of the while loop
    if (condition == 0) {
        // Need to find the matching OP_END_WHILE
        int depth = 1;
        while (depth > 0 && vm->programCounter < vm->programSize) {
            byte opcode = vm->program[vm->programCounter++];
            if (opcode == OP_BEGIN_WHILE) {
                depth++;
            } else if (opcode == OP_END_WHILE) {
                depth--;
            }

            // Skip operands based on opcode
            switch (opcode) {
                case OP_PUSHN:
                    vm->programCounter += 2;
                    break;
                case OP_STORE:
                case OP_LOAD:
                    vm->programCounter += 3;
                    break;
                case OP_CALL:
                    vm->programCounter += 2;
                    break;
                case OP_MAKE_STACK_FRAME:
                case OP_DROP_STACK_FRAME:
                    vm->programCounter += 2;
                    break;
            }
        }

        // Pop the while stack entry since we're skipping the loop
        vm->whileStackPtr--;
    }
}

void vm_end_while(VM* vm) {
    if (vm->whileStackPtr <= 0) {
        fprintf(stderr, "Error: Unmatched end_while\n");
        exit(1);
    }

    // Jump back to the beginning of the while loop
    vm->programCounter = vm->whileStack[--vm->whileStackPtr];

    // The next instruction will be OP_BEGIN_WHILE, which will evaluate the condition again
}

void vm_store(VM* vm, uint24_t size) {
    if (size <= 0) return;

    PTR ptr = vm_stack_pop(vm);
    if (ptr + size > HEAP_SIZE) {
        fprintf(stderr, "Error: Memory access out of bounds\n");
        exit(1);
    }

    // Pop values from stack and store them in memory in reverse order
    for (uint24_t i = 0; i < size; i++) {
        i16 value = vm_stack_pop(vm);
        vm->heap[ptr + i] = (byte)value;  // Store as byte
    }
}

void vm_load(VM* vm, uint24_t size) {
    if (size <= 0) return;

    PTR ptr = vm_stack_pop(vm);
    vm_stack_push(vm, size);  // Push size onto stack as per spec

    if (ptr + size > HEAP_SIZE) {
        fprintf(stderr, "Error: Memory access out of bounds\n");
        exit(1);
    }

    // Load bytes from memory and push them onto the stack
    for (uint24_t i = 0; i < size; i++) {
        byte value = vm->heap[ptr + i];
        vm_stack_push(vm, (i16)value);  // Push as i16
    }
}

void vm_call(VM* vm, i16 fnId) {
    // In a real implementation, this would jump to a function entry point
    // For now, we'll just simulate with a function table

    // Save return address on call stack
    if (vm->callStackPtr >= vm->callStackSize) {
        // Grow the call stack if needed
        vm->callStackSize *= 2;
        vm->callStack = (int*)realloc(vm->callStack, vm->callStackSize * sizeof(int));
        if (!vm->callStack) {
            fprintf(stderr, "Error: Memory allocation failed for call stack\n");
            exit(1);
        }
    }

    vm->callStack[vm->callStackPtr++] = vm->programCounter;

    // Jump to function by ID (this is simplified)
    // In a real implementation, you'd look up the function address in a table
    fprintf(stderr, "Error: Function calls not fully implemented yet\n");
    exit(1);
}

void vm_load_frame_ptr(VM* vm) {
    vm_stack_push(vm, vm->framePtr);
}

void vm_make_stack_frame(VM* vm, byte argSize, byte localScopeSize) {
    // Save args to temp stack
    for (int i = 0; i < argSize; i++) {
        vm_temp_stack_push(vm, vm_stack_pop(vm));
    }

    // Save current frame pointer
    vm_temp_stack_push(vm, vm->framePtr);

    // Update frame pointer to point to the first local variable
    vm->framePtr = vm->stackPtr;

    // Allocate space for local variables (initialized to zero)
    for (int i = 0; i < localScopeSize; i++) {
        vm_stack_push(vm, 0);
    }

    // Restore the frame pointer from temp stack
    vm->framePtr = vm_temp_stack_pop(vm);

    // Restore args from temp stack in reverse order
    for (int i = 0; i < argSize; i++) {
        vm_stack_push(vm, vm_temp_stack_pop(vm));
    }
}

void vm_drop_stack_frame(VM* vm, byte returnSize, byte localScopeSize) {
    // Save return values to temp stack
    for (int i = 0; i < returnSize; i++) {
        vm_temp_stack_push(vm, vm_stack_pop(vm));
    }

    // Drop the local scope
    vm->stackPtr -= localScopeSize;

    // Restore frame pointer
    vm->framePtr = vm_stack_pop(vm);

    // Restore return values from temp stack in reverse order
    for (int i = 0; i < returnSize; i++) {
        vm_stack_push(vm, vm_temp_stack_pop(vm));
    }
}

// Helper functions
void vm_flush_output(VM* vm) {
    if (vm->outputBufferPtr > 0) {
        fwrite(vm->outputBuffer, 1, vm->outputBufferPtr, stdout);
        vm->outputBufferPtr = 0;
    }
}

void vm_popstr(VM* vm) {
    // Pop the number of bytes to output
    i16 numBytes = vm_stack_pop(vm);

    // Pop the memory address to start from
    i16 address = vm_stack_pop(vm);

    // Check bounds
    if (address < 0 || address + numBytes > HEAP_SIZE) {
        fprintf(stderr, "Error: Memory access out of bounds in POPSTR\n");
        return;
    }

    // Output the memory contents to stdout
    fwrite(&vm->heap[address], 1, numBytes, stdout);
    fflush(stdout);
}

void vm_dump_state(VM* vm) {
    printf("VM State:\n");
    printf("  Stack Pointer: %d\n", vm->stackPtr);
    printf("  Frame Pointer: %d\n", vm->framePtr);
    printf("  Program Counter: %zu\n", vm->programCounter);

    printf("  Stack (top %d elements):\n", vm->stackPtr > 10 ? 10 : vm->stackPtr);
    for (int i = vm->stackPtr - 1; i >= 0 && i >= vm->stackPtr - 10; i--) {
        printf("    [%d]: %d\n", i, vm->stack[i]);
    }
}

/**
 * Print the contents of the VM stack for debugging purposes.
 *
 * @param vm The VM instance
 * @param max_elements Maximum number of elements to print (0 for all)
 * @param print_reverse If true, print from bottom to top, otherwise top to bottom
 * @param format Output format: 0 = decimal, 1 = hex, 2 = both
 * @param message Optional message to display before the stack (NULL for none)
 */
void vm_print_stack(VM* vm, int max_elements, bool print_reverse, int format, const char* message) {
    if (!vm) return;

    // Print optional message
    if (message) {
        printf("%s\n", message);
    }

    // Print stack size information
    printf("Stack size: %d/%d elements (stackPtr = %d, framePtr = %d)\n",
           vm->stackPtr, STACK_SIZE, vm->stackPtr, vm->framePtr);

    // If stack is empty
    if (vm->stackPtr <= 0) {
        printf("Stack is empty.\n");
        return;
    }

    // Calculate how many elements to print
    int elements_to_print = (max_elements > 0 && max_elements < vm->stackPtr) ?
                            max_elements : vm->stackPtr;

    // Print column headers based on format
    printf("\n%-6s | %-8s", "INDEX", format == 0 ? "VALUE(DEC)" :
                                    (format == 1 ? "VALUE(HEX)" : "VALUE(DEC/HEX)"));

    if (vm->framePtr >= 0) {
        printf(" | %s", "NOTES");
    }
    printf("\n");

    printf("--------------------------------------\n");

    // Print stack elements
    if (print_reverse) {
        // Print from bottom to top
        for (int i = 0; i < elements_to_print; i++) {
            printf("[%4d] | ", i);

            switch (format) {
                case 0: // Decimal
                    printf("%-10d", vm->stack[i]);
                    break;
                case 1: // Hex
                    printf("0x%-8x", vm->stack[i]);
                    break;
                case 2: // Both
                    printf("%-6d (0x%04x)", vm->stack[i], vm->stack[i]);
                    break;
            }

            // Mark frame pointer
            if (i == vm->framePtr) {
                printf(" | <- frame pointer");
            }

            printf("\n");
        }
    } else {
        // Print from top to bottom (most recent first)
        for (int i = vm->stackPtr - 1; i >= vm->stackPtr - elements_to_print; i--) {
            printf("[%4d] | ", i);

            switch (format) {
                case 0: // Decimal
                    printf("%-10d", vm->stack[i]);
                    break;
                case 1: // Hex
                    printf("0x%-8x", vm->stack[i]);
                    break;
                case 2: // Both
                    printf("%-6d (0x%04x)", vm->stack[i], vm->stack[i]);
                    break;
            }

            // Mark stack top and frame pointer
            if (i == vm->stackPtr - 1) {
                printf(" | <- stack top");
            } else if (i == vm->framePtr) {
                printf(" | <- frame pointer");
            }

            printf("\n");
        }
    }

    printf("\n");
}

// Main function
int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program.ppx>\n", argv[0]);
        fprintf(stderr, "\nNote: Program files (.ppx) should be in hexadecimal ASCII format\n");
        fprintf(stderr, "Each byte is represented by two hex characters (e.g., '00' for opcode PUSHN)\n");
        fprintf(stderr, "Comments starting with # and whitespace are ignored\n");
        return 1;
    }

    VM* vm = vm_create();
    if (!vm) {
        fprintf(stderr, "Error: Failed to create VM\n");
        return 1;
    }

    if (!vm_load_program(vm, argv[1])) {
        vm_destroy(vm);
        return 1;
    }

    printf("Running program: %s\n", argv[1]);
    printf("Loaded %zu bytes of hexadecimal ASCII bytecode\n", vm->programSize);

    if (!vm_run(vm)) {
        fprintf(stderr, "Error: Program execution failed\n");
        vm_destroy(vm);
        return 1;
    }

    vm_destroy(vm);
    return 0;
}
