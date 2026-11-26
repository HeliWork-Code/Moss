#include "parser.h"
#include <set>

ParseError::ParseError(const std::string& msg, int line, int col)
    : std::runtime_error("line " + std::to_string(line) + ":" +
                         std::to_string(col) + ": " + msg),
      line(line), col(col) {}

namespace {

class Parser {
 public:
  Parser(const std::vector<Token>& toks, bool moduleMode)
      : toks(toks), moduleMode(moduleMode) {}

  FileAst parseFile() {
    skipVersion();
    FileAst file;
    while (check(Tok::KwPick)) parsePickBlock(file.picks);
    if (moduleMode)
      throw ParseError("capability module must use 'cap', not 'fn'",
                       peek().line, peek().col);
    while (check(Tok::KwStruct)) file.structs.push_back(parseStruct());
    while (!check(Tok::Eof)) {
      if (!check(Tok::KwFn)) break;  // parseFn reports a good error
      file.functions.push_back(parseFn());
    }
    if (!check(Tok::Eof))
      throw ParseError(std::string("unexpected ") + tokName(peek().kind),
                       peek().line, peek().col);
    return file;
  }

  ModuleAst parseModuleFile(const std::string& name) {
    skipVersion();
    ModuleAst mod;
    mod.name = name;
    while (check(Tok::KwPick)) parsePickBlock(mod.picks);
    if (check(Tok::KwStruct))
      throw ParseError("structs are not supported in capability modules yet",
                       peek().line, peek().col);
    while (check(Tok::KwCap)) mod.caps.push_back(parseCap());
    if (!check(Tok::Eof))
      throw ParseError(std::string("unexpected ") + tokName(peek().kind) +
                           " (modules contain only 'pick' and 'cap')",
                       peek().line, peek().col);
    return mod;
  }

 private:
  const std::vector<Token>& toks;
  bool moduleMode = false;
  size_t pos = 0;
  std::set<std::string> imports;  // capability aliases, for `verb arg;`

  const Token& peek(int n = 0) const {
    size_t i = pos + (size_t)n;
    return i < toks.size() ? toks[i] : toks.back();
  }
  bool check(Tok k) const { return peek().kind == k; }
  bool match(Tok k) {
    if (!check(k)) return false;
    ++pos;
    return true;
  }
  const Token& advance() { return toks[pos++]; }

  const Token& expect(Tok k, const char* what) {
    if (!check(k)) {
      const Token& t = peek();
      throw ParseError(std::string("expected ") + what + " but found " +
                           tokName(t.kind),
                       t.line, t.col);
    }
    return advance();
  }

  [[noreturn]] void error(const std::string& msg) const {
    const Token& t = peek();
    throw ParseError(msg, t.line, t.col);
  }

  void skipVersion() {
    if (check(Tok::Ident) && peek().text == "moss" &&
        peek(1).kind == Tok::IntLit) {
      advance();
      advance();
      expect(Tok::Semi, "';' after version line");
    }
  }

  Type parseType() {
    const Token& t = peek();
    // `array T` names an array type; the length is given at the declaration.
    if (t.kind == Tok::KwArray) {
      advance();
      return Type::ofArray(parseType());
    }
    if (t.kind == Tok::Ident) {
      advance();
      return Type::ofStruct(t.text);
    }
    switch (t.kind) {
      case Tok::KwIntType: advance(); return TypeKind::Int;
      case Tok::KwBoolType: advance(); return TypeKind::Bool;
      case Tok::KwStringType: advance(); return TypeKind::Str;
      case Tok::KwVoidType: advance(); return TypeKind::Void;
      default:
        throw ParseError(std::string("expected a type but found ") +
                             tokName(t.kind),
                         t.line, t.col);
    }
  }

  void parsePickBlock(std::vector<PickEntry>& out) {
    expect(Tok::KwPick, "'pick'");
    expect(Tok::KwHead, "'head' after 'pick'");
    expect(Tok::LBrace, "'{'");
    while (!check(Tok::RBrace)) {
      if (check(Tok::Eof)) error("unterminated pick block");
      const Token& mod = expect(Tok::Ident, "module name");
      expect(Tok::Colon, "':' after module name");
      for (;;) {
        const Token& at = expect(Tok::At, "'@' before a capability name");
        const Token& cap = expect(Tok::Ident, "capability name");
        PickEntry e;
        e.moduleName = mod.text;
        e.capName = cap.text;
        e.alias = cap.text;
        e.line = at.line;
        if (match(Tok::KwAs)) e.alias = expect(Tok::Ident, "alias name").text;
        imports.insert(e.alias);
        out.push_back(std::move(e));
        if (!match(Tok::Comma)) break;
      }
      expect(Tok::Semi, "';' after a module's capability list");
    }
    expect(Tok::RBrace, "'}'");
  }

  std::vector<Param> parseParams(bool allowMut) {
    std::vector<Param> params;
    expect(Tok::LParen, "'('");
    if (match(Tok::KwArgs)) {
      // `args` just marks that the runtime supplies the program arguments.
    }
    if (!check(Tok::RParen)) {
      for (;;) {
        Param p;
        if (match(Tok::KwMut)) {
          if (!allowMut) error("'mut' is only allowed on capability parameters");
          p.mut = true;
        }
        p.type = parseType();
        p.name = expect(Tok::Ident, "parameter name").text;
        params.push_back(std::move(p));
        if (!match(Tok::Comma)) break;
      }
    }
    expect(Tok::RParen, "')'");
    return params;
  }

  StmtPtr parseBlock() {
    const Token& brace = expect(Tok::LBrace, "'{'");
    std::vector<StmtPtr> stmts;
    while (!check(Tok::RBrace)) {
      if (check(Tok::Eof)) error("unexpected end of file inside a block");
      stmts.push_back(parseStmt());
    }
    expect(Tok::RBrace, "'}'");
    return Stmt::block(std::move(stmts), brace.line);
  }

  std::unique_ptr<FuncDecl> parseFn() {
    const Token& start = expect(Tok::KwFn, "'fn'");
    auto fn = std::make_unique<FuncDecl>();
    fn->line = start.line;
    fn->returnType = parseType();  // return type comes before the name
    fn->name = expect(Tok::Ident, "function name").text;
    fn->params = parseParams(false);
    StmtPtr body = parseBlock();
    fn->body = std::move(body->stmts);
    fn->hasBody = true;
    fn->module = "";
    return fn;
  }

  std::unique_ptr<FuncDecl> parseCap() {
    const Token& start = expect(Tok::KwCap, "'cap'");
    auto cap = std::make_unique<FuncDecl>();
    cap->line = start.line;
    cap->name = expect(Tok::Ident, "capability name").text;
    cap->params = parseParams(true);
    expect(Tok::Colon, "':' and return type after capability parameters");
    cap->returnType = parseType();
    cap->module = "";  // filled in by the loader
    if (check(Tok::LBrace)) {
      StmtPtr body = parseBlock();
      cap->body = std::move(body->stmts);
      cap->hasBody = true;
    } else {
      expect(Tok::Semi, "';' (native declaration) or '{' (body)");
      cap->hasBody = false;
      cap->isNative = true;
    }
    return cap;
  }

  StmtPtr parseStmt() {
    const Token& t = peek();

    if (check(Tok::LBrace)) return parseBlock();

    // __go (a ~ b) [when (cond)] stmt
    if (check(Tok::Ident) && peek().text == "__go") {
      const Token& start = advance();
      expect(Tok::LParen, "'(' after __go");
      ExprPtr from = parseExpr();
      expect(Tok::Tilde, "'~' in the __go range");
      ExprPtr to = parseExpr();
      expect(Tok::RParen, "')'");
      ExprPtr filter;
      if (match(Tok::KwWhen)) {
        expect(Tok::LParen, "'(' after when");
        filter = parseExpr();
        expect(Tok::RParen, "')'");
      }
      StmtPtr body = parseStmt();
      return Stmt::goStmt(std::move(from), std::move(to), std::move(filter),
                          std::move(body), start.line);
    }

    if (check(Tok::KwArray)) {
      advance();
      Type elem = parseType();
      std::vector<DeclItem> items;
      for (;;) {
        DeclItem d;
        d.line = t.line;
        d.name = expect(Tok::Ident, "array name").text;
        d.type = Type::ofArray(elem);
        expect(Tok::LBracket, "'[' with the array length");
        if (check(Tok::Tilde)) {
          advance();
          d.size = Expr::intLit(0, t.line);
          d.growable = true;
          if (!check(Tok::RBracket)) d.maxSize = parseExpr();
        } else {
          d.size = parseExpr();
          if (match(Tok::Tilde)) {
            d.growable = true;
            if (!check(Tok::RBracket)) d.maxSize = parseExpr();
          }
        }
        expect(Tok::RBracket, "']'");
        items.push_back(std::move(d));
        if (!match(Tok::Comma)) break;
      }
      expect(Tok::Semi, "';'");
      return Stmt::declStmt(std::move(items), t.line);
    }

    if (check(Tok::KwIntType) || check(Tok::KwBoolType) ||
        check(Tok::KwStringType) ||
        (check(Tok::Ident) && peek(1).kind == Tok::Ident)) {
      Type ty = parseType();
      std::vector<DeclItem> items;
      for (;;) {
        DeclItem d;
        d.type = ty;
        d.line = t.line;
        d.name = expect(Tok::Ident, "variable name").text;
        if (match(Tok::Assign)) d.init = parseExpr();
        items.push_back(std::move(d));
        if (!match(Tok::Comma)) break;
      }
      expect(Tok::Semi, "';'");
      return Stmt::declStmt(std::move(items), t.line);
    }

    if (match(Tok::KwIf)) {
      expect(Tok::LParen, "'(' after 'if'");
      ExprPtr cond = parseExpr();
      expect(Tok::RParen, "')'");
      StmtPtr thenBody = parseStmt();
      StmtPtr elseBody;
      if (match(Tok::KwElse)) elseBody = parseStmt();
      return Stmt::ifStmt(std::move(cond), std::move(thenBody),
                          std::move(elseBody), t.line);
    }

    if (match(Tok::KwWhile)) {
      expect(Tok::LParen, "'(' after 'while'");
      ExprPtr cond = parseExpr();
      expect(Tok::RParen, "')'");
      StmtPtr body = parseStmt();
      return Stmt::whileStmt(std::move(cond), std::move(body), t.line);
    }

    if (match(Tok::KwFor)) {
      expect(Tok::LParen, "'(' after 'for'");
      StmtPtr init;
      if (!check(Tok::Semi)) init = parseSimpleStmt();
      expect(Tok::Semi, "';' after the for-initializer");
      ExprPtr cond;
      if (!check(Tok::Semi)) cond = parseExpr();
      expect(Tok::Semi, "';' after the for-condition");
      StmtPtr update;
      if (!check(Tok::RParen)) update = parseSimpleStmt();
      expect(Tok::RParen, "')'");
      StmtPtr body = parseStmt();
      return Stmt::forStmt(std::move(init), std::move(cond), std::move(update),
                           std::move(body), t.line);
    }

    if (match(Tok::KwReturn)) {
      ExprPtr value;
      if (!check(Tok::Semi)) value = parseExpr();
      expect(Tok::Semi, "';'");
      return Stmt::ret(std::move(value), t.line);
    }

    // Verb call: a picked capability name followed by its arguments.
    //   input @a;                 one mention
    //   ask "Age? " @a;           a prompt followed by a mention
    //   pair_up @x @2;            a mention list
    //   show add(a, b, c);        a single expression argument
    if (check(Tok::Ident) && imports.count(peek().text) &&
        peek(1).kind != Tok::LParen && peek(1).kind != Tok::Semi) {
      const Token& nameTok = advance();
      std::vector<ExprPtr> args;
      while (!check(Tok::Semi) && !check(Tok::Eof))
        args.push_back(parseExpr());
      expect(Tok::Semi, "';'");
      return Stmt::exprStmt(
          Expr::call(nameTok.text, std::move(args), nameTok.line), nameTok.line);
    }

    ExprPtr e = parseExpr();
    if (match(Tok::Assign)) {
      ExprPtr value = parseExpr();
      expect(Tok::Semi, "';'");
      return Stmt::assign(std::move(e), std::move(value), t.line);
    }
    expect(Tok::Semi, "';'");
    return Stmt::exprStmt(std::move(e), t.line);
  }

  ExprPtr parseExpr() { return parseOr(); }

  ExprPtr parseOr() {
    ExprPtr lhs = parseAnd();
    while (match(Tok::PipePipe)) {
      int line = peek().line;
      lhs = Expr::binary("||", std::move(lhs), parseAnd(), line);
    }
    return lhs;
  }
  ExprPtr parseAnd() {
    ExprPtr lhs = parseEquality();
    while (match(Tok::AmpAmp)) {
      int line = peek().line;
      lhs = Expr::binary("&&", std::move(lhs), parseEquality(), line);
    }
    return lhs;
  }
  ExprPtr parseEquality() {
    ExprPtr lhs = parseRelational();
    for (;;) {
      std::string op;
      int line = peek().line;
      if (match(Tok::Eq)) op = "==";
      else if (match(Tok::Ne)) op = "!=";
      else break;
      lhs = Expr::binary(op, std::move(lhs), parseRelational(), line);
    }
    return lhs;
  }
  ExprPtr parseRelational() {
    ExprPtr lhs = parseAdditive();
    for (;;) {
      std::string op;
      int line = peek().line;
      if (match(Tok::Lt)) op = "<";
      else if (match(Tok::Le)) op = "<=";
      else if (match(Tok::Gt)) op = ">";
      else if (match(Tok::Ge)) op = ">=";
      else break;
      lhs = Expr::binary(op, std::move(lhs), parseAdditive(), line);
    }
    return lhs;
  }
  ExprPtr parseAdditive() {
    ExprPtr lhs = parseMul();
    for (;;) {
      std::string op;
      int line = peek().line;
      if (match(Tok::Plus)) op = "+";
      else if (match(Tok::Minus)) op = "-";
      else break;
      lhs = Expr::binary(op, std::move(lhs), parseMul(), line);
    }
    return lhs;
  }
  ExprPtr parseMul() {
    ExprPtr lhs = parseUnary();
    for (;;) {
      std::string op;
      int line = peek().line;
      if (match(Tok::Star)) op = "*";
      else if (match(Tok::Slash)) op = "/";
      else if (match(Tok::Percent)) op = "%";
      else break;
      lhs = Expr::binary(op, std::move(lhs), parseUnary(), line);
    }
    return lhs;
  }
  ExprPtr parseUnary() {
    const Token& t = peek();
    if (match(Tok::Minus)) return Expr::unary("-", parseUnary(), t.line);
    if (match(Tok::Bang)) return Expr::unary("!", parseUnary(), t.line);
    // '@' marks a by-reference argument (like C's '&'): ask @a;
    if (match(Tok::At)) return Expr::mention(parseUnary(), t.line);
    return parsePrimary();
  }
  // A statement without its terminating ';' -- used by for(init;cond;update).
  StmtPtr parseSimpleStmt() {
    const Token& t = peek();
    if (check(Tok::KwArray)) {
      advance();
      Type elem = parseType();
      std::vector<DeclItem> items;
      for (;;) {
        DeclItem d;
        d.line = t.line;
        d.name = expect(Tok::Ident, "array name").text;
        d.type = Type::ofArray(elem);
        expect(Tok::LBracket, "'[' with the array length");
        if (check(Tok::Tilde)) {
          advance();
          d.size = Expr::intLit(0, t.line);
          d.growable = true;
          if (!check(Tok::RBracket)) d.maxSize = parseExpr();
        } else {
          d.size = parseExpr();
          if (match(Tok::Tilde)) {
            d.growable = true;
            if (!check(Tok::RBracket)) d.maxSize = parseExpr();
          }
        }
        expect(Tok::RBracket, "']'");
        items.push_back(std::move(d));
        if (!match(Tok::Comma)) break;
      }
      return Stmt::declStmt(std::move(items), t.line);
    }
    if (check(Tok::KwIntType) || check(Tok::KwBoolType) ||
        check(Tok::KwStringType) ||
        (check(Tok::Ident) && peek(1).kind == Tok::Ident)) {
      Type ty = parseType();
      std::vector<DeclItem> items;
      for (;;) {
        DeclItem d;
        d.type = ty;
        d.line = t.line;
        d.name = expect(Tok::Ident, "variable name").text;
        if (match(Tok::Assign)) d.init = parseExpr();
        items.push_back(std::move(d));
        if (!match(Tok::Comma)) break;
      }
      return Stmt::declStmt(std::move(items), t.line);
    }
    ExprPtr e = parseExpr();
    if (match(Tok::Assign)) {
      ExprPtr value = parseExpr();
      return Stmt::assign(std::move(e), std::move(value), t.line);
    }
    return Stmt::exprStmt(std::move(e), t.line);
  }

  ExprPtr parsePostfix(ExprPtr e) {
    for (;;) {
      if (match(Tok::Dot)) {
        const Token& f = expect(Tok::Ident, "field name");
        e = Expr::member(std::move(e), f.text, f.line);
        continue;
      }
      if (match(Tok::LBracket)) {
        const Token& open = toks[pos - 1];
        ExprPtr idx = parseExpr();
        expect(Tok::RBracket, "']'");
        e = Expr::index(std::move(e), std::move(idx), open.line);
        continue;
      }
      break;
    }
    return e;
  }

  std::unique_ptr<StructDecl> parseStruct() {
    const Token& start = expect(Tok::KwStruct, "'struct'");
    auto sd = std::make_unique<StructDecl>();
    sd->name = expect(Tok::Ident, "struct name").text;
    sd->line = start.line;
    expect(Tok::LBrace, "'{'");
    while (!check(Tok::RBrace)) {
      if (check(Tok::Eof)) error("unterminated struct declaration");
      StructField f;
      f.type = parseType();
      f.line = peek().line;
      f.name = expect(Tok::Ident, "field name").text;
      expect(Tok::Semi, "';' after a field");
      sd->fields.push_back(std::move(f));
    }
    expect(Tok::RBrace, "'}'");
    match(Tok::Semi);  // optional ';' after '}' (C++ style)
    return sd;
  }

  ExprPtr parsePrimary() {
    const Token& t = peek();
    if (match(Tok::IntLit)) return Expr::intLit(t.intVal, t.line);
    if (match(Tok::KwTrue)) return Expr::boolLit(true, t.line);
    if (match(Tok::KwFalse)) return Expr::boolLit(false, t.line);
    if (match(Tok::StrLit)) return Expr::strLit(t.text, t.line);
    if (match(Tok::LParen)) {
      ExprPtr e = parseExpr();
      expect(Tok::RParen, "')'");
      return e;
    }
    if (match(Tok::Ident)) {
      std::string name = t.text;
      if (!match(Tok::LParen)) return parsePostfix(Expr::var(name, t.line));
      std::vector<ExprPtr> args;
      if (!check(Tok::RParen)) {
        for (;;) {
          args.push_back(parseExpr());
          if (!match(Tok::Comma)) break;
        }
      }
      expect(Tok::RParen, "')'");
      return parsePostfix(Expr::call(name, std::move(args), t.line));
    }
    error(std::string("unexpected ") + tokName(t.kind) + " in expression");
  }
};

}  // namespace

FileAst parseMainFile(const std::vector<Token>& toks) {
  Parser p(toks, false);
  return p.parseFile();
}

ModuleAst parseModule(const std::vector<Token>& toks, const std::string& name) {
  Parser p(toks, true);
  return p.parseModuleFile(name);
}
