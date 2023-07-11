#ifndef clox_object_h
#define clox_object_h

#include <stdio.h>

#include "chunk.h"
#include "common.h"

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE,
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
static inline bool IsString(Value value);

typedef struct {
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString *name;
} ObjFunction;

ObjFunction *NewFunction();
static inline bool IsFunction(Value value);

typedef Value (*NativeFn) (int argc, Value *argv);

typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;

ObjNative *NewNative(NativeFn function);
static inline bool IsNative(Value value);

bool ObjsEqual(const Obj *a, const Obj *b);
void FreeObj(Obj *obj);
void PrintObj(const Obj *obj);
void FPrintObj(FILE *stream, const Obj *obj);

static inline bool IsString(Value value) {
    return value.type == VAL_OBJ && value.as.obj->type == OBJ_STRING;
}

static inline bool IsFunction(Value value) {
    return value.type == VAL_OBJ && value.as.obj->type == OBJ_FUNCTION;
}

static inline bool IsNative(Value value) {
    return value.type == VAL_OBJ && value.as.obj->type == OBJ_NATIVE;
}

#endif