import java.util.List;
import java.util.Map;
import java.util.HashMap;

class LoxInstance {
    private final LoxClass klass;
    private final Map<String, Object> fields = new HashMap<>();
    
    private static class MetaClass extends LoxClass {
        MetaClass() {
            super("MetaClass", new HashMap<>());
        }
    }
    private static final LoxClass metaClass = new MetaClass();
    
    LoxInstance() { // For metaclass instances
        this.klass = metaClass;
    }
    
    LoxInstance(LoxClass klass) { // For class instances
        this.klass = klass;
    }
    
    Object get(Token name) {
        if (name.lexeme.equals("init"))
            throw new RuntimeError(name, "Undefined property 'init'.");
            
        return getAny(name);
    }
    
    Object getAny(Token name) {
        if (fields.containsKey(name.lexeme)) return fields.get(name.lexeme);
            
        if (!klass.hasMethod(name)) {
            throw new RuntimeError(name, "Undefined property '" + name.lexeme + "'.");
        }
        
        LoxFunction method = klass.getMethod(name);
        
        Environment closure = new Environment(method.closure);
        closure.declare("this", this);
        
        return new LoxFunction(method.name, method.params, method.body, closure);
    }
    
    void assign(Token name, Object value) {
        if (name.lexeme.equals("init"))
            throw new RuntimeError(name, "Cannot assign property with name 'init' (reserved for constructors).");
            
        if (klass.hasMethod(name))
            throw new RuntimeError(name, "Cannot reassign instance method '" + name.lexeme + "'.");

        fields.put(name.lexeme, value);
    }
    
    @Override
    public String toString() {
        return "<instance " + klass.name + ">";
    }
}