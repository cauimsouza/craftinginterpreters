import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;

class LoxInstance {
    private static final LoxClass kMetaClass = new MetaClass();
    
    LoxClass klass;
    private final Map<String, Object> fields;
    
    private static class MetaClass extends LoxClass {
        MetaClass() {
            super("MetaClass", new HashMap<>());
        }
    }
    
    LoxInstance() { // For metaclass instances
        this.klass = kMetaClass;
        this.fields = new HashMap<> ();
    }
    
    LoxInstance(LoxClass klass) { // For class instances
        this.klass = klass;
        this.fields = new HashMap<> ();
    }
    
    // Used only for instantiating "super".
    private LoxInstance(LoxClass superClass, Map<String, Object> fields) {
        this.klass = superClass;
        this.fields = fields;
    }
    
    Object get(Interpreter interpreter, Token name) {
        if (name.lexeme.equals("init"))
            throw new RuntimeError(name, "Undefined property 'init'.");
            
        return getAny(interpreter, name);
    }
    
    Object getAny(Interpreter interpreter, Token name) {
        if (fields.containsKey(name.lexeme)) {
            return fields.get(name.lexeme);
        }
        
        LoxClass.MethodClass methodClass = klass.getMethod(name);
        if (methodClass == null) {
            throw new RuntimeError(name, "Undefined property '" + name.lexeme + "'.");
        }
        
        Environment closure = new Environment(methodClass.method.closure);
        closure.declare("this", this);
        closure.declare("super", new LoxInstance(methodClass.klass.superClass, fields));
        
        LoxFunction method = new LoxFunction(methodClass.method, closure);
        
        if (method.getter) {
            return method.call(interpreter, new ArrayList<>());
        }
        
        return method;
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