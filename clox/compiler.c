#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "object.h"
#include "scanner.h"
#include "value.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

void Push(Value value);
Value Pop();

typedef struct {
  Token previous;
  Token current;
  bool had_error;
  bool panic_mode;
} Parser;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,
  PREC_OR,
  PREC_AND,
  PREC_EQUALITY,
  PREC_COMPARISON,
  PREC_TERM,
  PREC_FACTOR,
  PREC_UNARY,
  PREC_CALL,
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool can_assign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef enum {
  TYPE_FUNCTION,
  TYPE_SCRIPT
} FunctionType;

typedef struct {
  Obj *name;
  bool is_const;
  
  bool reassigned; // true iff the global was reassigned
  Token reassign_token; // assignment operator (=) where the global was reassigned
} Global;

typedef struct {
  Token name;
  bool is_const;
  bool is_captured;
  
  // depth of the scope in which the local variable was declared or -1.
  // depth is -1 iff the variable was added to the scope but is not yet ready for use.
  int depth;
} Local;

typedef struct {
  uint8_t index;
  bool is_local; 
  bool is_const;
} Upvalue;

typedef enum {
  LOOP_WHILE,
  LOOP_FOR,
} LoopType;

typedef struct {
  LoopType type;
  
  // address of the destination of 'continue' statements immediately enclosed by the loop
  int address;
  
  // depth of the scope where the 'while' or 'for' appears
  int depth; 
  
  int *breaks;
  int break_count;
  int break_capacity;
} Loop;

typedef struct Compiler {
  struct Compiler *enclosing;
  ObjFunction *function;
  FunctionType type;
  Upvalue upvalues[UINT8_COUNT]; // TODO: Make the array resizable.
  
  Local *locals;
  int local_count;
  int local_capacity;
  
  Loop *loops;
  int loop_count;
  int loop_capacity;
  
  // depth of the scope in which the function being declared is,
  // or -1 if no function is being declared
  int function_depth;
  int scope_depth;
} Compiler;

Parser parser;
Compiler *current = NULL;
Global *Globals = NULL;
int Global_count = 0;
int Global_capacity = 0;
  

static Chunk *currentChunk() {
  return &current->function->chunk;
}

static Local *newLocal(Compiler *compiler);

static void initCompiler(Compiler *compiler, FunctionType type) {
  compiler->enclosing = current;
  compiler->function = NULL; // We nullify the field before initialising it for the GC
  compiler->type = type;
  
  compiler->locals = NULL;
  compiler->local_count = 0;
  compiler->local_capacity = 0;
  
  compiler->loops = NULL;
  compiler->loop_count = 0;
  compiler->loop_capacity = 0;
  
  compiler->function_depth = -1;
  compiler->scope_depth = 0;
  compiler->function = NewFunction(); IncrementRefcountObject((Obj*) compiler->function);
  current = compiler;
  
  // The function being compiled itself is the first local.
  Local *local = newLocal(current);
  local->is_captured = false;
  local->depth = 0;
  local->name.start = "";
  local->name.length = 0;
}

static void freeCompiler(Compiler *compiler) {
  FREE_ARRAY(Local, compiler->locals, compiler->local_capacity);
  FREE_ARRAY(Loop, compiler->loops, compiler->loop_capacity);
}

static void nil(bool can_assign);
static void emitByte(uint8_t byte);
  
static ObjFunction *endCompiler() {
  nil(/*can_assign=*/false);
  emitByte(OP_RETURN);
  
  ObjFunction *function = current->function;
  
  #ifdef DEBUG_PRINT_CODE
  if (!parser.had_error) {
    // The implicit function we use for the top-level code doesn't have name.
    DisassembleChunk(currentChunk(), function->name != NULL ? function->name->chars : "<script>");
  }
  #endif
  
  current = current->enclosing;
  
  return function;
}

static bool sameVariable(Token a, Token b) {
    return a.length == b.length && memcmp(a.start, b.start, a.length) == 0;
}

static Local *newLocal(Compiler *compiler) {
  if (compiler->local_count < compiler->local_capacity) {
    return &compiler->locals[compiler->local_count++];
  }
  
  compiler->local_capacity = GROW_CAPACITY(compiler->local_capacity);
  compiler->locals = GROW_ARRAY(Local, compiler->locals, compiler->local_count, compiler->local_capacity);
  return &compiler->locals[compiler->local_count++];
}

// Returns true iff the local was successfully declared.
// declareLocal fails iff there already is a local with the same name in the scope.
static bool declareLocal(Compiler *compiler, Token name, bool is_const) {
  for (int i = compiler->local_count - 1; i >= 0; i--) {
    Local *local = &compiler->locals[i];
    if (local->depth < compiler->scope_depth) {
      break;
    }
    
    if (sameVariable(name, local->name)) {
      return false;
    }
  }
  
  Local *local = newLocal(compiler);
  local->name = name;
  local->is_const = is_const;
  local->is_captured = false;
  local->depth = -1;
  
  return true;
}

// duplicateTopLocal is used to create a copy of the local variable at the top
// of the stack. The new variable is in the current scope (which might or might
// not be the scope of the local variable copied).
// This function can be used to make for-loops make a copy of the loop variable
// for each iteration.
static void duplicateTopLocal() {
  Local *top_local = &current->locals[current->local_count - 1];
  Local *local = newLocal(current);
  local->name = top_local->name;
  local->is_const = top_local->is_const;
  local->is_captured = false;
  local->depth = current->scope_depth;
}

// Returns the index of the local or -1 if the local doesn't exist.
static int findLocal(Compiler *compiler, Token name) {
  for (int i = compiler->local_count - 1; i >= 0; i--) {
    if (sameVariable(name, compiler->locals[i].name)) {
      return i;
    }
  }
  
  return -1;
}

// numLocals returns the number of locals with at least the given depth.
static uint8_t numLocals(Compiler *compiler, int depth) {
  int n = 0;
  for (int i = compiler->local_count - 1; i >= 0; i--) {
    if (compiler->locals[i].depth < depth) {
      break;
    }
    n++;
  }
  
  return n;
}

static void growGlobals() {
  Global_capacity = GROW_CAPACITY(Global_capacity);
  Globals = GROW_ARRAY(Global, Globals, Global_count, Global_capacity);
}

// getGlobal returns the global with the given name (if it already exists) or
// creates a new global with the given name (if it doesn't exist yet).
static Global *getGlobal(Obj *name) {
  for (uint8_t i = 0; i < Global_count; i++) {
    if (Globals[i].name == name) {
      return &Globals[i];
    }
  }
  
  if (Global_count == Global_capacity) {
    growGlobals();
  }
  
  Global *global = &Globals[Global_count++];
  global->name = name;
  global->is_const = false;
  global->reassigned = false;
  
  return global;
}

static void growLoops(Compiler *compiler) {
  compiler->loop_capacity = GROW_CAPACITY(compiler->loop_capacity);
  compiler->loops = GROW_ARRAY(Loop, compiler->loops, compiler->loop_count, compiler->loop_capacity);
}

static void declareLoop(Compiler *compiler, LoopType type, int address, int depth) {
  if (compiler->loop_count == compiler->loop_capacity) {
    growLoops(compiler);
  }
  
  Loop *loop = &compiler->loops[compiler->loop_count++];
  loop->type = type;
  loop->address = address;
  loop->depth = depth;
  loop->breaks = NULL;
  loop->break_count = 0;
  loop->break_capacity = 0;
}

static void patchJump(int jump_instr, int jump_dst);

static void deleteLoop(Compiler *compiler, int address_end) {
  Loop *loop = &compiler->loops[compiler->loop_count - 1];
  for (int i = 0; i < loop->break_count; i++) {
    patchJump(loop->breaks[i], address_end);
  }
  
  FREE_ARRAY(int, loop->breaks, loop->break_capacity);
  
  compiler->loop_count--;
}

static void scheduleBreakJump(Loop *loop, int jmp_instr) {
  if (loop->break_count == loop->break_capacity) {
    loop->break_capacity = GROW_CAPACITY(loop->break_capacity);
    loop->breaks = GROW_ARRAY(int, loop->breaks, loop->break_count, loop->break_capacity);
  }
  
  loop->breaks[loop->break_count++] = jmp_instr;
}

static Loop *enclosingLoop(Compiler *compiler) {
  if (compiler->loop_count == 0) {
    return NULL;
  }
  
  return &compiler->loops[compiler->loop_count - 1];
}

static Precedence nextPrecedence(Precedence precedence) {
  return (Precedence) (precedence + 1); 
}

static void errorAt(Token *token, const char *message) {
  if (parser.panic_mode) {
    return;
  }
  
  parser.panic_mode = true;
  fprintf(stderr, "[line %d] Error", token->line);
  
  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type != TOKEN_ERROR) {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }
  
  fprintf(stderr, ": %s\n", message);
  parser.had_error = true;
}

static void error(const char *message) {
  errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char *message) {
  errorAt(&parser.current, message); 
}

static void advance() {
  parser.previous = parser.current;
  
  for (;;) {
    parser.current = ScanToken();
    
    if (parser.current.type != TOKEN_ERROR) {
      break;
    }
    
    errorAtCurrent(parser.current.start);
  }
}

static bool check(TokenType type) {
  return parser.current.type == type;
}

static void consume(TokenType type, const char *message) {
  if (check(type)) {
    advance();
    return;
  }
  errorAtCurrent(message);
}

static bool match(TokenType type) {
  if (check(type)) {
    advance();
    return true;
  }
  return false;
}

static int ip() {
  return currentChunk()->count;
}

static void emitByte(uint8_t byte) {
  WriteChunk(currentChunk(), byte, parser.previous.line);
}

static void emitByteAt(uint8_t byte, uint8_t address) {
  currentChunk()->code[address] = byte;
}

static void emitConstant(Value value) {
  Push(value);
  WriteConstant(currentChunk(), OP_CONSTANT, OP_CONSTANT_LONG, value, parser.previous.line);
  Pop();
}

static void emitClosure(ObjFunction *function, Upvalue *upvalues) {
  Value fun_val = FromObj((Obj*) function);
  Push(fun_val);
  WriteConstant(currentChunk(), OP_CLOSURE, OP_CLOSURE_LONG, fun_val, parser.previous.line);
  Pop();
  
  for (int i = 0; i < function->upvalue_count; i++) {
    emitByte(upvalues[i].is_local ? 1 : 0);
    emitByte(upvalues[i].index);
  }
}

static void emitBoolean(bool boolean) {
  uint8_t byte = boolean ? OP_TRUE : OP_FALSE;
  emitByte(byte);
}

// emitJump emits the byte instructions for a jump with a dummy operand value, 
// and returns the address of the opcode.
static int emitJump(OpCode jump_type) {
  emitByte(jump_type);
  emitByte(0);
  emitByte(0);
  return ip() - 3;
}

// patchJump patches a previous issued jump instruction.
//
// jump_instr is the address of the opcode of the jump instruction.
// jump_dst is the address the VM should jump to.
static void patchJump(int jump_instr, int jump_dst) {
  int16_t size = jump_dst - jump_instr - 3;
  emitByteAt(size & 0xFF, jump_instr + 1); // little endian
  size >>= 8;
  emitByteAt(size & 0xFF, jump_instr + 2);
}

static void beginScope() {
  current->scope_depth++;
}

static void endScope() {
  while (current->local_count > 0 && current->locals[current->local_count - 1].depth >= current->scope_depth) {
    if (current->locals[current->local_count - 1].is_captured) {
      emitByte(OP_CLOSE_UPVALUE);
    } else {
      emitByte(OP_POP);
    }
    current->local_count--;
  }
  
  current->scope_depth--;
}

static ParseRule *getRule(TokenType token_type);

static void parsePrecedence(Precedence precedence) {
  advance();
  
  ParseFn prefix_rule = getRule(parser.previous.type)->prefix;
  if (prefix_rule == NULL) {
    error("Expect expression.");
    return;
  }
  
  bool can_assign = precedence <= PREC_ASSIGNMENT;
  prefix_rule(can_assign);
  
  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infix_rule = getRule(parser.previous.type)->infix;
    infix_rule(can_assign);
  }
  
  if (can_assign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}

static void expression() {
  parsePrecedence(PREC_ASSIGNMENT);
}

static void number(bool can_assign) {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(FromDouble(value));
}

static void nil(bool can_assign) {
  emitConstant(FromNil());
}

static void boolean(bool can_assign) {
  emitBoolean(parser.previous.type == TOKEN_TRUE);
}

static void string(bool can_assign) {
  size_t length = parser.previous.length - 2;
  const char *chars = parser.previous.start + 1;
  emitConstant(FromObj(FromString(chars, length))); 
}

static int addUpvalue(Compiler *compiler, uint8_t index, bool is_local, bool is_const) {
  int count = compiler->function->upvalue_count;
  
  for (int i = 0; i < count; i++) {
    Upvalue *upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->is_local == is_local) {
      return i;
    }
  } 
  
  if (count == UINT8_COUNT) {
    error("Too many closure variables in function.");
    return 0;
  }
  
  Upvalue *upvalue = &compiler->upvalues[count];
  upvalue->index = index;
  upvalue->is_local = is_local;
  upvalue->is_const = is_const;
  
  return compiler->function->upvalue_count++;
}

static int resolveUpvalue(Compiler *compiler, Token name) {
  if (compiler->enclosing == NULL) {
    return -1;
  }
  
  int local = findLocal(compiler->enclosing, name);
  if (local >= 0) {
    compiler->enclosing->locals[local].is_captured = true;
    return addUpvalue(compiler, (uint8_t) local, /*is_local=*/true, compiler->enclosing->locals[local].is_const);
  }
  
  int upvalue = resolveUpvalue(compiler->enclosing, name);
  if (upvalue >= 0) {
    return addUpvalue(compiler, (uint8_t) upvalue, /*is_local=*/false, compiler->enclosing->upvalues[upvalue].is_const);
  }
  
  return -1;
}

// Similar to identifier, but for cases where the identifier is in the scope of the current function.
static void identifierLocal(Compiler *compiler, bool can_assign, int local) {
  if (compiler->locals[local].depth < 0) {
      error("Can't read local variable being initialised.");
      return;
  }
  
  if (can_assign && match(TOKEN_EQUAL)) {
    if (compiler->locals[local].is_const) {
      error("Can't reassign to const local variable.");
    }
    
    parsePrecedence(PREC_ASSIGNMENT);
    
    emitByte(OP_ASSIGN_LOCAL);
    emitByte(local);
    
    return;
  }
  
  emitByte(OP_IDENT_LOCAL);
  emitByte(local);
}

// Similar to identifier, but for cases where the identifier is in the scope of an enclosing function.
static void identifierUpvalue(Compiler *compiler, bool can_assign, int upvalue) {
  if (can_assign && match(TOKEN_EQUAL)) {
    if (compiler->upvalues[upvalue].is_const) {
      error("Can't reassign to const local variable.");
    }
    
    parsePrecedence(PREC_ASSIGNMENT);
    
    emitByte(OP_ASSIGN_UPVALUE);
    emitByte(upvalue);
    
    return;
  }
  
  emitByte(OP_IDENT_UPVALUE);
  emitByte(upvalue);
}

static void identifier(bool can_assign) {
  int local = findLocal(current, parser.previous);
  if (local >= 0) {
    identifierLocal(current, can_assign, local);
    return;
  }
  
  int upvalue = resolveUpvalue(current, parser.previous);
  if (upvalue >= 0) {
    identifierUpvalue(current, can_assign, upvalue);
    return;
  }
  
  Obj *name = FromString(parser.previous.start, parser.previous.length);
  emitConstant(FromObj(name)); 
  
  if (can_assign && match(TOKEN_EQUAL)) {
    Token op = parser.previous;
    
    parsePrecedence(PREC_ASSIGNMENT);
    emitByte(OP_ASSIGN_GLOBAL);
    
    Global *global = getGlobal(name);
    global->reassigned = true;
    global->reassign_token = op;
    
    return;
  }
  
  emitByte(OP_IDENT_GLOBAL);
}

static void grouping(bool can_assign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary(bool can_assign) {
  TokenType op = parser.previous.type;
  
  parsePrecedence(PREC_UNARY);
  
  switch (op) {
    case TOKEN_MINUS:
      emitByte(OP_NEGATE);
      break;
    case TOKEN_BANG:
      emitByte(OP_NOT);
      break;
  }
}

static void binary(bool can_assign) {
  TokenType op_type = parser.previous.type;
  
  ParseRule *rule = getRule(op_type);
  parsePrecedence(nextPrecedence(rule->precedence));
  
  switch (op_type) {
    case TOKEN_PLUS:
      emitByte(OP_ADD);
      break;
    case TOKEN_MINUS:
      emitByte(OP_SUBTRACT);
      break;
    case TOKEN_STAR:
      emitByte(OP_MULTIPLY);
      break;
    case TOKEN_SLASH:
      emitByte(OP_DIVIDE);
      break;
    case TOKEN_BANG_EQUAL:
      emitByte(OP_NEQ);
      break;
    case TOKEN_EQUAL_EQUAL:
      emitByte(OP_EQ);
      break;
    case TOKEN_GREATER:
      emitByte(OP_GREATER);
      break;
    case TOKEN_GREATER_EQUAL:
      emitByte(OP_GREATER_EQ);
      break;
    case TOKEN_LESS:
      emitByte(OP_LESS);
      break;
    case TOKEN_LESS_EQUAL:
      emitByte(OP_LESS_EQ);
      break;
  }
}

static void and(bool can_assign) {
  // This will evaluate a && b as follows:
  // - If the value at the top of the stack is falsey (the value of a), we leave
  //     it at the top of the stack and jump to right after the code block that
  //     evaluates b.
  // - If the value at the top of the stack is truthy, we pop the stack and
  //     continue the execution to the code block that evaluates b. At the end of
  //     that block, the value of b will be at the top of the stack, which is the
  //     result of a && b.
  
  uint8_t false_jump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  parsePrecedence(nextPrecedence(PREC_AND));
  patchJump(false_jump, ip());
}

static void or(bool can_assign) {
  // This will evaluate a or b as follows:
  // - If the value at the top of the stack is falsey (the value of a), we jump
  //     to the block that evaluates b, pop the stack, then execute that block, 
  //     leaving the result at the top of the stack.
  // - If the value at the top of the stack is truthy, we jump to right after
  //     the block that evaluates b.
  
  int false_jump = emitJump(OP_JUMP_IF_FALSE);
  int true_jump = emitJump(OP_JUMP);
  patchJump(false_jump, ip());
  emitByte(OP_POP); 
  parsePrecedence(nextPrecedence(PREC_OR));
  patchJump(true_jump, ip());
}

static void call(bool can_assign) {
  uint8_t argc = 0;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      if (argc == 255) {
        error("Can't have more than 255 arguments.");
      }
      argc++;
      expression();  
    } while (match(TOKEN_COMMA)); 
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
  
  emitByte(OP_CALL); 
  emitByte(argc);
}

ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, call,   PREC_CALL},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {identifier,NULL,  PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     and,    PREC_AND},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {boolean,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {nil,      NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     or,     PREC_OR},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {boolean,  NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

static ParseRule *getRule(TokenType token_type) {
  return &rules[token_type];
}

static void statement();
static void declaration();

static void synchronise() {
  parser.panic_mode = false;
  
  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) {
      return;
    }
    
    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_WHILE:
      case TOKEN_IF:
      case TOKEN_RETURN:
      case TOKEN_FOR:
      case TOKEN_VAR:
        return;
    }
    
    advance();
  }
}

static void variableDeclarationLocal(bool is_const) {
  consume(TOKEN_IDENTIFIER, "Expect variable name.");
  Token name = parser.previous;
  
  if (!declareLocal(current, name, is_const)) {
      error("Already a variable with this name in this scope."); 
  }
  
  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  
  current->locals[current->local_count - 1].depth = current->scope_depth;
  
  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
}

static void variableDeclaration(bool is_const) {
  if (current->scope_depth > 0) {
    variableDeclarationLocal(is_const);   
    return;
  }
  
  // Variable declaration at the global scope.
  
  consume(TOKEN_IDENTIFIER, "Expect variable name.");
  size_t length = parser.previous.length;
  const char *chars = parser.previous.start;
  Obj *name = FromString(chars, length);
  emitConstant(FromObj(name)); 
  
  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  
  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
  
  emitByte(OP_VAR_DECL);
  
  Global *global = getGlobal(name);
  global->is_const = is_const;
}

static void expressionStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emitByte(OP_POP);
}

static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }
  
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void ifStatement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after if-condition expression.");
  
  int false_jump = emitJump(OP_JUMP_IF_FALSE);
  
  emitByte(OP_POP);
  statement();
  int true_jump = emitJump(OP_JUMP);
  
  patchJump(false_jump, ip());
  
  emitByte(OP_POP);
  if (match(TOKEN_ELSE)) {
    statement();
  }
  
  patchJump(true_jump, ip());
}

static void whileStatement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  
  int cond_addr = ip();
  declareLoop(current, LOOP_WHILE, cond_addr, current->scope_depth);
  expression();
  
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after while-condition expression.");
  
  int false_jump = emitJump(OP_JUMP_IF_FALSE);
  
  emitByte(OP_POP);
  statement();
  int true_jump = emitJump(OP_JUMP);
  patchJump(true_jump, cond_addr);
  
  patchJump(false_jump, ip()); 
  emitByte(OP_POP);
  
  deleteLoop(current, ip());
}

static void forStatement() {
  // A for loop has three clauses, all optional:
  // - An initiliaser that can be either a variable declaration or an expression.
  //   It runs only once, at the beginning of the loop.
  // - A condition clause, executed _after_ the initiliaser and _before_ each iteration.
  // - An increment expression, which runs _after_ each iteration.
  
  beginScope();
  
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
  
  bool has_loop_variable = false;
  if (match(TOKEN_VAR)) {
    variableDeclarationLocal(/*is_const=*/false);
    has_loop_variable = true;
  } else if (match(TOKEN_CONST)) {
    variableDeclarationLocal(/*is_const=*/true);
  } else if (match(TOKEN_SEMICOLON)) {
    // do nothing
  } else {
    expressionStatement();
  }
  
  int loop_start = ip();
  int block_end = -1;
  if (!check(TOKEN_SEMICOLON)) {
    expression(); // condition expression
    block_end = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after for-condition.");
  
  int end_block_target = loop_start; // if no increment expression, jump to the condition
  if (!check(TOKEN_RIGHT_PAREN)) {
    int body_jump = emitJump(OP_JUMP);
    
    end_block_target = ip();
    expression();
    emitByte(OP_POP); // expression() leaves the value at the top of the stack, so we pop it
    int addr = emitJump(OP_JUMP);
    patchJump(addr, loop_start); // back to condition expression
    
    patchJump(body_jump, ip());
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after for-increment.");
  
  declareLoop(current, LOOP_FOR, end_block_target, current->scope_depth - 1);
  
  // If the for-loop declares a variable in the first clause, then we create a
  // new loop variable for each loop iteration.
  // This makes the behaviour of the program more intuitive when the body of the
  // loop creates closures that close over the loop variable.
  if (has_loop_variable) {
    beginScope();
    duplicateTopLocal();
    emitByte(OP_DUPLICATE);
    
    statement();
    
    endScope();
  } else {
    statement();
  }
  
  int body_end_jump = emitJump(OP_JUMP);
  patchJump(body_end_jump, end_block_target); // to the increment expression
  
  if (block_end != -1) {
    patchJump(block_end, ip()); 
    emitByte(OP_POP);
  }
  
  endScope();
  
  deleteLoop(current, ip());
}

static void switchStatement() {
  // We assume the following:
  // - If there is a "default" clause, it's always the last clause.
  // - There is no fallthrough and no "break" statements.
  
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'switch'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
  consume(TOKEN_LEFT_BRACE, "Expect '{' after ')'.");
  
  int jmps_cap = 8;
  int jmps_size = 0;
  int *jmps = ALLOCATE(int, jmps_cap);
  int jmp_instr = 0; // Jump from a previous 'case' clause that didn't match
  while (match(TOKEN_CASE)) {
    if (jmps_size > 0) {
      patchJump(jmp_instr, ip());
      emitByte(OP_POP); // The result of the previous case test, which is always 'false'.
    }
    
    emitByte(OP_DUPLICATE);
    expression();
    consume(TOKEN_COLON, "Expect ':' after 'case' expression.");
    emitByte(OP_EQ);
    jmp_instr = emitJump(OP_JUMP_IF_FALSE); // To the next 'case'
    emitByte(OP_POPN); // Pops the 'true' from the case test, and the value of the switch expression.
    emitByte(2);
    
    beginScope();
    while (true) {
      if (check(TOKEN_CASE) || check(TOKEN_DEFAULT) || check(TOKEN_RIGHT_BRACE)) {
        // When we stop assuming there's always a default, we have to add one check in the test above.
        break; 
      }
      declaration();
    }
    endScope();
    
    if (jmps_size == jmps_cap) {
      jmps_cap = GROW_CAPACITY(jmps_cap);
      jmps = GROW_ARRAY(int, jmps, jmps_size, jmps_cap);
    }
    jmps[jmps_size++] = emitJump(OP_JUMP); // To right after the switch
  }
  
  // This is executed if none of the 'case' expressions matched the 'switch' expression.
  if (jmps_size > 0) {
    patchJump(jmp_instr, ip());
    emitByte(OP_POPN);
    emitByte(2);
  } else {
    emitByte(OP_POP);
  }
  
  if (match(TOKEN_DEFAULT)) {
    consume(TOKEN_COLON, "Expect ':' after 'default'.");
    
    beginScope();
    while (true) {
      if (check(TOKEN_RIGHT_BRACE)) {
        break;
      }
      declaration();
    }
    endScope();
  }
  
  consume(TOKEN_RIGHT_BRACE, "Expect '}' at the end of 'switch' statement.");
  
  for (int i = 0; i < jmps_size; i++) {
    patchJump(jmps[i], ip());
  }
  
  FREE_ARRAY(int, jmps, jmps_cap);
  
  endScope();
}

static void continueStatement() {
  Loop *loop = enclosingLoop(current);
  if (loop == NULL) {
    error("'continue' statement not enclosed in a loop.");
    return;
  }
  
  consume(TOKEN_SEMICOLON, "Expect ';' after 'continue'.");
  
  uint8_t n;
  if (loop->type == LOOP_WHILE) {
    n = numLocals(current, loop->depth + 1);
  } else if (loop->type == LOOP_FOR) {
    n = numLocals(current, loop->depth + 2);
  } else {
    error("Unsupported loop type.");
    return;
  }
  
  if (n == 1) {
    emitByte(OP_POP);
  } else if (n > 1) {
    emitByte(OP_POPN);
    emitByte(n);
  }
  
  int instr = emitJump(OP_JUMP);
  patchJump(instr, loop->address);
}

static void breakStatement() {
  Loop *loop = enclosingLoop(current);
  if (loop == NULL) {
    error("'break' statement not enclosed in a loop.");
    return;
  }
  
  consume(TOKEN_SEMICOLON, "Expect ';' after 'break'.");
  
  if (loop->type != LOOP_WHILE && loop->type != LOOP_FOR) {
    error("Unsupported loop type.");
    return;
  }
  uint8_t n = numLocals(current, loop->depth + 1);
  
  if (n == 1) {
    emitByte(OP_POP);
  } else if (n > 1) {
    emitByte(OP_POPN);
    emitByte(n);
  }
  
  int instr = emitJump(OP_JUMP);
  scheduleBreakJump(loop, instr);
}

static void functionDeclaration() {
  consume(TOKEN_IDENTIFIER, "Expect identifier after 'fun'.");
  Token name = parser.previous; 
  Obj *name_obj = FromString(name.start, name.length);
  
  // Declare the function before defining it because the function might be recursive.
  bool is_global = false;
  if (current->scope_depth > 0) {
    // Function declared inside a block or inside the body of another function
    if (!declareLocal(current, name, /*is_const=*/ false)) {
        error("Already an identifier with this name in this scope."); 
    }
    current->locals[current->local_count - 1].depth = current->scope_depth;
  } else {
    // Function declared in the global scope
    is_global = true;
    Global *global = getGlobal(name_obj);
    global->is_const = false;
    emitConstant(FromObj(name_obj)); 
  }
  
  Compiler compiler;
  initCompiler(&compiler, TYPE_FUNCTION);
  compiler.function->name = (ObjString*) name_obj; IncrementRefcountObject((Obj*) name_obj);
  beginScope();
  
  consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
  
  // Parse parameters
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      current->function->arity++;
      if (current->function->arity > 255) {
        errorAtCurrent("Can't have more than 255 parameters.");
      }
      
      consume(TOKEN_IDENTIFIER, "Expect parameter name.");
      Token par = parser.previous;
      if (!declareLocal(current, par, /*is_const=*/ false)) {
          error("Already a parameter with this name in parameter list."); 
      }
      current->locals[current->local_count - 1].depth = current->scope_depth;
    } while (match(TOKEN_COMMA)); 
  }
  
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters list.");
  consume(TOKEN_LEFT_BRACE, "Expect '{' after ')'.");
  block();
  
  ObjFunction *function = endCompiler();
  emitClosure(function, compiler.upvalues);
  if (is_global) {
    emitByte(OP_VAR_DECL); 
  }
  DecrementRefcountObject((Obj*) function); // initCompiler() initialises the function's refcount to 1
}

static void returnStatement() {
  if (current->type == TYPE_SCRIPT) {
    error("Can't return from top-level code.");  
  }
  
  if (match(TOKEN_SEMICOLON)) {
    nil(/*can_assign=*/ false);
    emitByte(OP_RETURN);
    return;
  }
  
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after return expression.");
  emitByte(OP_RETURN);
}

static void statement() {
  if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
  } else if (match(TOKEN_IF)) {
    ifStatement();
  } else if (match(TOKEN_WHILE)) {
    whileStatement();  
  } else if (match(TOKEN_FOR)) {
    forStatement();  
  } else if (match(TOKEN_SWITCH)) {
    switchStatement();
  } else if (match(TOKEN_CONTINUE)) {
    continueStatement();  
  } else if (match(TOKEN_BREAK)) {
    breakStatement();  
  } else if (match(TOKEN_RETURN)) {
    returnStatement();  
  } else {
    expressionStatement();
  }
}

static void declaration() {
  if (match(TOKEN_VAR)) {
    variableDeclaration(/*is_const=*/false);
  } else if (match(TOKEN_CONST)) {
    variableDeclaration(/*is_const=*/true);
  } else if (match(TOKEN_FUN)) {
    functionDeclaration();
  } else {
    statement(); 
  }
  
  if (parser.panic_mode) {
    synchronise();
  }
}

ObjFunction *Compile(const char *source) {
  InitScanner(source);
  parser.had_error = false;
  parser.panic_mode = false;
  
  Compiler compiler;
  initCompiler(&compiler, TYPE_SCRIPT);
  
  advance();
  
  while (!match(TOKEN_EOF)) {
    declaration();
  }
  
  for (uint8_t i = 0; i < Global_count; i++) {
    if (Globals[i].is_const && Globals[i].reassigned) {
      errorAt(&Globals[i].reassign_token, "Can't reassign to const global variable.");
    }
  }
  FREE_ARRAY(Global, Globals, Global_capacity);
  
  ObjFunction *function = endCompiler();
  
  freeCompiler(&compiler);
  current = NULL;
  
  return parser.had_error ? NULL : function;
}
