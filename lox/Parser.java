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
        if (peek().type == TokenType.FUN && next().type == TokenType.IDENTIFIER) {
            match(TokenType.FUN);
            return funDeclStmt();
        }
        
        return statement();
    }
  
    private Stmt varDeclStmt() {
        consume(TokenType.IDENTIFIER, "Expect identifier.");
        Token t = previous();
        
        if (match(TokenType.EQUAL)) {
            Expr expr = expression();
            consume(TokenType.SEMICOLON, "Expect semicolon.");
            return new Stmt.VarDeclStmt(t, expr);
        }
        
        consume(TokenType.SEMICOLON, "Expect semicolon.");
        return new Stmt.VarDeclStmt(t);
    }
    
    private Stmt funDeclStmt() {
        consume(TokenType.IDENTIFIER, "Expect identifier.");
        Token funName = previous();
        
        consume(TokenType.LEFT_PAREN, "Expect '(' after function name.");
        
        List<Token> pars = parameters();
        
        consume(TokenType.RIGHT_PAREN, "Expect ')' after parameter list.");
        
        consume(TokenType.LEFT_BRACE, "Expect '{' after parameter list.");
        Stmt.BlockStmt body = (Stmt.BlockStmt) blockStmt(); 
        
        return new Stmt.FunDeclStmt(funName, pars, body);
    }
    
    // parameters doesn't consume the closing parenthesis ')'.
    private List<Token> parameters() {
        List<Token> pars = new ArrayList<>();
        
        if (peek().type == TokenType.RIGHT_PAREN) return pars;
        
        while (true) {
            if (pars.size() >= 255) {
                error(peek(), "Can't have more than 255 parameters.");
            }
            
            consume(TokenType.IDENTIFIER, "Expect identifier.");
            for (Token t : pars) {
                if (t.lexeme.equals(previous().lexeme)) {
                    error(previous(), "Can't have multiple parameters with the same name.");
                }
            }
            pars.add(previous());
            
            if (peek().type == TokenType.RIGHT_PAREN) break;
            consume(TokenType.COMMA, "Expect ',' or ')' after parameter.");
        } 
        return pars;
    }
    
    private Stmt statement() {
        if (match(TokenType.LEFT_BRACE)) return blockStmt();
        if (match(TokenType.IF)) return ifStmt();
        if (match(TokenType.WHILE)) return whileStmt();
        if (match(TokenType.FOR)) return forStmt();
        if (match(TokenType.BREAK)) return breakStmt();
        if (match(TokenType.RETURN)) return returnStmt();
        
        // If the next token doesn't look like any known kind of statement,
        // we assume it's an expression.
        Expr expr = expression();
        consume(TokenType.SEMICOLON, "Expect semicolon.");
        return new Stmt.ExprStmt(expr);
    }
  
    private Stmt blockStmt() {
        List<Stmt> stmts = new ArrayList<>();
        while (!isAtEnd() && peek().type != TokenType.RIGHT_BRACE) {
            stmts.add(declaration());
        }
        consume(TokenType.RIGHT_BRACE, "Expect right brace.");
        return new Stmt.BlockStmt(stmts);
    }
  
  
  private Stmt ifStmt() {
      consume(TokenType.LEFT_PAREN, "Expect '(' after 'if'.");
      Expr expr = expression();
      consume(TokenType.RIGHT_PAREN, "Expect ')' after if condition.");
      Stmt ifStmt = statement();
      
      Stmt elseStmt = null;
      if (match(TokenType.ELSE)) elseStmt = statement(); 
      
      return new Stmt.IfStmt(expr, ifStmt, elseStmt);
  }
  
    private Stmt whileStmt() {
        consume(TokenType.LEFT_PAREN, "Expect '(' after 'while'.");
        Expr expr = expression();
        consume(TokenType.RIGHT_PAREN, "Expect ')' after while condition.");
        Stmt stmt = statement();
        
        return new Stmt.WhileStmt(expr, stmt);
    }
  
    private Stmt forStmt() {
        consume(TokenType.LEFT_PAREN, "Expect '(' after for.");
        Token t = previous();
        
        Stmt initialiser = null;
        if (!match(TokenType.SEMICOLON)) {
            initialiser = declaration();
            if (!(initialiser instanceof Stmt.VarDeclStmt) && !(initialiser instanceof Stmt.ExprStmt)) {
                throw error(t, "Expect var declaration or expression or semicolon after '('.");
            }
        }
        
        Expr condition = null;
        if (!match(TokenType.SEMICOLON)) {
            condition = expression();
            consume(TokenType.SEMICOLON, "Expect ';' after for condition.");
        }
        
        Expr increment = null;
        if (!match(TokenType.RIGHT_PAREN)) {
            increment = expression();
            consume(TokenType.RIGHT_PAREN, "Expect ')' after for increment.");
        }
        
        Stmt body = statement();
        
        List<Stmt> stmts = new ArrayList<>(); 
        if (initialiser != null) stmts.add(initialiser);
        if (increment != null) {
            List<Stmt> whileBody = new ArrayList<>();
            whileBody.add(body);
            whileBody.add(new Stmt.ExprStmt(increment));
            body = new Stmt.BlockStmt(whileBody);
        }
        if (condition == null) condition = new Expr.Literal(true);
        stmts.add(new Stmt.WhileStmt(condition, body));
        return new Stmt.BlockStmt(stmts);
    }
    
    private Stmt breakStmt() {
        Token t = previous();
        consume(TokenType.SEMICOLON, "Expect semicolon after 'break'.");
        return new Stmt.BreakStmt(t);
    }
    
    private Stmt returnStmt() {
        Expr expr = null;
        if (peek().type != TokenType.SEMICOLON) {
            expr = expression();
        }
        consume(TokenType.SEMICOLON, "Expect semicolon after return expression.");
        return new Stmt.ReturnStmt(expr);
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
     Expr left = assignment();
     
     while (match(TokenType.COMMA)) {
         Token t = previous();
         Expr right = assignment();
         left = new Expr.Binary(left, t, right);
     }
     
     return left;
  }
  
  private Expr assignment() {
      Expr expr = ternary();
      
      if (match(TokenType.EQUAL)) {
          Token t = previous();
          Expr right = assignment();
          if (expr instanceof Expr.Variable) {
            return new Expr.Assign(((Expr.Variable) expr).name, right);
          }
          throw error(t, "Invalid assignment target.");
      }
      
      return expr;
  }
  
  private Expr ternary() {
      Expr left = or();
      
      if (match(TokenType.QUESTION)) {
          Token t = previous();
          Expr mid = or();
          consume(TokenType.COLON, "Expect ':' after expression.");
          Expr right = or();
          
          left = new Expr.Ternary(t, left, mid, right);
      }
      return left;
  }
  
  private Expr or() {
      Expr left = and();
      
      while (match(TokenType.OR)) {
          Token t = previous();
          Expr right = and();
          left = new Expr.Binary(left, t, right);
      }
      
      return left;
  }
  
  private Expr and() {
      Expr left = equality();
      
      while (match(TokenType.AND)) {
          Token t = previous();
          Expr right = equality();
          left = new Expr.Binary(left, t, right);
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
     
     Expr expr = call();
     
     for (int i = ops.size() - 1; i >= 0; i--) {
         expr = new Expr.Unary(ops.get(i), expr);
     }
     
     return expr;
  }
  
    private Expr call() {
        // primary -> primary (arguments?)*
        // arguments -> assignment ("," assignment)* It can be any expression with higher precedence than sequence
        Expr expr = primary();
        
        while (match(TokenType.LEFT_PAREN)) {
            Token paren = previous();
            List<Expr> args = arguments(); 
            consume (TokenType.RIGHT_PAREN, "Expect ')' after arguments.");
            expr = new Expr.Call(expr, paren, args);
        }
        return expr;
    }
    
    private List<Expr> arguments() {
        // arguments -> assignment ("," assignment)* It can be any expression with higher precedence than sequence
        List<Expr> args = new ArrayList<Expr>();
        
        if (peek().type == TokenType.RIGHT_PAREN) return args;
        
        while (true) {
            if (args.size() >= 255) {
                error(peek(), "Cant' have more than 255 arguments.");
            }
            Expr arg = assignment();
            args.add(arg);
            
            if (peek().type == TokenType.RIGHT_PAREN) break;
            consume(TokenType.COMMA, "Expect ',' after argument.");
        }
        return args;
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
        
        if (match(TokenType.FUN)) return lambdaExpr();
        
        throw error(peek(), "Expect expression.");
    }
    
    private Expr lambdaExpr() {
        consume(TokenType.LEFT_PAREN, "Expect '(' after 'fun'.");
        
        List<Token> pars = parameters();
        
        consume(TokenType.RIGHT_PAREN, "Expect ')' after parameter list.");
        
        consume(TokenType.LEFT_BRACE, "Expect '{' after parameter list.");
        Stmt.BlockStmt body = (Stmt.BlockStmt) blockStmt(); 
        
        return new Expr.Lambda(pars, body);
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
  
  private Token next() {
      return tokens.get(current + 1);
  }
}