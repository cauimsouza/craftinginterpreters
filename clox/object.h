#ifndef clox_object_h
#define clox_object_h

#include "common.h"

#define TO_STRING(value) ((ObjString*) (value).as.obj)

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
    // Flexible array member: https://www.wikiwand.com/en/Flexible_array_member
    char chars[];
} ObjString;

Obj *fromString(const char *chars, size_t length);
Obj *addStrings(ObjString *left_str, ObjString *right_str);
bool objsEqual(Obj *a, Obj *b);
void freeObj(Obj *obj);
void printObj(Obj *obj);

#endif