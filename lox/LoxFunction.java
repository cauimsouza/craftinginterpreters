import java.util.List;

class LoxFunction implements LoxCallable {
    private final Stmt.FunDeclStmt declaration;
    private final Environment closure;
    
    LoxFunction(Stmt.FunDeclStmt declaration, Environment closure) {
        this.declaration = declaration;
        this.closure = closure;
    }
    
    @Override
    public int arity() {
        return declaration.parameters.size();
    }
    
    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        Environment env = new Environment(closure);
        
        for (int i = 0; i < arguments.size(); i++) {
            env.declare(declaration.parameters.get(i), arguments.get(i));
        }
       
        try {
            interpreter.executeBlock(declaration.body, env); 
        } catch (Return ret) {
            return ret.value;
        }
        return null;
    }
    
    @Override
    public String toString() {
        return "<fn " + declaration.name.lexeme + ">";
    }
}