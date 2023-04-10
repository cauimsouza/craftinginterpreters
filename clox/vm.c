#include <stdio.h>

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

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

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
                    offset += READ_CONSTANT() * pot;
                    pot <<= 8;
                }
                push(vm.chunk->constants.values[offset]);
                break;
            case OP_NEGATE:
                push(-pop());
                break;
            case OP_ADD:
                push(pop() + pop());
                break;
            case OP_SUBTRACT:
                right = pop();
                left = pop();
                push(left - right);
                break;
            case OP_MULTIPLY:
                push(pop() * pop());
                break;
            case OP_DIVIDE: ;
                right = pop();
                left = pop();
                if (left == 0.0) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(left / right);
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

InterpretResult interpret(Chunk *chunk) {
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}