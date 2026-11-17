#include "evaluator.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace {
using namespace abel;

class PointValue final : public ExtensionValue {
  public:
    PointValue(double x, double y) : x(x), y(y) {}

    std::string type_name() const override { return "test.point"; }
    std::string to_string() const override {
        return "point(" + std::to_string(x) + ", " + std::to_string(y) + ")";
    }
    bool equals(const ExtensionValue &other) const override {
        const auto *point = dynamic_cast<const PointValue *>(&other);
        return point && x == point->x && y == point->y;
    }
    bool supports_indexing() const override { return true; }
    Value get_index(const Value &index,
                    SourceLocation location) const override {
        if (index.kind() != Value::Kind::Number ||
            (index.as_number() != 0 && index.as_number() != 1))
            throw RuntimeError(RuntimeErrorCategory::InvalidIndex,
                               "point index must be 0 or 1", location.line,
                               location.column);
        return Value(index.as_number() == 0 ? x : y);
    }
    void set_index(const Value &index, const Value &value,
                   SourceLocation location) override {
        if (index.kind() != Value::Kind::Number ||
            (index.as_number() != 0 && index.as_number() != 1) ||
            value.kind() != Value::Kind::Number)
            throw RuntimeError(
                RuntimeErrorCategory::InvalidIndex,
                "point assignment requires numeric index and value",
                location.line, location.column);
        (index.as_number() == 0 ? x : y) = value.as_number();
    }
    bool supports_length() const override { return true; }
    Value length(SourceLocation) const override { return Value(2.0); }
    bool supports_iteration() const override { return true; }
    std::vector<Value> iterate(SourceLocation) const override {
        return {Value(x), Value(y)};
    }

    double x;
    double y;
};

class MapResolver final : public ModuleResolver {
  public:
    std::unordered_map<std::string, std::string> sources;
    std::unordered_map<std::string, int> resolutions;

    std::optional<ModuleSource> resolve(std::string_view module,
                                        SourceLocation) override {
        ++resolutions[std::string(module)];
        const auto found = sources.find(std::string(module));
        if (found == sources.end())
            return std::nullopt;
        return ModuleSource{found->second};
    }
};

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
            e.evaluate("export fn hidden() { return 1 }");
        },
        RuntimeErrorCategory::InvalidArgument, 1, 1);
    require_error(
        [] {
            Evaluator e;
            e.evaluate("sum([1, true])");
        },
        RuntimeErrorCategory::TypeError, 1, 1);
    require_error(
        [] {
            Evaluator e;
            e.evaluate("values = [1]; values[-1]");
        },
        RuntimeErrorCategory::InvalidIndex, 1, 15);
    require_error(
        [] {
            Evaluator e;
            e.evaluate("min([])");
        },
        RuntimeErrorCategory::InvalidArgument, 1, 1);
    require_error(
        [] {
            Evaluator e;
            e.evaluate("length(42)");
        },
        RuntimeErrorCategory::TypeError, 1, 1);
}

void test_repl_evaluation() {
    std::ostringstream output;
    Evaluator evaluator(output);
    auto first = evaluator.evaluate_repl(parse("value = 7"));
    auto second = evaluator.evaluate_repl(parse("value * 3"));
    require(!first, "REPL assignment has no expression result");
    require(second && second->as_number() == 21,
            "REPL preserves state between cells");
}

void test_extension_registry() {
    ExtensionRegistry extensions;
    extensions.register_function(
        "double_value", 1,
        [](const std::vector<Value> &arguments, SourceLocation location) {
            if (arguments[0].kind() != Value::Kind::Number)
                throw RuntimeError(RuntimeErrorCategory::TypeError,
                                   "double_value requires a number",
                                   location.line, location.column);
            return Value(arguments[0].as_number() * 2);
        });

    Evaluator evaluator(std::cout, extensions);
    evaluator.evaluate("answer = double_value(21)");
    require(number(evaluator, "answer") == 42, "registered extension function");

    require_error([&] { evaluator.evaluate("double_value(\"x\")"); },
                  RuntimeErrorCategory::TypeError, 1, 1);

    ExtensionRegistry value_extensions;
    value_extensions.register_function(
        "point", 2, [](const std::vector<Value> &arguments, SourceLocation) {
            return Value(std::make_shared<PointValue>(
                arguments[0].as_number(), arguments[1].as_number()));
        });
    value_extensions.register_function(
        "point_x", 1,
        [](const std::vector<Value> &arguments, SourceLocation location) {
            if (arguments[0].kind() != Value::Kind::Extension ||
                arguments[0].as_extension()->type_name() != "test.point")
                throw RuntimeError(RuntimeErrorCategory::TypeError,
                                   "point_x requires a point", location.line,
                                   location.column);
            return Value(
                static_cast<const PointValue &>(*arguments[0].as_extension())
                    .x);
        });

    Evaluator value_evaluator(std::cout, value_extensions);
    value_evaluator.evaluate(
        "p = point(2, 3); q = point(2, 8); x = point_x(p); first = p[0]; "
        "p[1] = 8; size = length(p); total = 0; for item in p { total = total "
        "+ item }; "
        "same = p == q; shown = p");
    require(number(value_evaluator, "x") == 2 &&
                number(value_evaluator, "first") == 2 &&
                number(value_evaluator, "size") == 2 &&
                number(value_evaluator, "total") == 10 &&
                value_evaluator.lookup("same").as_boolean() &&
                value_evaluator.lookup("shown").to_string().find("point(2") ==
                    0,
            "custom extension value");

    ExtensionRegistry math_module;
    math_module.register_function(
        "double", 1, [](const std::vector<Value> &arguments, SourceLocation) {
            return Value(arguments[0].as_number() * 2);
        });
    ExtensionRegistry modules;
    modules.register_module("math", math_module);
    Evaluator module_evaluator(std::cout, modules);
    require_error([&] { module_evaluator.evaluate("math.double(21)"); },
                  RuntimeErrorCategory::UndefinedName, 1, 1);
    module_evaluator.evaluate("import math; answer = math.double(21)");
    require(number(module_evaluator, "answer") == 42,
            "qualified module export");

    auto resolver = std::make_shared<MapResolver>();
    resolver->sources["sample"] =
        "export fn twice(value) { return value * 2 }\n"
        "fn secret() { return 99 }";
    Evaluator source_evaluator(std::cout, ExtensionRegistry{}, resolver);
    source_evaluator.evaluate(
        "import sample; import sample; answer = sample.twice(21)");
    require(number(source_evaluator, "answer") == 42 &&
                resolver->resolutions["sample"] == 1,
            "source module evaluation and caching");
    require_error([&] { source_evaluator.evaluate("sample.secret()"); },
                  RuntimeErrorCategory::UndefinedName, 1, 1);

    auto cyclic_resolver = std::make_shared<MapResolver>();
    cyclic_resolver->sources["a"] = "import b";
    cyclic_resolver->sources["b"] = "import a";
    Evaluator cyclic_evaluator(std::cout, ExtensionRegistry{}, cyclic_resolver);
    require_error([&] { cyclic_evaluator.evaluate("import a"); },
                  RuntimeErrorCategory::InvalidArgument, 1, 1);
}
} // namespace

int main() {
    test_values_and_operators();
    test_control_flow_and_arrays();
    test_functions_closures_and_recursion();
    test_builtins_and_output();
    test_equality_and_short_circuit();
    test_errors();
    test_repl_evaluation();
    test_extension_registry();
    std::cout << "Evaluator tests passed\n";
    return EXIT_SUCCESS;
}
