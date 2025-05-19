// object.c - Object model implementation for Poplar2

#include "object.h"
#include "gc.h"
#include <string.h>
#include <stdio.h>

// Symbol table (for fast symbol lookup)
typedef struct {
    char* string;
    Value symbol;
} SymbolEntry;

static SymbolEntry symbol_table[256];
static int symbol_count = 0;

// Create a new object
Object* object_new(Value class, uint16_t size) {
    // Calculate total size in bytes
    uint32_t byte_size = sizeof(Object) + size * sizeof(Value);
    
    // Allocate memory
    Object* obj = (Object*)gc_allocate(byte_size);
    
    // Initialize object
    obj->class = class;
    obj->hash = 0; // Will be set when needed
    obj->flags = 0;
    obj->size = size;
    
    // Initialize fields to nil
    for (int i = 0; i < size; i++) {
        obj->fields[i] = vm->nil;
    }
    
    return obj;
}

// Create a new class
Class* class_new(const char* name, Value superclass, uint16_t instance_size) {
    // Create class object
    Object* obj = object_new(vm->class_Class, sizeof(Class) / sizeof(Value));
    Class* class = (Class*)obj;
    
    // Set fields
    class->name = symbol_for(name);
    class->superclass = superclass;
    class->methods = array_new(0);
    class->instance_size = make_int(instance_size);
    
    // Set class flag
    obj->flags |= FLAG_CLASS;
    
    return class;
}

// Create a new method
Method* method_new(const char* name, uint8_t num_args, uint8_t num_locals) {
    // Calculate size for method (fixed fields + bytecode array)
    Object* obj = object_new(vm->class_Method, sizeof(Method) / sizeof(Value));
    Method* method = (Method*)obj;
    
    // Set fields
    method->name = symbol_for(name);
    method->holder = vm->nil; // Will be set when added to a class
    method->num_args = num_args;
    method->num_locals = num_locals;
    method->bytecode_count = 0;
    
    // Set method flag
    obj->flags |= FLAG_METHOD;
    
    return method;
}

// Get a field from an object
Value object_get_field(Object* object, uint16_t index) {
    if (index >= object->size) {
        vm_error("Field index out of bounds: %d (size: %d)", index, object->size);
        return vm->nil;
    }
    return object->fields[index];
}

// Set a field in an object
void object_set_field(Object* object, uint16_t index, Value value) {
    if (index >= object->size) {
        vm_error("Field index out of bounds: %d (size: %d)", index, object->size);
        return;
    }
    object->fields[index] = value;
}

// Check if a class is a subclass of another class
bool class_is_subclass_of(Value class_value, Value superclass_value) {
    if (!is_object(class_value) || !is_object(superclass_value)) {
        return false;
    }
    
    Class* class = (Class*)as_object(class_value);
    Class* superclass = (Class*)as_object(superclass_value);
    
    // Check if it's the same class
    if (class == superclass) {
        return true;
    }
    
    // Follow superclass chain
    Value current = class->superclass;
    while (!is_nil(current)) {
        if (as_object(current) == superclass) {
            return true;
        }
        current = ((Class*)as_object(current))->superclass;
    }
    
    return false;
}

// Get the name of a class
Value class_get_name(Value class_value) {
    if (!is_object(class_value)) {
        return vm->nil;
    }
    
    Class* class = (Class*)as_object(class_value);
    return class->name;
}

// Lookup a method in a class hierarchy
Method* class_lookup_method(Value class_value, Value selector) {
    if (!is_object(class_value) || !is_object(selector)) {
        return NULL;
    }
    
    // Start from current class and walk up the hierarchy
    Value current = class_value;
    while (!is_nil(current)) {
        Class* class = (Class*)as_object(current);
        
        // Check methods in this class
        Value methods = class->methods;
        Object* methods_obj = as_object(methods);
        
        for (int i = 0; i < methods_obj->size; i++) {
            Value method_value = methods_obj->fields[i];
            Method* method = (Method*)as_object(method_value);
            
            if (value_equals(method->name, selector)) {
                return method;
            }
        }
        
        // Move up to superclass
        current = class->superclass;
    }
    
    // Method not found
    return NULL;
}

// Symbol handling
Value symbol_for(const char* string) {
    // First check if the symbol already exists
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].string, string) == 0) {
            return symbol_table[i].symbol;
        }
    }
    
    // Create a new symbol
    Value str = string_new(string);
    Object* str_obj = as_object(str);
    
    // Create symbol object (just a string with the symbol flag)
    Object* symbol_obj = object_new(vm->class_Symbol, str_obj->size);
    
    // Copy string data
    memcpy(symbol_obj->fields, str_obj->fields, str_obj->size * sizeof(Value));
    
    // Set symbol flag
    symbol_obj->flags |= FLAG_SYMBOL;
    
    // Create symbol value
    Value symbol = make_object(symbol_obj);
    
    // Add to symbol table
    if (symbol_count < 256) {
        symbol_table[symbol_count].string = strdup(string);
        symbol_table[symbol_count].symbol = symbol;
        symbol_count++;
    } else {
        vm_error("Symbol table full");
    }
    
    return symbol;
}

// Convert symbol to string
const char* symbol_to_string(Value symbol) {
    if (!is_object(symbol) || !(as_object(symbol)->flags & FLAG_SYMBOL)) {
        vm_error("Expected symbol");
        return "<not a symbol>";
    }
    
    return string_to_cstring(symbol);
}

// Create a new array
Value array_new(uint16_t size) {
    Object* array = object_new(vm->class_Array, size);
    array->flags |= FLAG_ARRAY;
    return make_object(array);
}

// Get array element at index
Value array_at(Value array_value, uint16_t index) {
    if (!is_object(array_value)) {
        vm_error("Expected array");
        return vm->nil;
    }
    
    Object* array = as_object(array_value);
    
    if (!(array->flags & FLAG_ARRAY)) {
        vm_error("Expected array");
        return vm->nil;
    }
    
    if (index >= array->size) {
        vm_error("Array index out of bounds: %d (size: %d)", index, array->size);
        return vm->nil;
    }
    
    return array->fields[index];
}

// Set array element at index
void array_at_put(Value array_value, uint16_t index, Value value) {
    if (!is_object(array_value)) {
        vm_error("Expected array");
        return;
    }
    
    Object* array = as_object(array_value);
    
    if (!(array->flags & FLAG_ARRAY)) {
        vm_error("Expected array");
        return;
    }
    
    if (index >= array->size) {
        vm_error("Array index out of bounds: %d (size: %d)", index, array->size);
        return;
    }
    
    array->fields[index] = value;
}

// Create a new string from a C string
Value string_new(const char* cstring) {
    size_t length = strlen(cstring);
    
    // Calculate how many Value slots we need
    // Each Value can hold 4 bytes (on 32-bit system)
    uint16_t size = (length + 4) / 4;
    
    // Create string object
    Object* string = object_new(vm->class_String, size + 1); // +1 for length
    
    // Store length in first field
    string->fields[0] = make_int(length);
    
    // Copy string data
    memcpy(&string->fields[1], cstring, length);
    
    return make_object(string);
}

// Extract C string from a string object
const char* string_to_cstring(Value string_value) {
    if (!is_object(string_value)) {
        vm_error("Expected string");
        return "<not a string>";
    }
    
    Object* string = as_object(string_value);
    
    // The string data starts after the length field
    return (const char*)&string->fields[1];
}

// Concatenate two strings
Value string_concat(Value str1, Value str2) {
    if (!is_object(str1) || !is_object(str2)) {
        vm_error("Expected strings");
        return vm->nil;
    }
    
    Object* s1 = as_object(str1);
    Object* s2 = as_object(str2);
    
    // Get string lengths
    int len1 = as_int(s1->fields[0]);
    int len2 = as_int(s2->fields[0]);
    
    // Create result string
    char buffer[len1 + len2 + 1];
    memcpy(buffer, &s1->fields[1], len1);
    memcpy(buffer + len1, &s2->fields[1], len2);
    buffer[len1 + len2] = '\0';
    
    return string_new(buffer);
}

// Object equality
bool object_equals(Value a, Value b) {
    return value_equals(a, b);
}

// Calculate object hash
uint8_t object_hash(Value obj) {
    if (is_int(obj)) {
        return (uint8_t)as_int(obj);
    } else if (is_special(obj)) {
        return as_special(obj);
    } else if (is_object(obj)) {
        Object* o = as_object(obj);
        
        // Cache hash if not set
        if (o->hash == 0) {
            if (o->flags & FLAG_SYMBOL || o->class.bits == vm->class_String.bits) {
                const char* str = string_to_cstring(obj);
                uint8_t hash = 0;
                
                while (*str) {
                    hash = (hash * 31) + *str++;
                }
                
                o->hash = hash ? hash : 1; // Ensure non-zero
            } else {
                // Use object address for hash
                uintptr_t addr = (uintptr_t)o;
                o->hash = (addr >> 2) & 0xFF; // Use some bits from the address
                if (o->hash == 0) o->hash = 1; // Ensure non-zero
            }
        }
        
        return o->hash;
    }
    
    return 0;
}

// Print object (for debugging)
void object_print(Value obj) {
    value_print(obj);
}
