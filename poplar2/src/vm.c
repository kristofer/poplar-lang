// vm.c - Core VM implementation for Poplar2

#include "vm.h"
#include "object.h"
#include "interpreter.h"
#include "gc.h"
#include "som_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

// Global VM instance
VM* vm = NULL;

// Initialize the VM
void vm_init() {
    // Allocate VM structure
    vm = (VM*)malloc(sizeof(VM));
    if (vm == NULL) {
        fprintf(stderr, "Failed to allocate VM\n");
        exit(1);
    }

    // Clear VM structure
    memset(vm, 0, sizeof(VM));

    // Initialize garbage collector
    gc_init();

    // Allocate frame stack
    vm->frames = (Frame*)malloc(sizeof(Frame) * FRAME_STACK_SIZE);
    if (vm->frames == NULL) {
        fprintf(stderr, "Failed to allocate frame stack\n");
        exit(1);
    }

    // Initialize globals and literals to nil
    for (int i = 0; i < MAX_GLOBALS; i++) {
        vm->globals[i] = make_special(SPECIAL_NIL);
    }

    for (int i = 0; i < MAX_LITERALS; i++) {
        vm->literals[i] = make_special(SPECIAL_NIL);
    }

    // Create special constants
    vm->nil = make_special(SPECIAL_NIL);
    vm->true_obj = make_special(SPECIAL_TRUE);
    vm->false_obj = make_special(SPECIAL_FALSE);

    // Bootstrap core classes
    vm_bootstrap_core_classes();
}

// Function to add a class to the globals table
void register_global_class(const char* name, Value class_obj) {
    // Find the first empty slot in the globals table
    for (int i = 0; i < MAX_GLOBALS; i++) {
        if (is_nil(vm->globals[i])) {
            vm->globals[i] = class_obj;
            return;
        }
    }

    vm_error("Globals table is full, cannot register class %s", name);
}

// Create core classes in a mutually recursive way
void vm_bootstrap_core_classes() {
    // First create Class class
    Object* class_class_obj = object_new(make_special(SPECIAL_NIL), sizeof(Class) / sizeof(Value));
    Class* class_class = (Class*)class_class_obj;
    class_class_obj->flags |= FLAG_CLASS;
    class_class->name = make_special(SPECIAL_NIL); // Will be set later
    class_class->superclass = make_special(SPECIAL_NIL); // Will be set to Object
    class_class->methods = make_special(SPECIAL_NIL); // Will be set later
    class_class->instance_size = make_int(sizeof(Object) / sizeof(Value));

    // Temporarily store Class in VM
    vm->class_Class = make_object(class_class_obj);

    // Now create Object class
    Object* object_class_obj = object_new(vm->class_Class, sizeof(Class) / sizeof(Value));
    Class* object_class = (Class*)object_class_obj;
    object_class_obj->flags |= FLAG_CLASS;
    object_class->name = make_special(SPECIAL_NIL); // Will be set later
    object_class->superclass = make_special(SPECIAL_NIL); // Object has no superclass
    object_class->methods = make_special(SPECIAL_NIL); // Will be set later
    object_class->instance_size = make_int(0); // Default instance size

    // Store Object class in VM
    vm->class_Object = make_object(object_class_obj);

    // Fix Class's superclass to point to Object
    class_class->superclass = vm->class_Object;

    // Fix Class's class to point to itself
    class_class_obj->class = vm->class_Class;

    // Create other core classes
    vm->class_Method = make_object(class_new("Method", vm->class_Object, sizeof(Method) / sizeof(Value)));
    vm->class_Array = make_object(class_new("Array", vm->class_Object, 0));
    vm->class_String = make_object(class_new("String", vm->class_Object, 0));
    vm->class_Symbol = make_object(class_new("Symbol", vm->class_String, 0));
    vm->class_Integer = make_object(class_new("Integer", vm->class_Object, 0));
    vm->class_Block = make_object(class_new("Block", vm->class_Object, 0));

    // Now set names for Object and Class
    object_class->name = symbol_for("Object");
    class_class->name = symbol_for("Class");

    // Create empty arrays for methods
    object_class->methods = array_new(0);
    class_class->methods = array_new(0);

    // Register all core classes in the globals table
    register_global_class("Object", vm->class_Object);
    register_global_class("Class", vm->class_Class);
    register_global_class("Method", vm->class_Method);
    register_global_class("Array", vm->class_Array);
    register_global_class("String", vm->class_String);
    register_global_class("Symbol", vm->class_Symbol);
    register_global_class("Integer", vm->class_Integer);
    register_global_class("Block", vm->class_Block);

    // Create and register singleton instances
    Object* nil_class_obj = class_new("Nil", vm->class_Object, 0);
    Object* true_class_obj = class_new("True", vm->class_Object, 0);
    Object* false_class_obj = class_new("False", vm->class_Object, 0);

    register_global_class("Nil", make_object(nil_class_obj));
    register_global_class("True", make_object(true_class_obj));
    register_global_class("False", make_object(false_class_obj));

    // Register singleton instances
    register_global("nil", vm->nil);
    register_global("true", vm->true_obj);
    register_global("false", vm->false_obj);
}

// Helper to register any global (not just classes)
void register_global(const char* name, Value value) {
    // Find the first empty slot in the globals table
    for (int i = 0; i < MAX_GLOBALS; i++) {
        if (is_nil(vm->globals[i])) {
            vm->globals[i] = value;
            return;
        }
    }

    vm_error("Globals table is full, cannot register global %s", name);
}

// // Create core classes in a mutually recursive way
// void vm_bootstrap_core_classes() {
//     // First create Class class
//     Object* class_class_obj = object_new(make_special(SPECIAL_NIL), sizeof(Class) / sizeof(Value));
//     Class* class_class = (Class*)class_class_obj;
//     class_class_obj->flags |= FLAG_CLASS;
//     class_class->name = make_special(SPECIAL_NIL); // Will be set later
//     class_class->superclass = make_special(SPECIAL_NIL); // Will be set to Object
//     class_class->methods = make_special(SPECIAL_NIL); // Will be set later
//     class_class->instance_size = make_int(sizeof(Object) / sizeof(Value));

//     // Temporarily store Class in VM
//     vm->class_Class = make_object(class_class_obj);

//     // Now create Object class
//     Object* object_class_obj = object_new(vm->class_Class, sizeof(Class) / sizeof(Value));
//     Class* object_class = (Class*)object_class_obj;
//     object_class_obj->flags |= FLAG_CLASS;
//     object_class->name = make_special(SPECIAL_NIL); // Will be set later
//     object_class->superclass = make_special(SPECIAL_NIL); // Object has no superclass
//     object_class->methods = make_special(SPECIAL_NIL); // Will be set later
//     object_class->instance_size = make_int(0); // Default instance size

//     // Store Object class in VM
//     vm->class_Object = make_object(object_class_obj);

//     // Fix Class's superclass to point to Object
//     class_class->superclass = vm->class_Object;

//     // Fix Class's class to point to itself
//     class_class_obj->class = vm->class_Class;

//     // Create other core classes
//     vm->class_Method = make_object(class_new("Method", vm->class_Object, sizeof(Method) / sizeof(Value)));
//     vm->class_Array = make_object(class_new("Array", vm->class_Object, 0));
//     vm->class_String = make_object(class_new("String", vm->class_Object, 0));
//     vm->class_Symbol = make_object(class_new("Symbol", vm->class_String, 0));
//     vm->class_Integer = make_object(class_new("Integer", vm->class_Object, 0));
//     vm->class_Block = make_object(class_new("Block", vm->class_Object, 0));

//     // Now set names for Object and Class
//     object_class->name = symbol_for("Object");
//     class_class->name = symbol_for("Class");

//     // Create empty arrays for methods
//     object_class->methods = array_new(0);
//     class_class->methods = array_new(0);
// }

// Clean up VM resources
void vm_cleanup() {
    if (vm != NULL) {
        // Free the heap
        if (vm->heap_start != NULL) {
            free(vm->heap_start);
            vm->heap_start = NULL;
        }

        // Free the frame stack
        if (vm->frames != NULL) {
            free(vm->frames);
            vm->frames = NULL;
        }

        // Free VM structure
        free(vm);
        vm = NULL;
    }
}

// Execute a method
Value vm_execute_method(Method* method, Value receiver, Value* arguments, int arg_count) {
    return interpreter_execute_method(method, receiver, arguments, arg_count);
}

// Push a new frame onto the call stack
Frame* vm_push_frame(Method* method, Value receiver) {
    // Check if we've reached the maximum call depth
    if (vm->current_frame != NULL) {
        int depth = 0;
        Frame* frame = vm->current_frame;

        while (frame != NULL) {
            depth++;
            frame = frame->sender;
        }

        if (depth >= FRAME_STACK_SIZE) {
            vm_error("Stack overflow: maximum call depth exceeded");
            return NULL;
        }
    }

    // Get next frame from frame pool
    Frame* frame = &vm->frames[0]; // For simplicity, always use first frame

    if (vm->current_frame != NULL) {
        // Set sender
        frame->sender = vm->current_frame;
    } else {
        frame->sender = NULL;
    }

    // Initialize frame
    frame->method = method;
    frame->bytecode_index = 0;
    frame->receiver = receiver;
    frame->is_block_invocation = false;
    frame->context = make_special(SPECIAL_NIL);
    frame->stack_pointer = frame->stack;

    // Set as current frame
    vm->current_frame = frame;

    return frame;
}

// Pop the current frame from the call stack
void vm_pop_frame() {
    if (vm->current_frame == NULL) {
        vm_error("Stack underflow: no frames to pop");
        return;
    }

    // Get sender
    Frame* sender = vm->current_frame->sender;

    // Reset current frame
    vm->current_frame->method = NULL;
    vm->current_frame->bytecode_index = 0;
    vm->current_frame->receiver = make_special(SPECIAL_NIL);
    vm->current_frame->sender = NULL;
    vm->current_frame->is_block_invocation = false;
    vm->current_frame->context = make_special(SPECIAL_NIL);
    vm->current_frame->stack_pointer = vm->current_frame->stack;

    // Set sender as current frame
    vm->current_frame = sender;
}

// Allocate a new object
Object* vm_allocate_object(Value class, uint16_t size) {
    return object_new(class, size);
}

// Run garbage collection
void vm_collect_garbage() {
    gc_collect();
}

// Find a global variable by name
Value vm_find_global(const char* name) {
    Value symbol = symbol_for(name);
    if (DBUG) {
        printf("vm_find_global for %s\n", name);
    }

    // Search in globals table (linear search for simplicity)
    for (int i = 0; i < MAX_GLOBALS; i++) {
        if (DBUG) {
            printf("global[%d]: %d\n", i, vm->globals[i].value);
        }

        if (vm->globals[i].tag != TAG_SPECIAL || vm->globals[i].value != SPECIAL_NIL) {
            // Check if this is a class
            if (is_object(vm->globals[i]) && (as_object(vm->globals[i])->flags & FLAG_CLASS)) {
                Class* class = (Class*)as_object(vm->globals[i]);

                if (value_equals(class->name, symbol)) {
                    return vm->globals[i];
                }
            }
        }
    }

    return vm->nil;
}

// Find a class by name
Value vm_find_class(const char* name) {
    return vm_find_global(name); // Classes are stored as globals
}

// Find a method in a class
Method* vm_find_method(Value class, const char* name) {
    Value selector = symbol_for(name);
    return class_lookup_method(class, selector);
}

// Invoke a method on a receiver
Value vm_invoke_method(Value receiver, const char* name, Value* arguments, int arg_count) {
    Value class;

    // Get receiver's class
    if (is_int(receiver)) {
        class = vm->class_Integer;
    } else if (is_special(receiver)) {
        if (is_nil(receiver)) {
            class = vm->nil;
        } else if (is_true(receiver)) {
            class = vm->true_obj;
        } else if (is_false(receiver)) {
            class = vm->false_obj;
        } else {
            vm_error("Unknown special value");
            return vm->nil;
        }
    } else {
        class = as_object(receiver)->class;
    }

    // Find method
    Value selector = symbol_for(name);
    Method* method = class_lookup_method(class, selector);

    if (method == NULL) {
        vm_error("Method not found: %s", name);
        return vm->nil;
    }

    // Invoke method
    return vm_execute_method(method, receiver, arguments, arg_count);
}

// Error handling
void vm_error(const char* format, ...) {
    va_list args;

    fprintf(stderr, "VM Error: ");

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");

    // Print stack trace
    if (vm->current_frame != NULL) {
        fprintf(stderr, "Stack trace:\n");

        Frame* frame = vm->current_frame;
        int depth = 0;

        while (frame != NULL && depth < 10) {
            fprintf(stderr, "  %d: ", depth);

            if (frame->method != NULL) {
                fprintf(stderr, "%s", symbol_to_string(frame->method->name));
            } else {
                fprintf(stderr, "<unknown>");
            }

            fprintf(stderr, " (bytecode: %d)\n", frame->bytecode_index);

            frame = frame->sender;
            depth++;
        }

        if (frame != NULL) {
            fprintf(stderr, "  ... (more frames)\n");
        }
    }
}

// Load a SOM file and execute the 'run' method
Value vm_load_and_run(const char* filename) {
    printf("Loading %s...\n", filename);

    // Parse the SOM file
    if (!parse_file(filename)) {
        fprintf(stderr, "Failed to parse SOM file: %s\n", filename);
        return vm->nil;
    }

    // Look for Main class
    Value main_class = vm_find_class("Main");
    if (is_nil(main_class)) {
        fprintf(stderr, "Main class not found in %s\n", filename);
        return vm->nil;
    }

    // Create main instance
    Value main_instance = make_object(object_new(main_class, 0));

    // Look for run method
    Method* run_method = vm_find_method(main_class, "run");
    if (run_method == NULL) {
        fprintf(stderr, "run method not found in Main class\n");
        return vm->nil;
    }

    // Invoke run method
    return vm_execute_method(run_method, main_instance, NULL, 0);
}

// Main entry point for the VM
int main(int argc, char** argv) {
    // Initialize VM
    vm_init();

    // Check arguments
    if (argc < 2) {
        printf("Usage: %s <somfile>\n", argv[0]);
        vm_cleanup();
        return 1;
    }

    // Load SOM file and run
    const char* filename = argv[1];

    // If no SOM parser is available or for testing, use this fallback
    if (strstr(filename, "--test-hello") != NULL) {
        // Create test main class
        Value main_class = make_object(class_new("Main", vm->class_Object, 0));
        vm->globals[0] = main_class;

        // Create "run" method
        Method* run_method = method_new("run", 0, 0);
        run_method->holder = main_class;

        // Add simple bytecode to print "Hello, World!"
        uint8_t bytecodes[] = {
            BC_PUSH_CONSTANT, 0,  // Push string constant
            BC_PRIMITIVE, 16, 1,  // Primitive 16 (println) with 1 argument
            BC_PUSH_SPECIAL, SPECIAL_NIL,  // Push nil
            BC_RETURN_LOCAL        // Return nil
        };

        // Copy bytecodes to method
        memcpy(run_method->bytecode, bytecodes, sizeof(bytecodes));
        run_method->bytecode_count = sizeof(bytecodes);

        // Set up literals
        vm->literals[0] = string_new("Hello, Kristofer From POPLAR2!");

        // Add method to class
        Class* class_obj = (Class*)as_object(main_class);
        Value methods = class_obj->methods;

        // Convert methods to an array with the run method
        Value new_methods = array_new(1);
        array_at_put(new_methods, 0, make_object((Object*)run_method));
        class_obj->methods = new_methods;

        // Create main instance
        Value main_instance = make_object(object_new(main_class, 0));

        // Invoke "run" method
        vm_invoke_method(main_instance, "run", NULL, 0);
    } else {
        // Parse and run the SOM file
        vm_load_and_run(filename);
    }

    // Clean up
    vm_cleanup();

    return 0;
}
