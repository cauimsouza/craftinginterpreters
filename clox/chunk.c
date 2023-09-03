#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

static int addConstant(Chunk *chunk, Value value);
static void writeConstantSimple(Chunk *chunk, OpCode op, int offset, int line);
static void writeConstantLong(Chunk *chunk, OpCode op, int offset, int line);

void InitChunk(Chunk *chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    InitLines(&chunk->lines);
    InitValueArray(&chunk->constants);
}

void FreeChunk(Chunk *chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FreeLines(&chunk->lines);
    FreeValueArray(&chunk->constants);
    InitChunk(chunk);
}

void WriteChunk(Chunk *chunk, uint8_t byte, int line) {
    if (chunk->count == chunk->capacity) {
        chunk->capacity = GROW_CAPACITY(chunk->capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, chunk->count, chunk->capacity);
    }
    
    chunk->code[chunk->count] = byte;
    WriteLines(&chunk->lines, line);
    chunk->count++;
}

void WriteConstant(Chunk *chunk, OpCode op_simple, OpCode op_long, Value value, int line) {
    IncrementRefcountValue(value);
    
    int offset = addConstant(chunk, value);
    
    if (offset > 0xFF) {
        writeConstantLong(chunk, op_long, offset, line);
        return;
    }
    
    writeConstantSimple(chunk, op_simple, offset, line);
}

int GetLine(Chunk *chunk, int offset) {
    return GetLineAtOffset(&chunk->lines, offset);
}

static int addConstant(Chunk *chunk, Value value) {
    WriteValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}

static void writeConstantSimple(Chunk *chunk, OpCode op, int offset, int line) {
    WriteChunk(chunk, op, line);
    WriteChunk(chunk, offset, line);
}

static void writeConstantLong(Chunk *chunk, OpCode op, int offset, int line) {
    WriteChunk(chunk, op, line);
    
    for (int i = 0; i < 3; i++) {
        WriteChunk(chunk, offset & 0xFF, line);
        offset >>= 8;
    }
}
