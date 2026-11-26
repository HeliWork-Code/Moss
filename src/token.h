#pragma once
#include <string>

enum class Tok {
  Eof, IntLit, StrLit, Ident,
  KwPick, KwHead, KwAs, KwCap, KwMut, KwArgs, KwFn, KwStruct,
  KwIf, KwElse, KwWhile, KwFor, KwWhen, KwReturn, KwTrue, KwFalse,
  KwIntType, KwBoolType, KwStringType, KwVoidType,
  At,
  LParen, RParen, LBrace, RBrace, Comma, Semi, Colon, Dot,
  Assign, Eq, Ne, Lt, Le, Gt, Ge, AmpAmp, PipePipe, Bang, Tilde,
  Plus, Minus, Star, Slash, Percent,
};

struct Token {
  Tok kind = Tok::Eof;
  std::string text;   // identifier name or string literal contents
  long long intVal = 0;
  bool boolVal = false;
  int line = 1;
  int col = 1;
};

const char* tokName(Tok k);
