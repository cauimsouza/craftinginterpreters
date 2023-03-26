import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

class Scanner {
  private final String source;
  private final List<Token> tokens = new ArrayList<>();
  private int current; // Position of the next character to be read, an offset from the beginning of the file.
  private int start; // Starting position of the lexeme being scanned, an offset from the beginning of the file.
  private int line;
  private static final Map<String, TokenType> keywords;

  static {
    keywords = new HashMap<>();
    keywords.put("and",    TokenType.AND);
    keywords.put("class",  TokenType.CLASS);
    keywords.put("else",   TokenType.ELSE);
    keywords.put("false",  TokenType.FALSE);
    keywords.put("for",    TokenType.FOR);
    keywords.put("fun",    TokenType.FUN);
    keywords.put("if",     TokenType.IF);
    keywords.put("nil",    TokenType.NIL);
    keywords.put("or",     TokenType.OR);
    keywords.put("return", TokenType.RETURN);
    keywords.put("super",  TokenType.SUPER);
    keywords.put("this",   TokenType.THIS);
    keywords.put("true",   TokenType.TRUE);
    keywords.put("var",    TokenType.VAR);
    keywords.put("while",  TokenType.WHILE);
    keywords.put("break",  TokenType.BREAK);
  }

  Scanner(String source) {
    this.source = source;
    this.current = 0;
    this.start = 0;
    this.line = 1;
  }
  
  public List<Token> scanTokens() {
      while (!isAtEnd()) {
          // We are at the beginning of the next lexeme.
          start = current;
          scanToken();
      }
      
      tokens.add(new Token(TokenType.EOF, "", null, line));
      return tokens;
  }
  
  boolean isAtEnd() {
      return current >= source.length();
  }
  
  private void scanToken() {
     char c = source.charAt(current); 
     switch (c) {
        case '(':  addToken(TokenType.LEFT_PAREN); break;
        case ')':  addToken(TokenType.RIGHT_PAREN); break;
        case '{':  addToken(TokenType.LEFT_BRACE); break;
        case '}':  addToken(TokenType.RIGHT_BRACE); break;
        case '[':  addToken(TokenType.LEFT_BRACKET); break;
        case ']':  addToken(TokenType.RIGHT_BRACKET); break;
        case ',':  addToken(TokenType.COMMA); break;
        case '.':  addToken(TokenType.DOT); break;
        case '-':  addToken(TokenType.MINUS); break;
        case '+':  addToken(TokenType.PLUS); break;
        case ';':  addToken(TokenType.SEMICOLON); break;
        case '*':  addToken(TokenType.STAR); break;
        case '?':  addToken(TokenType.QUESTION); break;
        case ':':  addToken(TokenType.COLON); break;
        case '/':
            if (nextIs('/')) { // It's a one-line comment.
                current++;
               while (hasNext() && !nextIs('\n')) current++;
            } else if (nextIs('*')) { // Multine comment.
                current++;
                boolean endOfComment = false;
                int newLines = 0;
                while (hasNext()) {
                   current++;
                   if (source.charAt(current) == '*' && nextIs('/')) {
                       endOfComment = true;
                       current++;
                       break;
                   }
                   if (source.charAt(current) == '\n') newLines++;
               }
               if (!endOfComment) {
                   Lox.error(line, "Unterminated comment.");
               }
               line += newLines;
            } else addToken(TokenType.SLASH);
            break;
        case '!':
            if (nextIs('=')) {
                current++;
                addToken(TokenType.BANG_EQUAL);
            } else {
                addToken(TokenType.BANG);
            }
            break;

        case '=':
            if (nextIs('=')) {
                current++;
                addToken(TokenType.EQUAL_EQUAL);
            } else {
                addToken(TokenType.EQUAL);
            }
            break;

        case '>':
            if (nextIs('=')) {
                current++;
                addToken(TokenType.GREATER_EQUAL);
            } else {
                addToken(TokenType.GREATER);
            }
            break;

        case '<':
            if (nextIs('=')) {
                current++;
                addToken(TokenType.LESS_EQUAL);
            } else {
                addToken(TokenType.LESS);
            }
            break;

        case '"':
            int index = source.indexOf('"', current + 1);
            if (index >= 0) {
                current = index;
                String s = source.substring(start + 1, index); // Drop the enclosing double quotes from the lexeme.
                addToken(TokenType.STRING, s);
                for (int i = 0; i < s.length(); i++) { if (s.charAt(i) == '\n') line++; } 
            } else {
                Lox.error(line, "Unterminated string literal");
                current = source.length() - 1;
            }
            break;

        case '\n':
            line++;
            break;

        default:
            if (isDigit(c)) {
                scanNumber();
            } else if (isAlpha(c) || c == '_') {
                scanIdentifierOrKeyword(); 
            } else if (Character.isWhitespace(c)) {
            } else {
                Lox.error(line, "Invalid sequence of characters.");
            }
     }

     current++;
  }
  
  private void addToken(TokenType type) {
      tokens.add(new Token(type, source.substring(start, current + 1), null, line));
  }
  
  private void addToken(TokenType type, Object literal) {
      tokens.add(new Token(type, source.substring(start, current + 1), literal, line));
  }
  
  private void scanNumber() {
        // We have to read until a non-digit character.
        while (nextIsDigit()) current++;
        
        // Possibilities:
        // A) The next character is an underscore or an alphabetical character -> error
        // B) The next character is a dot not followed by a digit-> error
        // C) The next character is a dot followed by a digit-> We should continue parsing
        // D) Anything else -> end of the literal
        if (nextIs('_') || nextIsAlpha()) {
            Lox.error(line, "Numerical literal cannot be followed by underscore or letter.");
        } else if (nextIs('.')) {
            current++;
            
            if (nextIsDigit()) {
                current++; // Now we are at the first digit after the dot.
                
                while (nextIsDigit()) current++;
                
                if (nextIs('_') || nextIsAlpha()) {
                    Lox.error(line, "Numerical literal cannot be followed by underscore or letter.");
                } else {
                    String s = source.substring(start, current + 1);
                    Double n = Double.valueOf(s);
                    addToken(TokenType.NUMBER, n);
                }
            } else {
                Lox.error(line, "A period in a numerical literal must be followed by a digit.");
            }
        } else {
            // The literal has no dot.
            String s = source.substring(start, current + 1);
            Double n = Double.valueOf(s);
            addToken(TokenType.NUMBER, n);
        }
      
  }
  
  private void scanIdentifierOrKeyword() {
        while (nextIsDigit() || nextIsAlpha() || nextIs('_')) current++;
        
        String s = source.substring(start, current + 1);
        TokenType type = keywords.getOrDefault(s, TokenType.IDENTIFIER);
        
        switch (s) {
            case "false":
                addToken(type, false);
                break;
            case "true":
                addToken(type, true);
                break;
            case "nil":
                addToken(type, null);
                break;
            default:
                addToken(type);
        }
  }
  
  // Returns true if there is a next character and the next character is c.
  private boolean nextIs(char c) {
     return hasNext() && source.charAt(current + 1) == c;
  }
  
  private boolean hasNext() {
      return current + 1 < source.length();
  }
  
  // Returns true if there is a next character and it's a digit.
  private boolean nextIsDigit() {
      return hasNext() && isDigit(source.charAt(current + 1));
  }
  
  // Returns true if there is a next character and it's a letter from the English alphabet.
  private boolean nextIsAlpha() {
      return hasNext() && isAlpha(source.charAt(current + 1));
  }
  
  private boolean isAlpha(char c) {
      return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
  }
  
  private boolean isDigit(char c) {
      return c >= '0' && c <= '9';
  }
}