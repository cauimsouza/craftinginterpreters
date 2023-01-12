import java.util.List;
import java.util.ArrayList;

class Parser {
    // Private because error recovery is the parser's job.
  private static class ParseError extends RuntimeException {}

  private final List<Token> tokens;
  private int current = 0;

  Parser(List<Token> tokens) {
    this.tokens = tokens;
  }
  
  List<Stmt> parse() {
      try {
          List<Stmt> stmts = new ArrayList<>();
          
          while (!isAtEnd()) {
             Stmt s = declaration(); 
             stmts.add(s);
          }
          return stmts;
      } catch (ParseError e) {
          return null;
      }
  }
  
    private Stmt declaration() {
        if (match(TokenType.VAR)) return varDeclStmt();
        if (match(TokenType.LEFT_BRACE)) return blockStmt();
        
        return statement();
    }
  
    private Stmt varDeclStmt() {
        consume(TokenType.IDENTIFIER, "Expect identifier.");
        Token t = previous();
        
        Expr expr = null;
        if (match(TokenType.EQUAL)) expr = expression();
        
        consume(TokenType.SEMICOLON, "Expect semicolon.");
        return new Stmt.VarDeclStmt(t, expr);
    }
    
    private Stmt blockStmt() {
        List<Stmt> stmts = new ArrayList<>();
        while (!isAtEnd() && peek().type != TokenType.RIGHT_BRACE) {
            stmts.add(declaration());
        }
        consume(TokenType.RIGHT_BRACE, "Expect right brace.");
        return new Stmt.BlockStmt(stmts);
    }
  
    private Stmt statement() {
        if (match(TokenType.PRINT)) return printStmt();
        
        // If the next token doesn't look like any known kind of statement,
        // we assume it's an expression.
        Expr expr = expression();
        if (match(TokenType.EQUAL)) {
            Expr value = expression();
            consume(TokenType.SEMICOLON, "Expect semicolon.");
            if (expr instanceof Expr.Variable) {
                return new Stmt.AssignStmt(((Expr.Variable) expr).name, value);
            }
            throw error(previous(), "Invalid assignment target.");
        }
        
        consume(TokenType.SEMICOLON, "Expect semicolon.");
        return new Stmt.ExprStmt(expr);
    }
  
  private Stmt printStmt() {
      Expr expr = expression();
      consume(TokenType.SEMICOLON, "Expect semicolon.");
      return new Stmt.PrintStmt(expr);
  }
  
  private Stmt exprStmt() {
      Expr expr = expression();
      consume(TokenType.SEMICOLON, "Expect semicolon.");
      return new Stmt.ExprStmt(expr);
  }
  
  private Expr expression() {
      return sequence();
  }
  
  private Expr sequence() {
     Expr left = ternary();
     
     while (match(TokenType.COMMA)) {
         Token t = previous();
         Expr right = ternary();
         left = new Expr.Binary(left, t, right);
     }
     
     return left;
  }
  
  private Expr ternary() {
      Expr left = equality();
      
      if (match(TokenType.QUESTION)) {
          Token t = previous();
          Expr mid = ternary();
          consume(TokenType.COLON, "Expect ':' after expression.");
          Expr right = ternary();
          
          left = new Expr.Ternary(t, left, mid, right);
      }
      return left;
  }
  
  private Expr equality() {
      Expr left = comparison();
      
      while (match(TokenType.EQUAL_EQUAL, TokenType.BANG_EQUAL)) {
          Token t = previous();
          Expr right = comparison();
          left = new Expr.Binary(left, t, right);
       }
      
      return left;
  }
  
  private Expr comparison() {
     Expr left = term();
     
     while (match(TokenType.LESS, TokenType.LESS_EQUAL, TokenType.GREATER, TokenType.GREATER_EQUAL)) {
          Token t = previous();
          Expr right = term();
          left = new Expr.Binary(left, t, right);
     }
     
     return left;
  }
  
  private Expr term() {
     Expr left = factor();
     
     while (match(TokenType.PLUS, TokenType.MINUS)) {
          Token t = previous();
          Expr right = factor();
          left = new Expr.Binary(left, t, right);
     }
     
     return left;
  }
  
  private Expr factor() {
     Expr left = unary();
     
     while (match(TokenType.STAR, TokenType.SLASH)) {
          Token t = previous();
          Expr right = unary();
          left = new Expr.Binary(left, t, right);
     }
     
     return left;
  }
  
  private Expr unary() {
     ArrayList<Token> ops = new ArrayList<>();
     
     boolean readOp = true;
     while (readOp) {
         readOp = false;
         switch (peek().type) {
            case PLUS:
            case STAR:
            case SLASH:
            case BANG_EQUAL:
            case EQUAL_EQUAL:
            case GREATER_EQUAL:
            case GREATER:
            case LESS_EQUAL:
            case LESS:
              error(peek(), "Unary '" + peek().lexeme + "' expressions are not supported.");
            case MINUS:
            case BANG:
                ops.add(advance());
                readOp = true;
                break;
         }
     }
     
     Expr expr = primary();
     
     for (int i = ops.size() - 1; i >= 0; i--) {
         expr = new Expr.Unary(ops.get(i), expr);
     }
     
     return expr;
  }
  
  private Expr primary() {
      if (match(TokenType.NUMBER, TokenType.STRING,
                TokenType.FALSE, TokenType.TRUE,
                TokenType.NIL)) {
          return new Expr.Literal(previous().literal);
      }
      if (match(TokenType.IDENTIFIER)) return new Expr.Variable(previous());
      
      if (match(TokenType.LEFT_PAREN)) {
          Expr expr = expression();
          consume(TokenType.RIGHT_PAREN, "Expect ')' after expression.");
          return new Expr.Grouping(expr);
      }
      
      throw error(peek(), "Expect expression.");
    }
  
  private void consume(TokenType type, String message) {
     if (match(type)) return; 
     
     throw error(peek(), message);
  }
  
  private ParseError error(Token token, String message) {
      Lox.error(token, message);
      return new ParseError();
  }
  
  private void synchronise() {
      advance();
      
      while (!isAtEnd()) {
          if (previous().type == TokenType.SEMICOLON) return;
          
          switch (peek().type) {
              case CLASS:
              case FUN:
              case FOR:
              case IF:
              case PRINT:
              case RETURN:
              case VAR:
              case WHILE:
                  return;
          }
          
          advance();
      }
  }
  
  private boolean match(TokenType... types) {
      for (TokenType type : types) {
          if (check(type)) {
              advance();
              return true;
          }
      }
      return false;
  }
  
  private boolean check(TokenType type) {
    if (isAtEnd()) return false;
    return peek().type == type;
  }
  
  private Token advance() {
      if (!isAtEnd()) current++;
      return previous();
  }
  
  private boolean isAtEnd() {
      return peek().type == TokenType.EOF;
  }
  
  private Token peek() {
      return tokens.get(current);
  }
  
  private Token previous() {
      return tokens.get(current - 1);
  }
}