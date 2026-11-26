#include "vm.h"
#include <cstdio>
#include <iostream>

static Value defaultArrayValue(int tag) {
  if (tag == 1) return Value::bool_(false);
  if (tag == 2) return Value::str("");
  return Value::int_(0);
}

VM::VM(const BytecodeProgram& prog, std::string mainArgs)
    : prog(prog), mainArgs(std::move(mainArgs)) {}

Value VM::pop() {
  if (stack.empty()) throw RuntimeError("stack underflow");
  Value v = std::move(stack.back());
  stack.pop_back();
  return v;
}

std::string VM::format(const Value& v) {
  switch (v.tag) {
    case Value::Tag::Int: return std::to_string(v.i);
    case Value::Tag::Bool: return v.b ? "true" : "false";
    case Value::Tag::Str: return v.s;
    case Value::Tag::Unit: return "()";
    case Value::Tag::Obj: return "<object>";
    case Value::Tag::Arr: return "<array>";
  }
  return "?";
}

int VM::run() {
  stack.clear();
  heap.clear();
  arrays.clear();
  frames.clear();
  steps = 0;
  frames.push_back(Frame{0, 0, std::vector<Value>()});

  for (;;) {
    if (++steps > stepLimit)
      throw RuntimeError("step limit exceeded (possible infinite loop)");

    Frame& fr = frames.back();
    const BytecodeFn& fn = prog.functions[(size_t)fr.fn];
    if (fr.pc < 0 || (size_t)fr.pc >= fn.code.size())
      throw RuntimeError("pc out of range in '" + fn.name + "'");

    const Instr in = fn.code[(size_t)fr.pc++];

    switch (in.op) {
      case Op::PushInt: push(Value::int_(in.operand)); break;
      case Op::PushTrue: push(Value::bool_(true)); break;
      case Op::PushFalse: push(Value::bool_(false)); break;
      case Op::PushUnit: push(Value::unit()); break;
      case Op::PushStr:
        if (in.operand < 0 || (size_t)in.operand >= prog.strings.size())
          throw RuntimeError("string constant index out of range");
        push(Value::str(prog.strings[(size_t)in.operand]));
        break;

      case Op::LoadLocal:
        if (in.operand < 0 || (size_t)in.operand >= fr.locals.size())
          throw RuntimeError("bad local slot in '" + fn.name + "'");
        push(fr.locals[(size_t)in.operand]);
        break;

      case Op::StoreLocal:
        if (in.operand < 0 || (size_t)in.operand >= fr.locals.size())
          throw RuntimeError("bad local slot in '" + fn.name + "'");
        fr.locals[(size_t)in.operand] = pop();
        break;

      case Op::Dup: {
        Value v = pop();
        push(v);
        push(v);
        break;
      }
      case Op::Pop: pop(); break;

      case Op::AddInt: { auto b = pop().i, a = pop().i; push(Value::int_(a + b)); break; }
      case Op::SubInt: { auto b = pop().i, a = pop().i; push(Value::int_(a - b)); break; }
      case Op::MulInt: { auto b = pop().i, a = pop().i; push(Value::int_(a * b)); break; }
      case Op::DivInt: {
        auto b = pop().i, a = pop().i;
        if (b == 0) throw RuntimeError("division by zero");
        push(Value::int_(a / b));
        break;
      }
      case Op::ModInt: {
        auto b = pop().i, a = pop().i;
        if (b == 0) throw RuntimeError("division by zero");
        push(Value::int_(a % b));
        break;
      }
      case Op::NegInt: push(Value::int_(-pop().i)); break;
      case Op::AddStr: {
        Value b = pop(), a = pop();
        push(Value::str(a.s + b.s));
        break;
      }

      case Op::EqInt: { auto b = pop().i, a = pop().i; push(Value::bool_(a == b)); break; }
      case Op::NeInt: { auto b = pop().i, a = pop().i; push(Value::bool_(a != b)); break; }
      case Op::LtInt: { auto b = pop().i, a = pop().i; push(Value::bool_(a < b)); break; }
      case Op::LeInt: { auto b = pop().i, a = pop().i; push(Value::bool_(a <= b)); break; }
      case Op::GtInt: { auto b = pop().i, a = pop().i; push(Value::bool_(a > b)); break; }
      case Op::GeInt: { auto b = pop().i, a = pop().i; push(Value::bool_(a >= b)); break; }
      case Op::EqBool: { auto b = pop().b, a = pop().b; push(Value::bool_(a == b)); break; }
      case Op::NeBool: { auto b = pop().b, a = pop().b; push(Value::bool_(a != b)); break; }
      case Op::NotBool: push(Value::bool_(!pop().b)); break;
      case Op::EqStr: { Value b = pop(), a = pop(); push(Value::bool_(a.s == b.s)); break; }
      case Op::NeStr: { Value b = pop(), a = pop(); push(Value::bool_(a.s != b.s)); break; }

      case Op::Jmp: fr.pc = (int)in.operand; break;
      case Op::Jz: {
        Value v = pop();
        if (v.tag != Value::Tag::Bool)
          throw RuntimeError("internal: jz on a non-bool value");
        if (!v.b) fr.pc = (int)in.operand;
        break;
      }

      case Op::Call: {
        if (in.operand < 0 || (size_t)in.operand >= prog.functions.size())
          throw RuntimeError("call to bad function index");
        const BytecodeFn& callee = prog.functions[(size_t)in.operand];
        std::vector<Value> locals((size_t)callee.num_locals);
        // main's string parameter is supplied by the runtime, not by the
        // caller, so there is nothing on the stack to pop for it.
        bool injectArgs =
            (int)in.operand == prog.mainIndex && callee.num_params == 1;
        int toPop = injectArgs ? 0 : callee.num_params;
        for (int k = toPop - 1; k >= 0; --k)
          locals[(size_t)k] = pop();
        if (injectArgs) locals[0] = Value::str(mainArgs);
        frames.push_back(Frame{(int)in.operand, 0, std::move(locals)});
        break;
      }

      case Op::Ret: {
        Value v = pop();
        frames.pop_back();
        if (frames.empty()) return 0;
        push(v);
        break;
      }

      case Op::Print:
        std::printf("%s\n", format(pop()).c_str());
        break;

      case Op::PrintInline:
        std::printf("%s", format(pop()).c_str());
        std::fflush(stdout);
        break;

      case Op::ReadInt: {
        int64_t v = 0;
        if (!(std::cin >> v)) throw RuntimeError("failed to read an int");
        if (in.operand < 0 || (size_t)in.operand >= fr.locals.size())
          throw RuntimeError("bad local slot for read");
        fr.locals[(size_t)in.operand] = Value::int_(v);
        break;
      }
      case Op::ReadStr: {
        std::string s;
        if (!(std::cin >> s)) throw RuntimeError("failed to read a string");
        if (in.operand < 0 || (size_t)in.operand >= fr.locals.size())
          throw RuntimeError("bad local slot for read");
        fr.locals[(size_t)in.operand] = Value::str(s);
        break;
      }
      case Op::ReadBool: {
        std::string s;
        if (!(std::cin >> s)) throw RuntimeError("failed to read a bool");
        bool b;
        if (s == "true" || s == "1") b = true;
        else if (s == "false" || s == "0") b = false;
        else throw RuntimeError("expected true/false, got '" + s + "'");
        if (in.operand < 0 || (size_t)in.operand >= fr.locals.size())
          throw RuntimeError("bad local slot for read");
        fr.locals[(size_t)in.operand] = Value::bool_(b);
        break;
      }

      case Op::SkipSep: {
        Value v = pop();
        if (v.tag != Value::Tag::Str)
          throw RuntimeError("separator must be a string");
        const std::string& sep = v.s;
        size_t matched = 0;
        while (matched < sep.size()) {
          int c = std::cin.get();
          if (c == EOF)
            throw RuntimeError("input ended while looking for \"" + sep + "\"");
          if ((char)c == sep[matched]) {
            ++matched;
          } else {
            matched = (sep.size() > 1 && (char)c == sep[0]) ? 1 : 0;
          }
        }
        break;
      }

      case Op::NewObj: {
        if (in.operand < 0 || (size_t)in.operand >= prog.structs.size())
          throw RuntimeError("bad struct index");
        const StructInfo& si = prog.structs[(size_t)in.operand];
        std::vector<Value> fields;
        fields.reserve(si.fieldTypes.size());
        for (uint8_t t : si.fieldTypes) {
          if (t == 1) fields.push_back(Value::bool_(false));
          else if (t == 2) fields.push_back(Value::str(""));
          else fields.push_back(Value::int_(0));
        }
        heap.push_back(std::move(fields));
        push(Value::obj((int64_t)heap.size() - 1));
        break;
      }

      case Op::GetField: {
        Value o = pop();
        if (o.tag != Value::Tag::Obj)
          throw RuntimeError("internal: field access on a non-object");
        if (o.i < 0 || (size_t)o.i >= heap.size())
          throw RuntimeError("bad object id");
        auto& fields = heap[(size_t)o.i];
        if (in.operand < 0 || (size_t)in.operand >= fields.size())
          throw RuntimeError("bad field index");
        push(fields[(size_t)in.operand]);
        break;
      }

      case Op::SetField: {
        Value v = pop();
        Value o = pop();
        if (o.tag != Value::Tag::Obj)
          throw RuntimeError("internal: field assignment on a non-object");
        if (o.i < 0 || (size_t)o.i >= heap.size())
          throw RuntimeError("bad object id");
        auto& fields = heap[(size_t)o.i];
        if (in.operand < 0 || (size_t)in.operand >= fields.size())
          throw RuntimeError("bad field index");
        fields[(size_t)in.operand] = std::move(v);
        break;
      }

      case Op::NewArray: {
        int64_t maxLen = pop().i;
        int64_t n = pop().i;
        if (n < 0) throw RuntimeError("negative array length");
        if (maxLen >= 0 && n > maxLen)
          throw RuntimeError("initial array length exceeds capacity");
        ArrObj a;
        a.maxLen = maxLen;
        a.elemTag = (int)in.operand;
        a.elems.assign((size_t)n, defaultArrayValue(a.elemTag));
        arrays.push_back(std::move(a));
        push(Value::arr((int64_t)arrays.size() - 1));
        break;
      }

      case Op::GetIndex: {
        int64_t idx = pop().i;
        Value v = pop();
        if (v.tag != Value::Tag::Arr)
          throw RuntimeError("internal: index on a non-array");
        ArrObj& a = arrays[(size_t)v.i];
        if (idx < 0 || (size_t)idx >= a.elems.size())
          throw RuntimeError("array index " + std::to_string(idx) +
                             " out of range (length " +
                             std::to_string(a.elems.size()) + ")");
        push(a.elems[(size_t)idx]);
        break;
      }

      case Op::SetIndex: {
        Value val = pop();
        int64_t idx = pop().i;
        Value v = pop();
        if (v.tag != Value::Tag::Arr)
          throw RuntimeError("internal: index assignment on a non-array");
        ArrObj& a = arrays[(size_t)v.i];
        if (idx < 0 || (size_t)idx >= a.elems.size())
          throw RuntimeError("array index " + std::to_string(idx) +
                             " out of range (length " +
                             std::to_string(a.elems.size()) + ")");
        a.elems[(size_t)idx] = std::move(val);
        break;
      }

      case Op::Len: {
        Value v = pop();
        if (v.tag != Value::Tag::Arr)
          throw RuntimeError("internal: len on a non-array");
        push(Value::int_((int64_t)arrays[(size_t)v.i].elems.size()));
        break;
      }

      case Op::Grow: {
        int64_t n = pop().i;
        Value v = pop();
        if (v.tag != Value::Tag::Arr)
          throw RuntimeError("internal: grow on a non-array");
        ArrObj& a = arrays[(size_t)v.i];
        if (n < 0) throw RuntimeError("negative array growth");
        int64_t nl = (int64_t)a.elems.size() + n;
        if (a.maxLen >= 0 && nl > a.maxLen)
          throw RuntimeError("array capacity exceeded (max " +
                             std::to_string(a.maxLen) + ")");
        a.elems.resize((size_t)nl, defaultArrayValue(a.elemTag));
        break;
      }

      case Op::Shrink: {
        int64_t n = pop().i;
        Value v = pop();
        if (v.tag != Value::Tag::Arr)
          throw RuntimeError("internal: shrink on a non-array");
        ArrObj& a = arrays[(size_t)v.i];
        if (n < 0) throw RuntimeError("negative array shrink");
        if ((int64_t)a.elems.size() < n)
          throw RuntimeError("cannot shrink below zero");
        a.elems.resize(a.elems.size() - (size_t)n);
        break;
      }

      case Op::Halt: {
        Value v = pop();
        return v.tag == Value::Tag::Int ? (int)v.i : 0;
      }
    }
  }
}
