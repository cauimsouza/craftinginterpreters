import java.util.List;
import java.util.Map;

class LoxClass extends LoxInstance implements LoxCallable {
    final String name;
    private final Map<String, LoxFunction> methods;
    
    private final Token init = new Token(TokenType.IDENTIFIER, "init", null, 0);
    
    // Use to create metaclasses
    LoxClass(String name, Map<String, LoxFunction> classMethods) {
        super();
        this.name = name;
        this.methods = classMethods;
    }
    
    // Use to create classes
    LoxClass(String name, Map<String, LoxFunction> methods, LoxClass metaClass) {
        super(metaClass);
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