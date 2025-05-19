// value.h - Value type and operations for Poplar2

#ifndef POPLAR2_VALUE_H
#define POPLAR2_VALUE_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct Object Object;
typedef struct Class Class;

// Value tags
#define TAG_INT             0
#define TAG_OBJ             1
#define TAG_SPECIAL         2

// Special values
#define SPECIAL_NIL         0
#define SPECIAL_TRUE        1
#define SPECIAL_FALSE       2

// Tagged value representation
typedef struct {
    union {
        uint32_t bits;
        struct {
            uint32_t tag : 2;    // Tag (INT, OBJ, SPECIAL)
            uint32_t value : 30; // Value or pointer (24-bit address space + 6 bits for value encoding)
        };
    };
} Value;

// Value creation helpers
Value make_int(int16_t value);
Value make_object(Object* obj);
Value make_special(uint8_t special);

// Value testing
bool is_int(Value value);
bool is_object(Value value);
bool is_special(Value value);
bool is_nil(Value value);
bool is_true(Value value);
bool is_false(Value value);

// Value extraction
int16_t as_int(Value value);
Object* as_object(Value value);
uint8_t as_special(Value value);

// Value comparison
bool value_equals(Value a, Value b);
bool value_identical(Value a, Value b);

// Debug print
void value_print(Value value);

#endif /* POPLAR2_VALUE_H */