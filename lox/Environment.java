import java.util.Map;
import java.util.HashMap;

public class Environment {
    private static class Value {
        final String name;
        Object value;    
        boolean initialised;
        
        Value(String name) {
            this.name = name;
        }
    }
    
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
        variables.put(name.lexeme, new Value(name.lexeme));
    }
    
    // Declares a new variable and assigns it to a value.
    public void declare(String name, Object value) {
        Value v = new Value(name);
        v.value = value;
        v.initialised = true;
        
        variables.put(name, v);
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
            if (v.initialised) return v.value;
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
            Value v = variables.get(variable);
            v.value = value;
            v.initialised = true;
            return true;
        }
        
        if (env != null) return env.tryAssign(variable, value);
        return false;
    }
}