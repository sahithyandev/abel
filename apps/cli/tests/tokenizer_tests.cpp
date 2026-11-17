#include "tokenizer.h"

#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <string>
#include <vector>

namespace {

using abel::LexError;
using abel::LexErrorCategory;
using abel::Token;
using abel::TokenKind;

void require(bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void require_kinds(const std::vector<Token> &tokens,
                   std::initializer_list<TokenKind> expected) {
    require(tokens.size() == expected.size(), "unexpected token count");
    std::size_t index = 0;
    for (TokenKind kind : expected) {
        require(tokens[index].kind == kind,
                "unexpected token kind at index " + std::to_string(index));
        ++index;
    }
}

template <typename Function>
void require_error(Function function, LexErrorCategory category,
                   std::size_t line, std::size_t column) {
    try {
        function();
    } catch (const LexError &error) {
        require(error.category() == category,
                "unexpected lexical error category");
        require(error.line() == line, "unexpected lexical error line");
        require(error.column() == column, "unexpected lexical error column");
        return;
    }
    require(false, "expected lexical error");
}

void test_keywords_and_identifiers() {
    const auto tokens = abel::tokenize(
        "and else false for fn if in not null or return true while value Value "
        "sample_2 _temporary print");
    require_kinds(tokens,
                  {TokenKind::And, TokenKind::Else, TokenKind::False,
                   TokenKind::For, TokenKind::Fn, TokenKind::If, TokenKind::In,
                   TokenKind::Not, TokenKind::Null, TokenKind::Or,
                   TokenKind::Return, TokenKind::True, TokenKind::While,
                   TokenKind::Identifier, TokenKind::Identifier,
                   TokenKind::Identifier, TokenKind::Identifier,
                   TokenKind::Identifier, TokenKind::EndOfFile});
    require(tokens[13].lexeme == "value",
            "identifier lexeme was not preserved");
    require(tokens[13].line == 1 && tokens[13].column == 59,
            "identifier location was not tracked");
}

void test_literals_and_separators() {
    const auto tokens = abel::tokenize(
        R"abel(42 3.14 0.5
"hello\nworld\\"; [])abel");
    require_kinds(tokens,
                  {TokenKind::Number, TokenKind::Number, TokenKind::Number,
                   TokenKind::StatementEnd, TokenKind::String,
                   TokenKind::StatementEnd, TokenKind::LeftBracket,
                   TokenKind::RightBracket, TokenKind::EndOfFile});
    require(tokens[3].lexeme == "\n", "newline lexeme was not preserved");
    require(tokens[4].lexeme == R"abel("hello\nworld\\")abel",
            "string lexeme was not preserved");
}

void test_operators_and_comments() {
    const auto tokens = abel::tokenize(
        "= - + * / % < <= > >= == != ( ) [ ] { } , // comment\r\nnext");
    require_kinds(tokens, {TokenKind::Assign,      TokenKind::Minus,
                           TokenKind::Plus,        TokenKind::Star,
                           TokenKind::Slash,       TokenKind::Percent,
                           TokenKind::Less,        TokenKind::LessEqual,
                           TokenKind::Greater,     TokenKind::GreaterEqual,
                           TokenKind::EqualEqual,  TokenKind::NotEqual,
                           TokenKind::LeftParen,   TokenKind::RightParen,
                           TokenKind::LeftBracket, TokenKind::RightBracket,
                           TokenKind::LeftBrace,   TokenKind::RightBrace,
                           TokenKind::Comma,       TokenKind::StatementEnd,
                           TokenKind::Identifier,  TokenKind::EndOfFile});
    require(tokens[19].lexeme == "\r\n",
            "CRLF was not one statement separator");
    require(tokens[20].line == 2 && tokens[20].column == 1,
            "token after comment has incorrect location");
}

void test_spec_snippet() {
    const auto tokens = abel::tokenize("fn mean(values) {\n"
                                       "  return sum(values) / length(values)\n"
                                       "}\n"
                                       "scores = [72, 81, 95]");
    require(tokens.front().kind == TokenKind::Fn,
            "snippet did not start with fn");
    require(tokens.back().kind == TokenKind::EndOfFile,
            "snippet did not end at EOF");
}

void test_errors() {
    require_error([] { (void)abel::tokenize("@"); },
                  LexErrorCategory::InvalidCharacter, 1, 1);
    require_error([] { (void)abel::tokenize("1e3"); },
                  LexErrorCategory::InvalidNumber, 1, 2);
    require_error([] { (void)abel::tokenize("5."); },
                  LexErrorCategory::InvalidNumber, 1, 2);
    require_error([] { (void)abel::tokenize("\"bad\\t\""); },
                  LexErrorCategory::InvalidEscape, 1, 5);
    require_error([] { (void)abel::tokenize("\"unterminated"); },
                  LexErrorCategory::UnterminatedString, 1, 1);
    require_error([] { (void)abel::tokenize("!"); },
                  LexErrorCategory::InvalidCharacter, 1, 1);
}

} // namespace

int main() {
    test_keywords_and_identifiers();
    test_literals_and_separators();
    test_operators_and_comments();
    test_spec_snippet();
    test_errors();
    std::cout << "Tokenizer tests passed\n";
    return EXIT_SUCCESS;
}
