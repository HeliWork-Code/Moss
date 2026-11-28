#include "typeck.h"
#include <unordered_map>
#include <vector>

namespace {

struct VarInfo {
  Type type;
  bool mutable_ = true;
  // For array variables: the declared maximum length, or -1 when unbounded or
  // unknown (a parameter, or a non-constant declaration).
  long long cap = -1;
};

class Checker {
 public:
  void run(const LoadedProgram& p) {
    prog = &p;
    checkStructs(p);
    for (const FuncDecl* f : p.bodies) checkFunction(*f);
  }

 private:
  const LoadedProgram* prog = nullptr;
  const CapScope* currentScope = nullptr;
  std::vector<std::unordered_map<std::string, VarInfo>> scopes;
  Type currentRet = TypeKind::Void;
  bool currentIsMain = false;
  std::string currentStruct;  // set while checking a method

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

  void declare(int line, const std::string& name, Type type,
               bool mutable_ = true, long long cap = -1) {
    auto& scope = scopes.back();
    if (scope.count(name))
      error(line, "variable '" + name + "' is already declared in this scope");
    scope[name] = VarInfo{type, mutable_, cap};
  }

  const VarInfo* findVar(const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      auto found = it->find(name);
      if (found != it->end()) return &found->second;
    }
    return nullptr;
  }

  const VarInfo& lookup(int line, const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      auto found = it->find(name);
      if (found != it->end()) return found->second;
    }
    error(line, "use of undeclared variable '" + name + "'");
  }

  void checkArrayElem(Type t, int line) {
    if (t.kind != TypeKind::Array) return;
    TypeKind ek = t.elem ? t.elem->kind : TypeKind::Error;
    if (ek != TypeKind::Int && ek != TypeKind::Bool && ek != TypeKind::Str)
      error(line, "array elements must be int, bool or string");
  }

  // Checks an array literal against a known element type. `\null` entries are
  // holes: they occupy a position but write nothing.
  void checkLiteralElements(Expr& lit, const Type& elem) {
    for (size_t i = 0; i < lit.args.size(); ++i) {
      Expr& e = *lit.args[i];
      if (e.kind == Expr::Kind::NullLit) continue;
      if (e.kind == Expr::Kind::ArrayLit)
        error(e.line, "arrays of arrays are not supported yet");
      Type t = checkExpr(e);
      if (t != elem)
        error(e.line, "array literal element " + std::to_string(i + 1) +
                          " has type " + typeName(t) + ", expected " +
                          typeName(elem));
    }
    lit.type = Type::ofArray(elem);
  }

  void checkArrayInitializer(const DeclItem& d) {
    if (d.init->kind != Expr::Kind::ArrayLit)
      error(d.line, "an array can only be initialized with an array literal");
    // The length rules need compile-time constants.
    if (d.size->kind != Expr::Kind::IntLit)
      error(d.line, "an array literal requires a constant length");
    if (d.maxSize && d.maxSize->kind != Expr::Kind::IntLit)
      error(d.line, "an array literal requires a constant capacity");
    long long hi = d.growable ? (d.maxSize ? d.maxSize->intVal : -1)
                              : d.size->intVal;
    long long k = (long long)d.init->args.size();
    if (hi >= 0 && k > hi)
      error(d.line, "array literal has " + std::to_string(k) +
                        " element(s) but '" + d.name + "' can hold at most " +
                        std::to_string(hi));
    checkLiteralElements(*d.init, *d.type.elem);
    d.init->type = d.type;
  }

  // `a = {...}` and `a[X] = {...}`.
  void checkArrayAssign(const Stmt& s) {
    Expr& rhs = *s.expr;
    if (s.target->kind == Expr::Kind::Var) {
      const VarInfo& v = lookup(s.line, s.target->name);
      if (!v.mutable_)
        error(s.line, "cannot assign to constant '" + s.target->name + "'");
      if (!v.type.isArray())
        error(s.line, "array literal assigned to '" + s.target->name +
                          "', which is not an array");
      s.target->type = v.type;
      s.target->arrCap = v.cap;
      checkLiteralElements(rhs, *v.type.elem);
      return;
    }
    if (s.target->kind == Expr::Kind::Index) {
      Expr& base = *s.target->lhs;
      if (base.kind != Expr::Kind::Var)
        error(s.line, "a prefix assignment needs a plain array variable for now");
      const VarInfo& v = lookup(s.line, base.name);
      if (!v.mutable_)
        error(s.line, "cannot assign to constant '" + base.name + "'");
      if (!v.type.isArray())
        error(s.line, std::string("'[]' requires an array, got ") +
                          typeName(v.type));
      base.type = v.type;
      base.arrCap = v.cap;
      Type idx = requireValue(checkExpr(*s.target->rhs), s.line);
      if (idx != TypeKind::Int)
        error(s.line, std::string("array index must be int, not ") +
                          typeName(idx));
      if (s.target->rhs->kind != Expr::Kind::IntLit)
        error(s.line, "the prefix length must be a constant for now");
      long long x = s.target->rhs->intVal;
      long long k = (long long)rhs.args.size();
      if (s.roll && k != x)
        error(s.line, "array literal has " + std::to_string(k) +
                          " element(s) but the prefix is " + std::to_string(x) +
                          " (__roll needs exactly " + std::to_string(x) + ")");
      if (k > x)
        error(s.line, "array literal has " + std::to_string(k) +
                          " element(s) but the prefix is " + std::to_string(x));
      checkLiteralElements(rhs, *v.type.elem);
      return;
    }
    error(s.line,
          "an array literal can only be assigned to an array variable or a prefix");
  }

  // Struct fields may hold int, bool, string, another struct, or an array of
  // int/bool/string. A struct that contains itself would make the default
  // value infinite, so those cycles are rejected up front.
  void checkStructs(const LoadedProgram& p) {
    for (const auto& sd : p.structs) {
      for (const StructField& f : sd->fields) {
        if (f.type.kind == TypeKind::Void)
          error(f.line, "struct field '" + f.name + "' cannot be void");
        if (f.type.kind == TypeKind::Array) {
          TypeKind ek = f.type.elem ? f.type.elem->kind : TypeKind::Error;
          if (ek != TypeKind::Int && ek != TypeKind::Bool &&
              ek != TypeKind::Str)
            error(f.line, "array field '" + f.name +
                              "' must hold int, bool or string");
        }
        if (f.type.kind == TypeKind::Struct &&
            !p.structMap.count(f.type.sname))
          error(f.line, "unknown struct '" + f.type.sname + "'");
      }
    }
    std::unordered_map<std::string, int> state;  // 1 visiting, 2 done
    for (const auto& sd : p.structs) visitStruct(*sd, state);
  }

  void visitStruct(const StructDecl& sd,
                   std::unordered_map<std::string, int>& state) {
    int& st = state[sd.name];
    if (st == 2) return;
    st = 1;
    for (const StructField& f : sd.fields) {
      if (f.type.kind != TypeKind::Struct) continue;
      auto it = prog->structMap.find(f.type.sname);
      if (it == prog->structMap.end()) continue;  // reported already
      int& next = state[it->second->name];
      if (next == 1)
        error(f.line, "struct '" + sd.name +
                          "' contains itself through field '" + f.name + "'");
      if (next == 0) visitStruct(*it->second, state);
    }
    st = 2;
  }

  void checkFunction(const FuncDecl& f) {
    checkArrayElem(f.returnType, f.line);
    for (const Param& p : f.params) checkArrayElem(p.type, f.line);
    scopes.clear();
    scopes.emplace_back();
    currentScope = f.scope;
    currentRet = f.returnType;
    currentIsMain = f.isMain;
    currentStruct = f.methodOf;

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
          checkArrayElem(d.type, d.line);
          if (d.size) {
            Type st = checkExpr(*d.size);
            if (st != TypeKind::Int)
              error(d.line, "array length must be int, not " +
                                std::string(typeName(st)));
            if (d.maxSize) {
              Type mt = checkExpr(*d.maxSize);
              if (mt != TypeKind::Int)
                error(d.line, "array capacity must be int, not " +
                                  std::string(typeName(mt)));
            }
            if (d.init) checkArrayInitializer(d);
          }
          if (d.init && !d.size) {
            Type t = checkExpr(*d.init);
            if (t == TypeKind::Void)
              error(d.line, "cannot initialize a variable with a void expression");
            if (t != d.type)
              error(d.line, std::string("type mismatch: variable '") + d.name +
                                "' is declared as " + typeName(d.type) +
                                " but initialized with " + typeName(t));
          }
          long long cap = -1;
          if (d.size && d.size->kind == Expr::Kind::IntLit) {
            if (!d.growable) {
              cap = d.size->intVal;
            } else if (d.maxSize && d.maxSize->kind == Expr::Kind::IntLit) {
              cap = d.maxSize->intVal;
            }
          }
          declare(d.line, d.name, d.type, !d.isConst, cap);
        }
        break;

      case Stmt::Kind::Assign: {
        if (s.expr->kind == Expr::Kind::ArrayLit) {
          checkArrayAssign(s);
          break;
        }
        if (s.roll)
          error(s.line, "'__roll' needs an array literal on the right");
        Type targetType = checkLValue(*s.target);
        if (s.target->kind == Expr::Kind::Var) {
          const VarInfo& v = lookup(s.line, s.target->name);
          if (!v.mutable_)
            error(s.line,
                  "cannot assign to constant '" + s.target->name + "'");
        } else if (s.target->kind == Expr::Kind::Index) {
          // Writing through a constant array's elements is a write to the
          // constant, since arrays are references.
          const Expr* base = s.target->lhs.get();
          while (base->kind == Expr::Kind::Mention) base = base->lhs.get();
          if (base->kind == Expr::Kind::Var) {
            const VarInfo& v = lookup(s.line, base->name);
            if (!v.mutable_)
              error(s.line, "cannot assign to an element of constant array '" +
                                base->name + "'");
          }
        }
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

      case Stmt::Kind::ForEach: {
        scopes.emplace_back();
        if (s.isRange) {
          Type a = checkExpr(*s.from);
          Type b = checkExpr(*s.to);
          if (a != TypeKind::Int || b != TypeKind::Int)
            error(s.line, "for range bounds must be int");
          declare(s.line, s.varName, TypeKind::Int);
        } else {
          Type it = checkExpr(*s.expr);
          if (!it.isArray())
            error(s.line, std::string("'for ... in' requires an array, got ") +
                              typeName(it));
          declare(s.line, s.varName, *it.elem);
        }
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

      // Array literals are only accepted by checkArrayInitializer for now;
      // anywhere else there is no expected element type to check against.
      case Expr::Kind::ArrayLit:
        error(e.line,
              "array literals are only supported as array initializers yet");

      case Expr::Kind::Var: t = checkVar(e); break;

      case Expr::Kind::NullLit:
        error(e.line, "'\\null' may only appear inside an array literal");

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

      case Expr::Kind::MethodCall:
        t = checkMethodCall(e);
        break;

      case Expr::Kind::Index:
        t = checkIndex(e);
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
      e.type = checkVar(e);
      return e.type;
    }
    if (e.kind == Expr::Kind::Member) {
      e.type = checkMember(e);
      return e.type;
    }
    if (e.kind == Expr::Kind::Index) {
      e.type = checkIndex(e);
      return e.type;
    }
    error(e.line, "left-hand side of '=' must be a variable, field or element");
  }

  Type checkIndex(Expr& e) {
    Type base = requireValue(checkExpr(*e.lhs), e.line);
    if (!base.isArray())
      error(e.line, std::string("'[]' requires an array, got ") +
                        typeName(base));
    Type idx = requireValue(checkExpr(*e.rhs), e.line);
    if (idx != TypeKind::Int)
      error(e.line, std::string("array index must be int, not ") +
                        typeName(idx));
    return *base.elem;
  }

  // A bare name inside a method may refer to a field of the receiver.
  Type checkVar(Expr& e) {
    if (const VarInfo* v = findVar(e.name)) {
      e.arrCap = v->cap;
      return v->type;
    }
    if (!currentStruct.empty()) {
      auto sit = prog->structMap.find(currentStruct);
      if (sit != prog->structMap.end())
        for (const StructField& f : sit->second->fields)
          if (f.name == e.name) {
            ExprPtr self = Expr::var("self", e.line);
            self->type = Type::ofStruct(currentStruct);
            e.lhs = std::move(self);
            e.kind = Expr::Kind::Member;
            return f.type;
          }
    }
    error(e.line, "use of undeclared variable '" + e.name + "'");
  }

  Type checkMethodCall(Expr& e) {
    Type recv = requireValue(checkExpr(*e.lhs), e.line);
    if (!recv.isStruct())
      error(e.line, std::string("'.' method call requires a struct, got ") +
                        typeName(recv));
    auto sit = prog->methods.find(recv.sname);
    if (sit == prog->methods.end())
      error(e.line, "struct '" + recv.sname + "' has no method '" + e.name + "'");
    auto mit = sit->second.find(e.name);
    if (mit == sit->second.end())
      error(e.line, "struct '" + recv.sname + "' has no method '" + e.name + "'");
    const FuncDecl* m = mit->second;
    if (e.args.size() + 1 != m->params.size())
      error(e.line, "method '" + e.name + "' expects " +
                        std::to_string(m->params.size() - 1) +
                        " argument(s), got " + std::to_string(e.args.size()));
    for (size_t i = 0; i < e.args.size(); ++i) {
      Type at = checkExpr(*e.args[i]);
      if (at != m->params[i + 1].type)
        error(e.line, "argument " + std::to_string(i + 1) + " of method '" +
                          e.name + "': expected " + typeName(m->params[i + 1].type) +
                          " but got " + typeName(at));
    }
    e.resolved = m;
    return m->returnType;
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
      // A `mut` parameter may take any place that can be written: a variable,
      // a field or an element. That is what lets `grow @b.data 3` work on a
      // struct field holding an array.
      const bool place = arg->kind == Expr::Kind::Var ||
                         arg->kind == Expr::Kind::Member ||
                         arg->kind == Expr::Kind::Index;
      if (f->params[i].mut && !place)
        error(e.line, "argument " + std::to_string(i + 1) + " of '" + e.name +
                          "' must be a variable, a field or an element (the "
                          "parameter is 'mut')");
      if (f->params[i].mut && arg->kind == Expr::Kind::Var) {
        const VarInfo* v = findVar(arg->name);
        if (v && !v->mutable_)
          error(e.line, "argument " + std::to_string(i + 1) + " of '" +
                            e.name + "' must be mutable; '" + arg->name +
                            "' is a constant");
      }
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
