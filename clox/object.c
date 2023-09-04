#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "vm.h"

static uint32_t hashString(const char *chars, size_t length) {
    // Implementation of the FNV-1a hash algorithm.
    const uint32_t prime = 16777619u;
    const uint32_t base = 2166136261u;
    
    uint32_t hash = base;
    for (size_t i = 0; i < length; i++) {
        hash ^= (uint8_t) chars[i];
        hash *= prime;
    }
    
    return hash;
}

static Obj *allocateObj(size_t size, ObjType type) {
    Obj *obj = (Obj*) Reallocate(NULL, 0, size);
    obj->type = type;
    obj->refcount = 0;
    obj->marked = false;
    obj->prev = NULL;
    obj->next = vm.objects;
    if (vm.objects != NULL) {
        vm.objects->prev = obj;
    }
    vm.objects = obj;
    
#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*) obj, size, type);
#endif
    
    return obj;
}

#define ALLOCATE_OBJ(type, obj_type) (type*) allocateObj(sizeof(type), obj_type)

Obj *FromString(const char *chars, size_t length) {
    ObjString *obj = ALLOCATE_FAM(ObjString, char, length + 1);
    obj->obj.type = OBJ_STRING;
    obj->obj.refcount = 0;
    obj->obj.marked = false;
    obj->obj.prev = NULL;
    obj->obj.next = vm.objects;
    if (vm.objects != NULL) {
        vm.objects->prev = (Obj*) obj;
    }
    vm.objects = &obj->obj;
    
#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*) obj, length + 1, OBJ_STRING);
#endif

    obj->length = length;
    obj->hash = hashString(chars, length);
    
    for (size_t i = 0; i < length; i++) {
       obj->chars[i] = chars[i];
    }
    obj->chars[length] = '\0';
    
    ObjString *interned = Intern(&vm.strings, obj);
    if (interned != NULL) {
        FreeObj((Obj*) obj);
        return (Obj*) interned;
    }
    Push(FromObj((Obj*) obj)); // Insert() may trigger a GC cycle
    Insert(&vm.strings, obj, FromNil());
    Pop();
    
    return (Obj*) obj;
}

Obj *Concatenate(const Obj *left_string, const Obj *right_string) {
    ObjString *left = (ObjString*) left_string;
    ObjString *right = (ObjString*) right_string;
    
    size_t length = left->length + right ->length;
    ObjString *obj = ALLOCATE_FAM(ObjString, char, length + 1);
    obj->obj.type = OBJ_STRING;
    obj->obj.marked = false;
    obj->obj.refcount = 0;
    obj->obj.prev = NULL;
    obj->obj.next = vm.objects;
    if (vm.objects != NULL) {
        vm.objects->prev = (Obj*) obj;
    }
    vm.objects = &obj->obj;
    
#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*) obj, length + 1, OBJ_STRING);
#endif
    
    obj->length = length;
    
    char *c = obj->chars;
    for (size_t i = 0; i < left->length; i++) {
       *c++ = left->chars[i]; 
    }
    for (size_t i = 0; i < right->length; i++) {
       *c++ = right->chars[i]; 
    }
    *c = '\0';
    
    obj->hash = hashString(obj->chars, obj->length);
    
    ObjString *interned = Intern(&vm.strings, obj);
    if (interned != NULL) {
        FreeObj((Obj*) obj);
        return (Obj*) interned;
    }
    Push(FromObj((Obj*) obj)); // Insert() may trigger a GC cycle
    Insert(&vm.strings, obj, FromNil());
    Pop();
    
    return (Obj*) obj;
}

ObjUpvalue *NewUpvalue(Value *slot) {
    ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->location = slot; IncrementRefcountValue(*slot);
    upvalue->closed = FromNil();
    upvalue->next = NULL;
    
    return upvalue;
}

ObjFunction *NewFunction() {
    ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalue_count = 0;
    InitChunk(&function->chunk);
    function->name = NULL;
    
    return function;
}

ObjClosure *NewClosure(ObjFunction *function) {
    // We initialise the array of upvalues before even creating the closure
    // to please the GC.
    ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue*, function->upvalue_count);
    for (int i = 0; i < function->upvalue_count; i++) {
        upvalues[i] = NULL;
    }
    
    ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function; IncrementRefcountObject((Obj*) function);
    closure->upvalues = upvalues;
    closure->upvalue_count = function->upvalue_count;
    
    return closure;
}

ObjNative *NewNative(NativeFn function, int arity) {
    ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->arity = arity;
    native->function = function;
    
    return native;
}

bool ObjsEqual(const Obj *a, const Obj *b) {
    if (a->type != b->type) {
        return false;
    }
    
    if (a->type == OBJ_STRING) {
        return a == b;
    }
    
    if (a->type == OBJ_NATIVE) {
        return ((ObjNative*) a)->function == ((ObjNative*) b)->function;
    }
    
    return false;
}

void FreeObj(Obj *obj) {
    #ifdef DEBUG_LOG_GC
        printf("freeing ");
        PrintObj(obj);
        printf(" at address %p\n", (void*) obj);
    #endif
    
    // During GC cycles, FreeObj() is called on objects with refcount > 0.
    // Outside GC cycles, FreeObj() should only be called when the object's refcount reaches 0.
    obj->refcount = 0;

    // Removes the object from the vm.objects linked list.
    Obj *prev = obj->prev;
    Obj *next = obj->next;
    if (prev != NULL) {
        prev->next = next;
    }
    if (next != NULL) {
        next->prev = prev;
    }
    if (vm.objects == obj) {
        vm.objects = next;
    }

    switch (obj->type) {
        case OBJ_STRING:
            FREE(ObjString, obj);
            break;
        case OBJ_FUNCTION: {
            ObjFunction *function = (ObjFunction*) obj;
            if (function->name != NULL) {
                // function->name == NULL when function = <script>
                DecrementRefcountObject((Obj*) function->name);
            }
            FreeChunk(&function->chunk);
            FREE(ObjFunction, function);
            break;
        }
        case OBJ_NATIVE:
            FREE(ObjNative, obj);
            break;
        case OBJ_CLOSURE: {
            ObjClosure *closure = (ObjClosure*) obj;
            
            // A closure doesn't own the function it encloses. In fact, there may be
            // many closures for the same function, so we don't free the enclosed function.
            DecrementRefcountObject((Obj*) closure->function);
            
            // We don't free upvalues (multiple closures may close over the same variable),
            // but we do free the array of upvalues.
            for (int i = 0; i < closure->upvalue_count; i++) {
                DecrementRefcountObject((Obj*) closure->upvalues[i]);
            }
            
            FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalue_count);
            FREE(ObjClosure, obj);
            break;
        }
        case OBJ_UPVALUE: {
            ObjUpvalue *upvalue = (ObjUpvalue*) obj;
            
            // Don't free the variable because multiple closures may close over it.
            DecrementRefcountValue(*upvalue->location);
            
            FREE(ObjUpvalue, obj);
            break;
        }
        default:
            printf("Freeing object at %p of invalid object type\n", (void*) obj);
            exit(1);
    }
}

void PrintObj(const Obj *obj) {
    FPrintObj(stdout, obj);
}

void FPrintObj(FILE *stream, const Obj *obj) {
    switch (obj->type) {
        case OBJ_STRING: {
            ObjString *objs = (ObjString*) obj;
            fprintf(stream, "%s", objs->chars);
            break;
        }
        case OBJ_FUNCTION:
        case OBJ_CLOSURE: {
            ObjFunction *function;
            if (obj->type == OBJ_FUNCTION) {
                function = (ObjFunction*) obj;
                fprintf(stream, "<fn ");
            } else {
                function = ((ObjClosure*) obj)->function;
                fprintf(stream, "<closure ");
            }
            
            if (function->name == NULL) {
                fprintf(stream, " script>");
                break;
            }
            fprintf(stream, "%s>", function->name->chars); 
            break;
        }
        case OBJ_NATIVE:
            fprintf(stream, "<native>");
            break;
        case OBJ_UPVALUE:
            printf("upvalue");
            break;
    }
}
