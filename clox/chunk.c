#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

int addConstant(Chunk* chunk, Value value);
void writeConstantSimple(Chunk* chunk, int offset, int line);
void writeConstantLong(Chunk* chunk, int offset, int line);

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    initLines(&chunk->lines);
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    freeLines(&chunk->lines);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->count == chunk->capacity) {
        chunk->capacity = GROW_CAPACITY(chunk->capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, chunk->count, chunk->capacity);
    }
    
    chunk->code[chunk->count] = byte;
    writeLines(&chunk->lines, line);
    chunk->count++;
}

void writeConstant(Chunk* chunk, Value value, int line) {
    int offset = addConstant(chunk, value);
    
    if (offset > 0xFF) {
        writeConstantLong(chunk, offset, line);
        return;
    }
    
    writeConstantSimple(chunk, offset, line);
}

int getLine(Chunk* chunk, int offset) {
    return getLineAtOffset(&chunk->lines, offset);
}

int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}

void writeConstantSimple(Chunk* chunk, int offset, int line) {
    writeChunk(chunk, OP_CONSTANT, line);
    writeChunk(chunk, offset, line);
}

void writeConstantLong(Chunk* chunk, int offset, int line) {
    writeChunk(chunk, OP_CONSTANT_LONG, line);
    
    for (int i = 0; i < 3; i++) {
        writeChunk(chunk, offset & 0xFF, line);
        offset >>= 8;
    }
}
