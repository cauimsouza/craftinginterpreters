#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef struct Obj Obj;

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj *obj;
    } as;
} Value;

typedef struct {
    Value value; // Undefined behaviour if error == true
    bool error;
} ValueOpt;

static inline Value FromBoolean(bool boolean);
static inline Value FromDouble(double number);
static inline Value FromNil();
static inline Value FromObj(Obj *obj);
static inline bool IsBoolean(Value value);
static inline bool IsNumber(Value value);
static inline bool IsNil(Value value);
static inline bool IsObj(Value value);
static inline bool IsTruthy(Value value);
bool ValuesEqual(Value a, Value b);
void PrintValue(Value value);

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

void InitValueArray(ValueArray* array);
void WriteValueArray(ValueArray* array, Value value);
void FreeValueArray(ValueArray* array);

static inline Value FromBoolean(bool boolean) {
    return (Value) {
        .type = VAL_BOOL,
        .as.boolean = boolean
    };
}

static inline Value FromDouble(double number) {
    return (Value) {
        .type = VAL_NUMBER,
        .as.number = number
    };
}

static inline Value FromNil() {
    return (Value) {
        .type = VAL_NIL
    };
}

static inline Value FromObj(Obj *obj) {
    return (Value) {
        .type = VAL_OBJ,
        .as.obj = obj
    };
}

static inline bool IsBoolean(Value value) {
    return value.type == VAL_BOOL;
}

static inline bool IsNumber(Value value) {
    return value.type == VAL_NUMBER;
}

static inline bool IsNil(Value value) {
    return value.type == VAL_NIL;
}

static inline bool IsObj(Value value) {
    return value.type == VAL_OBJ;
}

static inline bool IsTruthy(Value value) {
    return !(IsNil(value) || IsBoolean(value) && !value.as.boolean);
}

#endif