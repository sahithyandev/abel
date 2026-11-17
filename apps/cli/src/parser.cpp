#include "parser.h"

#include <cstdlib>
#include <sstream>
#include <utility>

namespace abel {
namespace {

std::string error_message(std::string message, std::size_t line,
                          std::size_t column) {
    std::ostringstream out;
    out << message << " at line " << line << ", column " << column;
    return out.str();
}

class Parser {
  public:
    explicit Parser(const std::vector<Token> &tokens) : tokens_(tokens) {}

    Program parse_program() {
        Program program;
        skip_separators();
        while (!check(TokenKind::EndOfFile)) {
            program.statements.push_back(parse_statement());
            if (!check(TokenKind::RightBrace) && !check(TokenKind::EndOfFile) &&
                !check(TokenKind::StatementEnd)) {
                fail("expected a statement separator");
            }
            skip_separators();
        }
        return program;
    }

  private:
    const std::vector<Token> &tokens_;
    std::size_t current_ = 0;

    const Token &peek() const { return tokens_[current_]; }
    const Token &previous() const { return tokens_[current_ - 1]; }
    bool check(TokenKind kind) const { return peek().kind == kind; }

    bool match(TokenKind kind) {
        if (!check(kind))
            return false;
        ++current_;
        return true;
    }

    const Token &consume(TokenKind kind, const char *message) {
        if (!match(kind))
            fail(message);
        return previous();
    }

    [[noreturn]] void fail(const std::string &message,
                           ParseErrorCategory category =
                               ParseErrorCategory::UnexpectedToken) const {
        throw ParseError(category, message, peek().line, peek().column,
                         check(TokenKind::EndOfFile));
    }

    void skip_separators() {
        while (match(TokenKind::StatementEnd)) {
        }
    }

    void skip_inner_separators() {
        while (match(TokenKind::StatementEnd)) {
        }
    }

    StatementPtr parse_statement() {
        if (match(TokenKind::Export))
            return std::make_unique<ExportStatement>(location(previous()),
                                                     parse_statement());
        if (match(TokenKind::Import))
            return parse_import(previous());
        if (match(TokenKind::LeftBrace))
            return parse_block_after_open(previous());
        if (match(TokenKind::If))
            return parse_if(previous());
        if (match(TokenKind::While))
            return parse_while(previous());
        if (match(TokenKind::For))
            return parse_for(previous());
        if (match(TokenKind::Fn))
            return parse_function(previous());
        if (match(TokenKind::Return))
            return parse_return(previous());
        return parse_expression_or_assignment();
    }

    std::unique_ptr<BlockStatement> parse_block() {
        const Token &open = consume(TokenKind::LeftBrace, "expected '{'");
        return parse_block_after_open(open);
    }

    std::unique_ptr<BlockStatement> parse_block_after_open(const Token &open) {
        auto block = std::make_unique<BlockStatement>(location(open));
        skip_separators();
        while (!check(TokenKind::RightBrace)) {
            if (check(TokenKind::EndOfFile))
                fail("unterminated block");
            block->statements.push_back(parse_statement());
            if (check(TokenKind::EndOfFile))
                fail("unterminated block");
            if (!check(TokenKind::RightBrace) &&
                !check(TokenKind::StatementEnd)) {
                fail("expected a statement separator");
            }
            skip_separators();
        }
        consume(TokenKind::RightBrace, "expected '}'");
        return block;
    }

    StatementPtr parse_if(const Token &start) {
        auto condition = parse_expression();
        auto then_branch = parse_block();
        auto statement = std::make_unique<IfStatement>(
            location(start), std::move(condition), std::move(then_branch));
        const std::size_t before_else = current_;
        skip_separators();
        if (!check(TokenKind::Else))
            current_ = before_else;
        if (match(TokenKind::Else)) {
            if (check(TokenKind::If))
                fail("expected '{' after 'else'");
            statement->else_branch = parse_block();
        }
        return statement;
    }

    StatementPtr parse_while(const Token &start) {
        auto condition = parse_expression();
        return std::make_unique<WhileStatement>(
            location(start), std::move(condition), parse_block());
    }

    StatementPtr parse_for(const Token &start) {
        const Token &variable = consume(TokenKind::Identifier,
                                        "expected loop variable after 'for'");
        consume(TokenKind::In, "expected 'in' after loop variable");
        auto iterable = parse_expression();
        return std::make_unique<ForStatement>(location(start), variable.lexeme,
                                              std::move(iterable),
                                              parse_block());
    }

    StatementPtr parse_function(const Token &start) {
        const Token &name =
            consume(TokenKind::Identifier, "expected function name after 'fn'");
        consume(TokenKind::LeftParen, "expected '(' after function name");
        std::vector<std::string> parameters;
        skip_inner_separators();
        if (!check(TokenKind::RightParen)) {
            do {
                parameters.push_back(
                    consume(TokenKind::Identifier, "expected parameter name")
                        .lexeme);
                skip_inner_separators();
            } while (match(TokenKind::Comma));
        }
        consume(TokenKind::RightParen, "expected ')' after parameters");
        return std::make_unique<FunctionStatement>(
            location(start), name.lexeme, std::move(parameters), parse_block());
    }

    StatementPtr parse_return(const Token &start) {
        if (check(TokenKind::StatementEnd) || check(TokenKind::RightBrace) ||
            check(TokenKind::EndOfFile)) {
            return std::make_unique<ReturnStatement>(location(start), nullptr);
        }
        return std::make_unique<ReturnStatement>(location(start),
                                                 parse_expression());
    }

    StatementPtr parse_import(const Token &start) {
        const Token &module = consume(TokenKind::Identifier,
                                      "expected module name after 'import'");
        return std::make_unique<ImportStatement>(location(start),
                                                 module.lexeme);
    }

    StatementPtr parse_expression_or_assignment() {
        auto expression = parse_expression();
        if (!match(TokenKind::Assign)) {
            return std::make_unique<ExpressionStatement>(expression->location,
                                                         std::move(expression));
        }
        if (dynamic_cast<VariableExpression *>(expression.get()) == nullptr &&
            dynamic_cast<IndexExpression *>(expression.get()) == nullptr) {
            fail("left side of assignment must be a name or array index",
                 ParseErrorCategory::InvalidAssignmentTarget);
        }
        auto value = parse_expression();
        return std::make_unique<AssignmentStatement>(
            expression->location, std::move(expression), std::move(value));
    }

    ExpressionPtr parse_expression() { return parse_or(); }

    ExpressionPtr parse_or() {
        auto expression = parse_and();
        while (match(TokenKind::Or))
            expression = binary(previous(), std::move(expression), parse_and());
        return expression;
    }

    ExpressionPtr parse_and() {
        auto expression = parse_equality();
        while (match(TokenKind::And))
            expression =
                binary(previous(), std::move(expression), parse_equality());
        return expression;
    }

    ExpressionPtr parse_equality() {
        auto expression = parse_comparison();
        while (check(TokenKind::EqualEqual) || check(TokenKind::NotEqual)) {
            const Token op = peek();
            ++current_;
            expression = binary(op, std::move(expression), parse_comparison());
        }
        return expression;
    }

    ExpressionPtr parse_comparison() {
        auto expression = parse_term();
        while (check(TokenKind::Less) || check(TokenKind::LessEqual) ||
               check(TokenKind::Greater) || check(TokenKind::GreaterEqual)) {
            const Token op = peek();
            ++current_;
            expression = binary(op, std::move(expression), parse_term());
        }
        return expression;
    }

    ExpressionPtr parse_term() {
        auto expression = parse_factor();
        while (check(TokenKind::Plus) || check(TokenKind::Minus)) {
            const Token op = peek();
            ++current_;
            expression = binary(op, std::move(expression), parse_factor());
        }
        return expression;
    }

    ExpressionPtr parse_factor() {
        auto expression = parse_unary();
        while (check(TokenKind::Star) || check(TokenKind::Slash) ||
               check(TokenKind::Percent)) {
            const Token op = peek();
            ++current_;
            expression = binary(op, std::move(expression), parse_unary());
        }
        return expression;
    }

    ExpressionPtr parse_unary() {
        if (match(TokenKind::Minus) || match(TokenKind::Not)) {
            const Token op = previous();
            return std::make_unique<UnaryExpression>(location(op), op.kind,
                                                     parse_unary());
        }
        return parse_postfix();
    }

    ExpressionPtr parse_postfix() {
        auto expression = parse_primary();
        for (;;) {
            if (match(TokenKind::LeftParen)) {
                auto call = std::make_unique<CallExpression>(
                    expression->location, std::move(expression));
                skip_inner_separators();
                if (!check(TokenKind::RightParen)) {
                    do {
                        call->arguments.push_back(parse_expression());
                        skip_inner_separators();
                    } while (match(TokenKind::Comma));
                }
                consume(TokenKind::RightParen, "expected ')' after arguments");
                expression = std::move(call);
            } else if (match(TokenKind::LeftBracket)) {
                auto index = parse_expression();
                consume(TokenKind::RightBracket, "expected ']' after index");
                expression = std::make_unique<IndexExpression>(
                    expression->location, std::move(expression),
                    std::move(index));
            } else if (match(TokenKind::Dot)) {
                const Token &member = consume(TokenKind::Identifier,
                                              "expected member name after '.'");
                expression = std::make_unique<MemberExpression>(
                    expression->location, std::move(expression), member.lexeme);
            } else {
                break;
            }
        }
        return expression;
    }

    ExpressionPtr parse_primary() {
        if (match(TokenKind::Number)) {
            return std::make_unique<LiteralExpression>(
                location(previous()), LiteralExpression::Kind::Number,
                std::strtod(previous().lexeme.c_str(), nullptr));
        }
        if (match(TokenKind::String)) {
            return std::make_unique<LiteralExpression>(
                location(previous()), LiteralExpression::Kind::String,
                decode_string(previous().lexeme));
        }
        if (match(TokenKind::True) || match(TokenKind::False)) {
            return std::make_unique<LiteralExpression>(
                location(previous()), LiteralExpression::Kind::Boolean,
                previous().kind == TokenKind::True);
        }
        if (match(TokenKind::Null)) {
            return std::make_unique<LiteralExpression>(
                location(previous()), LiteralExpression::Kind::Null,
                std::monostate{});
        }
        if (match(TokenKind::Identifier)) {
            return std::make_unique<VariableExpression>(location(previous()),
                                                        previous().lexeme);
        }
        if (match(TokenKind::LeftParen)) {
            auto expression = parse_expression();
            consume(TokenKind::RightParen, "expected ')' after expression");
            return expression;
        }
        if (match(TokenKind::LeftBracket)) {
            const Token open = previous();
            auto array = std::make_unique<ArrayExpression>(location(open));
            skip_inner_separators();
            if (!check(TokenKind::RightBracket)) {
                do {
                    array->elements.push_back(parse_expression());
                    skip_inner_separators();
                } while (match(TokenKind::Comma));
            }
            consume(TokenKind::RightBracket,
                    "expected ']' after array elements");
            return array;
        }
        fail("expected expression");
    }

    static SourceLocation location(const Token &token) {
        return {token.line, token.column};
    }

    static ExpressionPtr binary(const Token &op, ExpressionPtr left,
                                ExpressionPtr right) {
        return std::make_unique<BinaryExpression>(
            location(op), op.kind, std::move(left), std::move(right));
    }

    static std::string decode_string(const std::string &lexeme) {
        std::string result;
        for (std::size_t i = 1; i + 1 < lexeme.size(); ++i) {
            if (lexeme[i] != '\\')
                result += lexeme[i];
            else {
                const char escaped = lexeme[++i];
                result += escaped == 'n' ? '\n' : escaped;
            }
        }
        return result;
    }
};

} // namespace

ParseError::ParseError(ParseErrorCategory category, std::string message,
                       std::size_t line, std::size_t column, bool incomplete)
    : std::runtime_error(error_message(std::move(message), line, column)),
      category_(category), line_(line), column_(column),
      incomplete_(incomplete) {}

Program parse(const std::vector<Token> &tokens) {
    return Parser(tokens).parse_program();
}

Program parse(std::string_view source) {
    const auto tokens = tokenize(source);
    return parse(tokens);
}

} // namespace abel
