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
  Token name;
  
  // depth is the depth of the scope in which the local variable was declared or -1.
  // depth is -1 iff the variable was added to the scope but is not yet ready for use.
  int depth;
} Local;

typedef struct {
  Local locals[UINT8_COUNT];
  int local_count;
  int scope_depth;
} Compiler;

Parser parser;
Compiler *current = NULL;
Chunk *compilingChunk;

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

static void emitConstant(Value value) {
  WriteConstant(compilingChunk, value, parser.previous.line);
}

static void initCompiler(Compiler *compiler) {
  compiler->local_count = 0;
  compiler->scope_depth = 0;
  current = compiler;
}

static void emitBoolean(bool boolean) {
  uint8_t byte = boolean ? OP_TRUE : OP_FALSE;
  emitByte(byte);
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

static bool sameVariable(Token a, Token b) {
    return a.length == b.length && memcmp(a.start, b.start, a.length) == 0;
}

static int findLocal(Token name) {
  for (int i = current->local_count - 1; i >= 0; i--) {
    if (sameVariable(name, current->locals[i].name)) {
      if (current->locals[i].depth < 0) {
        error("Can't read local variable being initialised.");
      }
      return i;
    }
  }
  
  return -1;
}

// Similar to identifier, but for cases where the identifier is in a local scope.
static void identifierLocal(bool can_assign, int local_index) {
  if (can_assign && match(TOKEN_EQUAL)) {
    parsePrecedence(PREC_ASSIGNMENT);
    
    emitByte(OP_ASSIGN_LOCAL);
    emitByte(local_index);
    
    return;
  }
  
  emitByte(OP_IDENT_LOCAL);
  emitByte(local_index);
}

static void identifier(bool can_assign) {
  int index = findLocal(parser.previous);
  if (index >= 0) {
    identifierLocal(can_assign, index);
    return;
  }
  
  size_t length = parser.previous.length;
  const char *chars = parser.previous.start;
  emitConstant(FromObj(FromString(chars, length))); 
  
  if (can_assign && match(TOKEN_EQUAL)) {
    parsePrecedence(PREC_ASSIGNMENT);
    emitByte(OP_ASSIGN_GLOBAL);
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
    case TOKEN_OR:
      emitByte(OP_OR);
      break;
    case TOKEN_AND:
      emitByte(OP_AND);
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
  [TOKEN_IDENTIFIER]    = {identifier,NULL, PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     binary, PREC_AND},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {boolean,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {nil,      NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     binary, PREC_OR},
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
  for (; current->local_count > 0; current->local_count--) {
    if (current->locals[current->local_count - 1].depth != current->scope_depth) {
      break;
    }
    emitByte(OP_POP);
  }
  
  current->scope_depth--;
}

static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }
  
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void statement() {
  if (match(TOKEN_PRINT)) {
    printStatement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
  } else {
    expressionStatement();
  }
}

static void variableDeclarationLocal() {
  if (current->local_count == UINT8_COUNT) {
    error("Too many local variables in this function.");   
    return;
  }
  
  consume(TOKEN_IDENTIFIER, "Expect variable name.");
  Token name = parser.previous;
  
  for (size_t i = current->local_count; i > 0; i--) {
    Local *local = &current->locals[i - 1];
    if (local->depth < current->scope_depth) {
      break;
    }
    
    if (sameVariable(name, local->name)) {
      error("Already a variable with this name in this scope."); 
      return;
    }
  }
  
  Local *local = &current->locals[current->local_count++];
  local->name = name;
  local->depth = -1;
  
  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  
  local->depth = current->scope_depth;
  
  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
}

static void variableDeclaration() {
  if (current->scope_depth > 0) {
    variableDeclarationLocal();   
    return;
  }
  
  // Variable declaration at the global scope.
  
  consume(TOKEN_IDENTIFIER, "Expect variable name.");
  size_t length = parser.previous.length;
  const char *chars = parser.previous.start;
  emitConstant(FromObj(FromString(chars, length))); 
  
  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  
  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
  
  emitByte(OP_VAR_DECL);
}

static void declaration() {
  if (match(TOKEN_VAR)) {
    variableDeclaration();
  } else {
    statement(); 
  }
  
  if (parser.panic_mode) {
    synchronise();
  }
}

bool Compile(const char *source, Chunk *chunk) {
  InitScanner(source);
  Compiler compiler;
  initCompiler(&compiler);
  parser.had_error = false;
  parser.panic_mode = false;
  compilingChunk = chunk;
  
  advance();
  
  while (!match(TOKEN_EOF)) {
    declaration();
  }
  
  emitByte(OP_RETURN);
  
  #ifdef DEBUG_PRINT_CODE
  if (!parser.had_error) {
    DisassembleChunk(compilingChunk, "code");
  }
  #endif
  
  return !parser.had_error;
}