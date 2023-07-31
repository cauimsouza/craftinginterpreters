#include <stdio.h>

#include "debug.h"
#include "object.h"
#include "value.h"

static int simpleInstruction(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int constantInstruction(const char *name, Chunk *chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %8d '", name, constant);
    PrintValue(chunk->constants.values[constant]);
    printf("'\n");
    
    return offset + 2;
}

static int constantLongInstruction(const char *name, Chunk *chunk, int offset) {
    int constant = 0;
    for (int i = 0; i < 3; i++) {
        constant += chunk->code[offset + 1 + i] * (1 << (8 * i));
    }
    
    printf("%-16s %8d '", name, constant);
    PrintValue(chunk->constants.values[constant]);
    printf("'\n");
    
    return offset + 4;
}

static int closureInstruction(const char *name, Chunk *chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %8d '", name, constant);
    
    Value value = chunk->constants.values[constant];
    PrintValue(value);
    printf("'\n");
    
    offset += 2;
    
    ObjFunction *function = (ObjFunction*) value.as.obj;
    for (int i = 0; i < function->upvalue_count; i++) {
        bool local = chunk->code[offset] == 1;
        uint8_t index = chunk->code[offset + 1];
        printf("%04d    |                          %s %d\n", offset, local ? "local" : "upvalue", index);
        offset += 2;
    }
    
    return offset;
}

// TODO: Unify logic of closureLongInstruction and closureInstruction
static int closureLongInstruction(const char *name, Chunk *chunk, int offset) {
    int constant = 0;
    for (int i = 0; i < 3; i++) {
        constant += chunk->code[offset + 1 + i] * (1 << (8 * i));
    }
    printf("%-16s %8d '", name, constant);
    
    Value value = chunk->constants.values[constant];
    PrintValue(value);
    printf("'\n");
    
    offset += 4;
    
    ObjFunction *function = (ObjFunction*) value.as.obj;
    for (int i = 0; i < function->upvalue_count; i++) {
        bool local = chunk->code[offset] == 1;
        uint8_t index = chunk->code[offset + 1];
        printf("%04d    |                          %s %d\n", offset, local ? "local" : "upvalue", index);
        offset += 2;
    }
    
    return offset;
}

static int byteInstruction(const char *name, Chunk *chunk, int offset) {
    printf("%-16s %8d\n", name, chunk->code[offset + 1]);
    return offset + 2;
}

static int shortInstructions(const char *name, Chunk *chunk, int offset) {
    int16_t operand = chunk->code[offset + 1] | chunk->code[offset + 2] << 8;
    printf("%-16s %8d\n", name, operand);
    return offset + 3;
}

int DisassembleInstruction(Chunk *chunk, int offset) {
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
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_POPN:
            return byteInstruction("OP_POPN", chunk, offset);
        case OP_VAR_DECL:
            return simpleInstruction("OP_VAR_DECL", offset);
        case OP_IDENT_GLOBAL:
            return simpleInstruction("OP_IDENT_GLOBAL", offset);
        case OP_ASSIGN_GLOBAL:
            return simpleInstruction("OP_ASSIGN_GLOBAL", offset);
        case OP_IDENT_LOCAL:
            return byteInstruction("OP_IDENT_LOCAL", chunk, offset);
        case OP_ASSIGN_LOCAL:
            return byteInstruction("OP_ASSIGN_LOCAL", chunk, offset);
        case OP_IDENT_UPVALUE:
            return byteInstruction("OP_IDENT_UPVALUE", chunk, offset);
        case OP_ASSIGN_UPVALUE:
            return byteInstruction("OP_ASSIGN_UPVALUE", chunk, offset);
        case OP_CLOSE_UPVALUE:
            return simpleInstruction("OP_CLOSE_UPVALUE", offset);
        case OP_JUMP_IF_FALSE:
            return shortInstructions("OP_JUMP_IF_FALSE", chunk, offset);
        case OP_JUMP:
            return shortInstructions("OP_JUMP", chunk, offset);
        case OP_DUPLICATE:
            return simpleInstruction("OP_DUPLICATE", offset);
        case OP_CALL:
            return byteInstruction("OP_CALL", chunk, offset);
        case OP_CLOSURE:
            return closureInstruction("OP_CLOSURE", chunk, offset);
        case OP_CLOSURE_LONG:
            return closureLongInstruction("OP_CLOSURE_LONG", chunk, offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

void DisassembleChunk(Chunk *chunk, const char *name) {
    printf("== %s ==\n", name);
    
    for (int offset = 0; offset < chunk->count;) {
        offset = DisassembleInstruction(chunk, offset);     
    }
}
