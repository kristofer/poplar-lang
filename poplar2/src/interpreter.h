// interpreter.h - Bytecode interpreter for Poplar2

#ifndef POPLAR2_INTERPRETER_H
#define POPLAR2_INTERPRETER_H

#include "vm.h"
#include "object.h"

// Interpreter initialization
void interpreter_init();

// Execute a method
Value interpreter_execute_method(Method* method, Value receiver, Value* arguments, int arg_count);

// Frame operations
void interpreter_push_frame(Method* method, Value receiver);
void interpreter_pop_frame();

// Stack operations
void interpreter_push(Value value);
Value interpreter_pop();
Value interpreter_peek();

// Handle bytecode instruction
void interpreter_handle_bytecode(uint8_t bytecode);

// Dispatch message send
Value interpreter_send(Value receiver, Value selector, int arg_count, Value* args);
Value interpreter_super_send(Value selector, int arg_count, Value* args);

// Primitive handling
Value interpreter_primitive(uint8_t primitive_id, Value* args, int arg_count);

#endif /* POPLAR2_INTERPRETER_H */
