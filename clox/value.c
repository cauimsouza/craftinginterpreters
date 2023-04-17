#include <stdlib.h>
#include <stdio.h>

#include "value.h"
#include "memory.h"

Value fromBoolean(bool boolean) {
    return (Value) {
        .type = VAL_BOOL,
        .as.boolean = boolean
    };
}

Value fromDouble(double number) {
    return (Value) {
        .type = VAL_NUMBER,
        .as.number = number
    };
}

Value fromNil() {
    return (Value) {
        .type = VAL_NIL
    };
}

Value fromObj(Obj *obj) {
    return (Value) {
        .type = VAL_OBJ,
        .as.obj = obj
    };
}

bool isBoolean(Value value) {
    return value.type == VAL_BOOL;
}

bool isNumber(Value value) {
    return value.type == VAL_NUMBER;
}

bool isNil(Value value) {
    return value.type == VAL_NIL;
}

bool isObj(Value value) {
    return value.type == VAL_OBJ;
}

bool isString(Value value) {
    return value.type == VAL_OBJ && value.as.obj->type == OBJ_STRING;
}

bool isTruthy(Value value) {
    return !(isNil(value) || isBoolean(value) && !value.as.boolean);
}

bool valuesEqual(Value a, Value b) {
    return
        isBoolean(a) && isBoolean(b) && a.as.boolean == b.as.boolean ||
        isNil(a) && isNil(b) ||
        isNumber(a) && isNumber(b) && a.as.number == b.as.number ||
        isObj(a) && isObj(b) && objsEqual(a.as.obj, b.as.obj);
}

void printValue(Value value) {
    if (isBoolean(value)) {
        printf("%s", value.as.boolean ? "true" : "false");
    } else if (isNil(value)) {
        printf("nil");
    } else if (isNumber(value)) {
        printf("%g", value.as.number);
    } else {
        printObj(value.as.obj);
    }
}

void initValueArray(ValueArray* array) {
    array->count = 0;
    array->capacity = 0;
    array->values = NULL;
}

void writeValueArray(ValueArray* array, Value value) {
    if (array->count == array->capacity) {
        array->capacity = GROW_CAPACITY(array->capacity);
        array->values = GROW_ARRAY(Value, array->values, array->count, array->capacity);
    }
    
    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}
