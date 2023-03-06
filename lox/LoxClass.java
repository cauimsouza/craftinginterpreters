import java.util.List;
import java.util.Map;

class LoxClass extends LoxInstance implements LoxCallable {
    final String name;
    private final Map<String, LoxFunction> methods;
    private final LoxClass superClass;
    
    private final Token init = new Token(TokenType.IDENTIFIER, "init", null, 0);
    
    // Use to create metaclasses
    LoxClass(String name, Map<String, LoxFunction> classMethods) {
        super();
        this.name = name;
        this.methods = classMethods;
        this.superClass = null;
    }
    
    // Use to create classes
    LoxClass(String name, Map<String, LoxFunction> methods, LoxClass metaClass, LoxClass superClass) {
        super(metaClass);
        this.name = name;
        this.methods = methods;
        this.superClass = superClass;
    }
    
    @Override
    public int arity() {
        if (!methods.containsKey("init")) return 0;
        return methods.get("init").arity();
    }
    
    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        LoxInstance instance;
        if (superClass != null) {
            instance = (LoxInstance) (superClass.call(interpreter, arguments));
            instance.klass = this;
        } else instance = new LoxInstance(this);
        
        if (!methods.containsKey("init")) return instance;
        
        ((LoxFunction)instance.getAny(interpreter, init)).call(interpreter, arguments);
        
        return instance;
    }
    
    @Override
    public String toString() {
        return "<class " + name + ">";
    }
    
    boolean hasMethod(Token name) {
        return methods.containsKey(name.lexeme) ||
            (superClass != null && superClass.hasMethod(name));
    }
    
    LoxFunction getMethod(Token name) {
        if (methods.containsKey(name.lexeme)) return methods.get(name.lexeme);
        return superClass.getMethod(name);
    }
}