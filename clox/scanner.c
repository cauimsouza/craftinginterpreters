#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "scanner.h"

typedef struct {
  const char *start;
  const char *current;
  int line;
} Scanner;

Scanner scanner;

void InitScanner(const char *source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static char current() {
    return *scanner.current;
}

static char next() {
    return *(scanner.current + 1);
}

static char advance() {
    return *scanner.current++;
}

static bool match(char c) {
    if (*scanner.current == c) {
        scanner.current++;
        return true;
    }
    return false;
}

static bool isAtEnd() {
    return current() == '\0';
}

static bool isAlpha(char c) {
    return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_';
}

static bool isAlphaNum(char c) {
    return isAlpha(c) || isdigit(c);
}

static Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int) (scanner.current - scanner.start);
    token.line = scanner.line;
    
    return token;
}

static Token errorToken(const char *message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int) strlen(message);
    token.line = scanner.line;
    
    return token;
}

static Token scanIdentifierOrKeyword() {
    while (isAlphaNum(current())) {
        scanner.current++;
    }
    
    char lexeme[64];
    char *lexemep = lexeme;
    for (const char *c = scanner.start; c != scanner.current; c++) {
        *lexemep = *c;
        lexemep++;
    }
    *lexemep = '\0';
    
    TokenType type = TOKEN_IDENTIFIER;
    if (!strcmp(lexeme, "and")) {
        type = TOKEN_AND;
    } else if (!strcmp(lexeme, "class")) {
        type = TOKEN_CLASS;
    } else if (!strcmp(lexeme, "else")) {
        type = TOKEN_ELSE;
    } else if (!strcmp(lexeme, "false")) {
        type = TOKEN_FALSE;
    } else if (!strcmp(lexeme, "for")) {
        type = TOKEN_FOR;
    } else if (!strcmp(lexeme, "fun")) {
        type = TOKEN_FUN;
    } else if (!strcmp(lexeme, "if")) {
        type = TOKEN_IF;
    } else if (!strcmp(lexeme, "nil")) {
        type = TOKEN_NIL;
    } else if (!strcmp(lexeme, "or")) {
        type = TOKEN_OR;
    } else if (!strcmp(lexeme, "print")) {
        type = TOKEN_PRINT;
    } else if (!strcmp(lexeme, "return")) {
        type = TOKEN_RETURN;
    } else if (!strcmp(lexeme, "super")) {
        type = TOKEN_SUPER;
    } else if (!strcmp(lexeme, "this")) {
        type = TOKEN_THIS;
    } else if (!strcmp(lexeme, "true")) {
        type = TOKEN_TRUE;
    } else if (!strcmp(lexeme, "var")) {
        type = TOKEN_VAR;
    } else if (!strcmp(lexeme, "const")) {
        type = TOKEN_CONST;
    } else if (!strcmp(lexeme, "while")) {
        type = TOKEN_WHILE;
    }
    
    return makeToken(type);
}

static Token scanNumber() {
    while (isdigit(current())) {
        advance();
    }
    
    if (!match('.')) {
        return makeToken(TOKEN_NUMBER);
    }
    
    while (isdigit(current())) {
        advance();
    }
    
    return makeToken(TOKEN_NUMBER);
}

static Token scanString() {
    for (;;) {
        if (isAtEnd()) {
            break;
        }
        
        if (match('"')) {
            return makeToken(TOKEN_STRING);
        }
        
        if (match('\\') && isAtEnd()) {
            break;
        }
        
        advance();
    }
    
    return errorToken("Non-terminated string literal.");
}

static void skipSingleLineComment() {
    for (;;) {
        if (isAtEnd()) {
            return;
        }
        if (match('\n')) {
            scanner.line++;
            return;
        }
        advance();
    }
}

static bool skipMultiLineComment() {
    for (;;) {
        if (isAtEnd()) {
            return false;
        }
        
        if (current() == '*' && next() == '/') {
            advance();
            advance();
            return true;
        } 
        advance();
    }
}

static void skipSpace(char c) {
    for (;;) {
        if (c == '\n') {
            scanner.line++;
        }
        if (isspace(current())) {
            c = advance();
            continue;
        }
        return;
    }
}

Token ScanToken() {
    scanner.start = scanner.current;
    
    if (isAtEnd()) {
        return makeToken(TOKEN_EOF);
    }

    char c = advance();
    switch (c) {
        case '(':
            return makeToken(TOKEN_LEFT_PAREN);
        case ')':
            return makeToken(TOKEN_RIGHT_PAREN);
        case '{':
            return makeToken(TOKEN_LEFT_BRACE);
        case '}':
            return makeToken(TOKEN_RIGHT_BRACE);
        case ';':
            return makeToken(TOKEN_SEMICOLON);
        case ',':
            return makeToken(TOKEN_COMMA);
        case '.':
            return makeToken(TOKEN_DOT);
        case '-':
            return makeToken(TOKEN_MINUS);
        case '+':
            return makeToken(TOKEN_PLUS);
        case '/':
            if (match('/')) {
                skipSingleLineComment();
                return ScanToken();
            }
            if (match('*')) {
                if (skipMultiLineComment()) {
                    return ScanToken();
                }
                return errorToken("Non-terminated multi-line comment.");
            }
            return makeToken(TOKEN_SLASH);
        case '*':
            return makeToken(TOKEN_STAR);
        case '!':
            if (match('=')) {
                return makeToken(TOKEN_BANG_EQUAL);
            }
            return makeToken(TOKEN_BANG);
        case '=':
            if (match('=')) {
                return makeToken(TOKEN_EQUAL_EQUAL);
            }
            return makeToken(TOKEN_EQUAL);
        case '>':
            if (match('=')) {
                return makeToken(TOKEN_GREATER_EQUAL);
            }
            return makeToken(TOKEN_GREATER);
        case '<':
            if (match('=')) {
                return makeToken(TOKEN_LESS_EQUAL);
            }
            return makeToken(TOKEN_LESS);
        case '"':
            return scanString();
        default:
            if (isspace(c)) {
                skipSpace(c);
                return ScanToken();
            }
            if (isAlpha(c)) {
                return scanIdentifierOrKeyword();
            }
            if (isdigit(c)) {
                return scanNumber();
            }
    }
    
    return errorToken("Unexpected character.");
}
