#include "compiler.h"

#include <cerrno>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>

namespace {
int runCommand(const std::vector<std::string>& arguments) {
  std::vector<char*> argv;
  for (const auto& argument : arguments) argv.push_back(const_cast<char*>(argument.c_str()));
  argv.push_back(nullptr);
  pid_t child = fork();
  if (child < 0) return -1;
  if (child == 0) { execvp(argv[0], argv.data()); _exit(127); }
  int status = 0;
  while (waitpid(child, &status, 0) < 0) if (errno != EINTR) return -1;
  if (WIFEXITED(status)) return WEXITSTATUS(status);
  if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
  return -1;
}

bool writableDirectory(const std::filesystem::path& dir) {
  std::error_code err;
  if (dir.empty()) return false;
  auto status = std::filesystem::status(dir, err);
  if (err) return false;
  if (!std::filesystem::is_directory(status)) return false;
  return ::access(dir.c_str(), W_OK) == 0;
}

std::string defaultLlvmPath(const std::string& output) {
  std::filesystem::path outPath(output);
  std::filesystem::path candidate = outPath;
  candidate += ".ll";
  auto parent = candidate.parent_path();
  if (parent.empty()) parent = std::filesystem::current_path();
  if (writableDirectory(parent)) return candidate.string();

  const char* tmp = std::getenv("TMPDIR");
  if (tmp && *tmp) {
    std::filesystem::path tmpDir(tmp);
    if (writableDirectory(tmpDir)) {
      return (tmpDir / candidate.filename()).string();
    }
  }
  return candidate.filename().string();
}
}
int main(int argc, char** argv) {
  std::string input = argc > 1 ? argv[1] : "";
  try {
    if (argc < 2) {
      std::cerr << "usage: p64 source.pas [-o executable] [--emit-llvm file.ll] [--clang path] [--link library-or-object] [--unit-path directory] [--debug] [--verbose] [--run] [-- ...]\n";
      return 2;
    }
    std::string output = std::filesystem::path(input).stem().string(), llvm, clang = "clang";
    std::vector<std::string> linkInputs;
    std::vector<std::string> unitPaths;
    bool debug = false;
    bool run = false;
    bool verbose = false;
    std::vector<std::string> runArgs;
    for (int index = 2; index < argc; ++index) {
      std::string option = argv[index];
      if (option == "--") {
        for (++index; index < argc; ++index) runArgs.push_back(argv[index]);
        break;
      }
      if (option == "--debug" || option == "-g") { debug = true; continue; }
      if (option == "--run") { run = true; continue; }
      if (option == "--verbose") { verbose = true; continue; }
      if (option != "-o" && option != "--emit-llvm" && option != "--clang" && option != "--link" && option != "--unit-path") throw std::runtime_error("unknown option: " + option);
      if (++index >= argc) throw std::runtime_error("missing argument for " + option);
      if (option == "-o") output = argv[index];
      else if (option == "--emit-llvm") llvm = argv[index];
      else if (option == "--clang") clang = argv[index];
      else if (option == "--link") linkInputs.push_back(argv[index]);
      else unitPaths.push_back(argv[index]);
    }
    std::ifstream file(input);
    if (!file) throw std::runtime_error("cannot open " + input);
    std::stringstream source; source << file.rdbuf();
    auto program = parse(source.str(), input, unitPaths);
    analyze(*program);
    std::string code = generate(*program, debug);
    if (llvm.empty()) llvm = defaultLlvmPath(output);
    std::ofstream out(llvm);
    if (!out) throw std::runtime_error("cannot write " + llvm);
    out << code; out.close();
    if (!out) throw std::runtime_error("cannot write " + llvm);
    std::vector<std::string> command{clang, "-Wno-override-module", "-fexceptions", "-funwind-tables", llvm, "-o", output};
    if (debug) command.push_back("-g");
    for (const auto& linkInput : linkInputs) {
      command.push_back(linkInput);
      if (linkInput.size() > 3 && linkInput.ends_with(".so")) {
        std::filesystem::path libPath(linkInput);
        auto parent = libPath.parent_path();
        if (!parent.empty()) {
          command.push_back(std::string("-Wl,-rpath,") + parent.string());
        }
      }
    }
    command.push_back(std::string("-L") + P64_RUNTIME_DIR); command.push_back("-lp64rt"); command.push_back("-lstdc++");
    if (verbose) {
      std::cerr << "clang:";
      for (const auto& arg : command) std::cerr << ' ' << arg;
      std::cerr << "\n";
    }
    if (runCommand(command) != 0)
      throw std::runtime_error("Clang compilation/linking failed");

    if (!run)
      std::cout << "created " << output << " (LLVM IR: " << llvm << ")\n";

    if (run) {
      std::vector<std::string> execArgs;
      execArgs.push_back(output);
      for (const auto& arg : runArgs) execArgs.push_back(arg);
      return runCommand(execArgs);
    }

    return 0;
  } catch (const std::exception& error) {
    std::string message = error.what();
    if (message.rfind("line ", 0) == 0) message = input + ":" + message.substr(5);
    std::cerr << "p64: " << message << "\n";
    return 1;
  }
}
