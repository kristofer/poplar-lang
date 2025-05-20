// vm.h - Core VM definitions for Poplar2

#ifndef POPLAR2_VM_H
#define POPLAR2_VM_H

#include <stdint.h>
#include <stdbool.h>
#include "value.h"

#define DBUG 1

// Memory limits and configuration for Agon Light 2
#define HEAP_START          0x020000
#define HEAP_SIZE           0x060000  // 384KB heap
#define STACK_SIZE          256       // VM execution stack size
#define FRAME_STACK_SIZE    64        // Max number of frames
#define MAX_LITERALS        1024      // Global literals table size
#define MAX_GLOBALS         1024      // Global variables table size

// Object flags
#define FLAG_GC_MARK        0x01
#define FLAG_ARRAY          0x02
#define FLAG_CLASS          0x04
#define FLAG_METHOD         0x08
#define FLAG_SYMBOL         0x10
#define FLAG_CONTEXT        0x20
#define FLAG_PRIMITIVE      0x40

// Forward declarations
typedef struct Method Method;
typedef struct Frame Frame;

// Basic object header
struct Object {
    Value class;       // Pointer to class
    uint8_t hash;      // Object hash
    uint8_t flags;     // Object flags
    uint16_t size;     // Number of fields
    Value fields[];    // Variable-sized array of fields
};

// Class object
typedef struct Class {
    Object object;         // Base object header
    Value name;           // Symbol object with class name
    Value superclass;     // Pointer to superclass
    Value methods;        // Array of methods
    Value instance_size;  // Size of instances (excluding header)
} Class;

// Method object
typedef struct Method {
    Object object;         // Base object header
    Value name;           // Symbol object with method name
    Value holder;         // Class that holds this method
    uint8_t num_args;     // Number of arguments
    uint8_t num_locals;   // Number of local variables
    uint16_t bytecode_count; // Number of bytecodes
    uint8_t bytecode[];   // Variable-sized array of bytecodes
} Method;

// Bytecodes
enum {
    // Stack operations
    BC_PUSH_LOCAL = 0x01,    // Push local variable
    BC_PUSH_ARGUMENT,        // Push argument
    BC_PUSH_FIELD,           // Push field from this
    BC_PUSH_BLOCK,           // Push new block closure
    BC_PUSH_CONSTANT,        // Push constant
    BC_PUSH_GLOBAL,          // Push global
    BC_PUSH_SPECIAL,         // Push special value (nil, true, false)

    // Stack manipulation
    BC_POP = 0x10,           // Pop and discard top of stack
    BC_DUP,                  // Duplicate top of stack
    BC_PUSH_THIS,            // Push 'this'

    // Storage operations
    BC_STORE_LOCAL = 0x20,   // Store into local variable
    BC_STORE_ARGUMENT,       // Store into argument
    BC_STORE_FIELD,          // Store into field
    BC_STORE_GLOBAL,         // Store into global

    // Send operations
    BC_SEND = 0x30,          // Send message
    BC_SUPER_SEND,           // Send message to super
    BC_RETURN_LOCAL,         // Return from method with value
    BC_RETURN_NON_LOCAL,     // Return from block

    // Control operations
    BC_JUMP = 0x40,          // Jump
    BC_JUMP_IF_TRUE,         // Jump if true
    BC_JUMP_IF_FALSE,        // Jump if false

    // Primitive operations
    BC_PRIMITIVE = 0x50     // Call primitive
};

// Execution frame
typedef struct Frame {
    Method* method;          // Current method being executed
    uint16_t bytecode_index; // Current bytecode index
    Value* stack_pointer;    // Current stack position
    Value receiver;          // Message receiver
    struct Frame* sender;    // Sender frame
    bool is_block_invocation;// Whether this is a block invocation
    Value context;           // Block context (for non-local returns)
    Value stack[];           // Variable-sized value stack
} Frame;

// VM state
typedef struct {
    // Memory management
    void* heap_start;        // Start of heap
    void* heap_next;         // Next free location on heap
    void* heap_end;          // End of heap

    // Execution
    Frame* current_frame;    // Current execution frame
    Frame* frames;           // Frame stack (pre-allocated)
    Value globals[MAX_GLOBALS]; // Global variables
    Value literals[MAX_LITERALS]; // Literals table

    // Core classes
    Value class_Object;
    Value class_Class;
    Value class_Method;
    Value class_Array;
    Value class_String;
    Value class_Symbol;
    Value class_Integer;
    Value class_Block;

    // Special constants
    Value nil;
    Value true_obj;
    Value false_obj;

    // Statistics
    uint32_t gc_count;       // Number of GC runs
    uint32_t allocated;      // Total bytes allocated
} VM;

// VM initialization and execution
void vm_init();
void vm_cleanup();
Value vm_execute_method(Method* method, Value receiver, Value* arguments, int arg_count);
Frame* vm_push_frame(Method* method, Value receiver);
void vm_pop_frame();
void vm_bootstrap_core_classes();

// Memory management
Object* vm_allocate_object(Value class, uint16_t size);
void vm_collect_garbage();

// External interface
Value vm_find_global(const char* name);
Value vm_find_class(const char* name);
Method* vm_find_method(Value class, const char* name);
Value vm_invoke_method(Value receiver, const char* name, Value* arguments, int arg_count);

// Error handling
void vm_error(const char* format, ...);

// Global VM instance
extern VM* vm;

#endif /* POPLAR2_VM_H */
