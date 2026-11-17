#ifndef ABEL_EVALUATOR_H
#define ABEL_EVALUATOR_H

#include "ast.h"
#include "parser.h"

#include <functional>
#include <iosfwd>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace abel {

struct Callable;
using Array = std::vector<class Value>;

class Value {
  public:
    using ArrayPtr = std::shared_ptr<Array>;
    using CallablePtr = std::shared_ptr<Callable>;
    enum class Kind { Number, String, Boolean, Null, Array, Function };

    Value();
    explicit Value(double value);
    explicit Value(std::string value);
    explicit Value(bool value);
    explicit Value(Array value);
    explicit Value(CallablePtr value);

    Kind kind() const noexcept;
    bool is_null() const noexcept;
    double as_number() const;
    const std::string &as_string() const;
    bool as_boolean() const;
    const Array &as_array() const;
    Array &as_array();
    const CallablePtr &as_callable() const;
    bool truthy() const noexcept;
    std::string to_string() const;

  private:
    std::variant<double, std::string, bool, std::monostate, ArrayPtr,
                 CallablePtr>
        value_;
};

enum class RuntimeErrorCategory {
    UndefinedName,
    TypeError,
    DivisionByZero,
    InvalidIndex,
    NotCallable,
    ArityMismatch,
    ReturnOutsideFunction,
    InvalidArgument,
};

class RuntimeError : public std::runtime_error {
  public:
    RuntimeError(RuntimeErrorCategory category, std::string message,
                 std::size_t line, std::size_t column);
    RuntimeErrorCategory category() const noexcept { return category_; }
    std::size_t line() const noexcept { return line_; }
    std::size_t column() const noexcept { return column_; }

  private:
    RuntimeErrorCategory category_;
    std::size_t line_;
    std::size_t column_;
};

class Evaluator {
  public:
    explicit Evaluator(std::ostream &output);
    Evaluator();

    Value evaluate(const Program &program);
    std::optional<Value> evaluate_repl(Program program);
    Value evaluate(std::string_view source);
    const Value &lookup(const std::string &name) const;

  private:
    struct State;
    std::shared_ptr<State> state_;
};

} // namespace abel

#endif // ABEL_EVALUATOR_H
