class Token {
   final TokenType type; 
   final String lexeme; // The raw string from the source code that generated the token.
   final Object literal;
   final int line;
   
   Token(TokenType type, String lexeme, Object literal, int line) {
       this.type = type;
       this.lexeme = lexeme;
       this.literal = literal;
       this.line = line;
   }
   
   public String toString() {
       return lexeme;
       //return type + " " + lexeme + " " + literal;
   }
}