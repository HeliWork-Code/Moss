#include "lexer.h"
#include <cctype>
#include <cstdio>

const char* tokName(Tok k) {
  switch (k) {
    case Tok::Eof: return "end of file";
    case Tok::IntLit: return "integer literal";
    case Tok::StrLit: return "string literal";
    case Tok::Ident: return "identifier";
    case Tok::KwPick: return "pick";
    case Tok::KwHead: return "head";
    case Tok::KwAs: return "as";
    case Tok::KwCap: return "cap";
    case Tok::KwMut: return "mut";
    case Tok::KwArgs: return "args";
    case Tok::KwFn: return "fn";
    case Tok::KwStruct: return "struct";
    case Tok::KwArray: return "array";
    case Tok::KwConst: return "const";
    case Tok::KwIf: return "if";
    case Tok::KwElse: return "else";
    case Tok::KwWhile: return "while";
    case Tok::KwFor: return "for";
    case Tok::KwWhen: return "when";
    case Tok::KwIn: return "in";
    case Tok::KwReturn: return "return";
    case Tok::KwTrue: return "true";
    case Tok::KwFalse: return "false";
    case Tok::KwIntType: return "int";
    case Tok::KwBoolType: return "bool";
    case Tok::KwStringType: return "string";
    case Tok::KwVoidType: return "void";
    case Tok::At: return "@";
    case Tok::LParen: return "(";
    case Tok::RParen: return ")";
    case Tok::LBrace: return "{";
    case Tok::RBrace: return "}";
    case Tok::Comma: return ",";
    case Tok::Semi: return ";";
    case Tok::Colon: return ":";
    case Tok::Dot: return ".";
    case Tok::Assign: return "=";
    case Tok::Eq: return "==";
    case Tok::Ne: return "!=";
    case Tok::Lt: return "<";
    case Tok::Le: return "<=";
    case Tok::Gt: return ">";
    case Tok::Ge: return ">=";
    case Tok::AmpAmp: return "&&";
    case Tok::PipePipe: return "||";
    case Tok::Bang: return "!";
    case Tok::Tilde: return "~";
    case Tok::LBracket: return "[";
    case Tok::RBracket: return "]";
    case Tok::Plus: return "+";
    case Tok::Minus: return "-";
    case Tok::Star: return "*";
    case Tok::Slash: return "/";
    case Tok::Percent: return "%";
  }
  return "?";
}

LexError::LexError(const std::string& msg, int line, int col)
    : std::runtime_error("line " + std::to_string(line) + ":" +
                         std::to_string(col) + ": " + msg),
      line(line), col(col) {}

namespace {

struct Lexer {
  const std::string& src;
  size_t i = 0;
  int line = 1, col = 1;

  explicit Lexer(const std::string& s) : src(s) {}

  bool eof() const { return i >= src.size(); }
  char peek(int n = 0) const { return i + n < src.size() ? src[i + n] : '\0'; }
  void adv() {
    if (src[i] == '\n') { ++line; col = 1; } else { ++col; }
    ++i;
  }

  void skipSpaceAndComments() {
    for (;;) {
      while (!eof() && std::isspace((unsigned char)peek())) adv();
      if (peek() == '/' && peek(1) == '/') {
        while (!eof() && peek() != '\n') adv();
        continue;
      }
      if (peek() == '/' && peek(1) == '*') {
        int l = line, c = col;
        adv(); adv();
        while (!eof() && !(peek() == '*' && peek(1) == '/')) adv();
        if (eof()) throw LexError("unterminated block comment", l, c);
        adv(); adv();
        continue;
      }
      break;
    }
  }

  Token make(Tok k, int l, int c) {
    Token t;
    t.kind = k;
    t.line = l;
    t.col = c;
    t.text = tokName(k);
    return t;
  }

  Token lexString() {
    int l = line, c = col;
    adv();
    std::string out;
    for (;;) {
      if (eof() || peek() == '\n')
        throw LexError("unterminated string literal", l, c);
      char ch = peek();
      if (ch == '"') { adv(); break; }
      if (ch == '\\') {
        adv();
        char e = peek();
        switch (e) {
          case 'n': out += '\n'; break;
          case 't': out += '\t'; break;
          case '\\': out += '\\'; break;
          case '"': out += '"'; break;
          case '0': out += '\0'; break;
          default:
            throw LexError(std::string("unknown escape '\\") + e + "'", line,
                           col);
        }
        adv();
        continue;
      }
      out += ch;
      adv();
    }
    Token t = make(Tok::StrLit, l, c);
    t.text = out;
    return t;
  }

  std::vector<Token> run() {
    std::vector<Token> toks;
    for (;;) {
      skipSpaceAndComments();
      int l = line, c = col;
      if (eof()) {
        Token t = make(Tok::Eof, l, c);
        t.text.clear();
        toks.push_back(t);
        return toks;
      }

      char ch = peek();
      if (std::isdigit((unsigned char)ch)) {
        long long v = 0;
        while (!eof() && std::isdigit((unsigned char)peek())) {
          v = v * 10 + (peek() - '0');
          adv();
        }
        Token t = make(Tok::IntLit, l, c);
        t.intVal = v;
        toks.push_back(t);
        continue;
      }

      if (ch == '"') {
        toks.push_back(lexString());
        continue;
      }

      if (std::isalpha((unsigned char)ch) || ch == '_') {
        std::string id;
        while (!eof() && (std::isalnum((unsigned char)peek()) || peek() == '_')) {
          id += peek();
          adv();
        }
        Tok k = Tok::Ident;
        if (id == "pick") k = Tok::KwPick;
        else if (id == "head") k = Tok::KwHead;
        else if (id == "as") k = Tok::KwAs;
        else if (id == "cap") k = Tok::KwCap;
        else if (id == "mut") k = Tok::KwMut;
        else if (id == "args") k = Tok::KwArgs;
        else if (id == "fn") k = Tok::KwFn;
        else if (id == "struct") k = Tok::KwStruct;
        else if (id == "array") k = Tok::KwArray;
        else if (id == "const") k = Tok::KwConst;
        else if (id == "if") k = Tok::KwIf;
        else if (id == "else") k = Tok::KwElse;
        else if (id == "while") k = Tok::KwWhile;
        else if (id == "for") k = Tok::KwFor;
        else if (id == "when") k = Tok::KwWhen;
        else if (id == "in") k = Tok::KwIn;
        else if (id == "return") k = Tok::KwReturn;
        else if (id == "true") k = Tok::KwTrue;
        else if (id == "false") k = Tok::KwFalse;
        else if (id == "int") k = Tok::KwIntType;
        else if (id == "bool") k = Tok::KwBoolType;
        else if (id == "string") k = Tok::KwStringType;
        else if (id == "void") k = Tok::KwVoidType;

        Token t = make(k, l, c);
        if (k == Tok::Ident) t.text = id;
        else if (k == Tok::KwTrue) t.boolVal = true;
        toks.push_back(t);
        continue;
      }

      auto two = [&](char a, char b, Tok k) -> bool {
        if (peek() == a && peek(1) == b) {
          adv(); adv();
          toks.push_back(make(k, l, c));
          return true;
        }
        return false;
      };
      if (two('=', '=', Tok::Eq)) continue;
      if (two('!', '=', Tok::Ne)) continue;
      if (two('<', '=', Tok::Le)) continue;
      if (two('>', '=', Tok::Ge)) continue;
      if (two('&', '&', Tok::AmpAmp)) continue;
      if (two('|', '|', Tok::PipePipe)) continue;

      adv();
      switch (ch) {
        case '(': toks.push_back(make(Tok::LParen, l, c)); break;
        case ')': toks.push_back(make(Tok::RParen, l, c)); break;
        case '{': toks.push_back(make(Tok::LBrace, l, c)); break;
        case '}': toks.push_back(make(Tok::RBrace, l, c)); break;
        case ',': toks.push_back(make(Tok::Comma, l, c)); break;
        case ';': toks.push_back(make(Tok::Semi, l, c)); break;
        case ':': toks.push_back(make(Tok::Colon, l, c)); break;
        case '.': toks.push_back(make(Tok::Dot, l, c)); break;
        case '@': toks.push_back(make(Tok::At, l, c)); break;
        case '+': toks.push_back(make(Tok::Plus, l, c)); break;
        case '-': toks.push_back(make(Tok::Minus, l, c)); break;
        case '*': toks.push_back(make(Tok::Star, l, c)); break;
        case '/': toks.push_back(make(Tok::Slash, l, c)); break;
        case '%': toks.push_back(make(Tok::Percent, l, c)); break;
        case '=': toks.push_back(make(Tok::Assign, l, c)); break;
        case '<': toks.push_back(make(Tok::Lt, l, c)); break;
        case '>': toks.push_back(make(Tok::Gt, l, c)); break;
        case '!': toks.push_back(make(Tok::Bang, l, c)); break;
        case '~': toks.push_back(make(Tok::Tilde, l, c)); break;
        case '[': toks.push_back(make(Tok::LBracket, l, c)); break;
        case ']': toks.push_back(make(Tok::RBracket, l, c)); break;
        default:
          throw LexError(std::string("unexpected character '") + ch + "'", l, c);
      }
    }
  }
};

}  // namespace

std::vector<Token> lex(const std::string& src) {
  Lexer lx(src);
  return lx.run();
}

void dumpTokens(const std::vector<Token>& toks) {
  for (const Token& t : toks) {
    std::printf("  %4d:%-3d %-20s", t.line, t.col, tokName(t.kind));
    if (t.kind == Tok::Ident) std::printf(" %s", t.text.c_str());
    else if (t.kind == Tok::IntLit) std::printf(" %lld", t.intVal);
    else if (t.kind == Tok::StrLit) std::printf(" \"%s\"", t.text.c_str());
    std::printf("\n");
  }
}
