#include "program.h"
#include "lexer.h"
#include "parser.h"

#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

namespace fs = std::filesystem;

namespace {

std::string readFile(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("cannot open '" + path + "'");
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

class Loader {
 public:
  Loader(std::string srcDir, std::string exeDir)
      : srcDir(std::move(srcDir)), exeDir(std::move(exeDir)) {}

  LoadedProgram load(FileAst mainFile) {
    installCore();
    installStructs(mainFile);

    std::vector<const FuncDecl*> mainFns;
    for (auto& up : mainFile.functions) {
      up->module = "";
      mainFns.push_back(up.get());
      prog.functions.push_back(std::move(up));
    }

    CapScope& mainScope = newScope();
    for (const FuncDecl* f : mainFns) {
      if (mainScope.count(f->name))
        throw LoadError("duplicate function '" + f->name + "'", f->line);
      mainScope[f->name].push_back(f);
    }
    importInto(mainScope, mainFile.picks, "the main file");
    for (const FuncDecl* f : mainFns) const_cast<FuncDecl*>(f)->scope = &mainScope;

    auto it = mainScope.find("main");
    if (it == mainScope.end() || it->second.size() != 1)
      throw LoadError("no 'main' function defined", 1);
    FuncDecl* mainFn = const_cast<FuncDecl*>(it->second[0]);
    if (!mainFn->module.empty())
      throw LoadError("'main' must be defined in the main file", mainFn->line);
    if (mainFn->params.size() > 1 ||
        (mainFn->params.size() == 1 && mainFn->params[0].type != TypeKind::Str))
      throw LoadError("'main' takes at most one string parameter", mainFn->line);
    mainFn->isMain = true;
    prog.mainFn = mainFn;

    // Take ownership of every module capability so it outlives the loader.
    for (auto& mod : modules) {
      for (auto& cap : mod.second.caps) {
        if (cap->hasBody) prog.bodies.push_back(cap.get());
        prog.functions.push_back(std::move(cap));
      }
      mod.second.caps.clear();
    }
    for (const FuncDecl* f : mainFns) prog.bodies.push_back(f);
    return std::move(prog);
  }

 private:
  std::string srcDir, exeDir;
  LoadedProgram prog;
  std::map<std::string, ModuleAst> modules;     // owned loaded modules
  std::map<std::string, const CapScope*> moduleScopes;
  std::set<std::string> loading;

  CapScope& newScope() {
    prog.scopes.push_back(std::make_unique<CapScope>());
    return *prog.scopes.back();
  }

  void installCore() {
    if (modules.count("core")) return;
    ModuleAst core;
    core.name = "core";

    auto native = [&](const std::string& name, Type paramType, bool mut,
                      Type ret, NativeId id) {
      auto f = std::make_unique<FuncDecl>();
      f->name = name;
      f->params.push_back(Param{mut ? "x" : "x", paramType, mut});
      f->returnType = ret;
      f->hasBody = false;
      f->isNative = true;
      f->nativeId = id;
      f->module = "core";
      core.caps.push_back(std::move(f));
    };

    native("print_int", TypeKind::Int, false, TypeKind::Void, NativePrint);
    native("print_bool", TypeKind::Bool, false, TypeKind::Void, NativePrint);
    native("print_str", TypeKind::Str, false, TypeKind::Void, NativePrint);
    native("read_int", TypeKind::Int, true, TypeKind::Void, NativeReadInt);
    native("read_bool", TypeKind::Bool, true, TypeKind::Void, NativeReadBool);
    native("read_str", TypeKind::Str, true, TypeKind::Void, NativeReadStr);

    // read_int_sep(@a, ",", @b): read an int, consume the separator, read
    // another int. This is the primitive behind the `@`-chain style.
    {
      auto f = std::make_unique<FuncDecl>();
      f->name = "read_int_sep";
      f->params.push_back(Param{"a", TypeKind::Int, true});
      f->params.push_back(Param{"sep", TypeKind::Str, false});
      f->params.push_back(Param{"b", TypeKind::Int, true});
      f->returnType = TypeKind::Void;
      f->hasBody = false;
      f->isNative = true;
      f->nativeId = NativeReadIntSep;
      f->module = "core";
      core.caps.push_back(std::move(f));
    }

    // ask("prompt", @x) / ask(@x): print an optional prompt, then read.
    // "ask" is the language's answer to "问": it asks, then listens.
    auto askCap = [&](Type t, bool withPrompt, NativeId id) {
      auto f = std::make_unique<FuncDecl>();
      f->name = "ask";
      if (withPrompt) f->params.push_back(Param{"prompt", TypeKind::Str, false});
      f->params.push_back(Param{"x", t, true});
      f->returnType = TypeKind::Void;
      f->hasBody = false;
      f->isNative = true;
      f->nativeId = id;
      f->module = "core";
      core.caps.push_back(std::move(f));
    };
    // len / grow / shrink, one overload set per element type.
    auto arrCap = [&](const char* name, Type elem, NativeId id, bool withN) {
      auto f = std::make_unique<FuncDecl>();
      f->name = name;
      f->params.push_back(Param{"a", Type::ofArray(elem), false});
      if (withN) f->params.push_back(Param{"n", TypeKind::Int, false});
      f->returnType = id == NativeLen ? TypeKind::Int : TypeKind::Void;
      f->hasBody = false;
      f->isNative = true;
      f->nativeId = id;
      f->module = "core";
      core.caps.push_back(std::move(f));
    };
    for (Type elem : {Type(TypeKind::Int), Type(TypeKind::Bool),
                      Type(TypeKind::Str)}) {
      arrCap("len", elem, NativeLen, false);
      arrCap("grow", elem, NativeGrow, true);
      arrCap("shrink", elem, NativeShrink, true);
    }

    askCap(TypeKind::Int, false, NativeAskInt);
    askCap(TypeKind::Int, true, NativeAskInt);
    askCap(TypeKind::Str, false, NativeAskStr);
    askCap(TypeKind::Str, true, NativeAskStr);
    askCap(TypeKind::Bool, false, NativeAskBool);
    askCap(TypeKind::Bool, true, NativeAskBool);

    CapScope& scope = newScope();
    for (auto& c : core.caps) scope[c->name].push_back(c.get());
    for (auto& c : core.caps) {
      c->scope = &scope;
      prog.functions.push_back(std::move(c));
    }
    core.caps.clear();
    moduleScopes["core"] = &scope;
    modules["core"] = std::move(core);
  }

  void installStructs(FileAst& file) {
    for (auto& sd : file.structs) {
      if (prog.structMap.count(sd->name))
        throw LoadError("duplicate struct '" + sd->name + "'", sd->line);
      std::set<std::string> fieldNames;
      for (const StructField& f : sd->fields) {
        if (f.type.isStruct())
          throw LoadError("struct fields must be int, bool or string (for now)",
                          f.line);
        if (!fieldNames.insert(f.name).second)
          throw LoadError("duplicate field '" + f.name + "' in struct '" +
                              sd->name + "'",
                          f.line);
      }
      if (sd->fields.empty())
        throw LoadError("struct '" + sd->name + "' has no fields", sd->line);
      prog.structMap[sd->name] = sd.get();
      prog.structs.push_back(std::move(sd));
    }
  }

  const CapScope* getModule(const std::string& name, int line) {
    auto it = moduleScopes.find(name);
    if (it != moduleScopes.end()) return it->second;

    if (name == "core") {
      installCore();
      return moduleScopes["core"];
    }

    if (loading.count(name))
      throw LoadError("circular module dependency involving '" + name + "'",
                      line);
    loading.insert(name);

    std::string path = findModuleFile(name);
    if (path.empty()) {
      loading.erase(name);
      throw LoadError("cannot find module '" + name + ".msh'", line);
    }

    std::vector<Token> toks = lex(readFile(path));
    ModuleAst mod = parseModule(toks, name);

    // Build this module's own scope: its caps plus what it picks.
    CapScope& scope = newScope();
    for (auto& cap : mod.caps) {
      if (cap->isNative)
        throw LoadError("module '" + name +
                            "' cannot declare native capability '@" +
                            cap->name + "' (only 'core' is native)",
                        cap->line);
      cap->module = name;
      // Same name with different signatures is an overload; both are kept.
      scope[cap->name].push_back(cap.get());
    }
    importInto(scope, mod.picks, "module '" + name + "'");
    for (auto& cap : mod.caps) cap->scope = &scope;

    moduleScopes[name] = &scope;
    modules[name] = std::move(mod);
    loading.erase(name);
    return &scope;
  }

  void importInto(CapScope& scope, const std::vector<PickEntry>& picks,
                  const std::string& where) {
    for (const PickEntry& p : picks) {
      const CapScope* src = getModule(p.moduleName, p.line);
      auto it = src->find(p.capName);
      if (it == src->end())
        throw LoadError("module '" + p.moduleName + "' has no capability '@" +
                            p.capName + "'",
                        p.line);
      auto& dest = scope[p.alias];
      if (!dest.empty())
        throw LoadError("duplicate capability name '" + p.alias +
                            "' in " + where,
                        p.line);
      dest = it->second;
    }
  }

  std::string findModuleFile(const std::string& name) {
    std::vector<fs::path> candidates;
    fs::path file = name + ".msh";
    candidates.push_back(fs::path(srcDir) / file);
    candidates.push_back(fs::path(srcDir) / "lib" / file);
    candidates.push_back(fs::path(exeDir) / "lib" / file);
    for (const auto& c : candidates) {
      std::error_code ec;
      if (fs::exists(c, ec) && !ec) return c.string();
    }
    return "";
  }
};

}  // namespace

LoadedProgram loadProgram(FileAst mainFile, const std::string& sourceDir,
                          const std::string& exeDir) {
  Loader loader(sourceDir, exeDir);
  return loader.load(std::move(mainFile));
}
