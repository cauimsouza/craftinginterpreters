import java.util.List;

class LoxFunction implements LoxCallable {
    final String name;
    final List<Token> params;
    final Stmt.BlockStmt body;
    final boolean getter;
    final Environment closure;
    
    LoxFunction(Stmt.FunDeclStmt declaration, Environment closure) {
        this.name = declaration.name.lexeme;
        this.params = declaration.params;
        this.body = declaration.body;
        this.getter = declaration.getter;
        this.closure = closure;
    }
    
    LoxFunction(LoxFunction function, Environment closure) {
        this.name = function.name;
        this.params = function.params;
        this.body = function.body;
        this.getter = function.getter;
        this.closure = closure;
    }
    
    LoxFunction(String name, List<Token> params, Stmt.BlockStmt body, boolean getter, Environment closure) {
        this.name = name;
        this.params = params;
        this.body = body;
        this.getter = getter;
        this.closure = closure;
    }
    
    @Override
    public int arity() {
        return params.size();
    }
    
    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        Environment env = new Environment(closure);
        
        for (int i = 0; i < arguments.size(); i++) {
            env.declare(params.get(i).lexeme, arguments.get(i));
        }
       
        try {
            interpreter.executeBlock(body, env); 
        } catch (Return ret) {
            return ret.value;
        }
        return null;
    }
    
    @Override
    public String toString() {
        return "<fn " + name + ">";
    }
}