#include <stdio.h>

#include "compiler.h"
#include "common.h"
#include "debug.h"
#include "native.h"
#include "value.h"
#include "vm.h"

VM vm; 

static void resetStack() {
    vm.frame_count = 0;
    vm.stack_top = vm.stack;
}

static void defineNatives() {
    ObjString *name_obj = (ObjString*) FromString("rand", 4);
    Value value = FromObj((Obj*) NewNative(Rand, 0));
    Insert(&vm.globals, name_obj, value);
    
    name_obj = (ObjString*) FromString("clock", 5);
    value = FromObj((Obj*) NewNative(Clock, 0));
    Insert(&vm.globals, name_obj, value);
    
    name_obj = (ObjString*) FromString("sqrt", 4);
    value = FromObj((Obj*) NewNative(Sqrt, 1));
    Insert(&vm.globals, name_obj, value);
    
    name_obj = (ObjString*) FromString("len", 3);
    value = FromObj((Obj*) NewNative(Len, 1));
    Insert(&vm.globals, name_obj, value);
    
    name_obj = (ObjString*) FromString("print", 5);
    value = FromObj((Obj*) NewNative(Print, 1));
    Insert(&vm.globals, name_obj, value);
}

void InitVM() {
    resetStack();
    InitTable(&vm.strings);
    InitTable(&vm.globals);
    vm.objects = NULL;
    
    defineNatives();
}

void FreeVM() {
    FreeTable(&vm.strings);
    FreeTable(&vm.globals);
    
    Obj *obj = vm.objects;
    while (obj != NULL) {
        Obj *next = obj->next;
        FreeObj(obj);
        obj = next;
    }
}

static void push(Value value) {
    *vm.stack_top = value;
    vm.stack_top++;
}

static Value pop() {
    vm.stack_top--;
    return *vm.stack_top;
}

static Value peek(int index) {
    return *(vm.stack_top - index - 1);
}

static void runtimeError(const char *message) {
    fprintf(stderr, message);
    
    fprintf(stderr, "\nStacktrace (most recent call first):\n");
    for (int i = vm.frame_count - 1; i >= 0; i--) {
        CallFrame *frame = &vm.frames[i];
        int line = GetLine(&frame->function->chunk, frame->ip - frame->function->chunk.code - 1);
        
        fprintf(stderr, "[line %d] in ", line);
        FPrintObj(stderr, (Obj*) frame->function);
        fprintf(stderr, "\n");
    }
    
    resetStack();
}

static void concatenate() {
    Obj *right_string = pop().as.obj;
    Obj *left_string = pop().as.obj;
    Obj *sum = Concatenate(left_string, right_string);
    push(FromObj(sum));
}

static InterpretResult run() {
    CallFrame *frame = &vm.frames[vm.frame_count - 1];
    
    // We could avoid declaring a local variable 'ip' and just access the field 'ip' through 'frame'.
    // However, since 'ip' is used a lot, holding it in a local variable and using the 'register' hint
    // might make the compiler store it in a register rather than in the main memory, speeding up execution.
    // We need to be careful to load and store 'ip' back into the correct CallFrame when starting and ending function calls.
    register uint8_t *ip = frame->ip;
    
#define READ_BYTE() (*ip++)
#define READ_SHORT() \
    (ip += 2, (int16_t) (ip[-2] | (ip[-1] << 8)))
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
#define AS_STRING(value) ((ObjString*) (value).as.obj)
#define AS_FUNCTION(value) ((ObjFunction*) (value).as.obj)
#define AS_NATIVE(value) ((ObjNative*) (value).as.obj)
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

    for (;;) {
        
#ifdef DEBUG
        printf("\t\t");
        for (Value *st_el = vm.stack; st_el != vm.stack_top; st_el++) {
            printf("[ ");
            PrintValue(*st_el);
            printf(" ]");
        }
        printf("\n");
        DisassembleInstruction(&frame->function->chunk, (int) (ip - frame->function->chunk.code));
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
                push(frame->function->chunk.constants.values[offset]);
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
            case OP_POP:
                pop();
                break;
            case OP_POPN: {
                uint8_t n = READ_BYTE();
                for (uint8_t i = 0; i < n; i++) {
                    pop();
                }
                break;
            }
            case OP_VAR_DECL:
                right = pop(); // value
                left = pop(); // variable
                Insert(&vm.globals, AS_STRING(left), right);
                break;
            case OP_IDENT_GLOBAL:
                right = pop(); // variable
                if (Get(&vm.globals, AS_STRING(right), &left)) {
                    push(left);
                    break;
                }
                runtimeError("Undefined identifier.");
                return INTERPRET_RUNTIME_ERROR;
            case OP_IDENT_LOCAL: {
                uint8_t i = READ_BYTE();
                push(frame->slots[i]);
                break;
            }
            case OP_ASSIGN_GLOBAL:
                right = pop(); // value
                left = pop(); // variable
                if (Insert(&vm.globals, AS_STRING(left), right)) {
                   runtimeError("Undefined variable.");
                   return INTERPRET_RUNTIME_ERROR;
                }
                push(right);
                break;
            case OP_ASSIGN_LOCAL: {
                uint8_t i = READ_BYTE();
                frame->slots[i] = peek(0);
                break;
            }
            case OP_JUMP_IF_FALSE: {
                int16_t n = READ_SHORT();
                if (!IsTruthy(peek(0))) {
                    ip += n;
                }
                break;
            }
            case OP_JUMP:
                ip += READ_SHORT();
                break;
            case OP_DUPLICATE:
                push(peek(0));
                break;
            case OP_CALL: {
                uint8_t argc = READ_BYTE();
                
                Value called_value = peek(argc);
                if (!IsFunction(called_value) && !IsNative(called_value)) {
                    runtimeError("Can only call functions.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                if (IsNative(called_value)) {
                    ObjNative *native_obj = AS_NATIVE(called_value);
                    if (argc != native_obj->arity) {
                        runtimeError("Invalid number of arguments.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    
                    NativeFn fn = native_obj->function;
                    ValueOpt result = fn(argc, vm.stack_top - argc);
                    if (result.error) {
                        runtimeError("Call to native function failed.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    vm.stack_top -= argc + 1;
                    push(result.value);
                    break;
                }
                
                ObjFunction *function = AS_FUNCTION(called_value);
                if (argc != function->arity) {
                    runtimeError("Invalid number of arguments."); 
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                if (vm.frame_count == FRAMES_MAX) {
                    runtimeError("Stack overflow.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                frame->ip = ip;
                
                frame = &vm.frames[vm.frame_count++];
                frame->function = function;
                frame->ip = function->chunk.code;
                frame->slots = vm.stack_top - argc - 1;
                ip = frame->ip;
                
                break;
            }
            case OP_RETURN: {
                Value v = pop();
                
                vm.frame_count--;
                if (vm.frame_count == 0) {
                    pop();
                    return INTERPRET_OK;
                }
                
                vm.stack_top = frame->slots;
                push(v);
                
                frame = &vm.frames[vm.frame_count - 1];
                ip = frame->ip;
                break;
            }
        }
    }

#undef EXEC_NUM_BIN_OP
#undef AS_NATIVE
#undef AS_FUNCTION
#undef AS_STRING
#undef READ_SHORT
#undef READ_BYTE
#undef READ_CONSTANT
}

InterpretResult Interpret(const char *source) {
    ObjFunction *script = Compile(source);
    if (script == NULL) {
        FreeObj((Obj*) script);
        return INTERPRET_COMPILE_ERROR;
    }
    
    push(FromObj((Obj*) script));
    CallFrame *frame = &vm.frames[vm.frame_count++];
    frame->function = script;
    frame->ip = script->chunk.code;
    frame->slots = vm.stack;
    
    InterpretResult result = run();
    
    // TODO: For some we don't need to free the object. Figure out why and delete line below.
    FreeObj((Obj*) script);
    
    return result;
}