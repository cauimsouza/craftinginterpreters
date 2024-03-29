import java.util.List;

abstract class Expr {
  interface Visitor<R> {
    R visitThisExpr(This expr);
    R visitSuperExpr(Super expr);
    R visitUnaryExpr(Unary expr);
    R visitBinaryExpr(Binary expr);
    R visitTernaryExpr(Ternary expr);
    R visitGroupingExpr(Grouping expr);
    R visitListExpr(ListExpr expr);
    R visitListAccess(ListAccess expr);
    R visitListAssign(ListAssign expr);
    R visitLiteralExpr(Literal expr);
    R visitVariableExpr(Variable expr);
    R visitAssignExpr(Assign expr);
    R visitFieldAssignExpr(FieldAssign expr);
    R visitCallExpr(Call expr);
    R visitLambdaExpr(Lambda expr);
    R visitAccessExpr(Access expr);
  }
  
  static class This extends Expr {
    This(Token token) {
      this.token = token;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitThisExpr(this);
    }

    final Token token;
  }
  
  static class Super extends Expr {
    Super(Token keyword, Token method) {
      this.keyword = keyword;
      this.method = method;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitSuperExpr(this);
    }

    final Token keyword;
    final Token method;
  }
  
  static class Unary extends Expr {
    Unary(Token operator, Expr expr) {
      this.operator = operator;
      this.expr = expr;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitUnaryExpr(this);
    }

    final Token operator;
    final Expr expr;
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
    Grouping(Expr expr) {
      this.expr = expr;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitGroupingExpr(this);
    }

    final Expr expr;
  }
  
  static class ListExpr extends Expr {
    ListExpr(List<Expr> elements) {
      this.elements = elements;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitListExpr(this);
    }

    final List<Expr> elements;
  }
  
  static class ListAccess extends Expr {
    ListAccess(Expr list, Token leftBracket, Expr index) {
      this.list = list;
      this.leftBracket = leftBracket;
      this.index = index;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitListAccess(this);
    }

    final Expr list;
    final Token leftBracket;
    final Expr index;
  }
  
  static class ListAssign extends Expr {
    ListAssign(Expr list, Token leftBracket, Expr index, Token equal, Expr right) {
      this.list = list;
      this.leftBracket = leftBracket;
      this.index = index;
      this.equal = equal;
      this.right = right;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitListAssign(this);
    }

    final Expr list;
    final Token leftBracket;
    final Expr index;
    final Token equal;
    final Expr right;
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
  
  static class FieldAssign extends Expr {
    FieldAssign(Expr instance, Token field, Expr expr) {
      this.instance = instance;
      this.field = field;
      this.expr = expr;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitFieldAssignExpr(this);
    }

    final Expr instance;
    final Token field;
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
  
  static class Lambda extends Expr {
    Lambda(List<Token> params, Stmt.BlockStmt body) {
      this.params = params;
      this.body = body;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitLambdaExpr(this);
    }

    final List<Token> params;
    final Stmt.BlockStmt body;
  }
  
  static class Access extends Expr {
    Access(Expr expr, Token field) {
      this.expr = expr;
      this.field = field;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitAccessExpr(this);
    }

    final Expr expr;
    final Token field;
  }
  
  abstract <R> R accept(Visitor<R> visitor);
}
