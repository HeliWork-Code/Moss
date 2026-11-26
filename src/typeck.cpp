#include "typeck.h"
#include <unordered_map>
#include <vector>

namespace {

struct VarInfo {
  Type type;
};

class Checker {
 public:
  void run(const LoadedProgram& p) {
    prog = &p;
    for (const FuncDecl* f : p.bodies) checkFunction(*f);
  }

 private:
  const LoadedProgram* prog = nullptr;
  const CapScope* currentScope = nullptr;
  std::vector<std::unordered_map<std::string, VarInfo>> scopes;
  Type currentRet = TypeKind::Void;
  bool currentIsMain = false;

  [[noreturn]] void error(int line, const std::string& msg) const {
    throw TypeCheckError(msg, line);
  }

  std::string signature(const FuncDecl& f) const {
    std::string s = f.name + "(";
    for (size_t i = 0; i < f.params.size(); ++i) {
      if (i) s += ", ";
      s += typeName(f.params[i].type);
    }
    s += "): ";
    s += typeName(f.returnType);
    return s;
  }

  void declare(int line, const std::string& name, Type type) {
    auto& scope = scopes.back();
    if (scope.count(name))
      error(line, "variable '" + name + "' is already declared in this scope");
    scope[name] = VarInfo{type};
  }

  const VarInfo& lookup(int line, const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      auto found = it->find(name);
      if (found != it->end()) return found->second;
    }
    error(line, "use of undeclared variable '" + name + "'");
  }

  void checkFunction(const FuncDecl& f) {
    scopes.clear();
    scopes.emplace_back();
    currentScope = f.scope;
    currentRet = f.returnType;
    currentIsMain = f.isMain;

    for (const auto& p : f.params) declare(f.line, p.name, p.type);
    for (const auto& s : f.body) checkStmt(*s);

    if (f.returnType != TypeKind::Void && !f.isMain && !blockReturns(f.body))
      error(f.line, "function '" + f.name +
                        "' may not return a value on all paths");

    // For main, a fall-through returns 0.
    scopes.pop_back();
  }

  bool stmtReturns(const Stmt& s) const {
    switch (s.kind) {
      case Stmt::Kind::Return: return true;
      case Stmt::Kind::Block: return blockReturns(s.stmts);
      case Stmt::Kind::If:
        return s.elseBody && stmtReturns(*s.body) && stmtReturns(*s.elseBody);
      default: return false;
    }
  }
  bool blockReturns(const std::vector<StmtPtr>& stmts) const {
    return !stmts.empty() && stmtReturns(*stmts.back());
  }

  void checkStmt(const Stmt& s) {
    switch (s.kind) {
      case Stmt::Kind::Decl:
        for (const DeclItem& d : s.decls) {
          if (d.init) {
            Type t = checkExpr(*d.init);
            if (t == TypeKind::Void)
              error(d.line, "cannot initialize a variable with a void expression");
            if (t != d.type)
              error(d.line, std::string("type mismatch: variable '") + d.name +
                                "' is declared as " + typeName(d.type) +
                                " but initialized with " + typeName(t));
          }
          declare(d.line, d.name, d.type);
        }
        break;

      case Stmt::Kind::Assign: {
        Type targetType = checkLValue(*s.target);
        Type t = checkExpr(*s.expr);
        if (t == TypeKind::Void) error(s.line, "cannot assign a void expression");
        if (t != targetType)
          error(s.line, std::string("type mismatch: cannot assign ") +
                            typeName(t) + " to a place of type " +
                            typeName(targetType));
        break;
      }

      case Stmt::Kind::Expr:
        checkExpr(*s.expr);
        break;

      case Stmt::Kind::Return: {
        if (currentRet == TypeKind::Void) {
          if (s.expr) error(s.line, "void function cannot return a value");
        } else {
          if (!s.expr)
            error(s.line, std::string("function must return ") +
                              typeName(currentRet));
          Type t = checkExpr(*s.expr);
          if (t != currentRet)
            error(s.line, std::string("return type mismatch: expected ") +
                              typeName(currentRet) + " but got " + typeName(t));
        }
        break;
      }

      case Stmt::Kind::If: {
        Type c = checkExpr(*s.expr);
        if (c != TypeKind::Bool)
          error(s.line, std::string("if condition must be bool, not ") +
                            typeName(c));
        checkStmt(*s.body);
        if (s.elseBody) checkStmt(*s.elseBody);
        break;
      }

      case Stmt::Kind::While: {
        Type c = checkExpr(*s.expr);
        if (c != TypeKind::Bool)
          error(s.line, std::string("while condition must be bool, not ") +
                            typeName(c));
        checkStmt(*s.body);
        break;
      }

      case Stmt::Kind::Go: {
        Type a = checkExpr(*s.from);
        Type b = checkExpr(*s.to);
        if (a != TypeKind::Int || b != TypeKind::Int)
          error(s.line, "__go bounds must be int");
        scopes.emplace_back();
        declare(s.line, "ex", TypeKind::Int);
        if (s.filter) {
          Type f = checkExpr(*s.filter);
          if (f != TypeKind::Bool)
            error(s.line, std::string("when condition must be bool, not ") +
                              typeName(f));
        }
        checkStmt(*s.body);
        scopes.pop_back();
        break;
      }

      case Stmt::Kind::For: {
        scopes.emplace_back();
        if (s.init) checkStmt(*s.init);
        if (s.expr) {
          Type c = checkExpr(*s.expr);
          if (c != TypeKind::Bool)
            error(s.line, std::string("for condition must be bool, not ") +
                              typeName(c));
        }
        if (s.update) checkStmt(*s.update);
        checkStmt(*s.body);
        scopes.pop_back();
        break;
      }

      case Stmt::Kind::Block: {
        scopes.emplace_back();
        for (const auto& st : s.stmts) checkStmt(*st);
        scopes.pop_back();
        break;
      }
    }
  }

  Type requireValue(Type t, int line) const {
    if (t == TypeKind::Void)
      error(line, "void expression used where a value is required");
    return t;
  }

  Type checkExpr(Expr& e) {
    Type t = TypeKind::Error;
    switch (e.kind) {
      case Expr::Kind::IntLit: t = TypeKind::Int; break;
      case Expr::Kind::BoolLit: t = TypeKind::Bool; break;
      case Expr::Kind::StrLit: t = TypeKind::Str; break;
      case Expr::Kind::Var: t = lookup(e.line, e.name).type; break;

      case Expr::Kind::Unary: {
        Type a = requireValue(checkExpr(*e.lhs), e.line);
        if (e.op == "-") {
          if (a != TypeKind::Int)
            error(e.line, std::string("unary '-' expects int, got ") +
                              typeName(a));
          t = TypeKind::Int;
        } else {
          if (a != TypeKind::Bool)
            error(e.line, std::string("'!' expects bool, got ") + typeName(a));
          t = TypeKind::Bool;
        }
        break;
      }

      case Expr::Kind::Binary: {
        Type a = requireValue(checkExpr(*e.lhs), e.line);
        Type b = requireValue(checkExpr(*e.rhs), e.line);
        t = checkBinary(e, a, b);
        break;
      }

      case Expr::Kind::Call:
        t = checkCall(e);
        break;

      case Expr::Kind::Member:
        t = checkMember(e);
        break;

      // A mention '@x' is transparent: it just points at the thing named and
      // has the same type. Whether it may be written to is decided by the
      // capability's 'mut' marker, not by '@'.
      case Expr::Kind::Mention:
        t = requireValue(checkExpr(*e.lhs), e.line);
        break;
    }
    e.type = t;
    return t;
  }

  Type checkBinary(const Expr& e, Type a, Type b) {
    const std::string& op = e.op;
    auto bothInt = [&]() {
      if (a != TypeKind::Int || b != TypeKind::Int)
        error(e.line, "operator '" + op + "' expects int operands, got " +
                          typeName(a) + " and " + typeName(b));
    };

    if (op == "+") {
      if (a == TypeKind::Int && b == TypeKind::Int) return TypeKind::Int;
      if (a == TypeKind::Str && b == TypeKind::Str) return TypeKind::Str;
      error(e.line, "operator '+' expects int+int or string+string, got " +
                        std::string(typeName(a)) + "+" + typeName(b));
    }
    if (op == "-" || op == "*" || op == "/" || op == "%") {
      bothInt();
      return TypeKind::Int;
    }
    if (op == "<" || op == "<=" || op == ">" || op == ">=") {
      bothInt();
      return TypeKind::Bool;
    }
    if ((op == "==" || op == "!=") && (a.isStruct() || b.isStruct()))
      error(e.line, "structs cannot be compared yet");
    if (op == "==" || op == "!=") {
      if (a != b)
        error(e.line, "operator '" + op + "' compares incompatible types " +
                          std::string(typeName(a)) + " and " + typeName(b));
      if (a == TypeKind::Void) error(e.line, "cannot compare void values");
      return TypeKind::Bool;
    }
    if (op == "&&" || op == "||") {
      if (a != TypeKind::Bool || b != TypeKind::Bool)
        error(e.line, "operator '" + op + "' expects bool operands, got " +
                          std::string(typeName(a)) + " and " + typeName(b));
      return TypeKind::Bool;
    }
    error(e.line, "unknown operator '" + op + "'");
  }

  Type checkLValue(Expr& e) {
    if (e.kind == Expr::Kind::Var) {
      e.type = lookup(e.line, e.name).type;
      return e.type;
    }
    if (e.kind == Expr::Kind::Member) {
      e.type = checkMember(e);
      return e.type;
    }
    error(e.line, "left-hand side of '=' must be a variable or a field");
  }

  Type checkMember(Expr& e) {
    Type base = requireValue(checkExpr(*e.lhs), e.line);
    if (!base.isStruct())
      error(e.line, std::string("'.' requires a struct value, got ") +
                        typeName(base));
    auto it = prog->structMap.find(base.sname);
    if (it == prog->structMap.end())
      error(e.line, "unknown struct '" + base.sname + "'");
    for (const StructField& f : it->second->fields)
      if (f.name == e.name) return f.type;
    error(e.line, "struct '" + base.sname + "' has no field '" + e.name + "'");
  }

  Type checkCall(Expr& e) {
    // Type-check the arguments first.
    std::vector<Type> argTypes;
    for (auto& a : e.args) argTypes.push_back(checkExpr(*a));

    auto it = currentScope->find(e.name);
    if (it == currentScope->end())
      error(e.line, "unknown function or capability '" + e.name +
                        "' (did you forget to 'pick' it?)");

    std::vector<const FuncDecl*> matches;
    for (const FuncDecl* f : it->second) {
      if (f->params.size() != argTypes.size()) continue;
      bool ok = true;
      for (size_t i = 0; i < argTypes.size(); ++i)
        if (f->params[i].type != argTypes[i]) { ok = false; break; }
      if (ok) matches.push_back(f);
    }

    if (matches.empty()) {
      std::string expected;
      for (const FuncDecl* f : it->second) {
        if (!expected.empty()) expected += " or ";
        expected += "(";
        for (size_t i = 0; i < f->params.size(); ++i) {
          if (i) expected += ", ";
          expected += typeName(f->params[i].type);
        }
        expected += ")";
      }
      error(e.line, "no overload of '" + e.name + "' matches the arguments; expected " +
                        expected);
    }
    if (matches.size() > 1)
      error(e.line, "ambiguous call to '" + e.name + "'");

    const FuncDecl* f = matches[0];
    for (size_t i = 0; i < f->params.size(); ++i) {
      const Expr* arg = e.args[i].get();
      while (arg->kind == Expr::Kind::Mention) arg = arg->lhs.get();
      if (f->params[i].mut && arg->kind != Expr::Kind::Var)
        error(e.line, "argument " + std::to_string(i + 1) + " of '" + e.name +
                          "' must be a variable (the parameter is 'mut')");
    }
    e.resolved = f;
    return f->returnType;
  }
};

}  // namespace

void typeCheck(const LoadedProgram& prog) {
  Checker c;
  c.run(prog);
}
