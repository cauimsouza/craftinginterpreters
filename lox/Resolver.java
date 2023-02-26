import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

class Resolver implements Expr.Visitor<Void>, Stmt.Visitor<Void> {
    // The value associated with a key represents whether or not we have finished
    // resolving that variable's initialiser.
    private final Stack<Map<String, Boolean>> scopes = new Stack<>();
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
        define(fun.name);
        resolveFun(fun.params, fun.body, FunctionType.FUNCTION);
        return null;
    }
    
    @Override
    public Void visitBlockStmt(Stmt.BlockStmt block) {
        beginScope();
        resolve(block.stmts);
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
        if (!scopes.empty() && scopes.peek().get(varExpr.name.lexeme) == Boolean.FALSE)
            Lox.error(varExpr.name, "Can't read local variable in its own initialiser.");
            
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
    
    private void resolveFun(List<Token> params, Stmt.BlockStmt body, FunctionType type) {
        FunctionType prev = currentFunction;
        currentFunction = type;
        
        beginScope();
        for (Token par : params)
            define(par);
        resolve(body.stmts);
        endScope();
        
        currentFunction = prev;
    }
    
    private void resolveLocal(Expr expr, Token name) {
       for (int i = scopes.size() - 1; i >= 0; i--) {
           if (scopes.get(i).containsKey(name.lexeme)) {
            interpreter.resolve(expr, scopes.size() - 1 - i);
            return;
           }
       } 
    }
    
    private void declare(Token name) {
        if (scopes.empty()) return;
        
        Map<String, Boolean> scope = scopes.peek();
        if (scope.containsKey(name.lexeme))
            Lox.error(name, "Declaring variable of the same name in this scope.");
            
        scope.put(name.lexeme, false);
    }
    
    private void define(Token name) {
        if (scopes.empty()) return;
        scopes.peek().put(name.lexeme, true);
    }
    
    private void resolve(Stmt s) {
        s.accept(this);
    }
    
    private void resolve(Expr e) {
        e.accept(this);
    }
    
    private void beginScope() {
        scopes.push(new HashMap<String, Boolean>()); 
    }
    
    private void endScope() {
        scopes.pop();
    }
}

enum FunctionType {
    NONE,
    FUNCTION
}