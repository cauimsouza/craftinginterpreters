#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"
#include "object.h"
#include "value.h"

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)
    
#define GROW_ARRAY(type, pointer, old_count, new_count) \
    (type*)Reallocate(pointer, sizeof(type) * (old_count), \
        sizeof(type) * (new_count))
        
#define FREE_ARRAY(type, pointer, old_count) \
    GROW_ARRAY(type, pointer, old_count, 0);
    
#define FREE(type, pointer) \
    Reallocate(pointer, sizeof(type), 0);

#define ALLOCATE(type, count) \
    (type*) Reallocate(NULL, 0, sizeof(type) * (count))
    
// Same as ALLOCATE but for objects with flexible array members.
#define ALLOCATE_FAM(obj_type, ar_type, count) \
    (obj_type*) Reallocate(NULL, 0, sizeof(obj_type) + (count) * sizeof(ar_type))
        
void* Reallocate(void *pointer, size_t old_size, size_t new_size);

// GC related functions
void CollectGarbage();
void MarkObject(Obj *obj);
void MarkValue(Value value);

#endif