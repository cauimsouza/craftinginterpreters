enum TokenType {
    // Single-character tokens.
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,
    QUESTION, COLON, LEFT_BRACKET, RIGHT_BRACKET,
    
    // One or two character tokens.
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,
    PLUS_PLUS, MINUS_MINUS,
    
    // Literals.
    IDENTIFIER, STRING, NUMBER,
    
    // Keywords.
    AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
    RETURN, SUPER, THIS, TRUE, VAR, WHILE, BREAK,
    
    EOF
}