#pragma once
#include "ast.h"
#include "token.h"
#include <stdexcept>
#include <string>
#include <vector>

struct ParseError : std::runtime_error {
  int line, col;
  ParseError(const std::string& msg, int line, int col);
};

// A `.ms` program file: optional version line, pick blocks, then `fn`s.
FileAst parseMainFile(const std::vector<Token>& toks);

// A `.msh` module file: optional version line, pick blocks, then `cap`s.
ModuleAst parseModule(const std::vector<Token>& toks, const std::string& name);
