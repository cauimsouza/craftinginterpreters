import java.util.List;

abstract class Expr {
  interface Visitor<R> {
    R visitUnaryExpr(Unary expr);
    R visitBinaryExpr(Binary expr);
    R visitTernaryExpr(Ternary expr);
    R visitGroupingExpr(Grouping expr);
    R visitLiteralExpr(Literal expr);
    R visitVariableExpr(Variable expr);
    R visitAssignExpr(Assign expr);
    R visitCallExpr(Call expr);
  }
  
  static class Unary extends Expr {
    Unary(Token operator, Expr right) {
      this.operator = operator;
      this.right = right;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitUnaryExpr(this);
    }

    final Token operator;
    final Expr right;
  }

  static class Binary extends Expr {
    Binary(Expr left, Token operator, Expr right) {
      this.left = left;
      this.operator = operator;
      this.right = right;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitBinaryExpr(this);
    }

    final Expr left;
    final Token operator;
    final Expr right;
  }
  
  static class Ternary extends Expr {
    Ternary(Token operator, Expr left, Expr mid, Expr right) {
      this.operator = operator;
      this.left = left;
      this.mid = mid;
      this.right = right;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitTernaryExpr(this);
    }

    final Token operator;
    final Expr left;
    final Expr mid;
    final Expr right;
  }
  
  static class Grouping extends Expr {
    Grouping(Expr expression) {
      this.expression = expression;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitGroupingExpr(this);
    }

    final Expr expression;
  }
  
  static class Literal extends Expr {
    Literal(Object value) {
      this.value = value;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitLiteralExpr(this);
    }

    final Object value;
  }
  
  static class Variable extends Expr {
    Variable(Token name) {
      this.name = name;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitVariableExpr(this);
    }

    final Token name;
  }
  
  static class Assign extends Expr {
    Assign(Token name, Expr expr) {
      this.name = name;
      this.expr = expr;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitAssignExpr(this);
    }

    final Token name;
    final Expr expr;
  }
  
  static class Call extends Expr {
    Call(Expr expr, Token paren, List<Expr> arguments) {
      this.expr = expr;
      this.paren = paren;
      this.arguments = arguments;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitCallExpr(this);
    }

    final Expr expr;
    final Token paren;
    final List<Expr> arguments;
  }
  
  abstract <R> R accept(Visitor<R> visitor);
}
