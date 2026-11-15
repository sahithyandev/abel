#include "parser.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

using namespace abel;

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

template <typename T>
T& as(Expression& expression, const std::string& message) {
    auto* result = dynamic_cast<T*>(&expression);
    require(result != nullptr, message);
    return *result;
}

template <typename T>
T& as(Statement& statement, const std::string& message) {
    auto* result = dynamic_cast<T*>(&statement);
    require(result != nullptr, message);
    return *result;
}

void test_literals_and_collections() {
    const Program program = parse(R"abel(
        number = 3.14
        text = "hello\nworld"
        yes = true
        no = false
        nothing = null
        values = [1, "two", false, null]
    )abel");
    require(program.statements.size() == 6, "literal program statement count");
    auto& number = as<AssignmentStatement>(*program.statements[0], "number assignment");
    auto& number_value = as<LiteralExpression>(*number.value, "number literal");
    require(number_value.kind == LiteralExpression::Kind::Number &&
                std::get<double>(number_value.value) == 3.14,
            "number literal value");
    auto& text = as<LiteralExpression>(*as<AssignmentStatement>(*program.statements[1],
                                                               "text assignment").value,
                                       "string literal");
    require(std::get<std::string>(text.value) == "hello\nworld", "string decoding");
    auto& array = as<ArrayExpression>(*as<AssignmentStatement>(*program.statements[5],
                                                               "array assignment").value,
                                      "array literal");
    require(array.elements.size() == 4, "array element count");
}

void test_precedence_and_postfix() {
    const Program program = parse("result = (2 + 3) * -values[1] + fnc(1, 2)[0]");
    auto& assignment = as<AssignmentStatement>(*program.statements[0], "precedence assignment");
    auto& plus = as<BinaryExpression>(*assignment.value, "top-level plus");
    require(plus.op == TokenKind::Plus, "top-level operator");
    auto& multiply = as<BinaryExpression>(*plus.left, "multiplication precedence");
    require(multiply.op == TokenKind::Star, "multiplication operator");
    auto& unary = as<UnaryExpression>(*multiply.right, "unary precedence");
    require(unary.op == TokenKind::Minus, "unary minus operator");
    auto& index = as<IndexExpression>(*unary.operand, "index postfix");
    as<VariableExpression>(*index.object, "indexed variable");
    auto& right_index = as<IndexExpression>(*plus.right, "call then index");
    as<CallExpression>(*right_index.object, "call expression");
}

void test_statements() {
    const Program program = parse(R"abel(
        fn mean(values) {
            return sum(values) / length(values)
        }
        if score >= 50 {
            print("pass")
        }
        else {
            print("retry")
        }
        while count < 3 { count = count + 1 }
        for value in [10, 20] { print(value) }
        { answer = 42 }
        return
        after = 1
    )abel");
    require(program.statements.size() == 7, "statement form count");
    auto& function = as<FunctionStatement>(*program.statements[0], "function statement");
    require(function.name == "mean" && function.parameters.size() == 1 &&
                function.body->statements.size() == 1,
            "function declaration shape");
    as<IfStatement>(*program.statements[1], "if statement");
    as<WhileStatement>(*program.statements[2], "while statement");
    auto& loop = as<ForStatement>(*program.statements[3], "for statement");
    require(loop.variable == "value", "for variable");
    as<BlockStatement>(*program.statements[4], "standalone block");
    auto& returned = as<ReturnStatement>(*program.statements[5], "return statement");
    require(returned.value == nullptr, "bare return");
    as<AssignmentStatement>(*program.statements[6], "statement after return");
}

void test_assignments_and_separators() {
    const Program program = parse("x = 1; y = 2\nvalues[0] = x + y");
    require(program.statements.size() == 3, "semicolon and newline separators");
    auto& indexed = as<AssignmentStatement>(*program.statements[2], "indexed assignment");
    as<IndexExpression>(*indexed.target, "indexed assignment target");
}

template <typename Function>
void require_error(Function function, ParseErrorCategory category,
                   std::size_t line, std::size_t column) {
    try {
        function();
    } catch (const ParseError& error) {
        require(error.category() == category, "unexpected parse error category");
        require(error.line() == line, "unexpected parse error line");
        require(error.column() == column, "unexpected parse error column");
        return;
    }
    require(false, "expected parse error");
}

void test_errors() {
    require_error([] { (void)parse("x ="); }, ParseErrorCategory::UnexpectedToken, 1, 4);
    require_error([] { (void)parse("if true { x = 1"); }, ParseErrorCategory::UnexpectedToken,
                   1, 16);
    require_error([] { (void)parse("for value [1] {}"); }, ParseErrorCategory::UnexpectedToken,
                   1, 11);
    require_error([] { (void)parse("1 = 2"); }, ParseErrorCategory::InvalidAssignmentTarget,
                   1, 5);
    require_error([] { (void)parse("fn f(a a) {}"); }, ParseErrorCategory::UnexpectedToken,
                   1, 8);
    require_error([] { (void)parse("print(1 2)"); }, ParseErrorCategory::UnexpectedToken,
                   1, 9);
    require_error([] { (void)parse("[1, 2"); }, ParseErrorCategory::UnexpectedToken,
                   1, 6);
}

}  // namespace

int main() {
    test_literals_and_collections();
    test_precedence_and_postfix();
    test_statements();
    test_assignments_and_separators();
    test_errors();
    std::cout << "Parser tests passed\n";
    return EXIT_SUCCESS;
}
