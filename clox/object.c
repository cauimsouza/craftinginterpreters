#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "vm.h"

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

Obj *Concatenate(const ObjString *left_str, const ObjString *right_str) {
    size_t length = left_str->length + right_str ->length;
    ObjString *obj = ALLOCATE_FAM(ObjString, char, length + 1);
    obj->obj.type = OBJ_STRING;
    obj->length = length;
    
    char *c = obj->chars;
    for (size_t i = 0; i < left_str->length; i++) {
       *c++ = left_str->chars[i]; 
    }
    for (size_t i = 0; i < right_str->length; i++) {
       *c++ = right_str->chars[i]; 
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