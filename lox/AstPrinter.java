public class AstPrinter implements Expr.Visitor<String> {
   String print(Expr expr) {
       return expr.accept(this);
   }
   
   @Override
   public String visitBinaryExpr(Expr.Binary expr) {
       return String.format("(%s %s %s)", expr.operator, expr.left.accept(this), expr.right.accept(this));
   } 
   
   @Override
   public String visitGroupingExpr(Expr.Grouping expr) {
       return String.format("(group %s)", expr.expression.accept(this));
   } 
   
   @Override
   public String visitLiteralExpr(Expr.Literal expr) {
       return String.format("%s", expr.value);
   } 
   
   @Override
   public String visitUnaryExpr(Expr.Unary expr) {
       return String.format("(%s %s)", expr.operator, expr.right.accept(this));
   } 
   
    public static void main(String[] args) {
        Expr expression = new Expr.Binary(
        new Expr.Unary(
            new Token(TokenType.MINUS, "-", null, 1),
            new Expr.Literal(123)),
        new Token(TokenType.STAR, "*", null, 1),
        new Expr.Grouping(
            new Expr.Literal(45.67)));
    
        System.out.println(new AstPrinter().print(expression));
    }
}