#pragma once
#include "program.h"
#include <stdexcept>
#include <string>

struct TypeCheckError : std::runtime_error {
  int line;
  TypeCheckError(const std::string& msg, int line)
      : std::runtime_error("line " + std::to_string(line) + ": " + msg),
        line(line) {}
};

// Annotates every expression with its type and resolves every call to the
// FuncDecl it names. Throws TypeCheckError on the first problem found.
void typeCheck(const LoadedProgram& prog);
