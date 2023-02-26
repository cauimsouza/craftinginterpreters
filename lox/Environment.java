import java.util.Map;
import java.util.HashMap;

public class Environment {
    private static class Value {
        final Object value;    
        Value(Object value) {
            this.value = value;
        }
    }
    
    // Variables is a map from variable names to the values they're assigned to.
    // A variable is mapped to null iff it was declared but never assigned to.
    Map<String, Value> variables = new HashMap<>();
    
    // Immediate enclosing environment
    private final Environment env; 
    
    Environment() { 
        this.env = null;
    }
    
    Environment(Environment env) {
        this.env = env;
    }
    
    public Environment copy() {
        Environment newEnv;
        if (env == null) newEnv = new Environment();
        else newEnv = new Environment(env.copy());
        newEnv.variables = new HashMap<>(variables);
        
        return newEnv;
    }
    
    // Declares a new variable without assigning it to a value.
    public void declare(Token name) {
        variables.put(name.lexeme, null);
    }
    
    // Declares a new variable and assigns it to a value.
    public void declare(Token name, Object value) {
        variables.put(name.lexeme, new Value(value));
    }
    
    // Declares a new variable and assigns it to a value.
    public void declare(String name, Object value) {
        variables.put(name, new Value(value));
    }
    
    public void assign(Token name, Object value) {
        if (tryAssign(name.lexeme, value)) return;
        throw new RuntimeError(name, "Assignment to undefined variable '" + name.lexeme + "'.");
    }
    
    // Returns the value assigned to a variable.
    // If the variable was never initialised nor assigned to, it throws a
    // RuntimeError exception.
    public Object get(Token name) {
        if (variables.containsKey(name.lexeme)) {
            Value v =  variables.get(name.lexeme);
            if (v != null) return v.value;
            throw new RuntimeError(name, "Variable '" + name.lexeme + "' was not initialised nor assigned to.");
        }
        if (env != null) return env.get(name);
        throw new RuntimeError(name, "Reference to undefined variable '" + name.lexeme + "'.");
    }
    
    public Environment ancestor(int depth) {
       Environment e = this;
       for (int i = 0; i < depth; i++) e = e.env;
       return e;
    }
    
    private boolean tryAssign(String variable, Object value) {
        if (variables.containsKey(variable)) {
            variables.put(variable, new Value(value));
            return true;
        }
        
        if (env != null) return env.tryAssign(variable, value);
        return false;
    }
}