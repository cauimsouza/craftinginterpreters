/*
RplPrinter implements a visitor that pretty prints expressions in reverse Polish notation (RPN).
*/

public class RplPrinter implements Expr.Visitor<String> {
    @Override
    public String visitBinaryExpr(Expr.Binary expr) {
        return String.format("%s %s %s", expr.left.accept(this), expr.right.accept(this), expr.operator);        
    }
    
    @Override
    public String visitGroupingExpr(Expr.Grouping expr) {
        return String.format("%s group", expr.expression.accept(this));
    }
    
    @Override
    public String visitLiteralExpr(Expr.Literal expr) {
        return String.format("%s", expr.value);
    }
    
    @Override
    public String visitUnaryExpr(Expr.Unary expr) {
        return String.format("%s %s", expr.right, expr.operator);
    }
    
    public String print(Expr expr) {
        return expr.accept(this);
    }
    
    public static void main(String[] args) {
        Expr expression = new Expr.Binary(
        new Expr.Binary(
            new Expr.Literal(1),
            new Token(TokenType.PLUS, "+", null, 1),
            new Expr.Literal(2)),
        new Token(TokenType.STAR, "*", null, 1),
        new Expr.Binary(
            new Expr.Literal(4),
            new Token(TokenType.MINUS, "-", null, 1),
            new Expr.Literal(3)));
    
        System.out.println(new RplPrinter().print(expression));
    }
}