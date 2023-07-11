#ifndef clox_native_h
#define clox_native_h

#include "value.h"

// Implements the "rand" native function
Value Rand(int argc, Value *argv);

// Implements the "clock" native function
Value Clock(int argc, Value *argv);

#endif