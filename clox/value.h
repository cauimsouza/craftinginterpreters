#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
    } as;
} Value;

Value fromBoolean(bool boolean);
Value fromDouble(double number);
Value fromNil();
bool isBoolean(Value value);
bool isNumber(Value value);
bool isNil(Value value);
bool isTruthy(Value value);
bool equals(Value a, Value b);
void printValue(Value value);

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);

#endif