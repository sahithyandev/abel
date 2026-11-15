#include "cli.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void test_help_and_version() {
    char help[] = "--help";
    char* help_args[] = {const_cast<char*>("abel"), help};
    std::istringstream input;
    std::ostringstream output;
    std::ostringstream error;
    require(abel::run_cli(2, help_args, input, output, error) == 0,
            "help exit code");
    require(output.str().find("Usage: abel") != std::string::npos,
            "help output");

    char version[] = "--version";
    char* version_args[] = {const_cast<char*>("abel"), version};
    output.str("");
    require(abel::run_cli(2, version_args, input, output, error) == 0,
            "version exit code");
    require(output.str() == "abel 0.1.0\n", "version output");
}

void test_stdin_script() {
    char dash[] = "-";
    char* args[] = {const_cast<char*>("abel"), dash};
    std::istringstream input("1 + 2\nprint(4)\n");
    std::ostringstream output;
    std::ostringstream error;
    require(abel::run_cli(2, args, input, output, error) == 0,
            "stdin script exit code");
    require(output.str() == "4\n", "scripts only print explicitly");
    require(error.str().empty(), "stdin script diagnostics");
}

void test_repl_state_and_multiline() {
    char* args[] = {const_cast<char*>("abel")};
    std::istringstream input(
        "value = 5\n"
        "fn twice(value) {\n"
        "  return value * 2\n"
        "}\n"
        "twice(value)\n");
    std::ostringstream output;
    std::ostringstream error;
    require(abel::run_cli(1, args, input, output, error) == 0,
            "repl exit code");
    require(output.str() == "10\n", "repl state, multiline input, and echo");
    require(error.str().empty(), "repl diagnostics");
}

void test_errors() {
    char dash[] = "-";
    char* args[] = {const_cast<char*>("abel"), dash};
    std::istringstream input("missing\n");
    std::ostringstream output;
    std::ostringstream error;
    require(abel::run_cli(2, args, input, output, error) == 1,
            "runtime error exit code");
    require(error.str().find("runtime error") != std::string::npos,
            "runtime error diagnostic");

    char unknown[] = "--unknown";
    char* unknown_args[] = {const_cast<char*>("abel"), unknown};
    output.str("");
    error.str("");
    require(abel::run_cli(2, unknown_args, input, output, error) == 2,
            "unknown option exit code");
    require(error.str().find("unknown option") != std::string::npos,
            "unknown option diagnostic");
}

}  // namespace

int main() {
    test_help_and_version();
    test_stdin_script();
    test_repl_state_and_multiline();
    test_errors();
    std::cout << "CLI tests passed\n";
    return EXIT_SUCCESS;
}
