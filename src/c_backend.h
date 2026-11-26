#pragma once
#include "program.h"
#include <string>

// Translates a type-checked Moss program into a single self-contained C file.
std::string emitC(const LoadedProgram& prog);
