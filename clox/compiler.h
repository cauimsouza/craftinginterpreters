#ifndef clox_compiler_h
#define clox_compiler_h

#include "common.h"
#include "chunk.h"

bool Compile(const char *source, Chunk *chunk);

#endif