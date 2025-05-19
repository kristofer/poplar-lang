// value.c - Implementation of Value type and operations for Poplar2

#include "value.h"
#include <stdio.h>

// Value creation helpers
Value make_int(int16_t value) {
    Value v;
    v.tag = TAG_INT;
    v.value = (uint32_t)(value & 0x7FFF); // Keep only 15 bits for signed int
    // Set the sign bit separately
    if (value < 0) {
        v.value |= 0x8000; // Set the 16th bit as sign bit
    }
    return v;
}

Value make_object(Object* obj) {
    Value v;
    v.tag = TAG_OBJ;
    v.value = (uint32_t)obj;
    return v;
}

Value make_special(uint8_t special) {
    Value v;
    v.tag = TAG_SPECIAL;
    v.value = special;
    return v;
}

// Value testing
bool is_int(Value value) {
    return value.tag == TAG_INT;
}

bool is_object(Value value) {
    return value.tag == TAG_OBJ;
}

bool is_special(Value value) {
    return value.tag == TAG_SPECIAL;
}

bool is_nil(Value value) {
    return is_special(value) && as_special(value) == SPECIAL_NIL;
}

bool is_true(Value value) {
    return is_special
(value) && as_special(value) == SPECIAL_TRUE;
}

bool is_false(Value value) {
    return is_special(value) && as_special(value) == SPECIAL_FALSE;
}

// Value extraction
int16_t as_int(Value value) {
    if (!is_int(value)) {
        // Handle error: trying to use non-int as int
        fprintf(stderr, "Error: Trying to extract int from non-int value\n");
        return 0;
    }
    
    // Extract the value, handling the sign bit
    int16_t result = (int16_t)(value.value & 0x7FFF);
    if (value.value & 0x8000) {
        // If sign bit is set, make it negative
        result = -result;
    }
    return result;
}

Object* as_object(Value value) {
    if (!is_object(value)) {
        // Handle error: trying to use non-object as object
        fprintf(stderr, "Error: Trying to extract object from non-object value\n");
        return NULL;
    }
    return (Object*)value.value;
}

uint8_t as_special(Value value) {
    if (!is_special(value)) {
        // Handle error: trying to use non-special as special
        fprintf(stderr, "Error: Trying to extract special from non-special value\n");
        return 0;
    }
    return (uint8_t)value.value;
}

// Value comparison
bool value_equals(Value a, Value b) {
    // First check if they're the same type
    if (a.tag != b.tag) {
        return false;
    }
    
    // Then compare based on type
    switch (a.tag) {
        case TAG_INT:
            return as_int(a) == as_int(b);
        case TAG_OBJ:
            // For objects, this would typically call the equals method
            // For now, default to identity comparison
            return as_object(a) == as_object(b);
        case TAG_SPECIAL:
            return as_special(a) == as_special(b);
        default:
            return false;
    }
}

bool value_identical(Value a, Value b) {
    // Check if they have the same bits
    return a.bits == b.bits;
}

// Debug print
void value_print(Value value) {
    switch (value.tag) {
        case TAG_INT:
            printf("%d", as_int(value));
            break;
        case TAG_OBJ:
            printf("<object:%p>", as_object(value));
            break;
        case TAG_SPECIAL:
            switch (as_special(value)) {
                case SPECIAL_NIL:
                    printf("nil");
                    break;
                case SPECIAL_TRUE:
                    printf("true");
                    break;
                case SPECIAL_FALSE:
                    printf("false");
                    break;
                default:
                    printf("<special:%d>", as_special(value));
            }
            break;
        default:
            printf("<unknown>");
    }
}