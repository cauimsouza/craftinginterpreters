import java.util.List;
import java.util.Map;
import java.util.HashMap;

public class Interpreter implements Expr.Visitor<Object>, Stmt.Visitor<Void> {
    private Environment env = new Environment();
    
    public void interpret(List<Stmt> program) {
        try {
            for (Stmt s : program) {
                s.accept(this);
            }
        } catch (RuntimeError e) {
            Lox.runtimeError(e);
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
    public Void visitAssignStmt(Stmt.AssignStmt stmt) {
        env.assign(stmt.id, eval(stmt.expr));
        return null;
    }
    
    @Override
    public Void visitBlockStmt(Stmt.BlockStmt stmt) {
        Environment previous = env;
        try {
            env = new Environment(env);
            
            for (Stmt s : stmt.stmts) {
                s.accept(this);     
            }
        } finally {
            env = previous;
        }
        return null;
    }
    
    @Override
    public Void visitIfStmt(Stmt.IfStmt stmt) {
        if (isTruthy(stmt.expr.accept(this))) stmt.ifStmt.accept(this);
        else if (stmt.elseStmt != null) stmt.elseStmt.accept(this);
        return null;
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
}