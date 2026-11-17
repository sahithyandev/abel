#include "cli.h"

#include "evaluator.h"
#include "parser.h"
#include "tokenizer.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace abel {
namespace {

constexpr const char *kVersion = "0.1.0";

void usage(std::ostream &out) {
    out << "Usage: abel [--help] [--version] [FILE|-]\n"
        << "\n"
        << "Run an Abel program from FILE, or start the REPL when no FILE is "
           "given.\n"
        << "Use - to read Abel source from standard input.\n";
}

std::string error_category(const LexError &) { return "lexical error"; }
std::string error_category(const ParseError &) { return "syntax error"; }
std::string error_category(const RuntimeError &) { return "runtime error"; }

template <typename Error>
void report(const Error &error, const std::string &source_name,
            std::ostream &out) {
    out << source_name << ":" << error.line() << ":" << error.column() << ": "
        << error_category(error) << ": " << error.what() << '\n';
}

bool structurally_incomplete(const std::string &source) {
    try {
        (void)tokenize(source);
    } catch (const LexError &) {
        return false;
    }

    std::size_t braces = 0;
    std::size_t parentheses = 0;
    std::size_t brackets = 0;
    bool in_string = false;
    bool escaped = false;
    for (char character : source) {
        if (in_string) {
            if (escaped)
                escaped = false;
            else if (character == '\\')
                escaped = true;
            else if (character == '"')
                in_string = false;
            continue;
        }
        if (character == '"')
            in_string = true;
        else if (character == '{')
            ++braces;
        else if (character == '}' && braces > 0)
            --braces;
        else if (character == '(')
            ++parentheses;
        else if (character == ')' && parentheses > 0)
            --parentheses;
        else if (character == '[')
            ++brackets;
        else if (character == ']' && brackets > 0)
            --brackets;
    }
    return braces != 0 || parentheses != 0 || brackets != 0;
}

bool terminal_input(std::istream &input) {
    if (&input != &std::cin)
        return false;
#if defined(_WIN32)
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(fileno(stdin)) != 0;
#endif
}

int run_source(const std::string &source, const std::string &source_name,
               std::ostream &output, std::ostream &error) {
    try {
        Evaluator evaluator(output);
        evaluator.evaluate(source);
        return 0;
    } catch (const LexError &exception) {
        report(exception, source_name, error);
    } catch (const ParseError &exception) {
        report(exception, source_name, error);
    } catch (const RuntimeError &exception) {
        report(exception, source_name, error);
    }
    return 1;
}

int run_repl(std::istream &input, std::ostream &output, std::ostream &error) {
    Evaluator evaluator(output);
    std::string buffer;
    std::string line;
    bool interactive = terminal_input(input);

    while (true) {
        if (interactive)
            output << (buffer.empty() ? "abel> " : "... ") << std::flush;
        if (!std::getline(input, line)) {
            if (interactive)
                output << '\n';
            return 0;
        }
        buffer += line;
        buffer += '\n';

        try {
            Program program = parse(buffer);
            const auto result = evaluator.evaluate_repl(std::move(program));
            if (result && !result->is_null())
                output << result->to_string() << '\n';
            buffer.clear();
        } catch (const LexError &exception) {
            report(exception, "<stdin>", error);
            buffer.clear();
        } catch (const ParseError &exception) {
            if (exception.incomplete() || structurally_incomplete(buffer))
                continue;
            report(exception, "<stdin>", error);
            buffer.clear();
        } catch (const RuntimeError &exception) {
            report(exception, "<stdin>", error);
            buffer.clear();
        }
    }
}

} // namespace

int run_cli(int argc, char *argv[], std::istream &input, std::ostream &output,
            std::ostream &error) {
    if (argc > 2) {
        error << "abel: expected at most one input file\n";
        usage(error);
        return 2;
    }
    if (argc == 2) {
        const std::string argument = argv[1];
        if (argument == "--help" || argument == "-h") {
            usage(output);
            return 0;
        }
        if (argument == "--version" || argument == "-V") {
            output << "abel " << kVersion << '\n';
            return 0;
        }
        if (argument == "-") {
            std::ostringstream source;
            source << input.rdbuf();
            return run_source(source.str(), "<stdin>", output, error);
        }
        if (!argument.empty() && argument.front() == '-') {
            error << "abel: unknown option '" << argument << "'\n";
            usage(error);
            return 2;
        }

        std::ifstream file(argument);
        if (!file) {
            error << "abel: cannot open '" << argument << "'\n";
            return 2;
        }
        std::ostringstream source;
        source << file.rdbuf();
        return run_source(source.str(), argument, output, error);
    }
    return run_repl(input, output, error);
}

} // namespace abel
