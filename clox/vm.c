#include <stdio.h>

#include "compiler.h"
#include "common.h"
#include "debug.h"
#include "value.h"
#include "vm.h"

VM vm; 

static void resetStack() {
    vm.stackTop = vm.stack;
}

void InitVM() {
    resetStack();
    vm.objects = NULL;
}

void FreeVM() {
    Obj *obj = vm.objects;
    while (obj != NULL) {
        Obj *next = obj->next;
        FreeObj(obj);
        obj = next;
    }
}

static void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

static Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek(int index) {
    return *(vm.stackTop - index - 1);
}

static void runtimeError(const char *message) {
    fprintf(stderr, message);
    
    int instruction_offset = vm.ip - vm.chunk->code - 1;
    int line = GetLine(vm.chunk, instruction_offset);
    fprintf(stderr, "\n[line %d] in script\n", line);
    resetStack();
}

static void concatenate() {
    Obj *right_string = pop().as.obj;
    Obj *left_string = pop().as.obj;
    Obj *sum = Concatenate(left_string, right_string);
    push(FromObj(sum));
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define EXEC_NUM_BIN_OP(op, toValue) \
    do { \
        Value right = pop(); \
        Value left = pop(); \
        if (!IsNumber(right) || !IsNumber(left)) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        Value result = toValue(left.as.number op right.as.number); \
        push(result); \
    } while (false)
#define EXEC_LOG_BIN_OP(op) \
    do { \
        bool right = IsTruthy(pop()); \
        bool left = IsTruthy(pop()); \
        Value result = FromBoolean(left op right); \
        push(result); \
    } while (false)

    for (;;) {
#ifdef DEBUG
        printf("\t\t");
        for (Value *st_el = vm.stack; st_el != vm.stackTop; st_el++) {
            printf("[ ");
            PrintValue(*st_el);
            printf(" ]");
        }
        printf("\n");
        DisassembleInstruction(vm.chunk, (int) (vm.ip - vm.chunk->code));
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
                push(FromNil());
                break;
            case OP_TRUE:
                push(FromBoolean(true));
                break;
            case OP_FALSE:
                push(FromBoolean(false));
                break;
            case OP_NEGATE:
                left = pop();
                if (!IsNumber(left)) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(FromDouble(-left.as.number));
                break;
            case OP_NOT:
                left = pop();
                push(FromBoolean(!IsTruthy(left)));
                break;
            case OP_OR:
                EXEC_LOG_BIN_OP(||);
                break;
            case OP_AND:
                EXEC_LOG_BIN_OP(&&);
                break;
            case OP_EQ:
                push(FromBoolean(ValuesEqual(pop(), pop())));
                break;
            case OP_NEQ:
                push(FromBoolean(!ValuesEqual(pop(), pop())));
                break;
            case OP_LESS:
                EXEC_NUM_BIN_OP(<, FromBoolean);
                break;
            case OP_LESS_EQ:
                EXEC_NUM_BIN_OP(<=, FromBoolean);
                break;
            case OP_GREATER:
                EXEC_NUM_BIN_OP(>, FromBoolean);
                break;
            case OP_GREATER_EQ:
                EXEC_NUM_BIN_OP(>=, FromBoolean);
                break;
            case OP_ADD:
                if (IsString(peek(0)) && IsString(peek(1))) {
                    concatenate();
                    break;
                }
                if (IsNumber(peek(0)) && IsNumber(peek(1))) {
                   push(FromDouble(pop().as.number + pop().as.number));
                   break;
                }
                runtimeError("Operands must be two strings or two numbers.");
                return INTERPRET_RUNTIME_ERROR;
            case OP_SUBTRACT:
                EXEC_NUM_BIN_OP(-, FromDouble);
                break;
            case OP_MULTIPLY:
                EXEC_NUM_BIN_OP(*, FromDouble);
                break;
            case OP_DIVIDE:
                EXEC_NUM_BIN_OP(/, FromDouble);
                break;
            case OP_RETURN:
                PrintValue(pop());
                printf("\n");
                return INTERPRET_OK;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
}

InterpretResult Interpret(const char *source) {
    Chunk chunk;
    InitChunk(&chunk);
    
    if (!Compile(source, &chunk)) {
        FreeChunk(&chunk);
        return INTERPRET_COMPILER_ERROR;
    }
    
    vm.chunk = &chunk;
    vm.ip = chunk.code;
    
    InterpretResult result = run();
    
    FreeChunk(&chunk);
    
    return result;
}