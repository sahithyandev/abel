#include "evaluator.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace {
using namespace abel;

void require(bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

double number(const Evaluator &evaluator, const std::string &name) {
    return evaluator.lookup(name).as_number();
}

template <typename Function>
void require_error(Function function, RuntimeErrorCategory category,
                   std::size_t line = 1, std::size_t column = 1) {
    try {
        function();
    } catch (const RuntimeError &error) {
        require(error.category() == category,
                "unexpected runtime error category");
        require(error.line() == line && error.column() == column,
                "unexpected runtime error location");
        return;
    }
    require(false, "expected runtime error");
}

void test_values_and_operators() {
    Evaluator evaluator;
    evaluator.evaluate(R"abel(
        a = 2 + 3 * 4
        b = (2 + 3) * 4
        c = -a
        text = "Abel " + "runtime"
        truth = not false and 0 == 0 or false
        zero_is_true = 0
        empty_is_true = ""
    )abel");
    require(number(evaluator, "a") == 14 && number(evaluator, "b") == 20,
            "arithmetic precedence");
    require(number(evaluator, "c") == -14 &&
                evaluator.lookup("text").as_string() == "Abel runtime",
            "unary and string arithmetic");
    require(evaluator.lookup("truth").as_boolean() &&
                evaluator.lookup("zero_is_true").truthy() &&
                evaluator.lookup("empty_is_true").truthy(),
            "truthiness and logic");
}

void test_control_flow_and_arrays() {
    Evaluator evaluator;
    evaluator.evaluate(R"abel(
        total = 0
        i = 0
        while i < 4 { total = total + i; i = i + 1 }
        values = [1, 2, 3]
        for value in values { value = value * 10; total = total + value }
        values[1] = 8
        selected = values[1]
        if total > 10 { branch = "then" } else { branch = "else" }
    )abel");
    require(number(evaluator, "total") == 66 &&
                number(evaluator, "selected") == 8,
            "loops and array mutation");
    require(evaluator.lookup("branch").as_string() == "then",
            "conditional branch");
    require(evaluator.lookup("values").as_array()[0].as_number() == 1,
            "loop variable does not mutate array");
}

void test_functions_closures_and_recursion() {
    Evaluator evaluator;
    evaluator.evaluate(R"abel(
        base = 10
        fn add_base(value) { return value + base }
        fn factorial(value) { if value <= 1 { return 1 }; return value * factorial(value - 1) }
        fn set_base(value) { base = value }
        first = add_base(5)
        fact = factorial(5)
        set_base(3)
        second = add_base(5)
    )abel");
    require(number(evaluator, "first") == 15 &&
                number(evaluator, "fact") == 120 &&
                number(evaluator, "second") == 8,
            "closures and recursion");
}

void test_builtins_and_output() {
    std::ostringstream output;
    Evaluator evaluator(output);
    evaluator.evaluate(R"abel(
        values = [4, 1, 9]
        size = length(values)
        text_size = length("hello")
        total = sum(values)
        low = min(values)
        high = max(values)
        distance = abs(-3)
        root = sqrt(9)
        print(values)
        print(null)
    )abel");
    require(number(evaluator, "size") == 3 &&
                number(evaluator, "text_size") == 5,
            "length builtin");
    require(number(evaluator, "total") == 14 && number(evaluator, "low") == 1 &&
                number(evaluator, "high") == 9,
            "array builtins");
    require(number(evaluator, "distance") == 3 &&
                number(evaluator, "root") == 3,
            "numeric builtins");
    require(output.str() == "[4, 1, 9]\nnull\n", "print output");
}

void test_equality_and_short_circuit() {
    Evaluator evaluator;
    evaluator.evaluate("same = [1, [2]] == [1, [2]]; different = [1] != [2]; "
                       "safe = false and missing; also_safe = true or missing");
    require(evaluator.lookup("same").as_boolean() &&
                evaluator.lookup("different").as_boolean(),
            "deep equality");
    require(!evaluator.lookup("safe").as_boolean() &&
                evaluator.lookup("also_safe").as_boolean(),
            "short circuit");
}

void test_errors() {
    require_error(
        [] {
            Evaluator e;
            e.evaluate("missing");
        },
        RuntimeErrorCategory::UndefinedName, 1, 1);
    require_error(
        [] {
            Evaluator e;
            e.evaluate("1 / 0");
        },
        RuntimeErrorCategory::DivisionByZero, 1, 3);
    require_error(
        [] {
            Evaluator e;
            e.evaluate("x = [1]; x[1]");
        },
        RuntimeErrorCategory::InvalidIndex, 1, 10);
    require_error(
        [] {
            Evaluator e;
            e.evaluate("x = 1; x()");
        },
        RuntimeErrorCategory::NotCallable, 1, 8);
    require_error(
        [] {
            Evaluator e;
            e.evaluate("print()");
        },
        RuntimeErrorCategory::ArityMismatch, 1, 1);
    require_error(
        [] {
            Evaluator e;
            e.evaluate("return 1");
        },
        RuntimeErrorCategory::ReturnOutsideFunction, 1, 1);
    require_error(
        [] {
            Evaluator e;
            e.evaluate("sqrt(-1)");
        },
        RuntimeErrorCategory::InvalidArgument, 1, 1);
    require_error(
        [] {
            Evaluator e;
            e.evaluate("sum([1, true])");
        },
        RuntimeErrorCategory::TypeError, 1, 1);
}
} // namespace

int main() {
    test_values_and_operators();
    test_control_flow_and_arrays();
    test_functions_closures_and_recursion();
    test_builtins_and_output();
    test_equality_and_short_circuit();
    test_errors();
    std::cout << "Evaluator tests passed\n";
    return EXIT_SUCCESS;
}
