#include "bytecode.h"
#include <ostream>

const char* opName(Op op) {
  switch (op) {
    case Op::PushInt: return "push_int";
    case Op::PushTrue: return "push_true";
    case Op::PushFalse: return "push_false";
    case Op::PushUnit: return "push_unit";
    case Op::PushStr: return "push_str";
    case Op::LoadLocal: return "load";
    case Op::StoreLocal: return "store";
    case Op::Dup: return "dup";
    case Op::Pop: return "pop";
    case Op::AddInt: return "add_int";
    case Op::SubInt: return "sub_int";
    case Op::MulInt: return "mul_int";
    case Op::DivInt: return "div_int";
    case Op::ModInt: return "mod_int";
    case Op::NegInt: return "neg_int";
    case Op::AddStr: return "add_str";
    case Op::EqInt: return "eq_int";
    case Op::NeInt: return "ne_int";
    case Op::LtInt: return "lt_int";
    case Op::LeInt: return "le_int";
    case Op::GtInt: return "gt_int";
    case Op::GeInt: return "ge_int";
    case Op::EqBool: return "eq_bool";
    case Op::NeBool: return "ne_bool";
    case Op::NotBool: return "not_bool";
    case Op::EqStr: return "eq_str";
    case Op::NeStr: return "ne_str";
    case Op::Jmp: return "jmp";
    case Op::Jz: return "jz";
    case Op::Call: return "call";
    case Op::Ret: return "ret";
    case Op::Print: return "print";
    case Op::PrintInline: return "print_inline";
    case Op::ReadInt: return "read_int";
    case Op::ReadStr: return "read_str";
    case Op::ReadBool: return "read_bool";
    case Op::SkipSep: return "skip_sep";
    case Op::NewObj: return "new_obj";
    case Op::GetField: return "get_field";
    case Op::SetField: return "set_field";
    case Op::NewArray: return "new_array";
    case Op::GetIndex: return "get_index";
    case Op::SetIndex: return "set_index";
    case Op::Len: return "len";
    case Op::Grow: return "grow";
    case Op::Shrink: return "shrink";
    case Op::Halt: return "halt";
  }
  return "?";
}

void disassemble(const BytecodeProgram& prog, std::ostream& os) {
  if (!prog.strings.empty()) {
    os << "--- strings ---\n";
    for (size_t i = 0; i < prog.strings.size(); ++i)
      os << "  [" << i << "] \"" << prog.strings[i] << "\"\n";
  }
  for (size_t fi = 0; fi < prog.functions.size(); ++fi) {
    const BytecodeFn& fn = prog.functions[fi];
    os << "--- fn #" << fi << " " << fn.name << " (params=" << fn.num_params
       << ", locals=" << fn.num_locals << ") ---\n";
    for (size_t pc = 0; pc < fn.code.size(); ++pc) {
      const Instr& in = fn.code[pc];
      os << "  " << pc << ":  " << opName(in.op) << " " << in.operand;
      if (in.op == Op::Call && in.operand >= 0 &&
          (size_t)in.operand < prog.functions.size())
        os << "  ; " << prog.functions[(size_t)in.operand].name;
      else if (in.op == Op::PushStr && in.operand >= 0 &&
               (size_t)in.operand < prog.strings.size())
        os << "  ; \"" << prog.strings[(size_t)in.operand] << "\"";
      os << "\n";
    }
  }
}
