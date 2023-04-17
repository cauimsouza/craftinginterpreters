#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "vm.h"

Obj *fromString(int length, char *chars) {
    ObjString *obj = ALLOCATE(ObjString, 1);
    obj->obj.type = OBJ_STRING;
    obj->length = length;
    obj->chars = chars;
    
    obj->obj.next = vm.objects;
    vm.objects = &obj->obj;
    
    return (Obj*) obj;
}

Obj *addStrings(ObjString *left_str, ObjString *right_str) {
    int length = left_str->length + right_str ->length;
    char *chars = ALLOCATE(char, length + 1);
    
    char *c = chars; 
    for (size_t i = 0; i < left_str->length; i++) {
       *c++ = left_str->chars[i]; 
    }
    for (size_t i = 0; i < right_str->length; i++) {
       *c++ = right_str->chars[i]; 
    }
    *c = '\0';
    
    return fromString(length, chars);
}

bool objsEqual(Obj *a, Obj *b) {
    if (a->type != b->type) {
        return false;
    }
    if (a->type == OBJ_STRING) {
        return strcmp(((ObjString*)a)->chars, ((ObjString*)b)->chars) == 0;
    }
    return false;
}

void freeObj(Obj *obj) {
    if (obj->type == OBJ_STRING) {
        ObjString *string = (ObjString*)obj;
        FREE_ARRAY(char, string->chars, string->length + 1);
        FREE(ObjString, string);
        return;
    }
}

void printObj(Obj *obj) {
    if (obj->type == OBJ_STRING) {
        ObjString *objs = (ObjString*) obj;
        printf("%s", objs->chars);
    }
}