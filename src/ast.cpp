#include "ast.h"
#include <cstdio>

Type Type::ofArray(Type elem) {
  Type t;
  t.kind = TypeKind::Array;
  t.elem = std::make_shared<Type>(elem);
  t.sname = "array<" + std::string(typeName(elem)) + ">";
  return t;
}

const char* typeName(const Type& t) {
  switch (t.kind) {
    case TypeKind::Int: return "int";
    case TypeKind::Bool: return "bool";
    case TypeKind::Str: return "string";
    case TypeKind::Void: return "void";
    case TypeKind::Struct: return t.sname.c_str();
    case TypeKind::Array: return t.sname.c_str();
    case TypeKind::Error: return "<error>";
  }
  return "?";
}

ExprPtr Expr::intLit(long long v, int line) {
  ExprPtr e(new Expr(Kind::IntLit));
  e->intVal = v;
  e->line = line;
  return e;
}
ExprPtr Expr::boolLit(bool v, int line) {
  ExprPtr e(new Expr(Kind::BoolLit));
  e->boolVal = v;
  e->line = line;
  return e;
}
ExprPtr Expr::strLit(const std::string& s, int line) {
  ExprPtr e(new Expr(Kind::StrLit));
  e->strVal = s;
  e->line = line;
  return e;
}
ExprPtr Expr::var(const std::string& name, int line) {
  ExprPtr e(new Expr(Kind::Var));
  e->name = name;
  e->line = line;
  return e;
}
ExprPtr Expr::unary(const std::string& op, ExprPtr operand, int line) {
  ExprPtr e(new Expr(Kind::Unary));
  e->op = op;
  e->lhs = std::move(operand);
  e->line = line;
  return e;
}
ExprPtr Expr::binary(const std::string& op, ExprPtr lhs, ExprPtr rhs, int line) {
  ExprPtr e(new Expr(Kind::Binary));
  e->op = op;
  e->lhs = std::move(lhs);
  e->rhs = std::move(rhs);
  e->line = line;
  return e;
}
ExprPtr Expr::call(const std::string& name, std::vector<ExprPtr> args, int line) {
  ExprPtr e(new Expr(Kind::Call));
  e->name = name;
  e->args = std::move(args);
  e->line = line;
  return e;
}
ExprPtr Expr::member(ExprPtr base, const std::string& field, int line) {
  ExprPtr e(new Expr(Kind::Member));
  e->name = field;
  e->lhs = std::move(base);
  e->line = line;
  return e;
}
ExprPtr Expr::index(ExprPtr base, ExprPtr idx, int line) {
  ExprPtr e(new Expr(Kind::Index));
  e->lhs = std::move(base);
  e->rhs = std::move(idx);
  e->line = line;
  return e;
}

ExprPtr Expr::methodCall(ExprPtr recv, const std::string& name,
                         std::vector<ExprPtr> args, int line) {
  ExprPtr e(new Expr(Kind::MethodCall));
  e->lhs = std::move(recv);
  e->name = name;
  e->args = std::move(args);
  e->line = line;
  return e;
}

ExprPtr Expr::mention(ExprPtr operand, int line) {
  ExprPtr e(new Expr(Kind::Mention));
  e->lhs = std::move(operand);
  e->line = line;
  return e;
}

StmtPtr Stmt::declStmt(std::vector<DeclItem> items, int line) {
  StmtPtr s(new Stmt(Kind::Decl));
  s->decls = std::move(items);
  s->line = line;
  return s;
}
StmtPtr Stmt::assign(ExprPtr target, ExprPtr value, int line) {
  StmtPtr s(new Stmt(Kind::Assign));
  s->target = std::move(target);
  s->expr = std::move(value);
  s->line = line;
  return s;
}
StmtPtr Stmt::exprStmt(ExprPtr e, int line) {
  StmtPtr s(new Stmt(Kind::Expr));
  s->expr = std::move(e);
  s->line = line;
  return s;
}
StmtPtr Stmt::ret(ExprPtr e, int line) {
  StmtPtr s(new Stmt(Kind::Return));
  s->expr = std::move(e);
  s->line = line;
  return s;
}
StmtPtr Stmt::ifStmt(ExprPtr cond, StmtPtr thenBody, StmtPtr elseBody, int line) {
  StmtPtr s(new Stmt(Kind::If));
  s->expr = std::move(cond);
  s->body = std::move(thenBody);
  s->elseBody = std::move(elseBody);
  s->line = line;
  return s;
}
StmtPtr Stmt::whileStmt(ExprPtr cond, StmtPtr body, int line) {
  StmtPtr s(new Stmt(Kind::While));
  s->expr = std::move(cond);
  s->body = std::move(body);
  s->line = line;
  return s;
}
StmtPtr Stmt::forStmt(StmtPtr init, ExprPtr cond, StmtPtr update,
                      StmtPtr body, int line) {
  StmtPtr s(new Stmt(Kind::For));
  s->init = std::move(init);
  s->expr = std::move(cond);
  s->update = std::move(update);
  s->body = std::move(body);
  s->line = line;
  return s;
}
StmtPtr Stmt::goStmt(ExprPtr from, ExprPtr to, ExprPtr filter, StmtPtr body,
                     int line) {
  StmtPtr s(new Stmt(Kind::Go));
  s->from = std::move(from);
  s->to = std::move(to);
  s->filter = std::move(filter);
  s->body = std::move(body);
  s->line = line;
  return s;
}
StmtPtr Stmt::forEach(const std::string& var, bool isRange, ExprPtr from,
                      ExprPtr to, ExprPtr iterable, StmtPtr body, int line) {
  StmtPtr s(new Stmt(Kind::ForEach));
  s->varName = var;
  s->isRange = isRange;
  s->from = std::move(from);
  s->to = std::move(to);
  s->expr = std::move(iterable);
  s->body = std::move(body);
  s->line = line;
  return s;
}
StmtPtr Stmt::block(std::vector<StmtPtr> body, int line) {
  StmtPtr s(new Stmt(Kind::Block));
  s->stmts = std::move(body);
  s->line = line;
  return s;
}

namespace {
void pad(int n) {
  for (int i = 0; i < n; ++i) std::printf("  ");
}
}  // namespace

void dumpExpr(const Expr& e, int ind) {
  pad(ind);
  switch (e.kind) {
    case Expr::Kind::IntLit: std::printf("Int %lld\n", e.intVal); break;
    case Expr::Kind::BoolLit:
      std::printf("Bool %s\n", e.boolVal ? "true" : "false");
      break;
    case Expr::Kind::StrLit: std::printf("Str \"%s\"\n", e.strVal.c_str()); break;
    case Expr::Kind::Var: std::printf("Var %s\n", e.name.c_str()); break;
    case Expr::Kind::Unary:
      std::printf("Unary %s\n", e.op.c_str());
      dumpExpr(*e.lhs, ind + 1);
      break;
    case Expr::Kind::Binary:
      std::printf("Binary %s\n", e.op.c_str());
      dumpExpr(*e.lhs, ind + 1);
      dumpExpr(*e.rhs, ind + 1);
      break;
    case Expr::Kind::Call:
      std::printf("Call %s\n", e.name.c_str());
      for (const auto& a : e.args) dumpExpr(*a, ind + 1);
      break;
    case Expr::Kind::Mention:
      std::printf("Mention\n");
      dumpExpr(*e.lhs, ind + 1);
      break;
    case Expr::Kind::Member:
      std::printf("Member .%s\n", e.name.c_str());
      dumpExpr(*e.lhs, ind + 1);
      break;
    case Expr::Kind::Index:
      std::printf("Index\n");
      dumpExpr(*e.lhs, ind + 1);
      dumpExpr(*e.rhs, ind + 1);
      break;
  }
}

void dumpStmt(const Stmt& s, int ind) {
  pad(ind);
  switch (s.kind) {
    case Stmt::Kind::Decl:
      std::printf("Decl\n");
      for (const auto& d : s.decls) {
        pad(ind + 1);
        std::printf("%s: %s\n", d.name.c_str(), typeName(d.type));
        if (d.size) dumpExpr(*d.size, ind + 2);
        if (d.maxSize) dumpExpr(*d.maxSize, ind + 2);
        if (d.init) dumpExpr(*d.init, ind + 2);
      }
      break;
    case Stmt::Kind::Assign:
      std::printf("Assign\n");
      dumpExpr(*s.target, ind + 1);
      dumpExpr(*s.expr, ind + 1);
      break;
    case Stmt::Kind::Expr:
      std::printf("ExprStmt\n");
      dumpExpr(*s.expr, ind + 1);
      break;
    case Stmt::Kind::Return:
      std::printf("Return\n");
      if (s.expr) dumpExpr(*s.expr, ind + 1);
      break;
    case Stmt::Kind::If:
      std::printf("If\n");
      dumpExpr(*s.expr, ind + 1);
      dumpStmt(*s.body, ind + 1);
      if (s.elseBody) dumpStmt(*s.elseBody, ind + 1);
      break;
    case Stmt::Kind::While:
      std::printf("While\n");
      dumpExpr(*s.expr, ind + 1);
      dumpStmt(*s.body, ind + 1);
      break;
    case Stmt::Kind::For:
      std::printf("For\n");
      if (s.init) dumpStmt(*s.init, ind + 1);
      if (s.expr) dumpExpr(*s.expr, ind + 1);
      if (s.update) dumpStmt(*s.update, ind + 1);
      dumpStmt(*s.body, ind + 1);
      break;
    case Stmt::Kind::Go:
      std::printf("Go (ex)\n");
      dumpExpr(*s.from, ind + 1);
      dumpExpr(*s.to, ind + 1);
      if (s.filter) dumpExpr(*s.filter, ind + 1);
      dumpStmt(*s.body, ind + 1);
      break;
    case Stmt::Kind::Block:
      std::printf("Block\n");
      for (const auto& st : s.stmts) dumpStmt(*st, ind + 1);
      break;
  }
}

void dumpFunc(const FuncDecl& f) {
  std::printf("%s %s(", f.hasBody ? "fn" : "cap", f.name.c_str());
  for (size_t i = 0; i < f.params.size(); ++i) {
    if (i) std::printf(", ");
    if (f.params[i].mut) std::printf("mut ");
    std::printf("%s: %s", f.params[i].name.c_str(),
                typeName(f.params[i].type));
  }
  std::printf("): %s\n", typeName(f.returnType));
  for (const auto& s : f.body) dumpStmt(*s, 1);
}
