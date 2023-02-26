import java.util.List;

class LoxFunction implements LoxCallable {
    private final String name;
    private final List<Token> params;
    private final Stmt.BlockStmt body;
    private final Environment closure;
    
    LoxFunction(Stmt.FunDeclStmt declaration, Environment closure) {
        this.name = declaration.name.lexeme;
        this.params = declaration.params;
        this.body = declaration.body;
        this.closure = closure;
    }
    
    LoxFunction(String name, List<Token> params, Stmt.BlockStmt body, Environment closure) {
        this.name = name;
        this.params = params;
        this.body = body;
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
            env.declare(params.get(i), arguments.get(i));
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