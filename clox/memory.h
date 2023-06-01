#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)
    
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)Reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))
        
#define FREE_ARRAY(type, pointer, oldCount) \
    GROW_ARRAY(type, pointer, oldCount, 0);
    
#define FREE(type, pointer) \
    Reallocate(pointer, sizeof(type), 0);

#define ALLOCATE(type, count) \
    (type*) Reallocate(NULL, 0, sizeof(type) * (count))
    
// Same as ALLOCATE but for objects with flexible array members.
#define ALLOCATE_FAM(obj_type, ar_type, count) \
    (obj_type*) Reallocate(NULL, 0, sizeof(obj_type) + (count) * sizeof(ar_type))
        
void* Reallocate(void *pointer, size_t oldSize, size_t newSize);

#endif