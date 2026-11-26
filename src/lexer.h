#pragma once
#include "token.h"
#include <stdexcept>
#include <string>
#include <vector>

struct LexError : std::runtime_error {
  int line, col;
  LexError(const std::string& msg, int line, int col);
};

std::vector<Token> lex(const std::string& src);
void dumpTokens(const std::vector<Token>& toks);
