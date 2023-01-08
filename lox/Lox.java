import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

public class Lox {
  static boolean hadError = false;
  
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
    Expr expr = parser.parse();
    if (hadError) return;
    
    System.out.println("Printing AST...");
    AstPrinter printer = new AstPrinter();
    System.out.println(printer.print(expr));
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
}