#pragma once
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

// Runtime value. Tagged so that `print` works for any type and so later
// features (ADTs, lists) can share the same representation.
struct Value {
  enum class Tag { Int, Bool, Str, Unit, Obj, Arr };

  Tag tag = Tag::Unit;
  int64_t i = 0;
  bool b = false;
  std::string s;

  static Value int_(int64_t v) {
    Value x;
    x.tag = Tag::Int;
    x.i = v;
    return x;
  }
  static Value bool_(bool v) {
    Value x;
    x.tag = Tag::Bool;
    x.b = v;
    return x;
  }
  static Value str(std::string v) {
    Value x;
    x.tag = Tag::Str;
    x.s = std::move(v);
    return x;
  }
  static Value unit() { return Value(); }
  static Value arr(int64_t id) {
    Value x;
    x.tag = Tag::Arr;
    x.i = id;
    return x;
  }
  static Value obj(int64_t id) {
    Value x;
    x.tag = Tag::Obj;
    x.i = id;
    return x;
  }
};

enum class Op : unsigned char {
  PushInt, PushTrue, PushFalse, PushUnit, PushStr,
  LoadLocal, StoreLocal, Dup, Pop,
  AddInt, SubInt, MulInt, DivInt, ModInt, NegInt, AddStr,
  EqInt, NeInt, LtInt, LeInt, GtInt, GeInt,
  EqBool, NeBool, NotBool,
  EqStr, NeStr,
  Jmp, Jz, Call, Ret,
  Print,        // pop and print with a trailing newline
  PrintInline,  // pop and print without a newline (prompts)
  ReadInt,      // read an int from stdin into locals[operand]
  ReadStr,      // read a whitespace-delimited string into locals[operand]
  ReadBool,     // read true/false/0/1 into locals[operand]
  SkipSep,      // pop a string and consume it from stdin
  NewObj,       // push a fresh object of structs[operand]
  GetField,     // pop an object, push its field `operand`
  SetField,     // pop a value and an object, store the value in field
  NewArray,     // pop a length, push a fresh array (operand = element tag)
  GetIndex,     // pop index and array, push the element
  SetIndex,     // pop value, index and array, store the element
  Len,          // pop an array, push its length
  Grow,         // pop n and an array, extend it by n elements
  Shrink,       // pop n and an array, drop n elements
  Halt,
};

struct Instr {
  Op op;
  int64_t operand = 0;
};

struct BytecodeFn {
  std::string name;
  int num_params = 0;
  int num_locals = 0;
  std::vector<Instr> code;
};

// One struct field, as the VM needs it: what a fresh instance starts with.
struct FieldLayout {
  uint8_t kind = 0;    // 0 int, 1 bool, 2 string, 3 struct, 4 array
  int32_t extra = -1;  // kind 3: the struct index; kind 4: the element kind
};

struct StructInfo {
  std::string name;
  std::vector<FieldLayout> fields;
};

struct BytecodeProgram {
  std::vector<BytecodeFn> functions;  // functions[0] is `_start`
  std::vector<std::string> strings;   // constant pool
  std::vector<StructInfo> structs;    // object layouts
  int mainIndex = -1;
};

const char* opName(Op op);
void disassemble(const BytecodeProgram& prog, std::ostream& os);
