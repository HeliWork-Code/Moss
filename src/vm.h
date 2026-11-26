#pragma once
#include "bytecode.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

struct RuntimeError : std::runtime_error {
  explicit RuntimeError(const std::string& msg) : std::runtime_error(msg) {}
};

class VM {
 public:
  VM(const BytecodeProgram& prog, std::string mainArgs);

  // Runs the program. Returns main()'s int result, or 0 for void main.
  int run();

  void setStepLimit(long long limit) { stepLimit = limit; }

 private:
  struct Frame {
    int fn;
    int pc = 0;
    std::vector<Value> locals;
  };

  const BytecodeProgram& prog;
  std::string mainArgs;
  std::vector<Value> stack;
  std::vector<std::vector<Value>> heap;  // struct instances

  struct ArrObj {
    std::vector<Value> elems;
    int64_t maxLen = -1;  // -1 means unbounded
    int elemTag = 0;      // 0 int, 1 bool, 2 string
  };
  std::vector<ArrObj> arrays;
  std::vector<Frame> frames;
  long long steps = 0;
  long long stepLimit = 100000000;

  void push(Value v) { stack.push_back(std::move(v)); }
  Value pop();

  static std::string format(const Value& v);
};
