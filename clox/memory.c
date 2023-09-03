#include <stdlib.h>

#include "compiler.h"
#include "memory.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

void *Reallocate(void *pointer, size_t old_size, size_t new_size) {
    if (new_size == 0) {
        free(pointer);
        return NULL;
    }
    
    void* result = realloc(pointer, new_size);
    if (result == NULL) {
        exit(1);
    }
    return result;
}

void IncrementRefcountValue(Value value) {
    if (!IsObj(value)) {
        return;
    }
    
    IncrementRefcountObject(value.as.obj);
}

void IncrementRefcountObject(Obj *obj) {
    obj->refcount++;    
    
    #ifdef DEBUG_LOG_GC
    printf("Increment refcount of ");
    PrintObj(obj);
    printf(" to %d\n", obj->refcount);
    #endif
}

void DecrementRefcountValue(Value value) {
    if (!IsObj(value)) {
        return;
    }
    
    DecrementRefcountObject(value.as.obj);
}

void DecrementRefcountObject(Obj *obj) {
    obj->refcount--;
    
    #ifdef DEBUG_LOG_GC
    printf("Decrement refcount of ");
    PrintObj(obj);
    printf(" to %d\n", obj->refcount);
    #endif
    
    if (obj->refcount == 0) {
        FreeObj(obj);
    }
}
