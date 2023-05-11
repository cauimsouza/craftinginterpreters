#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "scanner.h"
#include "value.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

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

typedef struct {
  Obj *name;
  bool is_const;
  
  bool reassigned; // true iff the global was reassigned
  Token reassign_token; // assignment operator (=) where the global was reassigned
} Global;

typedef struct {
  Token name;
  bool is_const;
  
  // depth is the depth of the scope in which the local variable was declared or -1.
  // depth is -1 iff the variable was added to the scope but is not yet ready for use.
  int depth;
} Local;

typedef struct {
  Global *globals;
  int global_count;
  int global_capacity;
  
  Local *locals;
  int local_count;
  int local_capacity;
  
  int scope_depth;
} Compiler;

Parser parser;
Compiler *current = NULL;
Chunk *compilingChunk;

static bool sameVariable(Token a, Token b) {
    return a.length == b.length && memcmp(a.start, b.start, a.length) == 0;
}

static void initCompiler(Compiler *compiler) {
  compiler->globals = NULL;
  compiler->global_count = 0;
  compiler->global_capacity = 0;
  
  compiler->locals = NULL;
  compiler->local_count = 0;
  compiler->local_capacity = 0;
  
  compiler->scope_depth = 0;
}

static void freeCompiler(Compiler *compiler) {
  FREE_ARRAY(Global, compiler->globals, compiler->global_capacity);
  FREE_ARRAY(Local, compiler->locals, compiler->local_capacity);
  initCompiler(compiler);
}

static void growLocals(Compiler *compiler) {
  compiler->local_capacity = GROW_CAPACITY(compiler->local_capacity);
  compiler->locals = GROW_ARRAY(Local, compiler->locals, compiler->local_count, compiler->local_capacity);
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
  
  if (compiler->local_count == compiler->local_capacity) {
    growLocals(compiler);
  }
  
  Local *local = &compiler->locals[compiler->local_count++];
  local->name = name;
  local->is_const = is_const;
  local->depth = -1;
  
  return true;
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

static void deleteLocal(Compiler *compiler) {
  compiler->local_count--;
}

static void growGlobals(Compiler *compiler) {
  compiler->global_capacity = GROW_CAPACITY(compiler->global_capacity);
  compiler->globals = GROW_ARRAY(Global, compiler->globals, compiler->global_count, compiler->global_capacity);
}

// getGlobal returns the global with the given name (if it already exists) or
// creates a new global with the given name (if it doesn't exist yet).
static Global *getGlobal(Compiler *compiler, Obj *name) {
  for (uint8_t i = 0; i < compiler->global_count; i++) {
    if (compiler->globals[i].name == name) {
      return &compiler->globals[i];
    }
  }
  
  if (compiler->global_count == compiler->global_capacity) {
    growGlobals(compiler);
  }
  
  Global *global = &compiler->globals[compiler->global_count++];
  global->name = name;
  global->is_const = false;
  global->reassigned = false;
  
  return global;
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

static void emitByte(uint8_t byte) {
  WriteChunk(compilingChunk, byte, parser.previous.line);
}

static void emitByteAt(uint8_t byte, uint8_t address) {
  compilingChunk->code[address] = byte;
}

static int ip() {
  return compilingChunk->count;
}

static void emitConstant(Value value) {
  WriteConstant(compilingChunk, value, parser.previous.line);
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
  int size = jump_dst - jump_instr - 3;
  emitByteAt(size & 0xFF, jump_instr + 1); // little endian
  size >>= 8;
  emitByteAt(size & 0xFF, jump_instr + 2);
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

// Similar to identifier, but for cases where the identifier is in a local scope.
static void identifierLocal(Compiler *compiler, bool can_assign, int index) {
  if (compiler->locals[index].depth < 0) {
      error("Can't read local variable being initialised.");
      return;
  }
  
  if (can_assign && match(TOKEN_EQUAL)) {
    if (compiler->locals[index].is_const) {
      error("Can't reassign to const local variable.");
      return;
    }
    
    parsePrecedence(PREC_ASSIGNMENT);
    
    emitByte(OP_ASSIGN_LOCAL);
    emitByte(index);
    
    return;
  }
  
  emitByte(OP_IDENT_LOCAL);
  emitByte(index);
}

static void identifier(bool can_assign) {
  int index = findLocal(current, parser.previous);
  if (index >= 0) {
    identifierLocal(current, can_assign, index);
    return;
  }
  
  Obj *name = FromString(parser.previous.start, parser.previous.length);
  emitConstant(FromObj(name)); 
  
  if (can_assign && match(TOKEN_EQUAL)) {
    Token op = parser.previous;
    
    parsePrecedence(PREC_ASSIGNMENT);
    emitByte(OP_ASSIGN_GLOBAL);
    
    Global *global = getGlobal(current, name);
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

ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
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
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
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
      case TOKEN_PRINT:
      case TOKEN_RETURN:
      case TOKEN_FOR:
      case TOKEN_VAR:
        return;
    }
    
    advance();
  }
}

static void printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emitByte(OP_PRINT);
}

static void expressionStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emitByte(OP_POP);
}

static void beginScope() {
  current->scope_depth++;
}

static void endScope() {
  uint8_t n = 0;
  for (; current->local_count > 0; deleteLocal(current)) {
    if (current->locals[current->local_count - 1].depth != current->scope_depth) {
      break;
    }
    n++;
  }
  
  current->scope_depth--;
  
  emitByte(OP_POPN);
  emitByte(n);
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

static void statement() {
  if (match(TOKEN_PRINT)) {
    printStatement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
  } else if (match(TOKEN_IF)) {
    ifStatement();
  } else {
    expressionStatement();
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
  
  Global *global = getGlobal(current, name);
  global->is_const = is_const;
}

static void declaration() {
  if (match(TOKEN_VAR)) {
    variableDeclaration(/*is_const=*/false);
  } else if (match(TOKEN_CONST)) {
    variableDeclaration(/*is_const=*/true);
  } else {
    statement(); 
  }
  
  if (parser.panic_mode) {
    synchronise();
  }
}

bool Compile(const char *source, Chunk *chunk) {
  InitScanner(source);
  parser.had_error = false;
  parser.panic_mode = false;
  compilingChunk = chunk;
  
  Compiler compiler;
  initCompiler(&compiler);
  current = &compiler;
  
  advance();
  
  while (!match(TOKEN_EOF)) {
    declaration();
  }
  
  emitByte(OP_RETURN);
  
  for (uint8_t i = 0; i < compiler.global_count; i++) {
    if (compiler.globals[i].is_const && compiler.globals[i].reassigned) {
      errorAt(&compiler.globals[i].reassign_token, "Can't reassign to const global variable.");
    }
  }
  
  #ifdef DEBUG_PRINT_CODE
  if (!parser.had_error) {
    DisassembleChunk(compilingChunk, "code");
  }
  #endif
  
  freeCompiler(&compiler);
  
  return !parser.had_error;
}