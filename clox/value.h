#ifndef clox_value_h
#define clox_value_h

#include "common.h"
#include "object.h"

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

Value FromBoolean(bool boolean);
Value FromDouble(double number);
Value FromNil();
Value FromObj(Obj *obj);
bool IsBoolean(Value value);
bool IsNumber(Value value);
bool IsNil(Value value);
bool IsObj(Value value);
bool IsString(Value value);
bool IsTruthy(Value value);
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

#endif