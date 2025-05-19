// interpreter.c - Bytecode interpreter for Poplar2

#include "interpreter.h"
#include "object.h"
#include "gc.h"
#include <stdio.h>
#include <stdlib.h>

// Execute a method
Value interpreter_execute_method(Method* method, Value receiver, Value* arguments, int arg_count) {
    // Push new frame
    Frame* frame = vm_push_frame(method, receiver);
    
    // Copy arguments to frame
    for (int i = 0; i < arg_count && i < method->num_args; i++) {
        frame->stack[i] = arguments[i];
    }
    
    // Initialize locals to nil
    for (int i = arg_count; i < method->num_args + method->num_locals; i++) {
        frame->stack[i] = vm->nil;
    }
    
    // Set stack pointer after locals
    frame->stack_pointer = &frame->stack[method->num_args + method->num_locals];
    
    // Execute bytecodes
    while (frame->bytecode_index < method->bytecode_count) {
        uint8_t bytecode = method->bytecode[frame->bytecode_index++];
        interpreter_handle_bytecode(bytecode);
        
        // Check if frame changed (due to return or new message)
        if (vm->current_frame != frame) {
            break;
        }
    }
    
    // Get return value (top of stack)
    Value result = vm->current_frame->stack_pointer > vm->current_frame->stack
                  ? *(vm->current_frame->stack_pointer - 1)
                  : vm->nil;
    
    // Restore sender frame if we're still in the current frame
    if (vm->current_frame == frame) {
        vm_pop_frame();
    }
    
    return result;
}

// Push value onto the stack
void interpreter_push(Value value) {
    Frame* frame = vm->current_frame;
    
    // Check for stack overflow
    if (frame->stack_pointer >= &frame->stack[STACK_SIZE]) {
        vm_error("Stack overflow");
        return;
    }
    
    *frame->stack_pointer++ = value;
}

// Pop value from the stack
Value interpreter_pop() {
    Frame* frame = vm->current_frame;
    
    // Check for stack underflow
    if (frame->stack_pointer <= frame->stack) {
        vm_error("Stack underflow");
        return vm->nil;
    }
    
    return *--frame->stack_pointer;
}

// Peek at the top value on the stack
Value interpreter_peek() {
    Frame* frame = vm->current_frame;
    
    // Check for empty stack
    if (frame->stack_pointer <= frame->stack) {
        vm_error("Stack is empty");
        return vm->nil;
    }
    
    return *(frame->stack_pointer - 1);
}

// Handle a single bytecode instruction
void interpreter_handle_bytecode(uint8_t bytecode) {
    Frame* frame = vm->current_frame;
    Method* method = frame->method;
    uint8_t arg;
    
    switch (bytecode) {
        case BC_PUSH_LOCAL: {
            // Next byte is local index
            arg = method->bytecode[frame->bytecode_index++];
            
            // Calculate local index (offset from frame base)
            uint8_t local_index = method->num_args + arg;
            
            // Push local onto stack
            if (local_index < method->num_args + method->num_locals) {
                interpreter_push(frame->stack[local_index]);
            } else {
                vm_error("Invalid local variable index: %d", arg);
                interpreter_push(vm->nil);
            }
            break;
        }
        
        case BC_PUSH_ARGUMENT: {
            // Next byte is argument index
            arg = method->bytecode[frame->bytecode_index++];
            
            // Push argument onto stack
            if (arg < method->num_args) {
                interpreter_push(frame->stack[arg]);
            } else {
                vm_error("Invalid argument index: %d", arg);
                interpreter_push(vm->nil);
            }
            break;
        }
        
        case BC_PUSH_FIELD: {
            // Next byte is field index
            arg = method->bytecode[frame->bytecode_index++];
            
            // Get receiver
            Object* receiver_obj = as_object(frame->receiver);
            
            // Push field onto stack
            if (arg < receiver_obj->size) {
                interpreter_push(receiver_obj->fields[arg]);
            } else {
                vm_error("Invalid field index: %d", arg);
                interpreter_push(vm->nil);
            }
            break;
        }
        
        case BC_PUSH_BLOCK: {
            // Next byte is block size (bytecode length)
            uint8_t block_size = method->bytecode[frame->bytecode_index++];
            
            // Skip the block for now (we'll handle blocks later)
            frame->bytecode_index += block_size;
            
            // Push a placeholder block object
            interpreter_push(vm->nil);
            break;
        }
        
        case BC_PUSH_CONSTANT: {
            // Next byte is constant index
            arg = method->bytecode[frame->bytecode_index++];
            
            // Push constant onto stack
            if (arg < MAX_LITERALS) {
                interpreter_push(vm->literals[arg]);
            } else {
                vm_error("Invalid constant index: %d", arg);
                interpreter_push(vm->nil);
            }
            break;
        }
        
        case BC_PUSH_GLOBAL: {
            // Next byte is global index
            arg = method->bytecode[frame->bytecode_index++];
            
            // Push global onto stack
            if (arg < MAX_GLOBALS) {
                interpreter_push(vm->globals[arg]);
            } else {
                vm_error("Invalid global index: %d", arg);
                interpreter_push(vm->nil);
            }
            break;
        }
        
        case BC_PUSH_SPECIAL: {
            // Next byte is special value code (nil, true, false)
            arg = method->bytecode[frame->bytecode_index++];
            
            // Push special value onto stack
            switch (arg) {
                case SPECIAL_NIL:
                    interpreter_push(vm->nil);
                    break;
                case SPECIAL_TRUE:
                    interpreter_push(vm->true_obj);
                    break;
                case SPECIAL_FALSE:
                    interpreter_push(vm->false_obj);
                    break;
                default:
                    vm_error("Invalid special value: %d", arg);
                    interpreter_push(vm->nil);
            }
            break;
        }
        
        case BC_POP: {
            // Pop and discard top of stack
            interpreter_pop();
            break;
        }
        
        case BC_DUP: {
            // Duplicate top of stack
            Value value = interpreter_peek();
            interpreter_push(value);
            break;
        }
        
        case BC_PUSH_THIS: {
            // Push receiver
            interpreter_push(frame->receiver);
            break;
        }
        
        case BC_STORE_LOCAL: {
            // Next byte is local index
            arg = method->bytecode[frame->bytecode_index++];
            
            // Calculate local index
            uint8_t local_index = method->num_args + arg;
            
            // Store top of stack to local
            if (local_index < method->num_args + method->num_locals) {
                frame->stack[local_index] = interpreter_peek();
            } else {
                vm_error("Invalid local variable index: %d", arg);
            }
            break;
        }
        
        case BC_STORE_ARGUMENT: {
            // Next byte is argument index
            arg = method->bytecode[frame->bytecode_index++];
            
            // Store top of stack to argument
            if (arg < method->num_args) {
                frame->stack[arg] = interpreter_peek();
            } else {
                vm_error("Invalid argument index: %d", arg);
            }
            break;
        }
        
        case BC_STORE_FIELD: {
            // Next byte is field index
            arg = method->bytecode[frame->bytecode_index++];
            
            // Get receiver
            Object* receiver_obj = as_object(frame->receiver);
            
            // Store top of stack to field
            if (arg < receiver_obj->size) {
                receiver_obj->fields[arg] = interpreter_peek();
            } else {
                vm_error("Invalid field index: %d", arg);
            }
            break;
        }
        
        case BC_STORE_GLOBAL: {
            // Next byte is global index
            arg = method->bytecode[frame->bytecode_index++];
            
            // Store top of stack to global
            if (arg < MAX_GLOBALS) {
                vm->globals[arg] = interpreter_peek();
            } else {
                vm_error("Invalid global index: %d", arg);
            }
            break;
        }
        
        case BC_SEND: {
            // Next byte is selector index followed by argument count
            uint8_t selector_idx = method->bytecode[frame->bytecode_index++];
            uint8_t arg_count = method->bytecode[frame->bytecode_index++];
            
            // Get selector from literals
            Value selector = vm->literals[selector_idx];
            
            // Arguments are on the stack, with receiver on the bottom
            // Pop arguments in reverse order
            Value args[16]; // Max 16 arguments
            
            for (int i = arg_count - 1; i >= 0; i--) {
                args[i] = interpreter_pop();
            }
            
            // Pop receiver
            Value receiver = interpreter_pop();
            
            // Perform message send
            Value result = interpreter_send(receiver, selector, arg_count, args);
            
            // Push result
            interpreter_push(result);
            break;
        }
        
        case BC_SUPER_SEND: {
            // Next byte is selector index followed by argument count
            uint8_t selector_idx = method->bytecode[frame->bytecode_index++];
            uint8_t arg_count = method->bytecode[frame->bytecode_index++];
            
            // Get selector from literals
            Value selector = vm->literals[selector_idx];
            
            // Arguments are on the stack
            Value args[16]; // Max 16 arguments
            
            for (int i = arg_count - 1; i >= 0; i--) {
                args[i] = interpreter_pop();
            }
            
            // Use current receiver for super sends
            // Perform super send
            Value result = interpreter_super_send(selector, arg_count, args);
            
            // Push result
            interpreter_push(result);
            break;
        }
        
        case BC_RETURN_LOCAL: {
            // Return from method with top of stack as result
            Value result = interpreter_pop();
            
            // Restore sender frame
            vm_pop_frame();
            
            // Push result on sender's stack
            if (vm->current_frame != NULL) {
                interpreter_push(result);
            }
            break;
        }
        
        case BC_RETURN_NON_LOCAL: {
            // Return from block (non-local return)
            // This is more complex and will be implemented later
            vm_error("Non-local return not implemented yet");
            break;
        }
        
        case BC_JUMP: {
            // Next two bytes are jump offset
            uint8_t high = method->bytecode[frame->bytecode_index++];
            uint8_t low = method->bytecode[frame->bytecode_index++];
            uint16_t offset = (high << 8) | low;
            
            // Jump to offset
            frame->bytecode_index = offset;
            break;
        }
        
        case BC_JUMP_IF_TRUE: {
            // Next two bytes are jump offset
            uint8_t high = method->bytecode[frame->bytecode_index++];
            uint8_t low = method->bytecode[frame->bytecode_index++];
            uint16_t offset = (high << 8) | low;
            
            // Pop condition
            Value condition = interpreter_pop();
            
            // Jump if true
            if (is_true(condition) || (!is_false(condition) && !is_nil(condition))) {
                frame->bytecode_index = offset;
            }
            break;
        }
        
        case BC_JUMP_IF_FALSE: {
            // Next two bytes are jump offset
            uint8_t high = method->bytecode[frame->bytecode_index++];
            uint8_t low = method->bytecode[frame->bytecode_index++];
            uint16_t offset = (high << 8) | low;
            
            // Pop condition
            Value condition = interpreter_pop();
            
            // Jump if false
            if (is_false(condition) || is_nil(condition)) {
                frame->bytecode_index = offset;
            }
            break;
        }
        
        case BC_PRIMITIVE: {
            // Next byte is primitive ID followed by argument count
            uint8_t primitive_id = method->bytecode[frame->bytecode_index++];
            uint8_t arg_count = method->bytecode[frame->bytecode_index++];
            
            // Arguments are on the stack
            Value args[16]; // Max 16 arguments
            
            for (int i = arg_count - 1; i >= 0; i--) {
                args[i] = interpreter_pop();
            }
            
            // Perform primitive operation
            Value result = interpreter_primitive(primitive_id, args, arg_count);
            
            // Push result
            interpreter_push(result);
            break;
        }
        
        default:
            vm_error("Unknown bytecode: %d", bytecode);
    }
}

// Send a message to a receiver
Value interpreter_send(Value receiver, Value selector, int arg_count, Value* args) {
    // Get receiver's class
    Value class;
    
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
    
    // Lookup method
    Method* method = class_lookup_method(class, selector);
    
    if (method == NULL) {
        vm_error("Method not found: %s", symbol_to_string(selector));
        return vm->nil;
    }
    
    // Execute method
    return interpreter_execute_method(method, receiver, args, arg_count);
}

// Send a message to super
Value interpreter_super_send(Value selector, int arg_count, Value* args) {
    Frame* frame = vm->current_frame;
    
    // Get current method's holder class
    Value holder = frame->method->holder;
    
    // Get superclass
    Value superclass = ((Class*)as_object(holder))->superclass;
    
    if (is_nil(superclass)) {
        vm_error("No superclass for super send");
        return vm->nil;
    }
    
    // Lookup method in superclass
    Method* method = class_lookup_method(superclass, selector);
    
    if (method == NULL) {
        vm_error("Method not found in superclass: %s", symbol_to_string(selector));
        return vm->nil;
    }
    
    // Execute method with current receiver
    return interpreter_execute_method(method, frame->receiver, args, arg_count);
}

// Execute a primitive operation
Value interpreter_primitive(uint8_t primitive_id, Value* args, int arg_count) {
    switch (primitive_id) {
        case 1: // Integer add
            if (arg_count == 2 && is_int(args[0]) && is_int(args[1])) {
                return make_int(as_int(args[0]) + as_int(args[1]));
            }
            break;
            
        case 2: // Integer subtract
            if (arg_count == 2 && is_int(args[0]) && is_int(args[1])) {
                return make_int(as_int(args[0]) - as_int(args[1]));
            }
            break;
            
        case 3: // Integer multiply
            if (arg_count == 2 && is_int(args[0]) && is_int(args[1])) {
                return make_int(as_int(args[0]) * as_int(args[1]));
            }
            break;
            
        case 4: // Integer divide
            if (arg_count == 2 && is_int(args[0]) && is_int(args[1]) && as_int(args[1]) != 0) {
                return make_int(as_int(args[0]) / as_int(args[1]));
            }
            break;
            
        case 5: // Integer modulo
            if (arg_count == 2 && is_int(args[0]) && is_int(args[1]) && as_int(args[1]) != 0) {
                return make_int(as_int(args[0]) % as_int(args[1]));
            }
            break;
            
        case 6: // Integer equality
            if (arg_count == 2 && is_int(args[0]) && is_int(args[1])) {
                return as_int(args[0]) == as_int(args[1]) ? vm->true_obj : vm->false_obj;
            }
            break;
            
        case 7: // Integer less than
            if (arg_count == 2 && is_int(args[0]) && is_int(args[1])) {
                return as_int(args[0]) < as_int(args[1]) ? vm->true_obj : vm->false_obj;
            }
            break;
            
        case 8: // Object equality
            if (arg_count == 2) {
                return value_equals(args[0], args[1]) ? vm->true_obj : vm->false_obj;
            }
            break;
            
        case 9: // Object class
            if (arg_count == 1) {
                if (is_int(args[0])) {
                    return vm->class_Integer;
                } else if (is_special(args[0])) {
                    if (is_nil(args[0])) return vm->nil;
                    if (is_true(args[0])) return vm->true_obj;
                    if (is_false(args[0])) return vm->false_obj;
                } else {
                    return as_object(args[0])->class;
                }
            }
            break;
            
        case 10: // String concatenation
            if (arg_count == 2 && is_object(args[0]) && is_object(args[1])) {
                return string_concat(args[0], args[1]);
            }
            break;
            
        case 11: // Array at
            if (arg_count == 2 && is_object(args[0]) && is_int(args[1])) {
                Object* array = as_object(args[0]);
                uint16_t index = as_int(args[1]);
                
                if (array->flags & FLAG_ARRAY && index < array->size) {
                    return array->fields[index];
                }
            }
            break;
            
        case 12: // Array at put
            if (arg_count == 3 && is_object(args[0]) && is_int(args[1])) {
                Object* array = as_object(args[0]);
                uint16_t index = as_int(args[1]);
                
                if (array->flags & FLAG_ARRAY && index < array->size) {
                    array->fields[index] = args[2];
                    return args[2];
                }
            }
            break;
            
        case 13: // Array size
            if (arg_count == 1 && is_object(args[0])) {
                Object* array = as_object(args[0]);
                
                if (array->flags & FLAG_ARRAY) {
                    return make_int(array->size);
                }
            }
            break;
            
        case 14: // String size
            if (arg_count == 1 && is_object(args[0])) {
                Object* string = as_object(args[0]);
                
                if (string->class.bits == vm->class_String.bits) {
                    return string->fields[0]; // Length stored in first field
                }
            }
            break;
            
        case 15: // Print
            if (arg_count == 1) {
                if (is_object(args[0]) && as_object(args[0])->class.bits == vm->class_String.bits) {
                    printf("%s", string_to_cstring(args[0]));
                } else {
                    value_print(args[0]);
                }
                return vm->nil;
            }
            break;
            
        case 16: // Println
            if (arg_count == 1) {
                if (is_object(args[0]) && as_object(args[0])->class.bits == vm->class_String.bits) {
                    printf("%s\n", string_to_cstring(args[0]));
                } else {
                    value_print(args[0]);
                    printf("\n");
                }
                return vm->nil;
            }
            break;
            
        // Agon-specific primitives
        case 100: // VDP draw pixel
            if (arg_count == 3 && is_int(args[0]) && is_int(args[1]) && is_int(args[2])) {
                int x = as_int(args[0]);
                int y = as_int(args[1]);
                int color = as_int(args[2]);
                // Call Agon VDP function (to be implemented)
                return vm->nil;
            }
            break;
            
        case 101: // VDP draw line
            if (arg_count == 5 && 
                is_int(args[0]) && is_int(args[1]) && 
                is_int(args[2]) && is_int(args[3]) && 
                is_int(args[4])) {
                int x1 = as_int(args[0]);
                int y1 = as_int(args[1]);
                int x2 = as_int(args[2]);
                int y2 = as_int(args[3]);
                int color = as_int(args[4]);
                // Call Agon VDP function (to be implemented)
                return vm->nil;
            }
            break;
            
        case 102: // VDP clear screen
            if (arg_count == 1 && is_int(args[0])) {
                int color = as_int(args[0]);
                // Call Agon VDP function (to be implemented)
                return vm->nil;
            }
            break;
            
        case 103: // Read keyboard input
            if (arg_count == 0) {
                // Call Agon keyboard function (to be implemented)
                // For now, return a dummy value
                return make_int(0);
            }
            break;
            
        case 104: // File open
            if (arg_count == 2 && 
                is_object(args[0]) && 
                is_object(args[1]) && 
                as_object(args[0])->class.bits == vm->class_String.bits &&
                as_object(args[1])->class.bits == vm->class_String.bits) {
                const char* filename = string_to_cstring(args[0]);
                const char* mode = string_to_cstring(args[1]);
                // Call Agon file function (to be implemented)
                return make_int(0); // Return file handle
            }
            break;
            
        default:
            vm_error("Unknown primitive: %d", primitive_id);
    }
    
    // If we reach here, primitive failed
    vm_error("Primitive failed: %d", primitive_id);
    return vm->nil;
}
