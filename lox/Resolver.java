import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

class Resolver implements Expr.Visitor<Void>, Stmt.Visitor<Void> {
    
    private static class Access {
        boolean defined;
        boolean used;
    }
    
    private static class TokenAccess extends Access {
        final Token token;
        TokenAccess(Token token) {
            this.token = token;
        }
    }
    
    private static class This extends Access {
        This() {
            this.defined = true;
            this.used = true;
        }
    }
    
    private final Stack<Map<String, Access>> scopes = new Stack<>();
    private FunctionType currentFunction = FunctionType.NONE;
    private final Interpreter interpreter;
    
    Resolver(Interpreter interpreter) {
        this.interpreter = interpreter;
    }
    
    void resolve(List<Stmt> stmts) {
        for (Stmt s : stmts) {
            resolve(s);
        }
    }
    
    @Override
    public Void visitExprStmt(Stmt.ExprStmt expr) {
        resolve(expr.expr);
        return null;
    }
    
    @Override
    public Void visitVarDeclStmt(Stmt.VarDeclStmt decl) {
        declare(decl.id); 
        if (decl.expr != null) resolve(decl.expr);
        define(decl.id);
        return null;
    }
    
    @Override
    public Void visitFunDeclStmt(Stmt.FunDeclStmt fun) {
        declare(fun.name);
        define(fun.name);
        resolveFun(fun.params, fun.body, FunctionType.FUNCTION);
        return null;
    }
    
    @Override
    public Void visitClassDeclStmt(Stmt.ClassDeclStmt classStmt) {
        declare(classStmt.name);
        define(classStmt.name);
        
        beginScope();
        // The first step is to initialise the new scope, which contains
        // "this" and the methods. This will allow methods defined at the top
        // of the class declaration to refer to methods declared later.
        defineThis();
        for (Stmt.FunDeclStmt fun : classStmt.mets) {
            declare(fun.name);
            define(fun.name);
        }
        
        // Now resolve all the methods, including the constructor if there is any.
        for (Stmt.FunDeclStmt fun : classStmt.mets)
            resolveFun(fun.params, fun.body, FunctionType.METHOD);
        if (classStmt.init != null)
            resolveFun(classStmt.init.params, classStmt.init.body, FunctionType.CONSTRUCTOR);
        
        endScope();
        return null;
    }
    
    @Override
    public Void visitBlockStmt(Stmt.BlockStmt block) {
        beginScope();
        resolve(block.stmts);
        checkUnusedVariables();
        endScope();
        return null;
    }
    
    @Override
    public Void visitIfStmt(Stmt.IfStmt stmt) {
        resolve(stmt.expr);
        resolve(stmt.ifStmt);
        if (stmt.elseStmt != null) resolve(stmt.elseStmt);
        return null;
    } 
    
    @Override
    public Void visitWhileStmt(Stmt.WhileStmt stmt) {
        resolve(stmt.expr);
        resolve(stmt.stmt);
        return null;
    } 
    
    @Override
    public Void visitBreakStmt(Stmt.BreakStmt stmt) {
        return null;
    }
    
    @Override
    public Void visitReturnStmt(Stmt.ReturnStmt stmt) {
        if (currentFunction == FunctionType.NONE) Lox.error(stmt.keyword, "'return' statement outside any function.");
        if (stmt.expr != null) resolve(stmt.expr);
        return null;
    }
    
    @Override
    public Void visitUnaryExpr(Expr.Unary unary) {
        resolve(unary.expr);
        return null;
    }
    
    @Override
    public Void visitBinaryExpr(Expr.Binary binary) {
        resolve(binary.left);
        resolve(binary.right);
        return null;
    }
    
    @Override
    public Void visitTernaryExpr(Expr.Ternary ternary) {
        resolve(ternary.left);
        resolve(ternary.mid);
        resolve(ternary.right);
        return null;
    }
    
    @Override
    public Void visitGroupingExpr(Expr.Grouping grouping) {
        resolve(grouping.expr);
        return null;
    }
    
    @Override
    public Void visitLiteralExpr(Expr.Literal literal) {
        return null;
    }
    
    @Override
    public Void visitVariableExpr(Expr.Variable varExpr) {
        String name = varExpr.name.lexeme;
        if (!scopes.empty() && scopes.peek().containsKey(name) && !scopes.peek().get(name).defined) {
            Lox.error(varExpr.name, "Can't read local variable in its own initialiser.");
        }
            
        resolveLocal(varExpr, varExpr.name);
        return null;
    }
    
    @Override
    public Void visitAssignExpr(Expr.Assign expr) {
        resolve(expr.expr);
        resolveLocal(expr, expr.name);
        return null;
    }
    
    @Override
    public Void visitFieldAssignExpr(Expr.FieldAssign expr) {
        resolve(expr.instance);
        resolve(expr.expr);
        return null;
    }
    
    @Override
    public Void visitCallExpr(Expr.Call call) {
        resolve(call.expr);
        for (Expr arg : call.arguments) resolve(arg);
        return null;
    }
    
    @Override
    public Void visitLambdaExpr(Expr.Lambda lambda) {
        resolveFun(lambda.params, lambda.body, FunctionType.FUNCTION);
        return null;
    }
    
    @Override
    public Void visitAccessExpr(Expr.Access access) {
        resolve(access.expr);
        return null;
    }
    
    private void resolveFun(List<Token> params, Stmt.BlockStmt body, FunctionType type) {
        FunctionType prev = currentFunction;
        currentFunction = type;
        
        beginScope();
        for (Token par : params) {
            declare(par);
            define(par);
        }
        resolve(body.stmts);
        endScope();
        
        currentFunction = prev;
    }
    
    private void resolveLocal(Expr expr, Token name) {
       for (int i = scopes.size() - 1; i >= 0; i--) {
           if (scopes.get(i).containsKey(name.lexeme)) {
               scopes.get(i).get(name.lexeme).used = true;
               interpreter.resolve(expr, scopes.size() - 1 - i);
               return;
           }
       } 
    }
    
    private void declare(Token name) {
        if (scopes.empty()) return;
        
        Map<String, Access> scope = scopes.peek();
        if (scope.containsKey(name.lexeme))
            Lox.error(name, "Trying to redeclare symbol.");
        
        scope.put(name.lexeme, new TokenAccess(name));
    }
    
    private void define(Token name) {
        if (scopes.empty()) return;
        
        Access a = new TokenAccess(name);
        a.defined = true;
        scopes.peek().put(name.lexeme, a);
    }
    
    private void defineThis() {
        Access t = new This();
        scopes.peek().put("this", t);
    }
    
    private void resolve(Stmt s) {
        s.accept(this);
    }
    
    private void resolve(Expr e) {
        e.accept(this);
    }
    
    private void beginScope() {
        scopes.push(new HashMap<>()); 
    }
    
    private void endScope() {
        scopes.pop();
    }
    
    private void checkUnusedVariables() {
        scopes.peek().forEach((k, v) -> {
            if (v.used || !(v instanceof TokenAccess)) return;
            Lox.error(((TokenAccess)v).token, "Unused variable.");
        });
    }
}

enum FunctionType {
    NONE,
    FUNCTION,
    METHOD,
    CONSTRUCTOR
}