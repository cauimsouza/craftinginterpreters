#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "object.h"
#include "table.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
  ObjClosure *closure;
  uint8_t *ip;
  Value *slots;
} CallFrame;

typedef struct {
  CallFrame frames[FRAMES_MAX];
  int frame_count;
  
  Value stack[STACK_MAX];
  Value *stack_top;
  
  Table strings;
  Table globals;
  
  ObjUpvalue *open_upvalues;
  
  // GC data structures
  size_t bytes_allocated;
  size_t next_gc; // Next GC cycle will be triggered when bytes_allocated > next_gc
  Obj *objects;
  Obj **grey_objects;
  int grey_count;
  int grey_capacity;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void InitVM();
void FreeVM();
void Push(Value value);
Value Pop();

InterpretResult Interpret(const char *source);

#endif
