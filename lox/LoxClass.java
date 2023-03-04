import java.util.List;

class LoxClass implements LoxCallable {
    private final String name;
    private final Stmt.FunDeclStmt initMet;
    private final List<Stmt.FunDeclStmt> mets;
    private final Environment closure;
    
    LoxClass(Stmt.ClassDeclStmt declaration, Environment closure) {
        this.name = declaration.name.lexeme;
        this.initMet = declaration.init;
        this.mets = declaration.mets; 
        this.closure = closure;
    }
    
    @Override
    public int arity() {
        if (this.initMet == null) return 0;
        return initMet.params.size();
    }
    
    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        Environment env = new Environment(closure); 
        
        // First we initialise the scope of the class body.
        
        LoxInstance instance = new LoxInstance(name); 
        env.declare("this", instance); // "this" now refers to the instance just created
        
        // We add the class to the environment so that it's possible for the instance
        // to have fields that are instances of the same class, and so that it's possible
        // for the methods to create new instances of the same class.
        env.declare(name, this);
        
        for (Stmt.FunDeclStmt metDecl : mets) {
            LoxFunction met = new LoxFunction(metDecl, env);
            env.declare(metDecl.name.lexeme, met);
            instance.assignMethod(metDecl.name.lexeme, met);
        }
        
        // Now we call the constructor. The constructor can call helper methods.
        // TODO: An init must not contain a return statement.
        if (this.initMet == null) return instance;
        LoxFunction constructor = new LoxFunction(this.initMet, env);
        constructor.call(interpreter, arguments);
        
        return instance;
    }
    
    @Override
    public String toString() {
        return "<class " + name + ">";
    }
}