#include <stdio.h>

#include "debug.h"
#include "object.h"
#include "value.h"

static int readNBytes(Chunk *chunk, int offset, int nbytes) {
    int constant = 0;
    for (int i = 0; i < nbytes; i++) {
        constant += chunk->code[offset + 1 + i] * (1 << (8 * i));
    }
    
    return constant;
}

static int simpleInstruction(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int constantInstruction(const char *name, Chunk *chunk, int offset, int size_operand) {
    int constant = readNBytes(chunk, offset, size_operand);
    printf("%-16s %8d '", name, constant);
    PrintValue(chunk->constants.values[constant]);
    printf("'\n");
    
    return offset + size_operand + 1;
}

static int closureInstruction(const char *name, Chunk *chunk, int offset, int size_operand) {
    int constant = readNBytes(chunk, offset, size_operand);
    printf("%-16s %8d '", name, constant);
    
    Value value = chunk->constants.values[constant];
    PrintValue(value);
    printf("'\n");
    
    offset += size_operand + 1;
    
    ObjFunction *function = (ObjFunction*) value.as.obj;
    for (int i = 0; i < function->upvalue_count; i++) {
        bool local = chunk->code[offset] == 1;
        uint8_t index = chunk->code[offset + 1];
        printf("%04d    |                         %s %d\n", offset, local ? "local" : "upvalue", index);
        offset += 2;
    }
    
    return offset;
}

static int byteInstruction(const char *name, Chunk *chunk, int offset) {
    int operand = readNBytes(chunk, offset, 1);
    printf("%-16s %8d\n", name, operand);
    return offset + 2;
}

static int shortInstructions(const char *name, Chunk *chunk, int offset) {
    int operand = readNBytes(chunk, offset, 2);
    printf("%-16s %8d\n", name, operand);
    return offset + 3;
}

static int invokeInstruction(const char *name, Chunk *chunk, int offset) {
    uint8_t first = readNBytes(chunk, offset, 1);
    uint8_t second = chunk->code[offset + 2];
    printf("%-16s ", name);
    PrintValue(chunk->constants.values[first]);
    printf(" %8d\n", second);
    return offset + 3;
}

static int invokeLongInstruction(const char *name, Chunk *chunk, int offset) {
    uint8_t first = readNBytes(chunk, offset, 2);
    uint8_t second = chunk->code[offset + 3];
    printf("%-16s ", name);
    PrintValue(chunk->constants.values[first]);
    printf(" %8d\n", second);
    return offset + 4;
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
            return constantInstruction("OP_CONSTANT", chunk, offset, 1);
        case OP_CONSTANT_LONG:
            return constantInstruction("OP_CONSTANT_LONG", chunk, offset, 3);
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
        case OP_IDENT_PROPERTY:
            return simpleInstruction("OP_IDENT_PROPERTY", offset);
        case OP_ASSIGN_PROPERTY:
            return simpleInstruction("OP_ASSIGN_PROPERTY", offset);
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
        case OP_INVOKE:
            return invokeInstruction("OP_INVOKE", chunk, offset);
        case OP_INVOKE_LONG:
            return invokeLongInstruction("OP_INVOKE_LONG", chunk, offset);
        case OP_CLOSURE:
            return closureInstruction("OP_CLOSURE", chunk, offset, 1);
        case OP_CLOSURE_LONG:
            return closureInstruction("OP_CLOSURE_LONG", chunk, offset, 3);
        case OP_METHOD:
            return simpleInstruction("OP_METHOD", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        case OP_INHERIT:
            return simpleInstruction("OP_INHERIT", offset);
        case OP_GET_SUPER:
            return simpleInstruction("OP_GET_SUPER", offset);
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
