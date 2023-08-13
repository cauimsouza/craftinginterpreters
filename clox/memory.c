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

#define GC_HEAP_GROW_FACTOR 2

void *Reallocate(void *pointer, size_t old_size, size_t new_size) {
    vm.bytes_allocated += new_size - old_size;
    
    if (new_size > old_size) {
        #ifdef DEBUG_STRESS_GC
        CollectGarbage();
        #else
        if (vm.bytes_allocated > vm.next_gc) {
            CollectGarbage();
        }
        #endif
    }
    
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

void MarkObject(Obj *obj) {
    if (obj == NULL || obj->marked) {
        return;
    }
    
    #ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*) obj);
    PrintValue(FromObj(obj));
    printf("\n");
    #endif

    obj->marked = true;
    
    if (vm.gray_capacity == vm.gray_count) {
        vm.gray_capacity = GROW_CAPACITY(vm.gray_capacity);
        // Use the system realloc function so that the GC doesn't interfere with the GC data structure :)
        vm.gray_stack = (Obj**) realloc(vm.gray_stack, sizeof(Obj*) * vm.gray_capacity);
        if (vm.gray_stack == NULL) {
            exit(1);
        }
    }
    vm.gray_stack[vm.gray_count++] = obj;
}

void MarkValue(Value value) {
    if (IsObj(value)) {
        MarkObject((Obj*) value.as.obj);
    }
}

static void markValueArray(ValueArray *array) {
    for (int i = 0; i < array->count; i++) {
        MarkValue(array->values[i]);
    }
}

static void blackendObject(Obj *obj) {
    #ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*) obj);
    PrintValue(FromObj(obj));
    printf("\n");
    #endif

    switch (obj->type) {
        case OBJ_NATIVE:
        case OBJ_STRING:
            break;
        case OBJ_UPVALUE:
            MarkValue(((ObjUpvalue*) obj)->closed);
            break;
        case OBJ_FUNCTION: {
            ObjFunction *function = (ObjFunction*) obj;
            MarkObject((Obj*) function->name);
            markValueArray(&function->chunk.constants);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure *closure = (ObjClosure*) obj;
            MarkObject((Obj*) closure->function);
            for (int i = 0; i < closure->upvalue_count; i++) {
                MarkObject((Obj*) closure->upvalues[i]);
            }
            break;
        }
    }
}

static void markRoots() {
    for (Value *slot = vm.stack; slot < vm.stack_top; slot++) {
        MarkValue(*slot);
    }
    
    for (size_t i = 0; i < vm.frame_count; i++) {
        MarkObject((Obj*) vm.frames[i].closure); 
    }
    
    for (ObjUpvalue *upvalue = vm.open_upvalues; upvalue != NULL; upvalue = upvalue->next) {
        MarkObject((Obj*) upvalue);
    }
    
    MarkTable(&vm.globals);
    
    MarkCompilerRoots();
}

static void traceReferences() {
    while (vm.gray_count > 0) {
        Obj *obj = vm.gray_stack[--vm.gray_count];
        blackendObject(obj);
    } 
}

static void sweep() {
    Obj *head = NULL;
    Obj *tail = NULL;
    
    for (Obj *obj = vm.objects; obj != NULL;) {
        if (obj->marked) {
            obj->marked = false;
            if (tail) {
                tail->next = obj;
                tail = tail->next;
            } else {
                head = tail = obj; 
            }
            obj = obj->next;
            continue;
        }
        
        Obj *temp = obj;
        obj = obj->next;
        FreeObj(temp);
    }
    if (tail) {
        tail->next = NULL;
    }
    
    vm.objects = head;
}

void CollectGarbage() {
    #ifdef  DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = vm.bytes_allocated;
    #endif

    markRoots();
    traceReferences();
    RemoveUnmarkedTableEntries(&vm.strings);
    sweep();
    
    vm.next_gc = vm.bytes_allocated * GC_HEAP_GROW_FACTOR;

    #ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    printf("   collected %zu bytes (from %zu to %zu)", before - vm.bytes_allocated, before, vm.bytes_allocated);
    
    #ifndef DEBUG_STRESS_GC
    printf(", next at %zu", vm.next_gc);
    #endif
    
    printf("\n");
        
    #endif
}