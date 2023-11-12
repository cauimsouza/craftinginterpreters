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

#define GC_HEAP_GROWTH_FACTOR 2

void *Reallocate(void *pointer, size_t old_size, size_t new_size) {
    #ifdef DEBUG_LOG_GC
        printf("Reallocate(%p, %d, %d)\n", pointer, old_size, new_size);
    #endif
    
    if (new_size > old_size) {
        // Because we check if new_size > old_size, we will never enter here
        // during a GC cycle.
        
        #ifdef DEBUG_STRESS_GC
            CollectGarbage();
        #else
            if (vm.bytes_allocated > vm.next_gc) {
                CollectGarbage();
            }
        #endif
    }
    
    // We can't simply vm.bytes_allocated += new_size - old_size because
    // new_size - old_size may be negative and size_t is an unsigned type.
    vm.bytes_allocated += new_size;
    vm.bytes_allocated -= old_size;
    
    if (new_size == 0) {
        free(pointer);
        return NULL;
    }
    
    pointer = realloc(pointer, new_size);
    if (pointer == NULL) {
        exit(1);
    }
    
    return pointer;
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
        printf(" (%p) to %d\n", (void*) obj, obj->refcount);
    #endif
}

void DecrementRefcountValue(Value value) {
    if (!IsObj(value)) {
        return;
    }
    
    DecrementRefcountObject(value.as.obj);
}

void DecrementRefcountObject(Obj *obj) {
    // During GC cycles, DecrementRefcountObject() may be called on objects with refcount == 0.
    if (obj->refcount == 0) {
        return;
    }
    
    obj->refcount--;
    
    #ifdef DEBUG_LOG_GC
        printf("Decrement refcount of ");
        PrintObj(obj);
        printf(" (%p) to %d\n", (void*) obj, obj->refcount);
    #endif
    
    if (obj->refcount == 0) {
        FreeObj(obj);
    }
}

void MarkObj(Obj *obj) {
    if (obj->marked) {
        return;
    }
    
    #ifdef DEBUG_LOG_GC
        printf("mark ");
        PrintObj(obj);
        printf(" (%p)\n", (void*) obj);
    #endif
    
    if (vm.grey_count >= vm.grey_capacity) {
        vm.grey_capacity = GROW_CAPACITY(vm.grey_capacity);
        
        #ifdef DEBUG_LOG_GC
            printf("MarkObj call to realloc(%p, %d)\n", (void*) vm.grey_objects, vm.grey_capacity);
        #endif
        
        vm.grey_objects = (Obj**) realloc(vm.grey_objects, vm.grey_capacity * sizeof(Obj*));
        if (vm.grey_objects == NULL) {
            exit(1);
        }
    }
    
    obj->marked = true;
    vm.grey_objects[vm.grey_count++] = obj;
}

void MarkValue(Value value) {
    if (!IsObj(value)) {
        return;
    }
    MarkObj(value.as.obj);
}

static void markRoots() {
    for (int i = 0; i < vm.frame_count; i++) {
        MarkObj((Obj*) vm.frames[i].closure);
    }
    
    for (Value *value = vm.stack; value != vm.stack_top; value++) {
        MarkValue(*value);
    }
    
    MarkTable(&vm.globals);
    
    for (ObjUpvalue *upvalue = vm.open_upvalues; upvalue != NULL; upvalue = upvalue->next) {
        MarkObj((Obj*) upvalue);
    }
    
    MarkCompilerRoots();
}

static void blackify(Obj *obj) {
    #ifdef DEBUG_LOG_GC
        printf("blackify ");
        PrintObj(obj);
        printf(" (%p)\n", (void*) obj);
    #endif
    
    switch (obj->type) {
        case OBJ_STRING:
            break;
        case OBJ_FUNCTION: {
            ObjFunction *function = (ObjFunction*) obj;
            if (function->name != NULL) {
                MarkObj((Obj*) function->name);
            }
            MarkChunk(&function->chunk);
            break;
        }
        case OBJ_NATIVE:
            break;
        case OBJ_CLOSURE: {
            ObjClosure *closure = (ObjClosure*) obj;
            MarkObj((Obj*) closure->function);
            for (int i = 0; i < closure->upvalue_count; i++) {
                Obj *upvalue = (Obj*) closure->upvalues[i];
                if (upvalue != NULL) {
                    // The GC may run *while* we construct the upvalues.
                    MarkObj((Obj*) closure->upvalues[i]);
                }
            }
            break;
        }
        case OBJ_UPVALUE: {
            ObjUpvalue *upvalue = (ObjUpvalue*) obj;
            MarkValue(*upvalue->location);
            break;
        }
        case OBJ_CLASS: {
            ObjClass *class = (ObjClass*) obj;
            MarkObj((Obj*) class->name);
            MarkTable(&class->methods);
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance *instance = (ObjInstance*) obj;
            MarkObj((Obj*) instance->class);
            MarkTable(&instance->fields);
            break;
        }
        case OBJ_BOUND_METHOD: {
            ObjBoundMethod *bound_method = (ObjBoundMethod*) obj; 
            MarkValue(bound_method->receiver);
            MarkObj((Obj*) bound_method->method);
            break;
        }
        default:
            exit(1);
    }
}

static void trace() {
    while (vm.grey_count > 0) {
        Obj *obj = vm.grey_objects[--vm.grey_count];
        blackify(obj);
    }
}

static void sweep() {
    Obj *tail = NULL;
    
    Obj *obj = vm.objects;
    while (obj != NULL) {
        if (!obj->marked) {
            // FreObj() fixes the list vm.objects
            FreeObj(obj);
            
            if (tail == NULL) {
                obj = vm.objects;
            } else {
                obj = tail->next;
            }
            continue;
        }
        
        obj->marked = false;
        obj->prev = tail;
        
        if (tail != NULL) {
            tail->next = obj;
        }
        tail = obj;
        
        obj = tail->next;
    }
}

void CollectGarbage() {
    markRoots();
    trace();
    RemoveUnmarkedKeys(&vm.strings);
    sweep();
    
    vm.next_gc = vm.bytes_allocated * GC_HEAP_GROWTH_FACTOR;
}
