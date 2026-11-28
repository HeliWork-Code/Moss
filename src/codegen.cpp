#include "codegen.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace {

struct CodegenError : std::runtime_error {
  explicit CodegenError(const std::string& msg) : std::runtime_error(msg) {}
};

class CodeGen {
 public:
  CodeGen(BytecodeProgram& bc, const LoadedProgram& prog)
      : bc(bc), prog(prog) {
    for (size_t i = 0; i < prog.structs.size(); ++i)
      structIndexByName[prog.structs[i]->name] = (int)i;
  }

  void genFunction(const FuncDecl& f) {
    out = &bc.functions[(size_t)f.bcIndex].code;
    scopes.clear();
    scopes.emplace_back();
    nextSlot = 0;

    for (const auto& p : f.params) declare(p.name);
    for (const auto& s : f.body) genStmt(*s);

    emit(Op::PushUnit);
    emit(Op::Ret);
    bc.functions[(size_t)f.bcIndex].num_locals = nextSlot;
    out = nullptr;
  }

 private:
  BytecodeProgram& bc;
  const LoadedProgram& prog;
  std::unordered_map<std::string, int> structIndexByName;
  std::vector<Instr>* out = nullptr;
  std::vector<std::unordered_map<std::string, int>> scopes;
  std::unordered_map<std::string, int> stringPool;
  int nextSlot = 0;

  void emit(Op op, int64_t operand = 0) { out->push_back({op, operand}); }
  int here() const { return (int)out->size(); }
  void patch(int at, int target) { (*out)[at].operand = target; }

  int internString(const std::string& s) {
    auto it = stringPool.find(s);
    if (it != stringPool.end()) return it->second;
    int idx = (int)bc.strings.size();
    bc.strings.push_back(s);
    stringPool[s] = idx;
    return idx;
  }

  int declare(const std::string& name) {
    int slot = nextSlot++;
    scopes.back()[name] = slot;
    return slot;
  }
  int lookup(const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      auto found = it->find(name);
      if (found != it->end()) return found->second;
    }
    throw CodegenError("internal: unknown variable '" + name + "'");
  }

  static const Expr* unwrapMention(const Expr* e) {
    while (e->kind == Expr::Kind::Mention) e = e->lhs.get();
    return e;
  }

  int structIndex(Type t) const {
    auto it = structIndexByName.find(t.sname);
    if (it == structIndexByName.end())
      throw CodegenError("internal: unknown struct '" + t.sname + "'");
    return it->second;
  }

  int fieldIndex(Type base, const std::string& field) const {
    auto it = prog.structMap.find(base.sname);
    if (it == prog.structMap.end())
      throw CodegenError("internal: unknown struct '" + base.sname + "'");
    const auto& fields = it->second->fields;
    for (size_t i = 0; i < fields.size(); ++i)
      if (fields[i].name == field) return (int)i;
    throw CodegenError("internal: unknown field '" + field + "'");
  }

  static int elemTag(Type t) {
    if (t.kind == TypeKind::Bool) return 1;
    if (t.kind == TypeKind::Str) return 2;
    return 0;
  }

  void genDefault(Type t) {
    switch (t.kind) {
      case TypeKind::Int: emit(Op::PushInt, 0); break;
      case TypeKind::Bool: emit(Op::PushFalse); break;
      case TypeKind::Str: emit(Op::PushStr, internString("")); break;
      case TypeKind::Struct: emit(Op::NewObj, structIndex(t)); break;
      default: emit(Op::PushUnit); break;
    }
  }

  // An array declaration with an initializer: build the array, store it in
  // the local, then store each literal element. Elements past the literal keep
  // the constructor's zero value.
  void genArrayInit(const DeclItem& d, int slot) {
    long long cap = d.growable ? (d.maxSize ? d.maxSize->intVal : -1)
                               : d.size->intVal;
    long long k = (long long)d.init->args.size();
    long long len = d.growable ? std::max(d.size->intVal, k) : d.size->intVal;
    emit(Op::PushInt, len);
    if (d.growable) emit(Op::PushInt, cap);
    else emit(Op::Dup);  // fixed length: maximum == length
    emit(Op::NewArray, elemTag(*d.type.elem));
    emit(Op::StoreLocal, slot);
    for (long long i = 0; i < k; ++i) {
      const Expr& e = *d.init->args[(size_t)i];
      if (e.kind == Expr::Kind::NullLit) continue;
      emit(Op::LoadLocal, slot);
      emit(Op::PushInt, i);
      genExpr(e);
      emit(Op::SetIndex);
    }
  }

  // Wraps a literal around a fixed number of slots: value j goes to slot
  // `pos`, then pos advances modulo the array's length. No growth, no zero
  // fill; a `\null` entry still advances the slot without writing it.
  void genRollWrite(int slot, int64_t count, const Expr& lit, Type elem) {
    (void)elem;
    int posSlot = nextSlot++;
    emit(Op::PushInt, 0);
    emit(Op::StoreLocal, posSlot);
    for (int64_t j = 0; j < count; ++j) {
      const Expr& e = *lit.args[(size_t)j];
      if (e.kind != Expr::Kind::NullLit) {
        emit(Op::LoadLocal, slot);
        emit(Op::LoadLocal, posSlot);
        genExpr(e);
        emit(Op::SetIndex);
      }
      if (j + 1 < count) {
        emit(Op::LoadLocal, posSlot);
        emit(Op::PushInt, 1);
        emit(Op::AddInt);
        emit(Op::LoadLocal, slot);
        emit(Op::Len);
        emit(Op::ModInt);
        emit(Op::StoreLocal, posSlot);
      }
    }
  }

  // Grows `slot` to at least `want` elements when it is currently shorter.
  void genGrowTo(int slot, int64_t want) {
    emit(Op::LoadLocal, slot);
    emit(Op::Len);
    emit(Op::PushInt, want);
    emit(Op::LtInt);  // len < want
    int jz = here();
    emit(Op::Jz, -1);
    emit(Op::LoadLocal, slot);  // the array
    emit(Op::PushInt, want);    // the wanted length
    emit(Op::LoadLocal, slot);
    emit(Op::Len);
    emit(Op::SubInt);  // want - len
    emit(Op::Grow);
    patch(jz, here());
  }

  // `a = {...}`: grow to the literal's length, write it, zero the rest.
  void genWholeAssign(int slot, const Expr& lit, Type elem, long long cap,
                      bool roll) {
    int64_t k = (int64_t)lit.args.size();
    if (roll && cap >= 0 && k > cap) {  // cannot grow that far: wrap instead
      genRollWrite(slot, k, lit, elem);
      return;
    }
    if (k > 0) genGrowTo(slot, k);
    for (int64_t i = 0; i < k; ++i) {
      const Expr& e = *lit.args[(size_t)i];
      if (e.kind == Expr::Kind::NullLit) continue;
      emit(Op::LoadLocal, slot);
      emit(Op::PushInt, i);
      genExpr(e);
      emit(Op::SetIndex);
    }
    int iSlot = nextSlot++;
    emit(Op::PushInt, k);
    emit(Op::StoreLocal, iSlot);
    int loop = here();
    emit(Op::LoadLocal, iSlot);
    emit(Op::LoadLocal, slot);
    emit(Op::Len);
    emit(Op::LtInt);
    int jz = here();
    emit(Op::Jz, -1);
    emit(Op::LoadLocal, slot);
    emit(Op::LoadLocal, iSlot);
    genDefault(elem);
    emit(Op::SetIndex);
    emit(Op::LoadLocal, iSlot);
    emit(Op::PushInt, 1);
    emit(Op::AddInt);
    emit(Op::StoreLocal, iSlot);
    emit(Op::Jmp, loop);
    patch(jz, here());
  }

  // `a[X] = {...}`: grow to X, then write the first X positions; positions the
  // literal does not reach become zero, and `\null` leaves its slot alone.
  void genPrefixWrite(int slot, int64_t x, const Expr& lit, Type elem,
                      long long cap, bool roll) {
    int64_t k = (int64_t)lit.args.size();
    if (roll && cap >= 0 && x > cap) {  // cannot grow that far: wrap instead
      genRollWrite(slot, x, lit, elem);
      return;
    }
    if (x > 0) genGrowTo(slot, x);
    for (int64_t i = 0; i < x; ++i) {
      emit(Op::LoadLocal, slot);
      emit(Op::PushInt, i);
      if (i < k) {
        const Expr& e = *lit.args[(size_t)i];
        if (e.kind == Expr::Kind::NullLit) {
          emit(Op::Pop);  // discard the index
          emit(Op::Pop);  // discard the array
          continue;
        }
        genExpr(e);
      } else {
        genDefault(elem);
      }
      emit(Op::SetIndex);
    }
  }

  void genStmt(const Stmt& s) {
    switch (s.kind) {
      case Stmt::Kind::Decl:
        for (const DeclItem& d : s.decls) {
          int slot = declare(d.name);
          if (d.size) {
            if (d.init) {
              genArrayInit(d, slot);
              continue;
            }
            genExpr(*d.size);
            if (d.growable) {
              if (d.maxSize) genExpr(*d.maxSize);
              else emit(Op::PushInt, -1);
            } else {
              emit(Op::Dup);  // fixed length: maximum == length
            }
            emit(Op::NewArray, elemTag(*d.type.elem));
          } else if (d.init) {
            genExpr(*d.init);
          } else {
            genDefault(d.type);
          }
          emit(Op::StoreLocal, slot);
        }
        break;

      case Stmt::Kind::Assign: {
        const Expr& tgt = *s.target;
        if (s.expr->kind == Expr::Kind::ArrayLit) {
          if (tgt.kind == Expr::Kind::Var) {
            genWholeAssign(lookup(tgt.name), *s.expr, *tgt.type.elem,
                           tgt.arrCap, s.roll);
          } else if (tgt.kind == Expr::Kind::Index) {
            int slot = lookup(unwrapMention(tgt.lhs.get())->name);
            genPrefixWrite(slot, tgt.rhs->intVal, *s.expr,
                           *tgt.lhs->type.elem, tgt.lhs->arrCap, s.roll);
          } else {
            throw CodegenError("internal: bad array assignment target");
          }
          break;
        }
        if (tgt.kind == Expr::Kind::Var) {
          genExpr(*s.expr);
          emit(Op::StoreLocal, lookup(tgt.name));
        } else if (tgt.kind == Expr::Kind::Member) {
          genExpr(*tgt.lhs);
          genExpr(*s.expr);
          emit(Op::SetField, fieldIndex(tgt.lhs->type, tgt.name));
        } else if (tgt.kind == Expr::Kind::Index) {
          genExpr(*tgt.lhs);
          genExpr(*tgt.rhs);
          genExpr(*s.expr);
          emit(Op::SetIndex);
        } else {
          throw CodegenError("internal: bad assignment target");
        }
        break;
      }

      case Stmt::Kind::Expr:
        genExpr(*s.expr);
        if (s.expr->type != TypeKind::Void) emit(Op::Pop);
        break;

      case Stmt::Kind::Return:
        if (s.expr) genExpr(*s.expr);
        else emit(Op::PushUnit);
        emit(Op::Ret);
        break;

      case Stmt::Kind::If: {
        genExpr(*s.expr);
        int jz = here();
        emit(Op::Jz, -1);
        genStmt(*s.body);
        if (s.elseBody) {
          int jmp = here();
          emit(Op::Jmp, -1);
          patch(jz, here());
          genStmt(*s.elseBody);
          patch(jmp, here());
        } else {
          patch(jz, here());
        }
        break;
      }

      case Stmt::Kind::While: {
        int loopStart = here();
        genExpr(*s.expr);
        int jz = here();
        emit(Op::Jz, -1);
        genStmt(*s.body);
        emit(Op::Jmp, loopStart);
        patch(jz, here());
        break;
      }

      case Stmt::Kind::Go: {
        scopes.emplace_back();
        int exSlot = declare("ex");
        int endSlot = nextSlot++;
        genExpr(*s.from);
        emit(Op::StoreLocal, exSlot);
        genExpr(*s.to);
        emit(Op::StoreLocal, endSlot);

        int loopStart = here();
        emit(Op::LoadLocal, exSlot);
        emit(Op::LoadLocal, endSlot);
        emit(Op::LeInt);
        int jzEnd = here();
        emit(Op::Jz, -1);

        int jzNext = -1;
        if (s.filter) {
          genExpr(*s.filter);
          jzNext = here();
          emit(Op::Jz, -1);
        }
        genStmt(*s.body);
        if (jzNext >= 0) patch(jzNext, here());

        emit(Op::LoadLocal, exSlot);
        emit(Op::PushInt, 1);
        emit(Op::AddInt);
        emit(Op::StoreLocal, exSlot);
        emit(Op::Jmp, loopStart);
        patch(jzEnd, here());
        scopes.pop_back();
        break;
      }

      case Stmt::Kind::For: {
        scopes.emplace_back();
        if (s.init) genStmt(*s.init);
        int loopStart = here();
        int jz = -1;
        if (s.expr) {
          genExpr(*s.expr);
          jz = here();
          emit(Op::Jz, -1);
        }
        genStmt(*s.body);
        if (s.update) genStmt(*s.update);
        emit(Op::Jmp, loopStart);
        if (s.expr) patch(jz, here());
        scopes.pop_back();
        break;
      }

      case Stmt::Kind::ForEach: {
        scopes.emplace_back();
        if (s.isRange) {
          int v = declare(s.varName);
          int endSlot = nextSlot++;
          genExpr(*s.from);
          emit(Op::StoreLocal, v);
          genExpr(*s.to);
          emit(Op::StoreLocal, endSlot);
          int loop = here();
          emit(Op::LoadLocal, v);
          emit(Op::LoadLocal, endSlot);
          emit(Op::LeInt);
          int jz = here();
          emit(Op::Jz, -1);
          genStmt(*s.body);
          emit(Op::LoadLocal, v);
          emit(Op::PushInt, 1);
          emit(Op::AddInt);
          emit(Op::StoreLocal, v);
          emit(Op::Jmp, loop);
          patch(jz, here());
        } else {
          int arrSlot = nextSlot++;
          genExpr(*s.expr);
          emit(Op::StoreLocal, arrSlot);
          int idxSlot = nextSlot++;
          emit(Op::PushInt, 0);
          emit(Op::StoreLocal, idxSlot);
          int v = declare(s.varName);
          int loop = here();
          emit(Op::LoadLocal, idxSlot);
          emit(Op::LoadLocal, arrSlot);
          emit(Op::Len);
          emit(Op::LtInt);
          int jz = here();
          emit(Op::Jz, -1);
          emit(Op::LoadLocal, arrSlot);
          emit(Op::LoadLocal, idxSlot);
          emit(Op::GetIndex);
          emit(Op::StoreLocal, v);
          genStmt(*s.body);
          emit(Op::LoadLocal, idxSlot);
          emit(Op::PushInt, 1);
          emit(Op::AddInt);
          emit(Op::StoreLocal, idxSlot);
          emit(Op::Jmp, loop);
          patch(jz, here());
        }
        scopes.pop_back();
        break;
      }

      case Stmt::Kind::Block: {
        scopes.emplace_back();
        for (const auto& st : s.stmts) genStmt(*st);
        scopes.pop_back();
        break;
      }
    }
  }

  void genExpr(const Expr& e) {
    switch (e.kind) {
      case Expr::Kind::IntLit: emit(Op::PushInt, e.intVal); break;
      case Expr::Kind::BoolLit:
        emit(e.boolVal ? Op::PushTrue : Op::PushFalse);
        break;
      case Expr::Kind::StrLit: emit(Op::PushStr, internString(e.strVal)); break;
      case Expr::Kind::ArrayLit:
        throw CodegenError(
            "internal: array literal reached codegen outside a declaration");
      case Expr::Kind::NullLit:
        throw CodegenError("internal: \\null reached codegen outside a literal");
      case Expr::Kind::Var: emit(Op::LoadLocal, lookup(e.name)); break;
      case Expr::Kind::Unary:
        genExpr(*e.lhs);
        emit(e.op == "-" ? Op::NegInt : Op::NotBool);
        break;
      case Expr::Kind::Binary: genBinary(e); break;
      case Expr::Kind::Call: genCall(e); break;
      case Expr::Kind::Mention: genExpr(*e.lhs); break;
      case Expr::Kind::Member:
        genExpr(*e.lhs);
        emit(Op::GetField, fieldIndex(e.lhs->type, e.name));
        break;
      case Expr::Kind::Index:
        genExpr(*e.lhs);
        genExpr(*e.rhs);
        emit(Op::GetIndex);
        break;
      case Expr::Kind::MethodCall: {
        const FuncDecl* f = e.resolved;
        if (!f) throw CodegenError("internal: unresolved method call");
        genExpr(*e.lhs);  // receiver becomes the `self` argument
        for (const auto& a : e.args) genExpr(*a);
        emit(Op::Call, f->bcIndex);
        break;
      }
    }
  }

  void genBinary(const Expr& e) {
    if (e.op == "&&") {
      genExpr(*e.lhs);
      int jzFalse = here();
      emit(Op::Jz, -1);
      genExpr(*e.rhs);
      int end = here();
      emit(Op::Jmp, -1);
      patch(jzFalse, here());
      emit(Op::PushFalse);
      patch(end, here());
      return;
    }
    if (e.op == "||") {
      genExpr(*e.lhs);
      int jzRhs = here();
      emit(Op::Jz, -1);
      emit(Op::PushTrue);
      int end = here();
      emit(Op::Jmp, -1);
      patch(jzRhs, here());
      genExpr(*e.rhs);
      patch(end, here());
      return;
    }

    genExpr(*e.lhs);
    genExpr(*e.rhs);

    Op op = Op::Halt;
    if (e.op == "+") op = (e.type == TypeKind::Str) ? Op::AddStr : Op::AddInt;
    else if (e.op == "-") op = Op::SubInt;
    else if (e.op == "*") op = Op::MulInt;
    else if (e.op == "/") op = Op::DivInt;
    else if (e.op == "%") op = Op::ModInt;
    else if (e.op == "<") op = Op::LtInt;
    else if (e.op == "<=") op = Op::LeInt;
    else if (e.op == ">") op = Op::GtInt;
    else if (e.op == ">=") op = Op::GeInt;
    else if (e.op == "==" || e.op == "!=") {
      bool eq = (e.op == "==");
      switch (e.lhs->type.kind) {
        case TypeKind::Int: op = eq ? Op::EqInt : Op::NeInt; break;
        case TypeKind::Bool: op = eq ? Op::EqBool : Op::NeBool; break;
        case TypeKind::Str: op = eq ? Op::EqStr : Op::NeStr; break;
        default: throw CodegenError("internal: bad equality operand type");
      }
    } else {
      throw CodegenError("internal: unknown operator '" + e.op + "'");
    }
    emit(op);
  }

  void genCall(const Expr& e) {
    const FuncDecl* f = e.resolved;
    if (!f) throw CodegenError("internal: unresolved call '" + e.name + "'");

    if (f->isNative) {
      switch (f->nativeId) {
        case NativePrint:
          genExpr(*e.args[0]);
          emit(Op::Print);
          return;
        case NativeReadInt:
          emit(Op::ReadInt, lookup(unwrapMention(e.args[0].get())->name));
          return;
        case NativeReadStr:
          emit(Op::ReadStr, lookup(unwrapMention(e.args[0].get())->name));
          return;
        case NativeReadBool:
          emit(Op::ReadBool, lookup(unwrapMention(e.args[0].get())->name));
          return;
        case NativeReadIntSep: {
          const Expr* a = unwrapMention(e.args[0].get());
          const Expr* b = unwrapMention(e.args[2].get());
          emit(Op::ReadInt, lookup(a->name));
          genExpr(*e.args[1]);  // the separator string
          emit(Op::SkipSep);
          emit(Op::ReadInt, lookup(b->name));
          return;
        }
        case NativeLen:
          genExpr(*e.args[0]);
          emit(Op::Len);
          return;
        case NativeGrow:
          genExpr(*e.args[0]);
          genExpr(*e.args[1]);
          emit(Op::Grow);
          return;
        case NativeShrink:
          genExpr(*e.args[0]);
          genExpr(*e.args[1]);
          emit(Op::Shrink);
          return;
        case NativeAskInt:
        case NativeAskStr:
        case NativeAskBool: {
          if (e.args.size() == 2) {
            genExpr(*e.args[0]);  // the prompt
            emit(Op::PrintInline);
          }
          const Expr* target = unwrapMention(e.args.back().get());
          Op readOp = f->nativeId == NativeAskInt    ? Op::ReadInt
                      : f->nativeId == NativeAskStr ? Op::ReadStr
                                                    : Op::ReadBool;
          emit(readOp, lookup(target->name));
          return;
        }
      }
      throw CodegenError("internal: unknown native capability");
    }

    for (const auto& a : e.args) genExpr(*a);
    emit(Op::Call, f->bcIndex);
  }
};

}  // namespace

BytecodeProgram generate(LoadedProgram& prog) {
  BytecodeProgram bc;
  bc.functions.push_back(BytecodeFn{"_start", 0, 0, {}});

  std::unordered_map<std::string, int> structIndexByName;
  for (size_t i = 0; i < prog.structs.size(); ++i)
    structIndexByName[prog.structs[i]->name] = (int)i;

  for (const auto& sd : prog.structs) {
    StructInfo si;
    si.name = sd->name;
    for (const StructField& f : sd->fields) {
      FieldLayout fl;
      switch (f.type.kind) {
        case TypeKind::Bool: fl.kind = 1; break;
        case TypeKind::Str: fl.kind = 2; break;
        case TypeKind::Struct: {
          fl.kind = 3;
          auto it = structIndexByName.find(f.type.sname);
          if (it == structIndexByName.end())
            throw CodegenError("internal: unknown struct '" + f.type.sname +
                               "' in a field");
          fl.extra = it->second;
          break;
        }
        case TypeKind::Array: {
          fl.kind = 4;
          TypeKind ek = f.type.elem ? f.type.elem->kind : TypeKind::Int;
          fl.extra = ek == TypeKind::Bool ? 1 : ek == TypeKind::Str ? 2 : 0;
          break;
        }
        default: fl.kind = 0; break;  // int
      }
      si.fields.push_back(fl);
    }
    bc.structs.push_back(std::move(si));
  }

  for (const FuncDecl* raw : prog.bodies) {
    FuncDecl* f = const_cast<FuncDecl*>(raw);
    f->bcIndex = (int)bc.functions.size();
    BytecodeFn fn;
    fn.name = f->module.empty() ? f->name : f->module + "::" + f->name;
    fn.num_params = (int)f->params.size();
    bc.functions.push_back(std::move(fn));
  }
  bc.mainIndex = prog.mainFn->bcIndex;

  CodeGen cg(bc, prog);
  for (const FuncDecl* f : prog.bodies) cg.genFunction(*f);

  BytecodeFn& entry = bc.functions[0];
  entry.code.push_back({Op::Call, bc.mainIndex});
  entry.code.push_back({Op::Halt, 0});
  return bc;
}
