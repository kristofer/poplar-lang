// gc.c - Garbage collector for Poplar2

#include "gc.h"
#include "object.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration for marking objects
static void gc_mark_value(Value value);

// Initialize the garbage collector
void gc_init() {
    // Allocate heap memory
    vm->heap_start = malloc(HEAP_SIZE);
    
    if (vm->heap_start == NULL) {
        vm_error("Failed to allocate heap memory");
        exit(1);
    }
    
    // Set initial heap pointers
    vm->heap_next = vm->heap_start;
    vm->heap_end = (char*)vm->heap_start + HEAP_SIZE;
    
    // Initialize statistics
    vm->gc_count = 0;
    vm->allocated = 0;
}

// Run garbage collection
void gc_collect() {
    // Increment GC counter
    vm->gc_count++;
    
    // Mark phase
    gc_mark_roots();
    
    // Sweep phase
    gc_sweep();
}

// Allocate object on the heap
Object* gc_allocate(uint16_t size_in_bytes) {
    // Ensure we have enough space
    if ((char*)vm->heap_next + size_in_bytes > (char*)vm->heap_end) {
        // Run garbage collection
        gc_collect();
        
        // Check if we have enough space now
        if ((char*)vm->heap_next + size_in_bytes > (char*)vm->heap_end) {
            vm_error("Out of memory: request for %d bytes", size_in_bytes);
            exit(1);
        }
    }
    
    // Allocate memory
    void* memory = vm->heap_next;
    vm->heap_next = (char*)vm->heap_next + size_in_bytes;
    
    // Clear memory
    memset(memory, 0, size_in_bytes);
    
    // Update statistics
    vm->allocated += size_in_bytes;
    
    return (Object*)memory;
}

// Mark phase roots
void gc_mark_roots() {
    // Mark global variables
    for (int i = 0; i < MAX_GLOBALS; i++) {
        gc_mark_value(vm->globals[i]);
    }
    
    // Mark literals
    for (int i = 0; i < MAX_LITERALS; i++) {
        gc_mark_value(vm->literals[i]);
    }
    
    // Mark all frames and their contents
    Frame* frame = vm->current_frame;
    while (frame != NULL) {
        // Mark receiver
        gc_mark_value(frame->receiver);
        
        // Mark all values on the stack
        for (Value* slot = frame->stack; slot < frame->stack_pointer; slot++) {
            gc_mark_value(*slot);
        }
        
        // Mark method
        gc_mark_object(make_object((Object*)frame->method));
        
        // Move to sender frame
        frame = frame->sender;
    }
    
    // Mark core classes
    gc_mark_value(vm->class_Object);
    gc_mark_value(vm->class_Class);
    gc_mark_value(vm->class_Method);
    gc_mark_value(vm->class_Array);
    gc_mark_value(vm->class_String);
    gc_mark_value(vm->class_Symbol);
    gc_mark_value(vm->class_Integer);
    gc_mark_value(vm->class_Block);
    
    // Mark special constants
    gc_mark_value(vm->nil);
    gc_mark_value(vm->true_obj);
    gc_mark_value(vm->false_obj);
}

// Mark an object as reachable
void gc_mark_object(Value value) {
    if (!is_object(value)) {
        return; // Not an object pointer
    }
    
    Object* object = as_object(value);
    
    // If already marked, do nothing
    if (object->flags & FLAG_GC_MARK) {
        return;
    }
    
    // Mark the object
    object->flags |= FLAG_GC_MARK;
    
    // Mark class
    gc_mark_value(object->class);
    
    // Mark fields
    for (int i = 0; i < object->size; i++) {
        gc_mark_value(object->fields[i]);
    }
}

// Mark a value (handles all value types)
static void gc_mark_value(Value value) {
    if (is_object(value)) {
        gc_mark_object(value);
    }
    // Nothing to do for INT or SPECIAL values
}

// Sweep phase
void gc_sweep() {
    // Reset heap
    void* new_next = vm->heap_start;
    
    // Iterate over all objects in the heap
    Object* object = (Object*)vm->heap_start;
    
    while ((void*)object < vm->heap_next) {
        // Calculate object size in bytes
        size_t size = sizeof(Object) + (object->size * sizeof(Value));
        
        // Check if object is marked
        if (object->flags & FLAG_GC_MARK) {
            // Clear mark for next GC
            object->flags &= ~FLAG_GC_MARK;
            
            // If object needs to be moved
            if ((void*)object != new_next) {
                // Move object to new location
                memmove(new_next, object, size);
                
                // Update pointers (would require a more complex solution in a real VM)
                // This is a simplified approach that won't work in a real system
                // A proper implementation would need to update all pointers to this object
            }
            
            // Update new_next
            new_next = (char*)new_next + size;
        }
        // If not marked, object is garbage and will be overwritten
        
        // Move to next object
        object = (Object*)((char*)object + size);
    }
    
    // Update heap_next
    vm->heap_next = new_next;
    
    // Update allocated memory
    vm->allocated = (char*)vm->heap_next - (char*)vm->heap_start;
}

// Get free memory
uint32_t gc_get_free_memory() {
    return (uint32_t)((char*)vm->heap_end - (char*)vm->heap_next);
}

// Get used memory
uint32_t gc_get_used_memory() {
    return (uint32_t)((char*)vm->heap_next - (char*)vm->heap_start);
}

// Get GC collection count
uint32_t gc_get_collection_count() {
    return vm->gc_count;
}
