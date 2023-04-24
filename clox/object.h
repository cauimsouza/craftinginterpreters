#ifndef clox_object_h
#define clox_object_h

#include "common.h"

typedef enum {
    OBJ_STRING
} ObjType;

struct Obj {
   ObjType type; 
   struct Obj *next;
};

typedef struct Obj Obj;

typedef struct {
    Obj obj;
    size_t length;
    uint32_t hash;
    // Flexible array member: https://www.wikiwand.com/en/Flexible_array_member
    char chars[];
} ObjString;

Obj *FromString(const char *chars, size_t length);
Obj *Concatenate(const Obj *left_string, const Obj *right_string);
bool ObjsEqual(const Obj *a, const Obj *b);
void FreeObj(Obj *obj);
void PrintObj(const Obj *obj);

#endif