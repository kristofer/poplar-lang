// gc.h - Simple garbage collector for Poplar2

#ifndef POPLAR2_GC_H
#define POPLAR2_GC_H

#include <stddef.h>

// Initialize the garbage collector
void gc_init();

// Allocate memory that will be managed by the GC
void* gc_allocate(size_t size);

// Run the garbage collector
void gc_collect();

// Mark an object as reachable (during GC)
void gc_mark_object(void* object);

// Clean up the garbage collector
void gc_cleanup();

#endif /* POPLAR2_GC_H */