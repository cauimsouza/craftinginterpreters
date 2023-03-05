import java.util.List;

abstract class Stmt {
    interface Visitor<R> {
        R visitExprStmt(ExprStmt stmt);
        R visitVarDeclStmt(VarDeclStmt stmt);
        R visitFunDeclStmt(FunDeclStmt stmt);
        R visitClassDeclStmt(ClassDeclStmt stmt);
        R visitBlockStmt(BlockStmt stmt);
        R visitIfStmt(IfStmt stmt);
        R visitWhileStmt(WhileStmt stmt);
        R visitBreakStmt(BreakStmt stmt);
        R visitReturnStmt(ReturnStmt stmt);
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
    
    static class VarDeclStmt extends Stmt {
        final Token id;
        final Expr expr;
        
        VarDeclStmt(Token id) {
           this.id = id;
           this.expr = null;
        }
        
        VarDeclStmt(Token id, Expr expr) {
            this.id = id;
            this.expr = expr;
        }
        
        @Override
        <R> R accept(Visitor<R> visitor) {
          return visitor.visitVarDeclStmt(this);
        }
    }
    
    static class FunDeclStmt extends Stmt {
        final Token name;
        final List<Token> params;
        final Stmt.BlockStmt body;
        
        FunDeclStmt(Token name, List<Token> params, Stmt.BlockStmt body) {
           this.name = name;
           this.params = params;
           this.body = body;
        }
        
        @Override
        <R> R accept(Visitor<R> visitor) {
          return visitor.visitFunDeclStmt(this);
        }
    }
    
    static class ClassDeclStmt extends Stmt {
        final Token name;
        final List<FunDeclStmt> classMethods;
        final List<FunDeclStmt> methods;
        
        ClassDeclStmt(Token name, List<FunDeclStmt> classMethods, List<FunDeclStmt> methods) {
           this.name = name;
           this.classMethods = classMethods;
           this.methods = methods;
        }
        
        @Override
        <R> R accept(Visitor<R> visitor) {
          return visitor.visitClassDeclStmt(this);
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
    
    static class IfStmt extends Stmt {
        final Expr expr;
        final Stmt ifStmt;
        final Stmt elseStmt;
        
        IfStmt(Expr expr, Stmt ifStmt, Stmt elseStmt) {
            this.expr = expr;
            this.ifStmt = ifStmt;
            this.elseStmt = elseStmt;
        }
        
        @Override
        <R> R accept(Visitor<R> visitor) {
          return visitor.visitIfStmt(this);
        }
    }
    
    static class WhileStmt extends Stmt {
        final Expr expr;
        final Stmt stmt;
        
        WhileStmt(Expr expr, Stmt stmt) {
            this.expr = expr;
            this.stmt = stmt;
        }
        
        @Override
        <R> R accept(Visitor<R> visitor) {
          return visitor.visitWhileStmt(this);
        }
    }
    
    static class BreakStmt extends Stmt {
        // It's a syntactical error to have a 'break' appear outside of any enclosing loop.
        // We use the token to tell the user where the 'break' appears.
        final Token token;
        
        BreakStmt(Token token) {
            this.token = token;
        }
        
        @Override
        <R> R accept(Visitor<R> visitor) {
          return visitor.visitBreakStmt(this);
        }
    }
    
    static class ReturnStmt extends Stmt {
        final Expr expr;
        final Token keyword;
        
        ReturnStmt(Token keyword, Expr expr) {
            this.keyword = keyword;
            this.expr = expr;
        }
        
        @Override
        <R> R accept(Visitor<R> visitor) {
          return visitor.visitReturnStmt(this);
        }
    }
}