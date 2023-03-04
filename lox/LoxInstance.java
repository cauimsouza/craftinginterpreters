import java.util.Map;
import java.util.HashMap;

class LoxInstance {
    private final Map<String, Object> fields = new HashMap<>();
    private final Map<String, Object> methods = new HashMap<>();
    private final String className;
    
    LoxInstance(String className) {
        this.className = className;
    }
    
    Object get(Token token) {
        if (methods.containsKey(token.lexeme))
            return methods.get(token.lexeme);
            
        if (fields.containsKey(token.lexeme))
            return fields.get(token.lexeme);
            
        throw new RuntimeError(token, "Instance has no method or property " + token.lexeme + ".");
    }
    
    void assign(Token token, Object value) {
        if (methods.containsKey(token.lexeme))
            throw new RuntimeError(token, "Cannot reassign instance method '" + token.lexeme + "'.");

        fields.put(token.lexeme, value);
    }
    
    void assignMethod(String name, Object method) {
        methods.put(name, method);
    }
    
    @Override
    public String toString() {
        return "<instance " + className + ">";
    }
}