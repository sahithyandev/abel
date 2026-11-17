#ifndef ABEL_AST_H
#define ABEL_AST_H

#include "tokenizer.h"

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace abel {

struct SourceLocation {
    std::size_t line;
    std::size_t column;
};

struct Expression {
    explicit Expression(SourceLocation location) : location(location) {}
    virtual ~Expression() = default;
    SourceLocation location;
};
using ExpressionPtr = std::unique_ptr<Expression>;

struct LiteralExpression final : Expression {
    enum class Kind { Number, String, Boolean, Null };
    LiteralExpression(
        SourceLocation location, Kind kind,
        std::variant<double, std::string, bool, std::monostate> value)
        : Expression(location), kind(kind), value(std::move(value)) {}
    Kind kind;
    std::variant<double, std::string, bool, std::monostate> value;
};

struct VariableExpression final : Expression {
    VariableExpression(SourceLocation location, std::string name)
        : Expression(location), name(std::move(name)) {}
    std::string name;
};

struct MemberExpression final : Expression {
    MemberExpression(SourceLocation location, ExpressionPtr object,
                     std::string member)
        : Expression(location), object(std::move(object)),
          member(std::move(member)) {}
    ExpressionPtr object;
    std::string member;
};

struct ArrayExpression final : Expression {
    explicit ArrayExpression(SourceLocation location) : Expression(location) {}
    std::vector<ExpressionPtr> elements;
};

struct UnaryExpression final : Expression {
    UnaryExpression(SourceLocation location, TokenKind op,
                    ExpressionPtr operand)
        : Expression(location), op(op), operand(std::move(operand)) {}
    TokenKind op;
    ExpressionPtr operand;
};

struct BinaryExpression final : Expression {
    BinaryExpression(SourceLocation location, TokenKind op, ExpressionPtr left,
                     ExpressionPtr right)
        : Expression(location), op(op), left(std::move(left)),
          right(std::move(right)) {}
    TokenKind op;
    ExpressionPtr left;
    ExpressionPtr right;
};

struct CallExpression final : Expression {
    CallExpression(SourceLocation location, ExpressionPtr callee)
        : Expression(location), callee(std::move(callee)) {}
    ExpressionPtr callee;
    std::vector<ExpressionPtr> arguments;
};

struct IndexExpression final : Expression {
    IndexExpression(SourceLocation location, ExpressionPtr object,
                    ExpressionPtr index)
        : Expression(location), object(std::move(object)),
          index(std::move(index)) {}
    ExpressionPtr object;
    ExpressionPtr index;
};

struct Statement {
    explicit Statement(SourceLocation location) : location(location) {}
    virtual ~Statement() = default;
    SourceLocation location;
};
using StatementPtr = std::unique_ptr<Statement>;

struct ImportStatement final : Statement {
    ImportStatement(SourceLocation location, std::string module)
        : Statement(location), module(std::move(module)) {}
    std::string module;
};

struct BlockStatement final : Statement {
    explicit BlockStatement(SourceLocation location) : Statement(location) {}
    std::vector<StatementPtr> statements;
};

struct ExpressionStatement final : Statement {
    ExpressionStatement(SourceLocation location, ExpressionPtr expression)
        : Statement(location), expression(std::move(expression)) {}
    ExpressionPtr expression;
};

struct AssignmentStatement final : Statement {
    AssignmentStatement(SourceLocation location, ExpressionPtr target,
                        ExpressionPtr value)
        : Statement(location), target(std::move(target)),
          value(std::move(value)) {}
    ExpressionPtr target;
    ExpressionPtr value;
};

struct IfStatement final : Statement {
    IfStatement(SourceLocation location, ExpressionPtr condition,
                std::unique_ptr<BlockStatement> then_branch)
        : Statement(location), condition(std::move(condition)),
          then_branch(std::move(then_branch)) {}
    ExpressionPtr condition;
    std::unique_ptr<BlockStatement> then_branch;
    std::unique_ptr<BlockStatement> else_branch;
};

struct WhileStatement final : Statement {
    WhileStatement(SourceLocation location, ExpressionPtr condition,
                   std::unique_ptr<BlockStatement> body)
        : Statement(location), condition(std::move(condition)),
          body(std::move(body)) {}
    ExpressionPtr condition;
    std::unique_ptr<BlockStatement> body;
};

struct ForStatement final : Statement {
    ForStatement(SourceLocation location, std::string variable,
                 ExpressionPtr iterable, std::unique_ptr<BlockStatement> body)
        : Statement(location), variable(std::move(variable)),
          iterable(std::move(iterable)), body(std::move(body)) {}
    std::string variable;
    ExpressionPtr iterable;
    std::unique_ptr<BlockStatement> body;
};

struct FunctionStatement final : Statement {
    FunctionStatement(SourceLocation location, std::string name,
                      std::vector<std::string> parameters,
                      std::unique_ptr<BlockStatement> body)
        : Statement(location), name(std::move(name)),
          parameters(std::move(parameters)), body(std::move(body)) {}
    std::string name;
    std::vector<std::string> parameters;
    std::unique_ptr<BlockStatement> body;
};

struct ReturnStatement final : Statement {
    ReturnStatement(SourceLocation location, ExpressionPtr value)
        : Statement(location), value(std::move(value)) {}
    ExpressionPtr value;
};

struct Program {
    std::vector<StatementPtr> statements;
};

} // namespace abel

#endif // ABEL_AST_H
