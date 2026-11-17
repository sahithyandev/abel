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

class ExtensionValue {
  public:
    virtual ~ExtensionValue() = default;
    virtual std::string type_name() const = 0;
    virtual std::string to_string() const = 0;
    virtual bool equals(const ExtensionValue &other) const;
    virtual bool has_member(std::string_view name) const;
    virtual Value get_member(std::string_view name,
                             SourceLocation location) const;
    virtual bool supports_indexing() const;
    virtual Value get_index(const Value &index, SourceLocation location) const;
    virtual void set_index(const Value &index, const Value &value,
                           SourceLocation location);
    virtual bool supports_length() const;
    virtual Value length(SourceLocation location) const;
    virtual bool supports_iteration() const;
    virtual std::vector<Value> iterate(SourceLocation location) const;
};

class Value {
  public:
    using ArrayPtr = std::shared_ptr<Array>;
    using CallablePtr = std::shared_ptr<Callable>;
    using ExtensionValuePtr = std::shared_ptr<ExtensionValue>;
    enum class Kind {
        Number,
        String,
        Boolean,
        Null,
        Array,
        Function,
        Extension
    };

    Value();
    explicit Value(double value);
    explicit Value(std::string value);
    explicit Value(bool value);
    explicit Value(Array value);
    explicit Value(CallablePtr value);
    explicit Value(ExtensionValuePtr value);

    Kind kind() const noexcept;
    bool is_null() const noexcept;
    double as_number() const;
    const std::string &as_string() const;
    bool as_boolean() const;
    const Array &as_array() const;
    Array &as_array();
    const CallablePtr &as_callable() const;
    const ExtensionValuePtr &as_extension() const;
    bool truthy() const noexcept;
    std::string to_string() const;

  private:
    std::variant<double, std::string, bool, std::monostate, ArrayPtr,
                 CallablePtr, ExtensionValuePtr>
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

class ExtensionRegistry {
  public:
    using NativeFunction =
        std::function<Value(const std::vector<Value> &, SourceLocation)>;

    ExtensionRegistry();
    ExtensionRegistry(const ExtensionRegistry &);
    ExtensionRegistry &operator=(const ExtensionRegistry &);
    ExtensionRegistry(ExtensionRegistry &&) noexcept;
    ExtensionRegistry &operator=(ExtensionRegistry &&) noexcept;
    ~ExtensionRegistry();

    void register_function(std::string name, std::size_t arity,
                           NativeFunction function);
    void register_module(std::string name, const ExtensionRegistry &module);

  private:
    struct Function;
    struct State;
    std::shared_ptr<State> state_;

    friend class Evaluator;
};

struct ModuleSource {
    std::string source;
};

class ModuleResolver {
  public:
    virtual ~ModuleResolver() = default;
    virtual std::optional<ModuleSource> resolve(std::string_view module,
                                                SourceLocation location) = 0;
};

class Evaluator {
  public:
    explicit Evaluator(std::ostream &output);
    Evaluator(std::ostream &output, const ExtensionRegistry &extensions);
    Evaluator(std::ostream &output, const ExtensionRegistry &extensions,
              std::shared_ptr<ModuleResolver> resolver);
    Evaluator();

    Value evaluate(const Program &program);
    std::optional<Value> evaluate_repl(Program program);
    Value evaluate(std::string_view source);
    const Value &lookup(const std::string &name) const;

  private:
    struct State;
    std::shared_ptr<State> state_;

    Value load_module(const std::string &name, SourceLocation location);
};

} // namespace abel

#endif // ABEL_EVALUATOR_H
