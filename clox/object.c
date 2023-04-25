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

Obj *FromString(const char *chars, size_t length) {
    ObjString *obj = ALLOCATE_FAM(ObjString, char, length + 1);
    obj->obj.type = OBJ_STRING;
    obj->obj.next = vm.objects;
    vm.objects = &obj->obj;
    obj->length = length;
    obj->hash = hashString(chars, length);
    
    for (size_t i = 0; i < length; i++) {
       obj->chars[i] = chars[i];
    }
    obj->chars[length] = '\0';
    
    
    ObjString *interned = Intern(&vm.strings, obj);
    if (interned != NULL) {
        vm.objects = obj->obj.next;
        FreeObj((Obj*) obj);
        return (Obj*) interned;
    }
    Insert(&vm.strings, obj, FromNil());
    
    return (Obj*) obj;
}

Obj *Concatenate(const Obj *left_string, const Obj *right_string) {
    ObjString *left = (ObjString*) left_string;
    ObjString *right = (ObjString*) right_string;
    
    size_t length = left->length + right ->length;
    ObjString *obj = ALLOCATE_FAM(ObjString, char, length + 1);
    obj->obj.type = OBJ_STRING;
    obj->obj.next = vm.objects;
    vm.objects = &obj->obj;
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
        vm.objects = obj->obj.next;
        FreeObj((Obj*) obj);
        return (Obj*) interned;
    }
    Insert(&vm.strings, obj, FromNil());
    
    return (Obj*) obj;
}

bool ObjsEqual(const Obj *a, const Obj *b) {
    if (a->type != b->type) {
        return false;
    }
    if (a->type == OBJ_STRING) {
        return a == b;
    }
    return false;
}

void FreeObj(Obj *obj) {
    if (obj->type == OBJ_STRING) {
        FREE(ObjString, (ObjString*) obj);
        return;
    }
}

void PrintObj(const Obj *obj) {
    if (obj->type == OBJ_STRING) {
        ObjString *objs = (ObjString*) obj;
        printf("%s", objs->chars);
    }
}