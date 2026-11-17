#include "evaluator.h"
#include "parser.h"
#include "tokenizer.h"

#include <emscripten/bind.h>

#include <sstream>
#include <string>
#include <vector>

namespace abel::wasm {

struct ErrorResult {
    std::string category;
    std::string message;
    int line = 0;
    int column = 0;
};

struct RunResult {
    bool ok = false;
    std::string output;
    std::string value;
    ErrorResult error;
};

struct NotebookCellResult {
    bool ok = false;
    std::string output;
    std::string value;
    ErrorResult error;
};

RunResult run(const std::string &source) {
    std::ostringstream output;
    RunResult result;

    try {
        Evaluator evaluator(output);
        result.value = evaluator.evaluate(source).to_string();
        result.ok = true;
    } catch (const LexError &error) {
        result.error = {"lexical", error.what(), static_cast<int>(error.line()),
                        static_cast<int>(error.column())};
    } catch (const ParseError &error) {
        result.error = {"syntax", error.what(), static_cast<int>(error.line()),
                        static_cast<int>(error.column())};
    } catch (const RuntimeError &error) {
        result.error = {"runtime", error.what(), static_cast<int>(error.line()),
                        static_cast<int>(error.column())};
    }

    result.output = output.str();
    return result;
}

std::vector<NotebookCellResult>
run_notebook(const std::vector<std::string> &sources) {
    std::ostringstream output;
    Evaluator evaluator(output);
    std::vector<NotebookCellResult> results;

    for (const auto &source : sources) {
        NotebookCellResult result;
        const auto output_start = output.str().size();
        try {
            auto value = evaluator.evaluate_repl(parse(source));
            result.value = value ? value->to_string() : "null";
            result.ok = true;
        } catch (const LexError &error) {
            result.error = {"lexical", error.what(),
                            static_cast<int>(error.line()),
                            static_cast<int>(error.column())};
        } catch (const ParseError &error) {
            result.error = {"syntax", error.what(),
                            static_cast<int>(error.line()),
                            static_cast<int>(error.column())};
        } catch (const RuntimeError &error) {
            result.error = {"runtime", error.what(),
                            static_cast<int>(error.line()),
                            static_cast<int>(error.column())};
        }
        const auto all_output = output.str();
        result.output = all_output.substr(output_start);
        results.push_back(std::move(result));
        if (!results.back().ok)
            break;
    }

    return results;
}

std::vector<NotebookCellResult> run_notebook_js(emscripten::val sources) {
    return run_notebook(emscripten::vecFromJSArray<std::string>(sources));
}

} // namespace abel::wasm

EMSCRIPTEN_BINDINGS(abel_wasm) {
    emscripten::value_object<abel::wasm::ErrorResult>("ErrorResult")
        .field("category", &abel::wasm::ErrorResult::category)
        .field("message", &abel::wasm::ErrorResult::message)
        .field("line", &abel::wasm::ErrorResult::line)
        .field("column", &abel::wasm::ErrorResult::column);
    emscripten::value_object<abel::wasm::RunResult>("RunResult")
        .field("ok", &abel::wasm::RunResult::ok)
        .field("output", &abel::wasm::RunResult::output)
        .field("value", &abel::wasm::RunResult::value)
        .field("error", &abel::wasm::RunResult::error);
    emscripten::value_object<abel::wasm::NotebookCellResult>(
        "NotebookCellResult")
        .field("ok", &abel::wasm::NotebookCellResult::ok)
        .field("output", &abel::wasm::NotebookCellResult::output)
        .field("value", &abel::wasm::NotebookCellResult::value)
        .field("error", &abel::wasm::NotebookCellResult::error);
    emscripten::register_vector<abel::wasm::NotebookCellResult>(
        "NotebookCellResultVector");
    emscripten::function("run", &abel::wasm::run);
    emscripten::function("runNotebook", &abel::wasm::run_notebook_js);
}
