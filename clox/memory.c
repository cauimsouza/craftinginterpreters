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
    // During GC cycles, DecrementRefcountObject() may be called on objects with refcount == 0.
    if (obj->refcount == 0) {
        return;
    }
    
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

void MarkObj(Obj *obj) {
    if (obj->marked) {
        return;
    }
    
    if (vm.grey_count >= vm.grey_capacity) {
        vm.grey_capacity = GROW_CAPACITY(vm.grey_capacity);
        vm.grey_objects = (Obj**) realloc(vm.grey_objects, vm.grey_capacity);
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
    
    MarkTable(&vm.strings); // TODO: These are weak references
    MarkTable(&vm.globals);
    
    for (ObjUpvalue *upvalue = vm.open_upvalues; upvalue != NULL; upvalue = upvalue->next) {
        MarkObj((Obj*) upvalue);
    }
    
    MarkCompilerRoots();
}

static void blackify(Obj *obj) {
    switch (obj->type) {
        case OBJ_STRING:
            break;
        case OBJ_FUNCTION: {
            ObjFunction *function = (ObjFunction*) obj;
            MarkObj((Obj*) function->name);
            MarkChunk(&function->chunk);
            break;
        }
        case OBJ_NATIVE:
            break;
        case OBJ_CLOSURE: {
            ObjClosure *closure = (ObjClosure*) obj;
            MarkObj((Obj*) closure->function);
            for (int i = 0; i < closure->upvalue_count; i++) {
                MarkObj((Obj*) closure->upvalues[i]);
            }
            break;
        }
        case OBJ_UPVALUE: {
            ObjUpvalue *upvalue = (ObjUpvalue*) obj;
            MarkValue(*upvalue->location);
            break;
        }
        default:
            exit(1);
    }
}

static void markReachable() {
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
    
    markReachable();
    
    sweep();
}
