import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

public class Lox {
  static boolean hadError = false;
  static boolean hadRuntimeError = false;
  private static Interpreter interpreter = new Interpreter();
  
  public static void main(String[] args) throws IOException {
    if (args.length > 1) {
      System.out.println("Usage: jlox [script]");
      System.exit(64); 
    } else if (args.length == 1) {
      runFile(args[0]);
    } else {
      runPrompt();
    }
  }
  
  private static void runFile(String path) throws IOException {
    byte[] bytes = Files.readAllBytes(Paths.get(path));
    run(new String(bytes));
    
    if (hadError) System.exit(64);
    if (hadRuntimeError) System.exit(70);
  }
  
  private static void runPrompt() throws IOException {
     InputStreamReader input = new InputStreamReader(System.in);
     BufferedReader reader = new BufferedReader(input);
     
     while (true) {
       System.out.print("> ");
       String line = reader.readLine();
       if (line == null) break;
       run(line);
       hadError = false;
     }
  } 
  
  private static void run(String source) {
    System.out.println("Scanning...");
    Scanner scanner = new Scanner(source);
    List<Token> tokens = scanner.scanTokens();
    if (hadError) return;
    
    System.out.println("Printing tokens...");
    for (Token t : tokens) {
      System.out.println(t);
    }
    
    System.out.println("Parsing...");
    Parser parser = new Parser(tokens);
    List<Stmt> prog = parser.parse();
    if (hadError) return;
    
    Resolver resolver = new Resolver(interpreter);
    resolver.resolve(prog);
    if (hadError) return;
    
    System.out.println("Interpreting...");
    interpreter.interpret(prog);
  }
  
  static void error(int line, String message) {
    report(line, "", message);
  }
  
  static void error(Token token, String message) {
    if (token.type == TokenType.EOF) {
      report(token.line, " at end", message);
      return;
    }
    
    report(token.line, " at '" + token.lexeme + "'", message); 
  }
  
  private static void report(int line, String where, String message) {
    System.out.println("line " + line + "] Error" + where + ": " + message); 
    hadError = true;
  }
  
  static void runtimeError(RuntimeError e) {
    if (e.token != null) {
      System.err.println(e.getMessage() +
        "\nWhile evaluation operation " + e.token.toString() + " at  [line " + e.token.line + "]");
    } else {
      System.err.println(e.getMessage());
    }
    
    hadRuntimeError = true;
  }
}