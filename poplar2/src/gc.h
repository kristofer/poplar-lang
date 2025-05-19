// gc.h - Garbage collector for Poplar2

#ifndef POPLAR2_GC_H
#define POPLAR2_GC_H

#include "vm.h"

// Initialize garbage collector
void gc_init();

// Run garbage collection
void gc_collect();

// Allocate object on the heap
Object* gc_allocate(uint16_t size_in_bytes);

// Mark phase
void gc_mark_roots();
void gc_mark_object(Value value);

// Sweep phase
void gc_sweep();

// Statistics
uint32_t gc_get_free_memory();
uint32_t gc_get_used_memory();
uint32_t gc_get_collection_count();

#endif /* POPLAR2_GC_H */
