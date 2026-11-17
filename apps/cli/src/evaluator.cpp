#include "evaluator.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace abel {

struct ExtensionRegistry::Function {
    std::size_t arity;
    NativeFunction function;
};

struct ExtensionRegistry::State {
    std::unordered_map<std::string, Function> functions;
    std::unordered_map<std::string, ExtensionRegistry> modules;
};

ExtensionRegistry::ExtensionRegistry() : state_(std::make_shared<State>()) {}
ExtensionRegistry::ExtensionRegistry(const ExtensionRegistry &other)
    : state_(std::make_shared<State>(*other.state_)) {}
ExtensionRegistry &
ExtensionRegistry::operator=(const ExtensionRegistry &other) {
    if (this != &other)
        state_ = std::make_shared<State>(*other.state_);
    return *this;
}
ExtensionRegistry::ExtensionRegistry(ExtensionRegistry &&) noexcept = default;
ExtensionRegistry &
ExtensionRegistry::operator=(ExtensionRegistry &&) noexcept = default;
ExtensionRegistry::~ExtensionRegistry() = default;

void ExtensionRegistry::register_function(std::string name, std::size_t arity,
                                          NativeFunction function) {
    if (name.empty())
        throw std::invalid_argument("extension function name cannot be empty");
    if (!function)
        throw std::invalid_argument("extension function cannot be empty");
    const auto [_, inserted] = state_->functions.emplace(
        std::move(name), Function{arity, std::move(function)});
    if (!inserted)
        throw std::invalid_argument("extension function is already registered");
}

void ExtensionRegistry::register_module(std::string name,
                                        const ExtensionRegistry &module) {
    if (name.empty())
        throw std::invalid_argument("extension module name cannot be empty");
    const auto [_, inserted] = state_->modules.emplace(std::move(name), module);
    if (!inserted)
        throw std::invalid_argument("extension module is already registered");
}

bool ExtensionValue::equals(const ExtensionValue &other) const {
    return this == &other;
}

bool ExtensionValue::has_member(std::string_view) const { return false; }

Value ExtensionValue::get_member(std::string_view, SourceLocation) const {
    return Value();
}

bool ExtensionValue::supports_indexing() const { return false; }

Value ExtensionValue::get_index(const Value &, SourceLocation) const {
    return Value();
}

void ExtensionValue::set_index(const Value &, const Value &, SourceLocation) {}

struct Callable {
    virtual ~Callable() = default;
    virtual Value call(const std::vector<Value> &arguments,
                       SourceLocation location) = 0;
};

namespace {

std::string diagnostic(std::string message, std::size_t line,
                       std::size_t column) {
    std::ostringstream out;
    out << message << " at line " << line << ", column " << column;
    return out.str();
}

struct Environment {
    explicit Environment(std::shared_ptr<Environment> parent = nullptr)
        : parent(std::move(parent)) {}
    std::unordered_map<std::string, Value> values;
    std::shared_ptr<Environment> parent;
};

struct NativeCallable final : Callable {
    NativeCallable(
        std::size_t arity,
        std::function<Value(const std::vector<Value> &, SourceLocation)> fn)
        : arity(arity), fn(std::move(fn)) {}
    std::size_t arity;
    std::function<Value(const std::vector<Value> &, SourceLocation)> fn;
    Value call(const std::vector<Value> &arguments,
               SourceLocation location) override {
        if (arguments.size() != arity) {
            throw RuntimeError(RuntimeErrorCategory::ArityMismatch,
                               "wrong number of arguments", location.line,
                               location.column);
        }
        return fn(arguments, location);
    }
};

struct ModuleValue final : ExtensionValue {
    explicit ModuleValue(std::unordered_map<std::string, Value> exports)
        : exports(std::move(exports)) {}

    std::string type_name() const override { return "module"; }
    std::string to_string() const override { return "<module>"; }
    bool has_member(std::string_view name) const override {
        return exports.find(std::string(name)) != exports.end();
    }
    Value get_member(std::string_view name,
                     SourceLocation location) const override {
        const auto found = exports.find(std::string(name));
        if (found == exports.end())
            throw RuntimeError(RuntimeErrorCategory::UndefinedName,
                               "module has no export '" + std::string(name) +
                                   "'",
                               location.line, location.column);
        return found->second;
    }

    std::unordered_map<std::string, Value> exports;
};

struct UserCallable final : Callable {
    std::vector<std::string> parameters;
    const BlockStatement *body;
    std::shared_ptr<Environment> closure;
    std::function<Value(const BlockStatement &, std::shared_ptr<Environment>)>
        execute;
    Value call(const std::vector<Value> &arguments,
               SourceLocation location) override {
        if (arguments.size() != parameters.size()) {
            throw RuntimeError(RuntimeErrorCategory::ArityMismatch,
                               "wrong number of arguments", location.line,
                               location.column);
        }
        auto environment = std::make_shared<Environment>(closure);
        for (std::size_t i = 0; i < parameters.size(); ++i)
            environment->values[parameters[i]] = arguments[i];
        return execute(*body, std::move(environment));
    }
};

bool equal_values(const Value &left, const Value &right);

class Runner {
  public:
    Runner(std::ostream &output, std::shared_ptr<Environment> globals,
           const std::unordered_map<std::string, Value> *modules)
        : globals_(std::move(globals)), environment_(globals_),
          modules_(modules) {
        (void)output;
    }

    Value run(const Program &program) {
        execute_statements(program.statements);
        return Value();
    }

    std::optional<Value> run_repl(const Program &program) {
        std::optional<Value> result;
        for (const auto &statement : program.statements) {
            if (auto *expression_statement =
                    dynamic_cast<const ExpressionStatement *>(
                        statement.get())) {
                result = expression_value(*expression_statement->expression);
            } else {
                execute(*statement);
            }
        }
        return result;
    }

    Value expression(const Expression &expression) {
        if (auto *literal =
                dynamic_cast<const LiteralExpression *>(&expression))
            return literal_value(*literal);
        if (auto *variable =
                dynamic_cast<const VariableExpression *>(&expression))
            return lookup(variable->name, expression.location);
        if (auto *member =
                dynamic_cast<const MemberExpression *>(&expression)) {
            Value object = expression_value(*member->object);
            if (object.kind() != Value::Kind::Extension ||
                !object.as_extension()->has_member(member->member))
                fail(RuntimeErrorCategory::UndefinedName,
                     "value has no member '" + member->member + "'",
                     expression.location);
            return object.as_extension()->get_member(member->member,
                                                     expression.location);
        }
        if (auto *array = dynamic_cast<const ArrayExpression *>(&expression)) {
            Array result;
            for (const auto &element : array->elements)
                result.push_back(expression_value(*element));
            return Value(std::move(result));
        }
        if (auto *unary = dynamic_cast<const UnaryExpression *>(&expression)) {
            Value value = expression_value(*unary->operand);
            if (unary->op == TokenKind::Not)
                return Value(!value.truthy());
            if (value.kind() != Value::Kind::Number)
                fail(RuntimeErrorCategory::TypeError,
                     "unary '-' requires a number", expression.location);
            return Value(-value.as_number());
        }
        if (auto *binary = dynamic_cast<const BinaryExpression *>(&expression))
            return binary_value(*binary);
        if (auto *call = dynamic_cast<const CallExpression *>(&expression)) {
            Value callee = expression_value(*call->callee);
            if (callee.kind() != Value::Kind::Function)
                fail(RuntimeErrorCategory::NotCallable, "value is not callable",
                     expression.location);
            std::vector<Value> arguments;
            for (const auto &argument : call->arguments)
                arguments.push_back(expression_value(*argument));
            return callee.as_callable()->call(arguments, expression.location);
        }
        auto *index = dynamic_cast<const IndexExpression *>(&expression);
        Value object = expression_value(*index->object);
        Value position = expression_value(*index->index);
        return indexed(object, position, expression.location);
    }

    void execute(const Statement &statement) {
        if (auto *block = dynamic_cast<const BlockStatement *>(&statement)) {
            execute_statements(block->statements);
            return;
        }
        if (auto *imported =
                dynamic_cast<const ImportStatement *>(&statement)) {
            const auto found = modules_->find(imported->module);
            if (found == modules_->end())
                fail(RuntimeErrorCategory::UndefinedName,
                     "undefined module '" + imported->module + "'",
                     statement.location);
            assign(imported->module, found->second);
            return;
        }
        if (auto *expr =
                dynamic_cast<const ExpressionStatement *>(&statement)) {
            expression_value(*expr->expression);
            return;
        }
        if (auto *assignment =
                dynamic_cast<const AssignmentStatement *>(&statement)) {
            Value value = expression_value(*assignment->value);
            if (auto *variable = dynamic_cast<const VariableExpression *>(
                    assignment->target.get()))
                assign(variable->name, value);
            else {
                auto *index = dynamic_cast<const IndexExpression *>(
                    assignment->target.get());
                Value object = expression_value(*index->object);
                Value position = expression_value(*index->index);
                set_index(object, position, value, statement.location);
            }
            return;
        }
        if (auto *conditional = dynamic_cast<const IfStatement *>(&statement)) {
            if (expression_value(*conditional->condition).truthy())
                execute(*conditional->then_branch);
            else if (conditional->else_branch)
                execute(*conditional->else_branch);
            return;
        }
        if (auto *loop = dynamic_cast<const WhileStatement *>(&statement)) {
            while (expression_value(*loop->condition).truthy())
                execute(*loop->body);
            return;
        }
        if (auto *loop = dynamic_cast<const ForStatement *>(&statement)) {
            Value iterable = expression_value(*loop->iterable);
            if (iterable.kind() != Value::Kind::Array)
                fail(RuntimeErrorCategory::TypeError, "for requires an array",
                     statement.location);
            for (const Value &item : iterable.as_array()) {
                assign(loop->variable, item);
                execute(*loop->body);
            }
            return;
        }
        if (auto *function =
                dynamic_cast<const FunctionStatement *>(&statement)) {
            auto callable = std::make_shared<UserCallable>();
            callable->parameters = function->parameters;
            callable->body = function->body.get();
            callable->closure = environment_;
            callable->execute = [this](const BlockStatement &body,
                                       std::shared_ptr<Environment> env) {
                auto saved = environment_;
                auto saved_function = in_function_;
                environment_ = std::move(env);
                in_function_ = true;
                try {
                    execute(body);
                } catch (const ReturnSignal &signal) {
                    environment_ = std::move(saved);
                    in_function_ = saved_function;
                    return signal.value;
                } catch (...) {
                    environment_ = std::move(saved);
                    in_function_ = saved_function;
                    throw;
                }
                environment_ = std::move(saved);
                in_function_ = saved_function;
                return Value();
            };
            assign(function->name, Value(std::move(callable)));
            return;
        }
        if (auto *returned =
                dynamic_cast<const ReturnStatement *>(&statement)) {
            if (!in_function_)
                fail(RuntimeErrorCategory::ReturnOutsideFunction,
                     "return outside a function", statement.location);
            throw ReturnSignal(
                returned->value ? expression_value(*returned->value) : Value());
        }
    }

    Value lookup(const std::string &name, SourceLocation location) const {
        for (auto current = environment_; current; current = current->parent) {
            auto found = current->values.find(name);
            if (found != current->values.end())
                return found->second;
        }
        fail(RuntimeErrorCategory::UndefinedName,
             "undefined name '" + name + "'", location);
    }

  private:
    struct ReturnSignal {
        explicit ReturnSignal(Value value) : value(std::move(value)) {}
        Value value;
    };
    std::shared_ptr<Environment> globals_, environment_;
    const std::unordered_map<std::string, Value> *modules_;
    bool in_function_ = false;
    Value expression_value(const Expression &e) { return expression(e); }
    [[noreturn]] void fail(RuntimeErrorCategory category,
                           const std::string &message,
                           SourceLocation location) const {
        throw RuntimeError(category, message, location.line, location.column);
    }
    void execute_statements(const std::vector<StatementPtr> &statements) {
        for (const auto &statement : statements)
            execute(*statement);
    }
    void assign(const std::string &name, const Value &value) {
        for (auto current = environment_; current; current = current->parent)
            if (current->values.count(name)) {
                current->values[name] = value;
                return;
            }
        environment_->values[name] = value;
    }
    Value literal_value(const LiteralExpression &literal) const {
        switch (literal.kind) {
        case LiteralExpression::Kind::Number:
            return Value(std::get<double>(literal.value));
        case LiteralExpression::Kind::String:
            return Value(std::get<std::string>(literal.value));
        case LiteralExpression::Kind::Boolean:
            return Value(std::get<bool>(literal.value));
        default:
            return Value();
        }
    }
    Value binary_value(const BinaryExpression &binary) {
        if (binary.op == TokenKind::And) {
            Value left = expression_value(*binary.left);
            return Value(left.truthy() &&
                         expression_value(*binary.right).truthy());
        }
        if (binary.op == TokenKind::Or) {
            Value left = expression_value(*binary.left);
            return Value(left.truthy() ||
                         expression_value(*binary.right).truthy());
        }
        Value left = expression_value(*binary.left),
              right = expression_value(*binary.right);
        if (binary.op == TokenKind::EqualEqual ||
            binary.op == TokenKind::NotEqual)
            return Value(binary.op == TokenKind::EqualEqual
                             ? equal_values(left, right)
                             : !equal_values(left, right));
        if (binary.op == TokenKind::Less || binary.op == TokenKind::LessEqual ||
            binary.op == TokenKind::Greater ||
            binary.op == TokenKind::GreaterEqual) {
            if (left.kind() != Value::Kind::Number ||
                right.kind() != Value::Kind::Number)
                fail(RuntimeErrorCategory::TypeError,
                     "ordering requires numbers", binary.location);
            double a = left.as_number(), b = right.as_number();
            bool result = binary.op == TokenKind::Less        ? a < b
                          : binary.op == TokenKind::LessEqual ? a <= b
                          : binary.op == TokenKind::Greater   ? a > b
                                                              : a >= b;
            return Value(result);
        }
        if (binary.op == TokenKind::Plus &&
            left.kind() == Value::Kind::String &&
            right.kind() == Value::Kind::String)
            return Value(left.as_string() + right.as_string());
        if (left.kind() != Value::Kind::Number ||
            right.kind() != Value::Kind::Number)
            fail(RuntimeErrorCategory::TypeError, "arithmetic requires numbers",
                 binary.location);
        double a = left.as_number(), b = right.as_number();
        if ((binary.op == TokenKind::Slash ||
             binary.op == TokenKind::Percent) &&
            b == 0)
            fail(RuntimeErrorCategory::DivisionByZero, "division by zero",
                 binary.location);
        if (binary.op == TokenKind::Plus)
            return Value(a + b);
        if (binary.op == TokenKind::Minus)
            return Value(a - b);
        if (binary.op == TokenKind::Star)
            return Value(a * b);
        if (binary.op == TokenKind::Slash)
            return Value(a / b);
        return Value(std::fmod(a, b));
    }
    Value indexed(Value &object, const Value &position,
                  SourceLocation location) {
        if (object.kind() == Value::Kind::Extension) {
            if (!object.as_extension()->supports_indexing())
                fail(RuntimeErrorCategory::InvalidIndex,
                     "value does not support indexing", location);
            return object.as_extension()->get_index(position, location);
        }
        if (object.kind() != Value::Kind::Array ||
            position.kind() != Value::Kind::Number)
            fail(RuntimeErrorCategory::InvalidIndex,
                 "array index must be a number", location);
        double raw = position.as_number();
        if (!std::isfinite(raw) || std::floor(raw) != raw || raw < 0 ||
            static_cast<std::size_t>(raw) >= object.as_array().size())
            fail(RuntimeErrorCategory::InvalidIndex, "invalid array index",
                 location);
        return object.as_array()[static_cast<std::size_t>(raw)];
    }
    void set_index(Value &object, const Value &position, const Value &value,
                   SourceLocation location) {
        if (object.kind() == Value::Kind::Extension) {
            if (!object.as_extension()->supports_indexing())
                fail(RuntimeErrorCategory::InvalidIndex,
                     "value does not support indexing", location);
            object.as_extension()->set_index(position, value, location);
            return;
        }
        if (object.kind() != Value::Kind::Array ||
            position.kind() != Value::Kind::Number)
            fail(RuntimeErrorCategory::InvalidIndex,
                 "array index must be a number", location);
        double raw = position.as_number();
        if (!std::isfinite(raw) || std::floor(raw) != raw || raw < 0 ||
            static_cast<std::size_t>(raw) >= object.as_array().size())
            fail(RuntimeErrorCategory::InvalidIndex, "invalid array index",
                 location);
        object.as_array()[static_cast<std::size_t>(raw)] = value;
    }
};

bool equal_values(const Value &left, const Value &right) {
    if (left.kind() != right.kind())
        return false;
    switch (left.kind()) {
    case Value::Kind::Null:
        return true;
    case Value::Kind::Number:
        return left.as_number() == right.as_number();
    case Value::Kind::String:
        return left.as_string() == right.as_string();
    case Value::Kind::Boolean:
        return left.as_boolean() == right.as_boolean();
    case Value::Kind::Function:
        return left.as_callable() == right.as_callable();
    case Value::Kind::Extension:
        return left.as_extension()->type_name() ==
                   right.as_extension()->type_name() &&
               left.as_extension()->equals(*right.as_extension());
    case Value::Kind::Array:
        if (left.as_array().size() != right.as_array().size())
            return false;
        for (std::size_t i = 0; i < left.as_array().size(); ++i)
            if (!equal_values(left.as_array()[i], right.as_array()[i]))
                return false;
        return true;
    }
    return false;
}

} // namespace

Value::Value() : value_(std::monostate{}) {}
Value::Value(double value) : value_(value) {}
Value::Value(std::string value) : value_(std::move(value)) {}
Value::Value(bool value) : value_(value) {}
Value::Value(Array value) : value_(std::make_shared<Array>(std::move(value))) {}
Value::Value(CallablePtr value) : value_(std::move(value)) {}
Value::Value(ExtensionValuePtr value) : value_(std::move(value)) {}
Value::Kind Value::kind() const noexcept {
    return static_cast<Kind>(value_.index());
}
bool Value::is_null() const noexcept { return kind() == Kind::Null; }
double Value::as_number() const { return std::get<double>(value_); }
const std::string &Value::as_string() const {
    return std::get<std::string>(value_);
}
bool Value::as_boolean() const { return std::get<bool>(value_); }
const Array &Value::as_array() const { return *std::get<ArrayPtr>(value_); }
Array &Value::as_array() { return *std::get<ArrayPtr>(value_); }
const Value::CallablePtr &Value::as_callable() const {
    return std::get<CallablePtr>(value_);
}
const Value::ExtensionValuePtr &Value::as_extension() const {
    return std::get<ExtensionValuePtr>(value_);
}
bool Value::truthy() const noexcept {
    return kind() != Kind::Null && !(kind() == Kind::Boolean && !as_boolean());
}
std::string Value::to_string() const {
    std::ostringstream out;
    switch (kind()) {
    case Kind::Null:
        return "null";
    case Kind::Number:
        out << std::setprecision(15) << as_number();
        return out.str();
    case Kind::String:
        return as_string();
    case Kind::Boolean:
        return as_boolean() ? "true" : "false";
    case Kind::Function:
        return "<function>";
    case Kind::Extension:
        return as_extension()->to_string();
    case Kind::Array:
        out << "[";
        for (std::size_t i = 0; i < as_array().size(); ++i) {
            if (i)
                out << ", ";
            out << as_array()[i].to_string();
        }
        out << "]";
        return out.str();
    }
    return "null";
}

RuntimeError::RuntimeError(RuntimeErrorCategory category, std::string message,
                           std::size_t line, std::size_t column)
    : std::runtime_error(diagnostic(std::move(message), line, column)),
      category_(category), line_(line), column_(column) {}

struct Evaluator::State {
    std::shared_ptr<Environment> globals;
    std::ostream *output;
    std::unordered_map<std::string, Value> modules;
    std::vector<std::shared_ptr<Program>> repl_programs;
};

Evaluator::Evaluator(std::ostream &output)
    : Evaluator(output, ExtensionRegistry{}) {}

Evaluator::Evaluator(std::ostream &output, const ExtensionRegistry &extensions)
    : state_(std::make_shared<State>()) {
    state_->globals = std::make_shared<Environment>();
    state_->output = &output;
    auto add = [&](const std::string &name, std::size_t arity, auto fn) {
        state_->globals->values[name] =
            Value(std::make_shared<NativeCallable>(arity, fn));
    };
    add("print", 1, [this](const std::vector<Value> &a, SourceLocation) {
        *state_->output << a[0].to_string() << '\n';
        return Value();
    });
    add("length", 1, [](const std::vector<Value> &a, SourceLocation l) {
        if (a[0].kind() == Value::Kind::String)
            return Value(static_cast<double>(a[0].as_string().size()));
        if (a[0].kind() == Value::Kind::Array)
            return Value(static_cast<double>(a[0].as_array().size()));
        throw RuntimeError(RuntimeErrorCategory::TypeError,
                           "length requires a string or array", l.line,
                           l.column);
    });
    add("sum", 1, [](const std::vector<Value> &a, SourceLocation l) {
        if (a[0].kind() != Value::Kind::Array)
            throw RuntimeError(RuntimeErrorCategory::TypeError,
                               "sum requires an array", l.line, l.column);
        double result = 0;
        for (const auto &v : a[0].as_array()) {
            if (v.kind() != Value::Kind::Number)
                throw RuntimeError(RuntimeErrorCategory::TypeError,
                                   "sum requires numeric elements", l.line,
                                   l.column);
            result += v.as_number();
        }
        return Value(result);
    });
    add("min", 1, [](const std::vector<Value> &a, SourceLocation l) {
        if (a[0].kind() != Value::Kind::Array || a[0].as_array().empty())
            throw RuntimeError(RuntimeErrorCategory::InvalidArgument,
                               "min requires a non-empty array", l.line,
                               l.column);
        double r = std::numeric_limits<double>::infinity();
        for (const auto &v : a[0].as_array()) {
            if (v.kind() != Value::Kind::Number)
                throw RuntimeError(RuntimeErrorCategory::TypeError,
                                   "min requires numeric elements", l.line,
                                   l.column);
            r = std::min(r, v.as_number());
        }
        return Value(r);
    });
    add("max", 1, [](const std::vector<Value> &a, SourceLocation l) {
        if (a[0].kind() != Value::Kind::Array || a[0].as_array().empty())
            throw RuntimeError(RuntimeErrorCategory::InvalidArgument,
                               "max requires a non-empty array", l.line,
                               l.column);
        double r = -std::numeric_limits<double>::infinity();
        for (const auto &v : a[0].as_array()) {
            if (v.kind() != Value::Kind::Number)
                throw RuntimeError(RuntimeErrorCategory::TypeError,
                                   "max requires numeric elements", l.line,
                                   l.column);
            r = std::max(r, v.as_number());
        }
        return Value(r);
    });
    add("abs", 1, [](const std::vector<Value> &a, SourceLocation l) {
        if (a[0].kind() != Value::Kind::Number)
            throw RuntimeError(RuntimeErrorCategory::TypeError,
                               "abs requires a number", l.line, l.column);
        return Value(std::fabs(a[0].as_number()));
    });
    add("sqrt", 1, [](const std::vector<Value> &a, SourceLocation l) {
        if (a[0].kind() != Value::Kind::Number || a[0].as_number() < 0)
            throw RuntimeError(RuntimeErrorCategory::InvalidArgument,
                               "sqrt requires a non-negative number", l.line,
                               l.column);
        return Value(std::sqrt(a[0].as_number()));
    });

    for (const auto &[name, function] : extensions.state_->functions) {
        add(name, function.arity, function.function);
    }

    for (const auto &[module_name, module] : extensions.state_->modules) {
        std::unordered_map<std::string, Value> exports;
        for (const auto &[name, function] : module.state_->functions) {
            exports.emplace(name, Value(std::make_shared<NativeCallable>(
                                      function.arity, function.function)));
        }
        state_->modules.emplace(
            module_name,
            Value(std::make_shared<ModuleValue>(std::move(exports))));
    }
}
Evaluator::Evaluator() : Evaluator(std::cout) {}
Value Evaluator::evaluate(const Program &program) {
    return Runner(*state_->output, state_->globals, &state_->modules)
        .run(program);
}
std::optional<Value> Evaluator::evaluate_repl(Program program) {
    auto retained = std::make_shared<Program>(std::move(program));
    const auto result =
        Runner(*state_->output, state_->globals, &state_->modules)
            .run_repl(*retained);
    state_->repl_programs.push_back(std::move(retained));
    return result;
}
Value Evaluator::evaluate(std::string_view source) {
    return evaluate(parse(source));
}
const Value &Evaluator::lookup(const std::string &name) const {
    auto found = state_->globals->values.find(name);
    if (found == state_->globals->values.end())
        throw std::out_of_range("unknown global name");
    return found->second;
}

} // namespace abel
