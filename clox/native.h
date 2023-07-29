#ifndef clox_native_h
#define clox_native_h

#include "value.h"

// Implements the "rand" function
ValueOpt Rand(int argc, Value *argv);

// Implements the "clock" function
ValueOpt Clock(int argc, Value *argv);

// Implements the "sqrt" function
ValueOpt Sqrt(int argc, Value *argv);

// Implements the "len" function
ValueOpt Len(int argc, Value *argv);

// Implements the "print" function
ValueOpt Print(int argc, Value *argv);

#endif