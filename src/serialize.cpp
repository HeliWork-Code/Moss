#include "serialize.h"
#include <cstring>

namespace {

void putU32(std::string& out, uint32_t v) {
  for (int i = 0; i < 4; ++i) out.push_back((char)((v >> (8 * i)) & 0xff));
}
void putU64(std::string& out, uint64_t v) {
  for (int i = 0; i < 8; ++i) out.push_back((char)((v >> (8 * i)) & 0xff));
}
void putStr(std::string& out, const std::string& s) {
  putU32(out, (uint32_t)s.size());
  out += s;
}

struct Reader {
  const std::string& data;
  size_t pos = 0;
  explicit Reader(const std::string& d) : data(d) {}

  void need(size_t n) const {
    if (pos + n > data.size()) throw SerializeError("truncated bytecode");
  }
  uint8_t u8() {
    need(1);
    return (uint8_t)data[pos++];
  }
  uint32_t u32() {
    need(4);
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) v |= (uint32_t)(uint8_t)data[pos++] << (8 * i);
    return v;
  }
  uint64_t u64() {
    need(8);
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= (uint64_t)(uint8_t)data[pos++] << (8 * i);
    return v;
  }
  std::string str() {
    uint32_t n = u32();
    need(n);
    std::string s = data.substr(pos, n);
    pos += n;
    return s;
  }
};

constexpr uint32_t kMagic = 0x43424f4d;  // "MOBC"
constexpr uint32_t kVersion = 2;

}  // namespace

std::string serializeProgram(const BytecodeProgram& prog) {
  std::string out;
  putU32(out, kMagic);
  putU32(out, kVersion);
  putU32(out, (uint32_t)prog.mainIndex);

  putU32(out, (uint32_t)prog.strings.size());
  for (const auto& s : prog.strings) putStr(out, s);

  putU32(out, (uint32_t)prog.structs.size());
  for (const auto& st : prog.structs) {
    putStr(out, st.name);
    putU32(out, (uint32_t)st.fieldTypes.size());
    for (uint8_t t : st.fieldTypes) out.push_back((char)t);
  }

  putU32(out, (uint32_t)prog.functions.size());
  for (const auto& fn : prog.functions) {
    putStr(out, fn.name);
    putU32(out, (uint32_t)fn.num_params);
    putU32(out, (uint32_t)fn.num_locals);
    putU32(out, (uint32_t)fn.code.size());
    for (const auto& in : fn.code) {
      out.push_back((char)(uint8_t)in.op);
      putU64(out, (uint64_t)in.operand);
    }
  }
  return out;
}

BytecodeProgram deserializeProgram(const std::string& data) {
  Reader r(data);
  if (r.u32() != kMagic) throw SerializeError("not a Moss bytecode file");
  if (r.u32() != kVersion) throw SerializeError("unsupported bytecode version");

  BytecodeProgram prog;
  prog.mainIndex = (int)r.u32();

  uint32_t nStrings = r.u32();
  prog.strings.reserve(nStrings);
  for (uint32_t i = 0; i < nStrings; ++i) prog.strings.push_back(r.str());

  uint32_t nStructs = r.u32();
  prog.structs.reserve(nStructs);
  for (uint32_t i = 0; i < nStructs; ++i) {
    StructInfo st;
    st.name = r.str();
    uint32_t nf = r.u32();
    r.need(nf);
    for (uint32_t k = 0; k < nf; ++k) st.fieldTypes.push_back(r.u8());
    prog.structs.push_back(std::move(st));
  }

  uint32_t nFns = r.u32();
  prog.functions.reserve(nFns);
  for (uint32_t i = 0; i < nFns; ++i) {
    BytecodeFn fn;
    fn.name = r.str();
    fn.num_params = (int)r.u32();
    fn.num_locals = (int)r.u32();
    uint32_t nCode = r.u32();
    fn.code.resize(nCode);
    for (uint32_t k = 0; k < nCode; ++k) {
      fn.code[k].op = (Op)r.u8();
      fn.code[k].operand = (int64_t)r.u64();
    }
    prog.functions.push_back(std::move(fn));
  }
  return prog;
}
