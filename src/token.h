#pragma once
#include <string>

enum class Tok {
  Eof, IntLit, StrLit, Ident,
  KwPick, KwHead, KwAs, KwCap, KwMut, KwArgs, KwFn, KwStruct, KwArray,
  KwConst,
  KwIf, KwElse, KwWhile, KwFor, KwWhen, KwIn, KwReturn, KwTrue, KwFalse,
  KwIntType, KwBoolType, KwStringType, KwVoidType,
  KwNull,
  At,
  LParen, RParen, LBrace, RBrace, Comma, Semi, Colon, Dot,
  Assign, Eq, Ne, Lt, Le, Gt, Ge, AmpAmp, PipePipe, Bang, Tilde,
  LBracket, RBracket,
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
