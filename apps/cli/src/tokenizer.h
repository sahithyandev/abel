#ifndef ABEL_TOKENIZER_H
#define ABEL_TOKENIZER_H

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace abel {

enum class TokenKind {
    Identifier,
    Number,
    String,

    And,
    Else,
    False,
    For,
    Fn,
    If,
    In,
    Not,
    Null,
    Or,
    Return,
    True,
    While,

    StatementEnd,
    EndOfFile,

    LeftParen,
    RightParen,
    LeftBracket,
    RightBracket,
    LeftBrace,
    RightBrace,
    Comma,

    Assign,
    Minus,
    Plus,
    Star,
    Slash,
    Percent,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    EqualEqual,
    NotEqual,
};

struct Token {
    TokenKind kind;
    std::string lexeme;
    std::size_t line;
    std::size_t column;
};

enum class LexErrorCategory {
    InvalidCharacter,
    InvalidNumber,
    InvalidEscape,
    UnterminatedString,
};

class LexError : public std::runtime_error {
  public:
    LexError(LexErrorCategory category, std::string message, std::size_t line,
             std::size_t column);

    LexErrorCategory category() const noexcept { return category_; }
    std::size_t line() const noexcept { return line_; }
    std::size_t column() const noexcept { return column_; }

  private:
    LexErrorCategory category_;
    std::size_t line_;
    std::size_t column_;
};

std::vector<Token> tokenize(std::string_view source);

} // namespace abel

#endif // ABEL_TOKENIZER_H
