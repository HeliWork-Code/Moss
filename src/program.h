#pragma once
#include "ast.h"
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// Native capabilities provided by the VM itself (the "core" module).
enum NativeId {
  NativePrint,
  NativeReadInt,
  NativeReadStr,
  NativeReadBool,
  NativeReadIntSep,
  NativeAskInt,
  NativeAskStr,
  NativeAskBool,
  NativeLen,
  NativeGrow,
  NativeShrink,
};

struct LoadError : std::runtime_error {
  int line;
  LoadError(const std::string& msg, int line)
      : std::runtime_error("line " + std::to_string(line) + ": " + msg),
        line(line) {}
};

struct LoadedProgram {
  std::vector<std::unique_ptr<FuncDecl>> functions;  // everything, in order
  std::vector<std::unique_ptr<CapScope>> scopes;     // stable addresses
  std::vector<std::unique_ptr<StructDecl>> structs;  // owned struct declarations
  std::map<std::string, const StructDecl*> structMap;
  // struct name -> method name -> method
  std::map<std::string, std::map<std::string, const FuncDecl*>> methods;
  std::vector<const FuncDecl*> bodies;               // functions to compile
  FuncDecl* mainFn = nullptr;
};

// Resolves every `pick` (recursively), builds scopes, and returns the program
// with all capabilities attached. Throws LoadError on missing modules,
// unknown capabilities, cycles, or duplicate names.
LoadedProgram loadProgram(FileAst mainFile, const std::string& sourceDir,
                          const std::string& exeDir);
