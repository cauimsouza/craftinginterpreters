#include <stdio.h>

#include "compiler.h"
#include "common.h"
#include "debug.h"
#include "vm.h"

VM vm; 

static void resetStack() {
    vm.stackTop = vm.stack;
}

void initVM() {
    resetStack();
}

void freeVM() {
}

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

static void runtimeError(const char *message) {
    fprintf(stderr, message);
    
    int instruction_offset = vm.ip - vm.chunk->code - 1;
    int line = getLine(vm.chunk, instruction_offset);
    fprintf(stderr, "\n[line %d] in script\n", line);
    resetStack();
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define EXEC_NUM_BIN_OP(op, toValue) \
    do { \
        Value right = pop(); \
        Value left = pop(); \
        if (!isNumber(right) || !isNumber(left)) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        Value result = toValue(left.as.number op right.as.number); \
        push(result); \
    } while (false)
#define EXEC_LOG_BIN_OP(op) \
    do { \
        bool right = isTruthy(pop()); \
        bool left = isTruthy(pop()); \
        Value result = fromBoolean(left op right); \
        push(result); \
    } while (false)

    for (;;) {
#ifdef DEBUG
        printf("\t\t");
        for (Value *st_el = vm.stack; st_el != vm.stackTop; st_el++) {
            printf("[ ");
            printValue(*st_el);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(vm.chunk, (int) (vm.ip - vm.chunk->code));
#endif

        Value right, left;
        uint8_t instruction = READ_BYTE();
        switch (instruction) {
            case OP_CONSTANT:
                push(READ_CONSTANT());
                break;
            case OP_CONSTANT_LONG: ;
                size_t offset = 0;
                for (size_t i = 0, pot = 1; i < 3; i++, pot = (pot << 8)) {
                    offset += READ_BYTE() * pot;
                    pot <<= 8;
                }
                push(vm.chunk->constants.values[offset]);
                break;
            case OP_NIL:
                push(fromNil());
                break;
            case OP_TRUE:
                push(fromBoolean(true));
                break;
            case OP_FALSE:
                push(fromBoolean(false));
                break;
            case OP_NEGATE:
                left = pop();
                if (!isNumber(left)) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(fromDouble(-left.as.number));
                break;
            case OP_NOT:
                left = pop();
                push(fromBoolean(!isTruthy(left)));
                break;
            case OP_OR:
                EXEC_LOG_BIN_OP(||);
                break;
            case OP_AND:
                EXEC_LOG_BIN_OP(&&);
                break;
            case OP_EQ:
                push(fromBoolean(equals(pop(), pop())));
                break;
            case OP_NEQ:
                push(fromBoolean(!equals(pop(), pop())));
                break;
            case OP_LESS:
                EXEC_NUM_BIN_OP(<, fromBoolean);
                break;
            case OP_LESS_EQ:
                EXEC_NUM_BIN_OP(<=, fromBoolean);
                break;
            case OP_GREATER:
                EXEC_NUM_BIN_OP(>, fromBoolean);
                break;
            case OP_GREATER_EQ:
                EXEC_NUM_BIN_OP(>=, fromBoolean);
                break;
            case OP_ADD:
                EXEC_NUM_BIN_OP(+, fromDouble);
                break;
            case OP_SUBTRACT:
                EXEC_NUM_BIN_OP(-, fromDouble);
                break;
            case OP_MULTIPLY:
                EXEC_NUM_BIN_OP(*, fromDouble);
                break;
            case OP_DIVIDE:
                EXEC_NUM_BIN_OP(/, fromDouble);
                break;
            case OP_RETURN:
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
}

InterpretResult interpret(const char *source) {
    Chunk chunk;
    initChunk(&chunk);
    
    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILER_ERROR;
    }
    
    vm.chunk = &chunk;
    vm.ip = chunk.code;
    
    InterpretResult result = run();
    
    freeChunk(&chunk);
    
    return result;
}