// mossrt -- the standalone runtime.
//
// A program built by `moss build prog.ms -o prog.exe` is this executable with
// the bytecode appended:
//
//     [ mossrt.exe ][ bytecode payload ][ u64 length ][ "MOSSBC01" ]
//
// mossrt locates its own file, checks the trailer, and runs the payload on the
// same VM used by the interpreter.
#include "bytecode.h"
#include "serialize.h"
#include "vm.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace {

const char kTrailer[8] = {'M', 'O', 'S', 'S', 'B', 'C', '0', '1'};

std::string selfPath(const char* argv0) {
  // Prefer the real module path on Windows so the program also works when it
  // is started without a path (e.g. from another directory).
#ifdef _WIN32
  char buf[MAX_PATH];
  DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
  if (n > 0 && n < MAX_PATH) return std::string(buf, n);
#endif
  return fs::absolute(argv0).string();
}

std::string readAll(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("cannot open '" + path + "'");
  std::string data((std::istreambuf_iterator<char>(in)),
                   std::istreambuf_iterator<char>());
  return data;
}

uint64_t le64(const char* p) {
  uint64_t v = 0;
  for (int i = 0; i < 8; ++i) v |= (uint64_t)(unsigned char)p[i] << (8 * i);
  return v;
}

// Extracts the bytecode payload from this executable's own image.
std::string extractPayload(const std::string& image) {
  if (image.size() < 16) throw std::runtime_error("no bytecode embedded");
  const char* end = image.data() + image.size();
  if (std::memcmp(end - 8, kTrailer, 8) != 0)
    throw std::runtime_error("no Moss bytecode embedded in this executable");
  uint64_t len = le64(end - 16);
  if (len + 16 > image.size()) throw std::runtime_error("corrupt bytecode trailer");
  return image.substr(image.size() - 16 - (size_t)len, (size_t)len);
}

}  // namespace

int main(int argc, char** argv) {
  try {
    std::string image = readAll(selfPath(argv[0]));
    std::string payload = extractPayload(image);

    BytecodeProgram prog = deserializeProgram(payload);

    std::string args;
    for (int i = 1; i < argc; ++i) {
      if (i > 1) args += " ";
      args += argv[i];
    }

    setvbuf(stdout, nullptr, _IONBF, 0);
    VM vm(prog, args);
    return vm.run();
  } catch (const std::exception& e) {
    std::fprintf(stderr, "mossrt: error: %s\n", e.what());
    return 1;
  }
}
