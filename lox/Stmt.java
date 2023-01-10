abstract class Stmt {
    interface Visitor<R> {
        R visitExprStmt(ExprStmt stmt);
        R visitPrintStmt(PrintStmt stmt);
    }
      
    abstract <R> R accept(Visitor<R> visitor);
  
    static class ExprStmt extends Stmt {
        final Expr expr;
        
        ExprStmt(Expr expr) {
            this.expr = expr;
        }
        
        @Override
        <R> R accept(Visitor<R> visitor) {
          return visitor.visitExprStmt(this);
        }
    }
    
    static class PrintStmt extends Stmt {
        final Expr expr;
        
        PrintStmt(Expr expr) {
            this.expr = expr;
        }
        
        @Override
        <R> R accept(Visitor<R> visitor) {
          return visitor.visitPrintStmt(this);
        }
    }
}