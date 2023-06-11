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
  OP_IDENT_LOCAL,
  OP_ASSIGN_GLOBAL,
  OP_ASSIGN_LOCAL,
  
  // Jumps if top of the stack is falsey. It has one 2B operand, the offset to move ip.
  OP_JUMP_IF_FALSE, 
  // Jumps unconditionally. It has one 2B operand, the offset to move ip. 
  OP_JUMP,
  
  OP_DUPLICATE, // Duplicate the value at the top of the stack. Only used for "switch".
  OP_CALL,
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

void WriteChunk(Chunk *chunk, uint8_t byte, int line);
void WriteConstant(Chunk *chunk, Value value, int line);

int GetLine(Chunk *chunk, int offset);

#endif