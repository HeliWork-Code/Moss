#pragma once
#include "bytecode.h"
#include <stdexcept>
#include <string>

struct SerializeError : std::runtime_error {
  explicit SerializeError(const std::string& msg) : std::runtime_error(msg) {}
};

// Encodes a program into a portable byte string ("MOBC" container).
std::string serializeProgram(const BytecodeProgram& prog);

// Decodes it back. Throws SerializeError on corrupt input.
BytecodeProgram deserializeProgram(const std::string& data);
