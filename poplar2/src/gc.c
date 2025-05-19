// gc.c - Simple garbage collector implementation for Poplar2

#include "gc.h"
#include "vm.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Simple bump allocator for now
static void* heap_start = NULL;
static void* heap_next = NULL;
static void* heap_end = NULL;

// GC statistics
static size_t total_allocated = 0;
static size_t current_allocated = 0;
static int collection_count = 0;

// Initialize GC
void gc_init() {
    // Allocate a fixed-size heap
    size_t heap_size = HEAP_SIZE;
    heap_start = malloc(heap_size);

    if (heap_start == NULL) {
        fprintf(stderr, "Failed to allocate heap\n");
        exit(1);
    }

    heap_next = heap_start;
    heap_end = (char*)heap_start + heap_size;

    // Store heap pointers in VM
    vm->heap_start = heap_start;
    vm->heap_next = heap_next;
    vm->heap_end = heap_end;

    printf("GC initialized with heap size: %d bytes\n", heap_size);
}

// Simple allocator - just bump the pointer
void* gc_allocate(size_t size) {
    // Align size to 4 bytes
    size = (size + 3) & ~3;

    // Check if we need to collect garbage
    if ((char*)heap_next + size > (char*)heap_end) {
        gc_collect();

        // If still not enough space, allocation fails
        if ((char*)heap_next + size > (char*)heap_end) {
            fprintf(stderr, "Out of memory: cannot allocate %d bytes\n", size);
            exit(1);
        }
    }

    // Allocate memory
    void* result = heap_next;
    heap_next = (char*)heap_next + size;

    // Update statistics
    total_allocated += size;
    current_allocated += size;

    // Update VM heap pointer
    vm->heap_next = heap_next;

    // Clear the allocated memory
    memset(result, 0, size);

    return result;
}

// Mark an object as reachable
void gc_mark_object(void* object) {
    if (object == NULL) return;

    Object* obj = (Object*)object;

    // Already marked
    if (obj->flags & FLAG_GC_MARK) return;

    // Mark the object
    obj->flags |= FLAG_GC_MARK;

    // Mark class
    if (is_object(obj->class)) {
        gc_mark_object(as_object(obj->class));
    }

    // Mark fields
    for (int i = 0; i < obj->size; i++) {
        Value value = obj->fields[i];
        if (is_object(value)) {
            gc_mark_object(as_object(value));
        }
    }
}

// Mark phase - mark all reachable objects
static void gc_mark_roots() {
    // Mark VM roots
    for (int i = 0; i < MAX_GLOBALS; i++) {
        if (is_object(vm->globals[i])) {
            gc_mark_object(as_object(vm->globals[i]));
        }
    }

    for (int i = 0; i < MAX_LITERALS; i++) {
        if (is_object(vm->literals[i])) {
            gc_mark_object(as_object(vm->literals[i]));
        }
    }

    // Mark stack frames
    Frame* frame = vm->current_frame;
    while (frame != NULL) {
        // Mark receiver
        if (is_object(frame->receiver)) {
            gc_mark_object(as_object(frame->receiver));
        }

        // Mark method
        if (frame->method != NULL) {
            gc_mark_object((Object*)frame->method);
        }

        // Mark stack values
        Value* slot = frame->stack;
        while (slot < frame->stack_pointer) {
            if (is_object(*slot)) {
                gc_mark_object(as_object(*slot));
            }
            slot++;
        }

        frame = frame->sender;
    }

    // Mark special values
    if (is_object(vm->nil)) gc_mark_object(as_object(vm->nil));
    if (is_object(vm->true_obj)) gc_mark_object(as_object(vm->true_obj));
    if (is_object(vm->false_obj)) gc_mark_object(as_object(vm->false_obj));

    // Mark core classes
    if (is_object(vm->class_Object)) gc_mark_object(as_object(vm->class_Object));
    if (is_object(vm->class_Class)) gc_mark_object(as_object(vm->class_Class));
    if (is_object(vm->class_Method)) gc_mark_object(as_object(vm->class_Method));
    if (is_object(vm->class_Array)) gc_mark_object(as_object(vm->class_Array));
    if (is_object(vm->class_String)) gc_mark_object(as_object(vm->class_String));
    if (is_object(vm->class_Symbol)) gc_mark_object(as_object(vm->class_Symbol));
    if (is_object(vm->class_Integer)) gc_mark_object(as_object(vm->class_Integer));
    if (is_object(vm->class_Block)) gc_mark_object(as_object(vm->class_Block));
}

// Sweep phase - free unreachable objects
static void gc_sweep() {
    // For a simple bump allocator, we just reset the heap
    // This is a stop-the-world collector that copies all live objects

    // Create a temporary buffer to hold live objects
    void* new_heap = malloc(HEAP_SIZE);
    if (new_heap == NULL) {
        fprintf(stderr, "Failed to allocate temporary heap for GC\n");
        exit(1);
    }

    void* new_next = new_heap;
    size_t new_allocated = 0;

    // Copy all marked objects
    Object* obj = (Object*)heap_start;
    while ((void*)obj < heap_next) {
        size_t size = sizeof(Object) + obj->size * sizeof(Value);
        size = (size + 3) & ~3; // Align to 4 bytes

        if (obj->flags & FLAG_GC_MARK) {
            // Copy the object
            memcpy(new_next, obj, size);

            // Unmark for next collection
            ((Object*)new_next)->flags &= ~FLAG_GC_MARK;

            // Move to next position in new heap
            new_next = (char*)new_next + size;
            new_allocated += size;
        }

        // Move to next object in old heap
        obj = (Object*)((char*)obj + size);
    }

    // Free old heap and use the new one
    free(heap_start);
    heap_start = new_heap;
    heap_next = new_next;
    heap_end = (char*)heap_start + HEAP_SIZE;

    // Update VM heap pointers
    vm->heap_start = heap_start;
    vm->heap_next = heap_next;
    vm->heap_end = heap_end;

    // Update statistics
    current_allocated = new_allocated;
}

// Run a full garbage collection cycle
void gc_collect() {
    size_t before = current_allocated;

    // Mark phase
    gc_mark_roots();

    // Sweep phase
    gc_sweep();

    // Update statistics
    collection_count++;
    vm->gc_count = collection_count;
    vm->allocated = current_allocated;

    printf("GC #%d: collected %zu bytes (from %zu to %zu) next: %p\n",
           collection_count,
           before - current_allocated,
           before,
           current_allocated,
           heap_next);
}

// Clean up GC resources
void gc_cleanup() {
    if (heap_start != NULL) {
        free(heap_start);
        heap_start = NULL;
        heap_next = NULL;
        heap_end = NULL;
    }
}
