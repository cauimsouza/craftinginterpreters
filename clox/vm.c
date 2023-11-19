#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "compiler.h"
#include "common.h"
#include "debug.h"
#include "memory.h"
#include "value.h"
#include "vm.h"

#define FIRST_GC 1024 * 1024

#define PUSH_OBJ(value) Push(FromObj((Obj*) (value)))

#define AS_STRING(value) ((ObjString*) (value).as.obj)
#define AS_FUNCTION(value) ((ObjFunction*) (value).as.obj)
#define AS_CLOSURE(value) ((ObjClosure*) (value).as.obj)
#define AS_NATIVE(value) ((ObjNative*) (value).as.obj)
#define AS_CLASS(value) ((ObjClass*) (value).as.obj)
#define AS_INSTANCE(value) ((ObjInstance*) (value).as.obj)
#define AS_BOUND_METHOD(value) ((ObjBoundMethod*) (value).as.obj)

VM vm; 

void Push(Value value) {
    *vm.stack_top = value;
    vm.stack_top++;
    IncrementRefcountValue(value);
}

Value Pop() {
    vm.stack_top--;
    Value value = *vm.stack_top;
    DecrementRefcountValue(value);
    return value;
}

// Beginning of declaration of native functions

ValueOpt Rand(int argc, Value *argv) {
    return (ValueOpt) {
        .value = FromDouble(rand()),
        .error = false
    };
}

ValueOpt Clock(int argc, Value *argv) {
    return (ValueOpt) {
        .value = FromDouble((double) clock() / CLOCKS_PER_SEC),
        .error = false
    };
}

ValueOpt Sqrt(int argc, Value *argv) {
    Value arg = argv[0];
    if (!IsNumber(arg) || arg.as.number < 0) {
        return (ValueOpt) {
            .error = true
        };
    }
    
    return (ValueOpt) {
        .value = FromDouble(sqrt(arg.as.number)),
        .error = false
    };
}

ValueOpt Len(int argc, Value *argv) {
    Value arg = argv[0];
    if (!IsString(arg)) {
        return (ValueOpt) {
            .error = true
        };
    }
    return (ValueOpt) {
        .value = FromDouble(((ObjString*) arg.as.obj)->length),
        .error = false
    };
}

ValueOpt Print(int argc, Value *argv) {
    PrintValue(argv[0]);
    printf("\n");
    return (ValueOpt) {
        .value = FromNil(),
        .error = false
    };
}

ValueOpt HasProp(int argc, Value *argv) {
    if (!IsInstance(argv[0]) || !IsString(argv[1])) {
        return (ValueOpt) {
            .error = true
        };
    }
    
    ObjInstance *instance = (ObjInstance*) argv[0].as.obj;
    ObjString *property = (ObjString*) argv[1].as.obj;
    
    Value v;
    return (ValueOpt) {
        .value = FromBoolean(Get(&instance->fields, property, &v)),
        .error = false
    };
}

ValueOpt SetProp(int argc, Value *argv) {
    if (!IsInstance(argv[0]) || !IsString(argv[1])) {
        return (ValueOpt) {
            .error = true
        };
    }
    
    ObjInstance *instance = (ObjInstance*) argv[0].as.obj;
    ObjString *property = (ObjString*) argv[1].as.obj;
    Value value = argv[2];
    
    Insert(&instance->fields, property, value);
    
    return (ValueOpt) {
        .value = FromNil(),
        .error = false
    };
}

ValueOpt GetProp(int argc, Value *argv) {
    if (!IsInstance(argv[0]) || !IsString(argv[1])) {
        return (ValueOpt) {
            .error = true
        };
    }
    
    ObjInstance *instance = (ObjInstance*) argv[0].as.obj;
    ObjString *property = (ObjString*) argv[1].as.obj;
    
    Value value;
    if (Get(&instance->fields, property, &value)) {
        return (ValueOpt) {
            .value = value,
            .error = false
        };
    }
    
    return (ValueOpt) {
        .error = true
    };
}

ValueOpt DelProp(int argc, Value *argv) {
    if (!IsInstance(argv[0]) || !IsString(argv[1])) {
        return (ValueOpt) {
            .error = true
        };
    }
    
    ObjInstance *instance = (ObjInstance*) argv[0].as.obj;
    ObjString *property = (ObjString*) argv[1].as.obj;
    
    Remove(&instance->fields, property);
    
    return (ValueOpt) {
        .value = FromNil(),
        .error = false
    };
}

static void defineNatives() {
    ObjString *name_obj = (ObjString*) FromString("rand", 4); PUSH_OBJ(name_obj);
    Value value = FromObj((Obj*) NewNative(Rand, 0)); Push(value);
    Insert(&vm.globals, name_obj, value); Pop(); Pop();
    
    name_obj = (ObjString*) FromString("clock", 5); PUSH_OBJ(name_obj);
    value = FromObj((Obj*) NewNative(Clock, 0)); Push(value);
    Insert(&vm.globals, name_obj, value); Pop(); Pop();
    
    name_obj = (ObjString*) FromString("sqrt", 4); PUSH_OBJ(name_obj);
    value = FromObj((Obj*) NewNative(Sqrt, 1)); Push(value);
    Insert(&vm.globals, name_obj, value); Pop(); Pop();
    
    name_obj = (ObjString*) FromString("len", 3); PUSH_OBJ(name_obj);
    value = FromObj((Obj*) NewNative(Len, 1)); Push(value);
    Insert(&vm.globals, name_obj, value); Pop(); Pop();
    
    name_obj = (ObjString*) FromString("print", 5); PUSH_OBJ(name_obj);
    value = FromObj((Obj*) NewNative(Print, 1)); Push(value);
    Insert(&vm.globals, name_obj, value); Pop(); Pop();
    
    name_obj = (ObjString*) FromString("hasProp", 7); PUSH_OBJ(name_obj);
    value = FromObj((Obj*) NewNative(HasProp, 2)); Push(value);
    Insert(&vm.globals, name_obj, value); Pop(); Pop();
    
    name_obj = (ObjString*) FromString("setProp", 7); PUSH_OBJ(name_obj);
    value = FromObj((Obj*) NewNative(SetProp, 3)); Push(value);
    Insert(&vm.globals, name_obj, value); Pop(); Pop();
    
    name_obj = (ObjString*) FromString("getProp", 7); PUSH_OBJ(name_obj);
    value = FromObj((Obj*) NewNative(GetProp, 2)); Push(value);
    Insert(&vm.globals, name_obj, value); Pop(); Pop();
    
    name_obj = (ObjString*) FromString("delProp", 7); PUSH_OBJ(name_obj);
    value = FromObj((Obj*) NewNative(DelProp, 2)); Push(value);
    Insert(&vm.globals, name_obj, value); Pop(); Pop();
}

// End of declaration of native functions

void InitVM() {
    vm.frame_count = 0;
    
    vm.stack_top = vm.stack;
    
    InitTable(&vm.strings);
    InitTable(&vm.globals);
    
    vm.open_upvalues = NULL;
    
    vm.bytes_allocated = 0;
    vm.next_gc = FIRST_GC;
    vm.objects = NULL;
    vm.grey_objects = NULL;
    vm.grey_count = 0;
    vm.grey_capacity = 0;
    
    vm.init_string = NULL;
    vm.init_string = (ObjString*) FromString("init", 4);
    defineNatives();
}

void FreeVM() {
    #ifdef DEBUG_LOG_GC
        printf("Freeing globals table.\n");
    #endif
    FreeTable(&vm.globals);
    
    #ifdef DEBUG_LOG_GC
        printf("Freeing string table.\n");
    #endif
    FreeTable(&vm.strings);
    
    vm.init_string = NULL;
    
    #ifdef DEBUG_LOG_GC
        printf("Freeing remaining objects.\n");
    #endif
    while (vm.objects != NULL) {
        FreeObj(vm.objects);
    }
    
    free(vm.grey_objects);
}

static Value peek(int index) {
    return *(vm.stack_top - index - 1);
}

static void runtimeError(const char *message) {
    fprintf(stderr, message);
    
    fprintf(stderr, "\nStacktrace (most recent call first):\n");
    for (int i = vm.frame_count - 1; i >= 0; i--) {
        CallFrame *frame = &vm.frames[i];
        int line = GetLine(&frame->closure->function->chunk, frame->ip - frame->closure->function->chunk.code - 1);
        
        fprintf(stderr, "[line %d] in ", line);
        FPrintObj(stderr, (Obj*) frame->closure->function);
        fprintf(stderr, "\n");
    }
    
    while (vm.stack_top != vm.stack) {
        Pop();
    }
    vm.frame_count = 0;
}

static void concatenate() {
    Obj *right_string = peek(0).as.obj;
    Obj *left_string = peek(1).as.obj;
    Obj *sum = Concatenate(left_string, right_string);
    Pop(); Pop();
    PUSH_OBJ(sum);
}

static ObjUpvalue *captureUpvalue(Value *slot) {
    if (vm.open_upvalues == NULL || vm.open_upvalues->location < slot) {
        ObjUpvalue *upvalue = NewUpvalue(slot);
        upvalue->next = vm.open_upvalues;
        vm.open_upvalues = upvalue;
        return upvalue;
    }
    
    ObjUpvalue *node = vm.open_upvalues;
    while (true) {
        if (node->location == slot) {
            return node;
        }
        
        if (node->next == NULL || node->next->location < slot) {
            ObjUpvalue *upvalue = NewUpvalue(slot);
            upvalue->next = node->next;
            node->next = upvalue;
            return upvalue;
        }
        
        node = node->next;
    }
    
    return NULL; // Should never be executed
}

static void closeUpvalues(Value *last) {
    while (vm.open_upvalues != NULL && vm.open_upvalues->location >= last) {
        ObjUpvalue *upvalue = vm.open_upvalues; 
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.open_upvalues = upvalue->next;
    }
}

static void setFrameFunctionCall(int argc, ObjClosure *closure, CallFrame **framep) {
        *framep = &vm.frames[vm.frame_count++];
        (*framep)->closure = closure;
        (*framep)->ip = closure->function->chunk.code;
        (*framep)->slots = vm.stack_top - argc - 1;
    
}

static bool callNative(int argc, CallFrame **framep) {
    ObjNative *native_obj = AS_NATIVE(peek(argc));
    if (argc != native_obj->arity) {
        runtimeError("Invalid number of arguments.");
        return false;
    }
    
    NativeFn fn = native_obj->function;
    ValueOpt result = fn(argc, vm.stack_top - argc);
    if (result.error) {
        runtimeError("Call to native function failed.");
        return false;
    }
    for (int i = 0; i < argc + 1; i++) {
        Pop();
    }
    Push(result.value);
    
    return true;
}

static bool callClass(int argc, CallFrame **framep) {
    Value called_value = peek(argc);
    ObjClass *class = AS_CLASS(called_value);
    Value init_val;
    if (Get(&class->methods, vm.init_string, &init_val)) { // "init" method
        ObjClosure *closure = AS_CLOSURE(init_val);
        if (argc != closure->function->arity) {
            runtimeError("Invalid number of arguments.");
            return false;
        }
        
        Value instance = FromObj((Obj*) NewInstance(class));
        *(vm.stack_top - (argc + 1)) = instance;
        IncrementRefcountValue(instance);
        DecrementRefcountValue(called_value);
        
        setFrameFunctionCall(argc, closure, framep);
    } else { // No "init" method
        if (argc != 0) {
            runtimeError("Default constructor takes no arguments.");
            return false;
        }
        
        ObjInstance *instance = NewInstance(class);
        Pop(); // class
        Push(FromObj((Obj*) instance));
    }
    
    return true;
}

static bool callClosure(int argc, CallFrame **framep) {
    ObjClosure *closure = AS_CLOSURE(peek(argc));
    if (argc != closure->function->arity) {
        runtimeError("Invalid number of arguments."); 
        return false;
    }
    
    if (vm.frame_count == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }
    
    setFrameFunctionCall(argc, closure, framep);
    
    return true;
}

static bool callBoundMethod(int argc, CallFrame **framep) {
    Value called_value = peek(argc);
    ObjBoundMethod *method = AS_BOUND_METHOD(called_value);    
    Value receiver = method->receiver;
    ObjClosure *closure = method->method;
    if (argc != closure->function->arity) {
        runtimeError("Invalid number of arguments");
        return false;
    }
    
    if (vm.frame_count == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }
    
    *(vm.stack_top - (argc + 1)) = receiver;
    IncrementRefcountValue(receiver);
    DecrementRefcountValue(called_value);
    
    setFrameFunctionCall(argc, closure, framep);
    
    return true;
}

static bool call(int argc,  CallFrame **framep) {
    Value called_value = peek(argc);
    if (!IsNative(called_value) &&
        !IsClass(called_value) &&
        !IsClosure(called_value) &&
        !IsBoundMethod(called_value)
       ) {
        runtimeError("Can only call functions, methods, and constructors.");
        return false;
    }
    
    if (IsNative(called_value)) {
        return callNative(argc, framep);
    } else if (IsClass(called_value)) {
        return callClass(argc, framep);
    } else if (IsClosure(called_value)) {
        return callClosure(argc, framep);
    } else { // bound method
        return callBoundMethod(argc, framep);
    }
}

static bool methodCall(int argc, ObjClosure *method, CallFrame **framep) {
    if (argc != method->function->arity) {
        runtimeError("Invalid number of arguments");
        return false;
    }
    
    if (vm.frame_count == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }
    
    setFrameFunctionCall(argc, method, framep);
    
    return true;
}

static InterpretResult run() {
    CallFrame *frame = &vm.frames[vm.frame_count - 1];
    
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() \
    (frame->ip += 2, (int16_t) (frame->ip[-2] | (frame->ip[-1] << 8)))
#define READ_CONSTANT(offset) (frame->closure->function->chunk.constants.values[(offset)])
#define EXEC_NUM_BIN_OP(op, toValue) \
    do { \
        Value right = peek(0); \
        Value left = peek(1); \
        if (!IsNumber(right) || !IsNumber(left)) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        Value result = toValue(left.as.number op right.as.number); \
        Pop(); Pop(); \
        Push(result); \
    } while (false)

    int itn = 0;
    for (;;) {
        
#ifdef DEBUG
        printf("\t\t");
        for (Value *st_el = vm.stack; st_el != vm.stack_top; st_el++) {
            printf("[ ");
            PrintValue(*st_el);
            printf(" ]");
        }
        printf("\n");
        DisassembleInstruction(&frame->closure->function->chunk, (int) (frame->ip - frame->closure->function->chunk.code));
#endif

        Value right, left;
        uint8_t instruction = READ_BYTE();
        switch (instruction) {
            case OP_CONSTANT: {
                size_t offset = READ_BYTE();
                Push(READ_CONSTANT(offset));
                break;
            }
            case OP_CONSTANT_LONG: {
                size_t offset = 0;
                for (size_t i = 0, pot = 1; i < 3; i++, pot = (pot << 8)) {
                    offset += READ_BYTE() * pot;
                }
                Push(READ_CONSTANT(offset));
                break;
            }
            case OP_NIL:
                Push(FromNil());
                break;
            case OP_TRUE:
                Push(FromBoolean(true));
                break;
            case OP_FALSE:
                Push(FromBoolean(false));
                break;
            case OP_NEGATE: {
                if (!IsNumber(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double d = -peek(0).as.number; Pop();
                Push(FromDouble(d));
                
                break;
            }
            case OP_NOT: {
                bool res = !IsTruthy(peek(0)); Pop();
                Push(FromBoolean(res));
                break;
            }
            case OP_EQ: {
                bool res = ValuesEqual(peek(0), peek(1)); Pop(); Pop();
                Push(FromBoolean(res));
                break;
            }
            case OP_NEQ: {
                bool res = !ValuesEqual(peek(0), peek(1)); Pop(); Pop();
                Push(FromBoolean(res));
                break;
            }
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
                    Value result = FromDouble(peek(0).as.number + peek(1).as.number);
                    Pop(); Pop();
                    Push(result);
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
                Pop();
                break;
            case OP_POPN: {
                uint8_t n = READ_BYTE();
                for (uint8_t i = 0; i < n; i++) {
                    Pop();
                }
                break;
            }
            case OP_VAR_DECL: {
                // The call to Insert() might trigger a GC cycle. If 'right' and 'left' are not on the stack, the GC might collect them.
                right = peek(0); // value
                left = peek(1); // variable
                if (!Insert(&vm.globals, AS_STRING(left), right)) {
                    runtimeError("Already a global variable with this name.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Pop();
                Pop();
                
                break;
            }
            case OP_IDENT_GLOBAL: {
                Value value;
                if (Get(&vm.globals, AS_STRING(peek(0)), &value)) {
                    Pop();
                    Push(value);
                    break;
                }
                runtimeError("Undefined identifier.");
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ASSIGN_GLOBAL: {
                Value value = peek(0);
                if (Insert(&vm.globals, AS_STRING(peek(1)), peek(0))) {
                   runtimeError("Undefined variable.");
                   return INTERPRET_RUNTIME_ERROR;
                }
                Pop();
                Pop();
                Push(value);
                
                break;
            }
            case OP_IDENT_LOCAL: {
                uint8_t i = READ_BYTE();
                Push(frame->slots[i]);
                break;
            }
            case OP_ASSIGN_LOCAL: {
                uint8_t i = READ_BYTE();
                DecrementRefcountValue(frame->slots[i]);
                frame->slots[i] = peek(0);
                IncrementRefcountValue(frame->slots[i]);
                break;
            }
            case OP_IDENT_UPVALUE: {
                uint8_t index = READ_BYTE();
                Push(*frame->closure->upvalues[index]->location);
                break;
            }
            case OP_ASSIGN_UPVALUE: {
                uint8_t index = READ_BYTE();
                Value *value = frame->closure->upvalues[index]->location;
                DecrementRefcountValue(*value);
                *value = peek(0); 
                IncrementRefcountValue(*value);
                
                break;
            }
            case OP_CLOSE_UPVALUE:
                closeUpvalues(vm.stack_top - 1);
                Pop();
                break;
            case OP_JUMP_IF_FALSE: {
                int16_t n = READ_SHORT();
                if (!IsTruthy(peek(0))) {
                    frame->ip += n;
                }
                break;
            }
            case OP_JUMP:
                frame->ip += READ_SHORT();
                break;
            case OP_DUPLICATE:
                Push(peek(0));
                break;
            case OP_CALL: {
                uint8_t argc = READ_BYTE();
                if (!call(argc,  &frame)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_INVOKE: {
                size_t offset = READ_BYTE();
                ObjString *property = AS_STRING(READ_CONSTANT(offset));
                
                uint8_t argc = READ_BYTE(); 
                if (!IsInstance(peek(argc))) {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                ObjInstance *instance = AS_INSTANCE(peek(argc));
                
                Value value;
                if (Get(&instance->fields, property, &value)) {
                    *(vm.stack_top - (argc + 1)) = value;
                    IncrementRefcountValue(value); 
                    DecrementRefcountObject((Obj*) instance); 
                    if (!call(argc, &frame)) {
                        return INTERPRET_RUNTIME_ERROR;
                    }
                } else if (Get(&instance->class->methods, property, &value)) {
                    if (!methodCall(argc, AS_CLOSURE(value), &frame)) {
                        return INTERPRET_RUNTIME_ERROR;
                    }
                } else {
                    runtimeError("Instance doesn't have property.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break; 
            }
            case OP_CLOSURE: {
                size_t offset = READ_BYTE();
                ObjFunction *function = AS_FUNCTION(READ_CONSTANT(offset));
                ObjClosure *closure = NewClosure(function);
                Push(FromObj((Obj*) closure));
                for (int i = 0; i < closure->upvalue_count; i++) {
                    uint8_t is_local = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (is_local) {
                        closure->upvalues[i] = captureUpvalue(frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                    IncrementRefcountObject((Obj*) closure->upvalues[i]);
                }
                
                break;
            }
            case OP_CLOSURE_LONG: {
                size_t offset = 0;
                for (size_t i = 0, pot = 1; i < 3; i++, pot = (pot << 8)) {
                    offset += READ_BYTE() * pot;
                }
                ObjFunction *function = AS_FUNCTION(READ_CONSTANT(offset));
                ObjClosure *closure = NewClosure(function);
                Push(FromObj((Obj*) closure));
                for (int i = 0; i < closure->upvalue_count; i++) {
                    uint8_t is_local = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (is_local) {
                        closure->upvalues[i] = captureUpvalue(frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                    IncrementRefcountObject((Obj*) closure->upvalues[i]);
                }
                
                break;
            }
            case OP_IDENT_PROPERTY: {
                if (!IsInstance(peek(1))) {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                ObjInstance *instance = AS_INSTANCE(peek(1));
                ObjString *field = AS_STRING(peek(0));
                
                Value value;
                if (Get(&instance->fields, field, &value)) {
                    // When we pop the instance from the stack, the instance
                    // refcount might drop to 0. If that happens, the field we are
                    // accessing could be collected. To prevent that, we increment
                    // the field's refcount first.
                    IncrementRefcountValue(value);
                    
                    Pop(); // field
                    Pop(); // instance
                    Push(value);
                    
                    // Now that it's in the stack, we can decrement the refcount.
                    DecrementRefcountValue(value);
                } else if (Get(&instance->class->methods, field, &value)) {
                    Value receiver = FromObj((Obj*) instance);
                    ObjClosure *method = AS_CLOSURE(value);
                    ObjBoundMethod *bound_method = NewBoundMethod(receiver, method);
                    value = FromObj((Obj*) bound_method);
                    
                    // Since NewBoundMethod() adds to the instance refcount, the
                    // instance refcount won't drop to 0 when we pop instance
                    // from the stack. So the GC won't be triggered and there is
                    // no risk of bound_method being collected before we push it
                    // onto the stack.
                    Pop(); // field
                    Pop(); // instance
                    Push(value);
                } else {
                    runtimeError("Instance does not have field or method.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                break;
            }
            case OP_ASSIGN_PROPERTY: {
                if (!IsInstance(peek(2))) {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                ObjInstance *instance = AS_INSTANCE(peek(2));
                ObjString *field = AS_STRING(peek(1));
                Value value = peek(0); IncrementRefcountValue(value);
                
                Insert(&instance->fields, field, value);
                
                Pop(); // value
                Pop(); // field
                Pop(); // instance
                Push(value); DecrementRefcountValue(value);
                
                break;
            }
            case OP_METHOD: {
                Value closure = peek(0);
                ObjString *name = AS_STRING(peek(1));
                ObjClass *class = AS_CLASS(peek(2));
                
                Insert(&class->methods, name, closure);
                
                Pop(); // closure
                Pop(); // name
                
                break; 
            }
            case OP_RETURN: {
                Value v = peek(0); IncrementRefcountValue(v); Pop();
                closeUpvalues(frame->slots);
                
                vm.frame_count--;
                if (vm.frame_count == 0) {
                    DecrementRefcountValue(v);
                    Pop();
                    return INTERPRET_OK;
                }
                
                while (vm.stack_top != frame->slots) {
                    Pop();
                }
                Push(v); DecrementRefcountValue(v);
                
                frame = &vm.frames[vm.frame_count - 1];
                
                break;
            }
        }
    }

#undef EXEC_NUM_BIN_OP
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
    
    PUSH_OBJ(script); // NewClosure() may trigger a GC cycle
    ObjClosure *closure = NewClosure(script);
    Pop(); // script
    Push(FromObj((Obj*) closure));
    DecrementRefcountObject((Obj*) script); // Compile() returns script with refcount = 1
    
    CallFrame *frame = &vm.frames[vm.frame_count++];
    frame->closure = closure;
    frame->ip = script->chunk.code;
    frame->slots = vm.stack;
    
    InterpretResult result = run();
    
    return result;
}

#undef AS_BOUND_METHOD
#undef AS_INSTANCE
#undef AS_CLASS
#undef AS_NATIVE
#undef AS_FUNCTION
#undef AS_STRING
