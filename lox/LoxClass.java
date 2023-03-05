import java.util.List;
import java.util.Map;

class LoxClass implements LoxCallable {
    final String name;
    private final Map<String, LoxFunction> methods;
    
    private final Token init = new Token(TokenType.IDENTIFIER, "init", null, 0);
    
    LoxClass(String name, Map<String, LoxFunction> methods) {
        this.name = name;
        this.methods = methods;
    }
    
    @Override
    public int arity() {
        if (!methods.containsKey("init")) return 0;
        return methods.get("init").arity();
    }
    
    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        if (!methods.containsKey("init")) 
            return new LoxInstance(this);
        
        LoxInstance instance = new LoxInstance(this);
        if (!methods.containsKey("init")) return instance;
        
        ((LoxFunction)instance.getAny(init)).call(interpreter, arguments);
        
        return instance;
    }
    
    @Override
    public String toString() {
        return "<class " + name + ">";
    }
    
    boolean hasMethod(Token name) {
        return methods.containsKey(name.lexeme);
    }
    
    LoxFunction getMethod(Token name) {
        return methods.get(name.lexeme);
    }
}