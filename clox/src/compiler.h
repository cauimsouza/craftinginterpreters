#ifndef clox_compiler_h
#define clox_compiler_h

#include "common.h"
#include "chunk.h"
#include "object.h"

ObjFunction *Compile(const char *source);

void MarkCompilerRoots();

#endif