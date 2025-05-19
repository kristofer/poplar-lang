// object.h - Object model for Poplar2

#ifndef POPLAR2_OBJECT_H
#define POPLAR2_OBJECT_H

#include "vm.h"

// Object creation
Object* object_new(Value class, uint16_t size);
Class* class_new(const char* name, Value superclass, uint16_t instance_size);
Method* method_new(const char* name, uint8_t num_args, uint8_t num_locals);

// Object access
Value object_get_field(Object* object, uint16_t index);
void object_set_field(Object* object, uint16_t index, Value value);

// Class operations
bool class_is_subclass_of(Value class, Value superclass);
Value class_get_name(Value class);
Method* class_lookup_method(Value class, Value selector);

// Symbol table
Value symbol_for(const char* string);
const char* symbol_to_string(Value symbol);

// Array operations
Value array_new(uint16_t size);
Value array_at(Value array, uint16_t index);
void array_at_put(Value array, uint16_t index, Value value);

// String operations
Value string_new(const char* cstring);
const char* string_to_cstring(Value string);
Value string_concat(Value str1, Value str2);

// Object comparison
bool object_equals(Value a, Value b);
uint8_t object_hash(Value obj);

// Object printing
void object_print(Value obj);

#endif /* POPLAR2_OBJECT_H */
