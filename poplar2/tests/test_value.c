// test_value.c - Test program for value.h operations

#include <stdio.h>
#include "value.h"

// Mock object structure for testing
typedef struct {
    Value class;
    uint8_t hash;
    uint8_t flags;
    uint16_t size;
} TestObject;

int main() {
    printf("Poplar2 Value System Test\n");
    printf("=========================\n\n");
    
    // Test integer values
    printf("Testing integer values:\n");
    Value v1 = make_int(42);
    Value v2 = make_int(-42);
    Value v3 = make_int(0);
    
    printf("Integer 42: "); value_print(v1); printf("\n");
    printf("Integer -42: "); value_print(v2); printf("\n");
    printf("Integer 0: "); value_print(v3); printf("\n");
    
    printf("Is v1 an int? %s\n", is_int(v1) ? "yes" : "no");
    printf("As int: %d\n\n", as_int(v1));
    
    // Test special values
    printf("Testing special values:\n");
    Value nil = make_special(SPECIAL_NIL);
    Value true_val = make_special(SPECIAL_TRUE);
    Value false_val = make_special(SPECIAL_FALSE);
    
    printf("nil: "); value_print(nil); printf("\n");
    printf("true: "); value_print(true_val); printf("\n");
    printf("false: "); value_print(false_val); printf("\n");
    
    printf("Is nil special? %s\n", is_special(nil) ? "yes" : "no");
    printf("Is true true? %s\n", is_true(true_val) ? "yes" : "no");
    printf("Is false false? %s\n\n", is_false(false_val) ? "yes" : "no");
    
    // Test object values
    printf("Testing object values:\n");
    TestObject obj = {0};
    obj.hash = 123;
    obj.flags = 7;
    obj.size = 16;
    
    Value obj_val = make_object((Object*)&obj);
    printf("Object: "); value_print(obj_val); printf("\n");
    printf("Is object? %s\n", is_object(obj_val) ? "yes" : "no");
    
    Object* extracted_obj = as_object(obj_val);
    printf("Object hash: %d\n", extracted_obj->hash);
    printf("Object flags: %d\n", extracted_obj->flags);
    printf("Object size: %d\n\n", extracted_obj->size);
    
    // Test value comparison
    printf("Testing value comparison:\n");
    Value v4 = make_int(42);
    printf("v1 equals v4 (same int)? %s\n", value_equals(v1, v4) ? "yes" : "no");
    printf("v1 equals v2 (different int)? %s\n", value_equals(v1, v2) ? "yes" : "no");
    printf("v1 equals nil (different types)? %s\n", value_equals(v1, nil) ? "yes" : "no");
    
    printf("true equals true? %s\n", value_equals(true_val, true_val) ? "yes" : "no");
    printf("true equals false? %s\n", value_equals(true_val, false_val) ? "yes" : "no");
    
    TestObject obj2 = {0};
    Value obj_val2 = make_object((Object*)&obj2);
    printf("obj1 equals obj2 (different objects)? %s\n", value_equals(obj_val, obj_val2) ? "yes" : "no");
    
    Value obj_val_same = make_object((Object*)&obj);
    printf("obj1 equals itself (same object)? %s\n", value_equals(obj_val, obj_val_same) ? "yes" : "no");
    
    printf("\nTests completed successfully!\n");
    return 0;
}