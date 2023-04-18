#include <stdlib.h>
#include <stdio.h>

#include "value.h"
#include "memory.h"

Value FromBoolean(bool boolean) {
    return (Value) {
        .type = VAL_BOOL,
        .as.boolean = boolean
    };
}

Value FromDouble(double number) {
    return (Value) {
        .type = VAL_NUMBER,
        .as.number = number
    };
}

Value FromNil() {
    return (Value) {
        .type = VAL_NIL
    };
}

Value FromObj(Obj *obj) {
    return (Value) {
        .type = VAL_OBJ,
        .as.obj = obj
    };
}

bool IsBoolean(Value value) {
    return value.type == VAL_BOOL;
}

bool IsNumber(Value value) {
    return value.type == VAL_NUMBER;
}

bool IsNil(Value value) {
    return value.type == VAL_NIL;
}

bool IsObj(Value value) {
    return value.type == VAL_OBJ;
}

bool IsString(Value value) {
    return value.type == VAL_OBJ && value.as.obj->type == OBJ_STRING;
}

bool IsTruthy(Value value) {
    return !(IsNil(value) || IsBoolean(value) && !value.as.boolean);
}

bool ValuesEqual(Value a, Value b) {
    return
        IsBoolean(a) && IsBoolean(b) && a.as.boolean == b.as.boolean ||
        IsNil(a) && IsNil(b) ||
        IsNumber(a) && IsNumber(b) && a.as.number == b.as.number ||
        IsObj(a) && IsObj(b) && ObjsEqual(a.as.obj, b.as.obj);
}

void PrintValue(Value value) {
    if (IsBoolean(value)) {
        printf("%s", value.as.boolean ? "true" : "false");
    } else if (IsNil(value)) {
        printf("nil");
    } else if (IsNumber(value)) {
        printf("%g", value.as.number);
    } else {
        PrintObj(value.as.obj);
    }
}

void InitValueArray(ValueArray* array) {
    array->count = 0;
    array->capacity = 0;
    array->values = NULL;
}

void WriteValueArray(ValueArray* array, Value value) {
    if (array->count == array->capacity) {
        array->capacity = GROW_CAPACITY(array->capacity);
        array->values = GROW_ARRAY(Value, array->values, array->count, array->capacity);
    }
    
    array->values[array->count] = value;
    array->count++;
}

void FreeValueArray(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    InitValueArray(array);
}
