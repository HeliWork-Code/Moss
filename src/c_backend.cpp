#include "c_backend.h"

#include <cstdio>
#include <map>
#include <sstream>

extern const char* const kRuntimePrelude;

namespace {

std::string arrayBase(Type t) {
  if (t.elem && t.elem->kind == TypeKind::Bool) return "moss_arr_bool";
  if (t.elem && t.elem->kind == TypeKind::Str) return "moss_arr_str";
  return "moss_arr_int";
}

std::string ctype(Type t) {
  switch (t.kind) {
    case TypeKind::Int: return "int64_t";
    case TypeKind::Bool: return "bool";
    case TypeKind::Str: return "moss_str";
    case TypeKind::Void: return "void";
    case TypeKind::Struct: return "S_" + t.sname + "*";
    case TypeKind::Array: return arrayBase(t) + "*";
    default: return "void";
  }
}

std::string zeroValue(Type t) {
  switch (t.kind) {
    case TypeKind::Int: return "0";
    case TypeKind::Bool: return "false";
    case TypeKind::Str: return "moss_str_make(\"\", 0)";
    case TypeKind::Struct:
      return "((S_" + t.sname + "*)calloc(1, sizeof(S_" + t.sname + ")))";
    case TypeKind::Array:
      return arrayBase(t) + "_new(0, 0)";
    default: return "0";
  }
}

std::string typeCode(Type t) {
  switch (t.kind) {
    case TypeKind::Int: return "i";
    case TypeKind::Bool: return "b";
    case TypeKind::Str: return "s";
    case TypeKind::Void: return "v";
    case TypeKind::Struct: return "S" + t.sname;
    case TypeKind::Array:
      return "A" + (t.elem ? typeCode(*t.elem) : std::string("e"));
    default: return "e";
  }
}

// Moss names are already [A-Za-z_][A-Za-z0-9_]*, so they are valid C
// identifiers. Locals get a "v_" prefix to avoid clashing with the runtime.
std::string cvar(const std::string& name) { return "v_" + name; }

std::string escapeC(const std::string& s) {
  std::string r;
  for (unsigned char c : s) {
    switch (c) {
      case '\\': r += "\\\\"; break;
      case '"': r += "\\\""; break;
      case '\n': r += "\\n"; break;
      case '\t': r += "\\t"; break;
      case '\r': r += "\\r"; break;
      default:
        if (c >= 32 && c < 127) {
          r += (char)c;
        } else {
          char buf[8];
          std::snprintf(buf, sizeof buf, "\\%03o", c);  // exactly 3 digits
          r += buf;
        }
    }
  }
  return r;
}

class CEmitter {
 public:
  std::string run(const LoadedProgram& prog) {
    assignNames(prog);

    out << kRuntimePrelude << "\n";

    for (const auto& sd : prog.structs) {
      out << "typedef struct S_" << sd->name << " {";
      for (const StructField& f : sd->fields)
        out << " " << ctype(f.type) << " f_" << f.name << ";";
      out << " } S_" << sd->name << ";\n";
    }
    out << "\n";

    for (const FuncDecl* f : prog.bodies)
      out << "static " << signature(*f) << ";\n";
    out << "\n";

    for (const FuncDecl* f : prog.bodies) emitFunction(*f);
    emitEntry(*prog.mainFn);
    return out.str();
  }

 private:
  std::ostringstream out;
  std::map<const FuncDecl*, std::string> names;
  int indentLevel = 0;
  int tmpCounter = 0;

  void pad() {
    for (int i = 0; i < indentLevel; ++i) out << "  ";
  }

  void assignNames(const LoadedProgram& prog) {
    int index = 0;
    for (const FuncDecl* f : prog.bodies) {
      std::string n = "m_";
      n += f->module.empty() ? "self" : f->module;
      n += "__";
      n += f->methodOf.empty() ? f->name : f->methodOf + "_" + f->name;
      n += "__";
      if (f->params.empty()) n += "v";
      for (const Param& p : f->params) {
        n += typeCode(p.type);
        if (p.mut) n += "x";
      }
      n += "_" + std::to_string(index++);
      names[f] = n;
    }
  }

  std::string signature(const FuncDecl& f) {
    std::string params;
    for (const Param& p : f.params) {
      if (!params.empty()) params += ", ";
      params += ctype(p.type);
      if (p.mut) params += "*";
      params += " " + cvar(p.name);
    }
    if (params.empty()) params = "void";
    return ctype(f.returnType) + " " + names.at(&f) + "(" + params + ")";
  }

  void emitFunction(const FuncDecl& f) {
    out << "static " << signature(f) << " {\n";
    indentLevel = 1;
    for (const auto& s : f.body) stmt(*s);
    if (f.returnType != TypeKind::Void) {
      pad();
      out << "return " << zeroValue(f.returnType) << ";\n";
    }
    indentLevel = 0;
    out << "}\n\n";
  }

  void emitBody(const Stmt& body) {
    out << "{\n";
    ++indentLevel;
    if (body.kind == Stmt::Kind::Block)
      for (const auto& s : body.stmts) stmt(*s);
    else
      stmt(body);
    --indentLevel;
    pad();
    out << "}\n";
  }

  // Renders a simple statement without indentation or trailing ';' -- used
  // for the three clauses of a generated C for-loop.
  std::string inlineStmt(const Stmt& s) {
    switch (s.kind) {
      case Stmt::Kind::Decl: {
        std::string res;
        for (const DeclItem& d : s.decls) {
          if (!res.empty()) res += ", ";
          res += ctype(d.type) + " " + cvar(d.name) + " = ";
          res += d.init ? expr(*d.init) : zeroValue(d.type);
        }
        return res;
      }
      case Stmt::Kind::Assign:
        return expr(*s.target) + " = " + expr(*s.expr);
      case Stmt::Kind::Expr:
        return expr(*s.expr);
      default:
        return "";
    }
  }

  void stmt(const Stmt& s) {
    switch (s.kind) {
      case Stmt::Kind::Decl:
        for (const DeclItem& d : s.decls) {
          pad();
          if (d.size) {
            std::string base = arrayBase(d.type);
            std::string maxExpr =
                d.growable
                    ? (d.maxSize ? expr(*d.maxSize) : std::string("-1"))
                    : expr(*d.size);
            out << base << "* " << cvar(d.name) << " = " << base << "_new("
                << expr(*d.size) << ", " << maxExpr << ");\n";
            continue;
          }
          out << ctype(d.type) << " " << cvar(d.name) << " = ";
          if (d.init) out << expr(*d.init);
          else out << zeroValue(d.type);
          out << ";\n";
        }
        break;

      case Stmt::Kind::Assign:
        pad();
        if (s.target->kind == Expr::Kind::Index) {
          out << "moss_set_" << arraySuffix(s.target->lhs->type) << "("
              << expr(*s.target->lhs) << ", " << expr(*s.target->rhs) << ", "
              << expr(*s.expr) << ");\n";
        } else {
          out << expr(*s.target) << " = " << expr(*s.expr) << ";\n";
        }
        break;

      case Stmt::Kind::Expr:
        pad();
        out << expr(*s.expr) << ";\n";
        break;

      case Stmt::Kind::Return:
        pad();
        if (s.expr) out << "return " << expr(*s.expr) << ";\n";
        else out << "return;\n";
        break;

      case Stmt::Kind::If:
        pad();
        out << "if (" << expr(*s.expr) << ") ";
        emitBody(*s.body);
        if (s.elseBody) {
          pad();
          out << "else ";
          emitBody(*s.elseBody);
        }
        break;

      case Stmt::Kind::While:
        pad();
        out << "while (" << expr(*s.expr) << ") ";
        emitBody(*s.body);
        break;

      case Stmt::Kind::Go:
        pad();
        out << "{\n";
        ++indentLevel;
        pad();
        out << "int64_t v_ex = " << expr(*s.from) << ";\n";
        pad();
        out << "int64_t v_goend = " << expr(*s.to) << ";\n";
        pad();
        out << "for (; v_ex <= v_goend; v_ex = v_ex + 1) {\n";
        ++indentLevel;
        if (s.filter) {
          pad();
          out << "if (!(" << expr(*s.filter) << ")) continue;\n";
        }
        if (s.body->kind == Stmt::Kind::Block)
          for (const auto& st : s.body->stmts) stmt(*st);
        else
          stmt(*s.body);
        --indentLevel;
        pad();
        out << "}\n";
        --indentLevel;
        pad();
        out << "}\n";
        break;

      case Stmt::Kind::For:
        pad();
        out << "for (" << (s.init ? inlineStmt(*s.init) : "") << "; "
            << (s.expr ? expr(*s.expr) : "") << "; "
            << (s.update ? inlineStmt(*s.update) : "") << ") ";
        emitBody(*s.body);
        break;

      case Stmt::Kind::ForEach: {
        pad();
        out << "{\n";
        ++indentLevel;
        if (s.isRange) {
          std::string v = cvar(s.varName);
          pad();
          out << "for (int64_t " << v << " = " << expr(*s.from) << "; " << v
              << " <= " << expr(*s.to) << "; " << v << " = " << v
              << " + 1) {\n";
          ++indentLevel;
          if (s.body->kind == Stmt::Kind::Block)
            for (const auto& st : s.body->stmts) stmt(*st);
          else
            stmt(*s.body);
          --indentLevel;
          pad();
          out << "}\n";
        } else {
          Type it = s.expr->type;
          int id = ++tmpCounter;
          std::string tmp = "v_it" + std::to_string(id);
          std::string idx = "v_i" + std::to_string(id);
          pad();
          out << arrayBase(it) << "* " << tmp << " = " << expr(*s.expr)
              << ";\n";
          pad();
          out << "for (int64_t " << idx << " = 0; " << idx << " < "
              << arrayBase(it) << "_len(" << tmp << "); " << idx << " = " << idx
              << " + 1) {\n";
          ++indentLevel;
          pad();
          out << ctype(*it.elem) << " " << cvar(s.varName) << " = moss_at_"
              << arraySuffix(it) << "(" << tmp << ", " << idx << ");\n";
          if (s.body->kind == Stmt::Kind::Block)
            for (const auto& st : s.body->stmts) stmt(*st);
          else
            stmt(*s.body);
          --indentLevel;
          pad();
          out << "}\n";
        }
        --indentLevel;
        pad();
        out << "}\n";
        break;
      }

      case Stmt::Kind::Block:
        pad();
        out << "{\n";
        ++indentLevel;
        for (const auto& st : s.stmts) stmt(*st);
        --indentLevel;
        pad();
        out << "}\n";
        break;
    }
  }

  static std::string arraySuffix(Type arr) {
    if (arr.elem && arr.elem->kind == TypeKind::Bool) return "bool";
    if (arr.elem && arr.elem->kind == TypeKind::Str) return "str";
    return "int";
  }

  static const Expr& unwrap(const Expr& e) {
    const Expr* p = &e;
    while (p->kind == Expr::Kind::Mention) p = p->lhs.get();
    return *p;
  }

  std::string lvalue(const Expr& e) {
    const Expr& v = unwrap(e);
    return cvar(v.name);
  }

  std::string expr(const Expr& e) {
    switch (e.kind) {
      case Expr::Kind::IntLit:
        return "INT64_C(" + std::to_string(e.intVal) + ")";
      case Expr::Kind::BoolLit:
        return e.boolVal ? "true" : "false";
      case Expr::Kind::StrLit:
        return "moss_str_make(\"" + escapeC(e.strVal) + "\", " +
               std::to_string(e.strVal.size()) + ")";
      case Expr::Kind::Var:
        return cvar(e.name);
      case Expr::Kind::Mention:
        return expr(*e.lhs);
      case Expr::Kind::Unary:
        if (e.op == "-") return "(-" + expr(*e.lhs) + ")";
        return "(!" + expr(*e.lhs) + ")";
      case Expr::Kind::Binary:
        return binary(e);
      case Expr::Kind::Call:
        return call(e);
      case Expr::Kind::MethodCall: {
        const FuncDecl* f = e.resolved;
        if (!f) return "0";
        std::string s = names.at(f) + "(" + expr(*e.lhs);
        for (const auto& a : e.args) s += ", " + expr(*a);
        return s + ")";
      }
      case Expr::Kind::Member:
        return "(" + expr(*e.lhs) + ")->f_" + e.name;
      case Expr::Kind::Index:
        return "moss_at_" + arraySuffix(e.lhs->type) + "(" + expr(*e.lhs) +
               ", " + expr(*e.rhs) + ")";
    }
    return "0";
  }

  std::string binary(const Expr& e) {
    std::string a = expr(*e.lhs);
    std::string b = expr(*e.rhs);
    if (e.op == "&&") return "(" + a + " && " + b + ")";
    if (e.op == "||") return "(" + a + " || " + b + ")";
    if (e.op == "+" && e.type == TypeKind::Str)
      return "moss_str_cat(" + a + ", " + b + ")";
    if (e.op == "/") return "moss_div(" + a + ", " + b + ")";
    if (e.op == "%") return "moss_mod(" + a + ", " + b + ")";
    if (e.op == "==" && e.lhs->type == TypeKind::Str)
      return "moss_str_eq(" + a + ", " + b + ")";
    if (e.op == "!=" && e.lhs->type == TypeKind::Str)
      return "(!moss_str_eq(" + a + ", " + b + "))";
    return "(" + a + " " + e.op + " " + b + ")";
  }

  std::string call(const Expr& e) {
    const FuncDecl* f = e.resolved;
    if (!f) return "0";
    if (f->isNative) return native(e, *f);

    std::string s = names.at(f) + "(";
    for (size_t i = 0; i < e.args.size(); ++i) {
      if (i) s += ", ";
      if (f->params[i].mut) s += "&" + lvalue(*e.args[i]);
      else s += expr(*e.args[i]);
    }
    s += ")";
    return s;
  }

  std::string native(const Expr& e, const FuncDecl& f) {
    switch (f.nativeId) {
      case NativePrint: {
        Type t = unwrap(*e.args[0]).type;
        std::string fn = t == TypeKind::Int    ? "moss_print_int"
                         : t == TypeKind::Bool ? "moss_print_bool"
                                           : "moss_print_str";
        return fn + "(" + expr(*e.args[0]) + ")";
      }
      case NativeReadInt:
        return "(" + lvalue(*e.args[0]) + " = moss_read_int())";
      case NativeReadStr:
        return "(" + lvalue(*e.args[0]) + " = moss_read_str())";
      case NativeReadBool:
        return "(" + lvalue(*e.args[0]) + " = moss_read_bool())";
      case NativeReadIntSep:
        return "(" + lvalue(*e.args[0]) + " = moss_read_int(), moss_skip_sep(" +
               expr(*e.args[1]) + "), " + lvalue(*e.args[2]) +
               " = moss_read_int())";
      case NativeLen:
        return arrayBase(e.args[0]->type) + "_len(" + expr(*e.args[0]) + ")";
      case NativeGrow:
        return arrayBase(e.args[0]->type) + "_grow(" + expr(*e.args[0]) +
               ", " + expr(*e.args[1]) + ")";
      case NativeShrink:
        return arrayBase(e.args[0]->type) + "_shrink(" + expr(*e.args[0]) +
               ", " + expr(*e.args[1]) + ")";
      case NativeAskInt:
      case NativeAskStr:
      case NativeAskBool: {
        std::string readfn = f.nativeId == NativeAskInt    ? "moss_read_int"
                             : f.nativeId == NativeAskStr  ? "moss_read_str"
                                                           : "moss_read_bool";
        std::string s = "(";
        if (e.args.size() == 2)
          s += "moss_print_inline(" + expr(*e.args[0]) + "), ";
        s += lvalue(*e.args.back()) + " = " + readfn + "()";
        return s + ")";
      }
    }
    return "0";
  }

  void emitEntry(const FuncDecl& mainFn) {
    std::string callName = names.at(&mainFn);
    std::string arg = mainFn.params.size() == 1 ? "moss_join_args(argc, argv)" : "";
    std::string call = callName + "(" + arg + ")";

    out << "int main(int argc, char** argv) {\n";
    out << "  (void)argc;\n  (void)argv;\n";
    if (mainFn.returnType == TypeKind::Void)
      out << "  " << call << ";\n  return 0;\n";
    else
      out << "  return (int)" << call << ";\n";
    out << "}\n";
  }
};

}  // namespace

std::string emitC(const LoadedProgram& prog) {
  CEmitter e;
  return e.run(prog);
}
