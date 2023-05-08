#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

static int addConstant(Chunk* chunk, Value value);
static void writeConstantSimple(Chunk* chunk, int offset, int line);
static void writeConstantLong(Chunk* chunk, int offset, int line);

void InitChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    InitLines(&chunk->lines);
    InitValueArray(&chunk->constants);
    InitIdentifiers(&chunk->identifiers);
}

void FreeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FreeLines(&chunk->lines);
    FreeValueArray(&chunk->constants);
    FreeIdentifiers(&chunk->identifiers);
    
    InitChunk(chunk);
}

void WriteChunk(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->count == chunk->capacity) {
        chunk->capacity = GROW_CAPACITY(chunk->capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, chunk->count, chunk->capacity);
    }
    
    chunk->code[chunk->count] = byte;
    WriteLines(&chunk->lines, line);
    chunk->count++;
}

void WriteConstant(Chunk* chunk, Value value, int line) {
    int offset = addConstant(chunk, value);
    
    if (offset > 0xFF) {
        writeConstantLong(chunk, offset, line);
        return;
    }
    
    writeConstantSimple(chunk, offset, line);
}

int GetLine(Chunk* chunk, int offset) {
    return GetLineAtOffset(&chunk->lines, offset);
}

static int addConstant(Chunk* chunk, Value value) {
    WriteValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}

static void writeConstantSimple(Chunk* chunk, int offset, int line) {
    WriteChunk(chunk, OP_CONSTANT, line);
    WriteChunk(chunk, offset, line);
}

static void writeConstantLong(Chunk* chunk, int offset, int line) {
    WriteChunk(chunk, OP_CONSTANT_LONG, line);
    
    for (int i = 0; i < 3; i++) {
        WriteChunk(chunk, offset & 0xFF, line);
        offset >>= 8;
    }
}

void ReadIdentifier(Chunk *chunk, ObjString *obj, int line) {
    WriteChunk(chunk, OP_IDENT, line);
    WriteChunk(chunk, AddIdentifier(&chunk->identifiers, obj), line);
}

void WriteIdentifier(Chunk *chunk, ObjString *obj, int line) {
    WriteChunk(chunk, OP_ASSIGN, line);
    WriteChunk(chunk, AddIdentifier(&chunk->identifiers, obj), line);
}