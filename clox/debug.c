#include <stdio.h>

#include "debug.h"
#include "value.h"

static int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %8d '", name, constant);
    PrintValue(chunk->constants.values[constant]);
    printf("'\n");
    
    return offset + 2;
}

static int constantLongInstruction(const char* name, Chunk* chunk, int offset) {
    int constant = 0;
    for (int i = 0; i < 3; i++) {
        constant += chunk->code[offset + 1 + i] * (1 << (8 * i));
    }
    
    printf("%-16s %8d '", name, constant);
    PrintValue(chunk->constants.values[constant]);
    printf("'\n");
    
    return offset + 4;
}

int DisassembleInstruction(Chunk* chunk, int offset) {
    printf("%04d ", offset);
    
    if (offset > 0 && GetLine(chunk, offset) == GetLine(chunk, offset - 1)) {
        printf("   | ");
    } else {
        printf("%4d ", GetLine(chunk, offset));
    }
    
    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_CONSTANT_LONG:
            return constantLongInstruction("OP_CONSTANT_LONG", chunk, offset);
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_OR:
            return simpleInstruction("OP_OR", offset);
        case OP_AND:
            return simpleInstruction("OP_AND", offset);
        case OP_EQ:
            return simpleInstruction("OP_EQ", offset);
        case OP_NEQ:
            return simpleInstruction("OP_NEQ", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_LESS_EQ:
            return simpleInstruction("OP_LESS_EQ", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_GREATER_EQ:
            return simpleInstruction("OP_GREATER_EQ", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        case OP_PRINT:
            return simpleInstruction("OP_PRINT", offset);
        case OP_EXPR:
            return simpleInstruction("OP_EXPR", offset);
        case OP_VAR_DECL:
            return simpleInstruction("OP_VAR_DECL", offset);
        case OP_IDENT:
            return simpleInstruction("OP_IDENT", offset);
        case OP_ASSIGN:
            return simpleInstruction("OP_ASSIGN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

void DisassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);
    
    for (int offset = 0; offset < chunk->count;) {
        offset = DisassembleInstruction(chunk, offset);     
    }
}
