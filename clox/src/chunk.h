#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"
#include "lines.h"

typedef enum {
  OP_CONSTANT,
  OP_CONSTANT_LONG,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_NEGATE,
  OP_NOT,
  OP_EQ,
  OP_NEQ,
  OP_LESS,
  OP_LESS_EQ,
  OP_GREATER,
  OP_GREATER_EQ,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_RETURN,
  OP_PRINT,
  OP_POP, // Expression statement. Simply pops the element at the top of the stack.
  OP_POPN, // Pops a variable number of elements from the top of the stack.
  OP_VAR_DECL,
  OP_IDENT_GLOBAL,
  OP_ASSIGN_GLOBAL,
  OP_IDENT_LOCAL,
  OP_ASSIGN_LOCAL,
  OP_IDENT_PROPERTY,
  OP_ASSIGN_PROPERTY,
  OP_IDENT_UPVALUE,
  OP_ASSIGN_UPVALUE,
  OP_CLOSE_UPVALUE,
  OP_JUMP_IF_FALSE, // Jumps if top of the stack is falsey. It has one 2B operand, the offset to move ip.
  OP_JUMP, // Jumps unconditionally. It has one 2B operand, the offset to move ip. 
  OP_DUPLICATE, // Duplicates the value at the top of the stack
  OP_CALL,
  OP_INVOKE,
  OP_INVOKE_LONG,
  OP_CLOSURE,
  OP_CLOSURE_LONG,
  OP_METHOD,
  OP_INHERIT,
  OP_GET_SUPER,
} OpCode;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    Lines lines;
    ValueArray constants;
} Chunk;

void InitChunk(Chunk *chunk);
void FreeChunk(Chunk *chunk);

void MarkChunk(Chunk *chunk);

void WriteChunk(Chunk *chunk, uint8_t byte, int line);
void WriteConstant(Chunk *chunk, OpCode op_simple, OpCode op_long, Value value, int line);

int GetLine(Chunk *chunk, int offset);

#endif