#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "vm.h"

typedef struct {
    Obj obj;
    size_t length;
    // Flexible array member: https://www.wikiwand.com/en/Flexible_array_member
    char chars[];
} ObjString;

Obj *FromString(const char *chars, size_t length) {
    ObjString *obj = ALLOCATE_FAM(ObjString, char, length + 1);
    obj->obj.type = OBJ_STRING;
    obj->length = length;
    
    for (size_t i = 0; i < length; i++) {
       obj->chars[i] = chars[i];
    }
    obj->chars[length] = '\0';
    
    return (Obj*) obj;
}

Obj *Concatenate(const Obj *left_string, const Obj *right_string) {
    ObjString *left = (ObjString*) left_string;
    ObjString *right = (ObjString*) right_string;
    
    size_t length = left->length + right ->length;
    ObjString *obj = ALLOCATE_FAM(ObjString, char, length + 1);
    obj->obj.type = OBJ_STRING;
    obj->length = length;
    
    char *c = obj->chars;
    for (size_t i = 0; i < left->length; i++) {
       *c++ = left->chars[i]; 
    }
    for (size_t i = 0; i < right->length; i++) {
       *c++ = right->chars[i]; 
    }
    *c = '\0';
    
    return (Obj*) obj;
}

bool ObjsEqual(const Obj *a, const Obj *b) {
    if (a->type != b->type) {
        return false;
    }
    if (a->type == OBJ_STRING) {
        return strcmp(((ObjString*)a)->chars, ((ObjString*)b)->chars) == 0;
    }
    return false;
}

void FreeObj(Obj *obj) {
    if (obj->type == OBJ_STRING) {
        ObjString *string = (ObjString*) obj;
        FREE_ARRAY(char, string->chars, string->length + 1);
        FREE(ObjString, string);
        return;
    }
}

void PrintObj(const Obj *obj) {
    if (obj->type == OBJ_STRING) {
        ObjString *objs = (ObjString*) obj;
        printf("%s", objs->chars);
    }
}