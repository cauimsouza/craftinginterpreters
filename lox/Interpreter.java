import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;

public class Interpreter implements Expr.Visitor<Object>, Stmt.Visitor<Void> {
    // globals tracks the outermost scope.
    final Environment globals = new Environment();
    
    // env tracks the "current" scope. It changes as we enter and exip local
    // scopes.
    private Environment env = globals;
    
    Interpreter() {
        globals.declare("clock", new LoxCallable() {
            @Override
            public int arity() { return 0; }
            
            @Override
            public Object call(Interpreter interpreter, List<Object> arguments) {
                return (double) System.currentTimeMillis() / 1000.0; 
            }
            
            @Override
            public String toString() { return "<native fn>"; }
        });
    }
    
    public void interpret(List<Stmt> program) {
        try {
            for (Stmt s : program) {
                s.accept(this);
            }
        } catch (RuntimeError e) {
            Lox.runtimeError(e);
        } catch (Break b) {
            Lox.runtimeError(new RuntimeError(b.token, "'break' outside any enclosing loop."));
        }
    }
    
    @Override
    public Void visitExprStmt(Stmt.ExprStmt stmt) {
        eval(stmt.expr);
        return null;
    }
    
    @Override
    public Void visitPrintStmt(Stmt.PrintStmt stmt) {
        System.out.println(stringify(eval(stmt.expr)));
        return null;
    }
    
    @Override
    public Void visitVarDeclStmt(Stmt.VarDeclStmt stmt) {
        if (stmt.expr == null) env.declare(stmt.id);
        else env.declare(stmt.id, eval(stmt.expr));
        return null;
    }
    
    @Override
    public Void visitFunDeclStmt(Stmt.FunDeclStmt stmt) {
        env.declare(stmt.name, new LoxFunction(stmt, env));
        return null;
    }
    
    @Override
    public Void visitBlockStmt(Stmt.BlockStmt stmt) {
        executeBlock(stmt, new Environment(env));
        return null;
    }
    
    void executeBlock(Stmt.BlockStmt stmt, Environment env) {
        Environment previous = this.env;
        try {
            this.env = env;
            for (Stmt s : stmt.stmts) {
                s.accept(this);     
            }
        } finally {
            this.env = previous;
        }
    }
    
    @Override
    public Void visitIfStmt(Stmt.IfStmt stmt) {
        if (isTruthy(stmt.expr.accept(this))) stmt.ifStmt.accept(this);
        else if (stmt.elseStmt != null) stmt.elseStmt.accept(this);
        return null;
    }
    
    @Override
    public Void visitWhileStmt(Stmt.WhileStmt stmt) {
        try {
            while (isTruthy(stmt.expr.accept(this))) stmt.stmt.accept(this);
        } catch (Break b) { }
        return null;
    }
    
    @Override
    public Void visitBreakStmt(Stmt.BreakStmt stmt) {
        throw new Break(stmt.token);
    }
    
    @Override
    public Void visitReturnStmt(Stmt.ReturnStmt stmt) {
        Object returnVal = null;
        if (stmt.expr != null) returnVal = eval(stmt.expr);
        throw new Return(returnVal);
    }
    
    private Object eval(Expr expr) {
        return expr.accept(this);
    }
   
    private String stringify(Object object) {
        if (object == null) return "nil";
        
        if (object instanceof Double) {
          String text = object.toString();
          if (text.endsWith(".0")) {
            text = text.substring(0, text.length() - 2);
          }
          return text;
        }
        
        return object.toString();
    }
   
   @Override
   public Object visitUnaryExpr(Expr.Unary expr) {
       Object v = expr.right.accept(this);
       
       switch (expr.operator.type) {
            case MINUS:
                checkNumberOperand(expr.operator, v);
                return -(double)v;
            case BANG:
                return !isTruthy(v);
       }
       
       return null;
   } 
   
   @Override
   public Object visitBinaryExpr(Expr.Binary expr) {
       Object left = expr.left.accept(this);
       
       // AND and OR can short-circuit the evaluation of expressions, so
       // they need to be handled differently.
       if (expr.operator.type == TokenType.AND) {
            if (!isTruthy(left))    return false;
            return isTruthy(expr.right.accept(this));
       }
       if (expr.operator.type == TokenType.OR) {
            if (isTruthy(left))    return true;
            return isTruthy(expr.right.accept(this));
       }
       
       Object right = expr.right.accept(this);
       
       switch (expr.operator.type) {
            case PLUS:
                if (left instanceof Double && right instanceof Double)
                    return (double) left + (double) right;
                if (left instanceof String || right instanceof String)
                    return stringify(left) + stringify(right);
                throw new RuntimeError(expr.operator, "Both operands must be numbers, or one of them must be a string.");
            case MINUS:
                checkNumbersOperand(expr.operator, left, right);
                return (double) left - (double) right;
            case SLASH:
                checkNumbersOperand(expr.operator, left, right);
                if ((double) right == 0)
                    throw new RuntimeError(expr.operator, "Division by zero.");
                return (double) left / (double) right;
            case STAR:
                if (left instanceof Double && right instanceof Double)
                    return (double) left * (double) right;
                if (left instanceof Double && right instanceof String) {
                    // Multiplication of double d by string s returns the concatenation of floor(d) copies of s.
                    try {
                        int n = ((Double) left).intValue();
                        if (n < 0) throw new RuntimeError(expr.operator, "Expression multiplying string must evaluate to a non-negative value.");
                        return new String(new char[n]).replace("\0", (String) right);  
                    } catch (OutOfMemoryError e) {
                        throw new RuntimeError(expr.operator, "Resulting string does not fit in memory.");
                    }
                }
                throw new RuntimeError(expr.operator, "Both operands must be numbers, or the left must be a non-negative number and the right a string.");
            case GREATER:
                if (left instanceof Double && right instanceof Double)
                    return (double) left > (double) right;
                if (left instanceof String && right instanceof String)
                    return ((String) left).compareTo((String) right) > 0;
                throw new RuntimeError(expr.operator, "Operands must be both numbers or both strings.");
            case GREATER_EQUAL:
                if (left instanceof Double && right instanceof Double)
                    return (double) left >= (double) right;
                if (left instanceof String && right instanceof String)
                    return ((String) left).compareTo((String) right) >= 0;
                throw new RuntimeError(expr.operator, "Operands must be both numbers or both strings.");
            case LESS:
                if (left instanceof Double && right instanceof Double)
                    return (double) left < (double) right;
                if (left instanceof String && right instanceof String)
                    return ((String) left).compareTo((String) right) < 0;
                throw new RuntimeError(expr.operator, "Operands must be both numbers or both strings.");
            case LESS_EQUAL:
                if (left instanceof Double && right instanceof Double)
                    return (double) left <= (double) right;
                if (left instanceof String && right instanceof String)
                    return ((String) left).compareTo((String) right) <= 0;
                throw new RuntimeError(expr.operator, "Operands must be both numbers or both strings.");
            case EQUAL_EQUAL:
                return isEqual(left, right);
            case BANG_EQUAL:
                return !isEqual(left, right);
            case COMMA:
                return right;
       }
       
       throw new RuntimeError(expr.operator, "Unexpected binary operator.");
   } 
   
   
   @Override
   public Object visitTernaryExpr(Expr.Ternary expr) {
       Object left = expr.left.accept(this);
       
       if (isTruthy(left)) {
           return expr.mid.accept(this);
       }
       return expr.right.accept(this);
   } 
   
   @Override
   public Object visitGroupingExpr(Expr.Grouping expr) {
       return expr.expression.accept(this);
   }
   
   @Override
   public Object visitLiteralExpr(Expr.Literal expr) {
      return expr.value; 
   } 
   
   @Override
   public Object visitVariableExpr(Expr.Variable expr) {
       return env.get(expr.name);
   } 
   
    public static void main(String[] args) {
        Expr expression = new Expr.Binary(
        new Expr.Unary(
            new Token(TokenType.MINUS, "-", null, 1),
            new Expr.Literal(123)),
        new Token(TokenType.STAR, "*", null, 1),
        new Expr.Grouping(
            new Expr.Literal(45.67)));
    
        System.out.println(new AstPrinter().print(expression));
    }
    
    @Override
    public Object visitAssignExpr(Expr.Assign expr) {
        Object v = eval(expr.expr);
        env.assign(expr.name, v);
        return v;
    }
    
    @Override
    public Object visitCallExpr(Expr.Call expr) {
        Object callee = eval(expr.expr);
        if (!(callee instanceof LoxCallable))
            throw new RuntimeError(expr.paren, "Can only call functions and classes.");
        LoxCallable function = (LoxCallable) callee;
            
        if (function.arity() != expr.arguments.size())
            throw new RuntimeError(expr.paren, "Wrong number of arguments.");
        
        List<Object> args = new ArrayList<>();
        for (Expr arg : expr.arguments) {
            args.add(eval(arg));
        }
        return function.call(this, args);
    }
    
    private boolean isEqual(Object left, Object right) {
       if (left == null && right == null) return true;
       if (left == null) return false;
       return left.equals(right);
   }
   
   private boolean isTruthy(Object operand) {
       if (operand == null) return false;
       if (operand instanceof Boolean) return (boolean) operand;
       return true;
   }
   
   private void checkNumberOperand(Token operator, Object operand) {
       if (operand instanceof Double) return;
       throw new RuntimeError(operator, "Operand must be a number.");
   }
   
   private void checkNumbersOperand(Token operator, Object left, Object right) {
       checkNumberOperand(operator, left);
       checkNumberOperand(operator, right);
   }
   
   // Used only for support to break statements.
   private static class Break extends RuntimeException { 
    final Token token;
    
    Break(Token token) {
        super(null, null, false, false);
        this.token = token;
    }
   }
}