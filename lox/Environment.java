import java.util.Map;
import java.util.HashMap;

public class Environment {
    private final Environment env;
    Map<String, Object> variables = new HashMap<>();
    
    Environment() { 
        this.env = null;
    }
    
    Environment(Environment env) {
        this.env = env;
    }
    
    public void declare(Token name, Object value) {
        variables.put(name.lexeme, value);
    }
    
    public void assign(Token name, Object value) {
        if (tryAssign(name.lexeme, value)) return;
        throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }
    
    public Object get(Token name) {
        if (variables.containsKey(name.lexeme)) return variables.get(name.lexeme);
        if (env != null) return env.get(name);
        throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }
    
    private boolean tryAssign(String variable, Object value) {
        if (variables.containsKey(variable)) {
            variables.put(variable, value);
            return true;
        }
        
        if (env != null) return env.tryAssign(variable, value);
        return false;
    }
}