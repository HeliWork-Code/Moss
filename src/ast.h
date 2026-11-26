#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

enum class TypeKind { Int, Bool, Str, Void, Error, Struct, Array };

// A Moss type: one of the builtins, or a struct identified by name.
struct Type {
  TypeKind kind = TypeKind::Error;
  std::string sname;  // struct name (Struct) or element name (Array)
  std::shared_ptr<Type> elem;  // element type when kind == Array

  Type() = default;
  Type(TypeKind k) : kind(k) {}  // NOLINT: implicit by design
  static Type ofStruct(std::string name) {
    Type t;
    t.kind = TypeKind::Struct;
    t.sname = std::move(name);
    return t;
  }
  static Type ofArray(Type elem);
  bool isStruct() const { return kind == TypeKind::Struct; }
  bool isArray() const { return kind == TypeKind::Array; }
  bool operator==(const Type& o) const {
    return kind == o.kind && sname == o.sname;
  }
  bool operator!=(const Type& o) const { return !(*this == o); }
};

const char* typeName(const Type& t);

struct FuncDecl;
struct Expr;
struct Stmt;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

// name -> overload set. Used for both module scopes and the main file scope.
using CapScope = std::unordered_map<std::string, std::vector<const FuncDecl*>>;

struct Expr {
  enum class Kind { IntLit, BoolLit, StrLit, Var, Unary, Binary, Call, Mention, Member, Index };

  Kind kind;
  Type type = TypeKind::Error;  // filled in by the type checker
  int line = 0;

  long long intVal = 0;
  bool boolVal = false;
  std::string strVal;
  std::string name;   // Var, Call
  std::string op;     // Unary, Binary
  ExprPtr lhs;
  ExprPtr rhs;
  std::vector<ExprPtr> args;

  const FuncDecl* resolved = nullptr;  // Call: the function it resolves to

  explicit Expr(Kind k) : kind(k) {}

  static ExprPtr intLit(long long v, int line);
  static ExprPtr boolLit(bool v, int line);
  static ExprPtr strLit(const std::string& s, int line);
  static ExprPtr var(const std::string& name, int line);
  static ExprPtr unary(const std::string& op, ExprPtr operand, int line);
  static ExprPtr binary(const std::string& op, ExprPtr lhs, ExprPtr rhs,
                        int line);
  static ExprPtr call(const std::string& name, std::vector<ExprPtr> args,
                      int line);
  static ExprPtr mention(ExprPtr operand, int line);
  static ExprPtr member(ExprPtr base, const std::string& field, int line);
  static ExprPtr index(ExprPtr base, ExprPtr idx, int line);
};

struct DeclItem {
  std::string name;
  Type type;       // full type; for arrays this is Array(element)
  ExprPtr init;    // may be null
  ExprPtr size;    // array length expression, non-null only for arrays
  int line = 0;
};

struct Stmt {
  enum class Kind { Decl, Assign, Expr, Return, If, While, For, Go, Block };

  Kind kind;
  int line = 0;

  std::vector<DeclItem> decls;  // Decl
  ExprPtr target;               // Assign: Var or Member
  ExprPtr expr;                 // Assign value, Expr, Return value,
                                // If/While condition
  ExprPtr from;                 // Go: range start
  ExprPtr to;                   // Go: range end
  ExprPtr filter;               // Go: optional `when` condition
  StmtPtr init;                 // For: initialization clause
  StmtPtr update;               // For: update clause
  StmtPtr body;                 // If then-branch, While/For body
  StmtPtr elseBody;             // If else-branch
  std::vector<StmtPtr> stmts;   // Block

  explicit Stmt(Kind k) : kind(k) {}

  static StmtPtr declStmt(std::vector<DeclItem> items, int line);
  static StmtPtr assign(ExprPtr target, ExprPtr value, int line);
  static StmtPtr exprStmt(ExprPtr e, int line);
  static StmtPtr ret(ExprPtr e, int line);
  static StmtPtr ifStmt(ExprPtr cond, StmtPtr thenBody, StmtPtr elseBody,
                        int line);
  static StmtPtr whileStmt(ExprPtr cond, StmtPtr body, int line);
  static StmtPtr forStmt(StmtPtr init, ExprPtr cond, StmtPtr update,
                         StmtPtr body, int line);
  static StmtPtr goStmt(ExprPtr from, ExprPtr to, ExprPtr filter,
                        StmtPtr body, int line);
  static StmtPtr block(std::vector<StmtPtr> body, int line);
};

struct Param {
  std::string name;
  Type type;
  bool mut = false;
};

struct FuncDecl {
  std::string name;
  std::vector<Param> params;
  Type returnType = TypeKind::Void;
  std::vector<StmtPtr> body;
  bool hasBody = true;
  int line = 0;

  std::string module;        // "" = main file
  bool isNative = false;
  int nativeId = -1;         // see NativeId in program.h
  int bcIndex = -1;          // assigned by codegen
  const CapScope* scope = nullptr;  // visible names while checking/codegen

  bool isMain = false;
};

struct PickEntry {
  std::string moduleName;
  std::string capName;
  std::string alias;
  int line = 0;
};

struct ModuleAst {
  std::string name;
  std::vector<PickEntry> picks;
  std::vector<std::unique_ptr<FuncDecl>> caps;
};

struct StructField {
  std::string name;
  Type type;
  int line = 0;
};

struct StructDecl {
  std::string name;
  std::vector<StructField> fields;
  int line = 0;
};

struct FileAst {
  std::vector<PickEntry> picks;
  std::vector<std::unique_ptr<StructDecl>> structs;
  std::vector<std::unique_ptr<FuncDecl>> functions;
};

void dumpFunc(const FuncDecl& f);
void dumpStmt(const Stmt& s, int indent = 0);
void dumpExpr(const Expr& e, int indent = 0);
