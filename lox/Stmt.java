import java.util.List;

abstract class Stmt {
    interface Visitor<R> {
        R visitExprStmt(ExprStmt stmt);
        R visitPrintStmt(PrintStmt stmt);
        R visitVarDeclStmt(VarDeclStmt stmt);
        R visitAssignStmt(AssignStmt stmt);
        R visitBlockStmt(BlockStmt stmt);
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
    
    static class VarDeclStmt extends Stmt {
        final Token id;
        final Expr expr;
        
        VarDeclStmt(Token id, Expr expr) {
            this.id = id;
            this.expr = expr;
        }
        
        @Override
        <R> R accept(Visitor<R> visitor) {
          return visitor.visitVarDeclStmt(this);
        }
    }
    
    static class AssignStmt extends Stmt {
        final Token id;
        final Expr expr;
        
        AssignStmt(Token id, Expr expr) {
            this.id = id;
            this.expr = expr;
        }
        
        @Override
        <R> R accept(Visitor<R> visitor) {
          return visitor.visitAssignStmt(this);
        }
    }
    
    static class BlockStmt extends Stmt {
        final List<Stmt> stmts;
        
        BlockStmt(List<Stmt> stmts) {
            this.stmts = stmts;
        }
        
        @Override
        <R> R accept(Visitor<R> visitor) {
          return visitor.visitBlockStmt(this);
        }
    }
}