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
    chunk->cache = ALLOCATE(ObjString*, 10);
    chunk->cache_size = 0;
}

void FreeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FreeLines(&chunk->lines);
    FreeValueArray(&chunk->constants);
    FREE_ARRAY(Obj*, chunk->cache, chunk->cache_size);
    
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

size_t addCache(Chunk *chunk, ObjString *obj) {
    for (size_t i = 0; i < chunk->cache_size; i++) {
        if (chunk->cache[i] == obj) {
            return i;
        }
    }
    
    size_t i = chunk->cache_size++;
    chunk->cache[i] = obj;
    return i;
}

void ReadCache(Chunk *chunk, ObjString *obj, int line) {
    size_t i = addCache(chunk, obj);
    WriteChunk(chunk, OP_RCACHE, line);
    WriteChunk(chunk, i, line);
}

void WriteCache(Chunk *chunk, ObjString *obj, int line) {
    size_t i = addCache(chunk, obj);
    WriteChunk(chunk, OP_WCACHE, line);
    WriteChunk(chunk, i, line);
}