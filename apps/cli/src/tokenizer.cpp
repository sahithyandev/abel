#include "tokenizer.h"

#include <cctype>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace abel {
namespace {

bool is_digit(char character) { return character >= '0' && character <= '9'; }

bool is_identifier_start(char character) {
    const unsigned char value = static_cast<unsigned char>(character);
    return std::isalpha(value) != 0 || character == '_';
}

bool is_identifier_part(char character) {
    const unsigned char value = static_cast<unsigned char>(character);
    return std::isalnum(value) != 0 || character == '_';
}

TokenKind keyword_kind(std::string_view word) {
    static const std::unordered_map<std::string_view, TokenKind> keywords = {
        {"and", TokenKind::And},       {"else", TokenKind::Else},
        {"export", TokenKind::Export}, {"false", TokenKind::False},
        {"for", TokenKind::For},       {"fn", TokenKind::Fn},
        {"if", TokenKind::If},         {"in", TokenKind::In},
        {"import", TokenKind::Import}, {"not", TokenKind::Not},
        {"null", TokenKind::Null},     {"or", TokenKind::Or},
        {"return", TokenKind::Return}, {"true", TokenKind::True},
        {"while", TokenKind::While},
    };

    const auto found = keywords.find(word);
    return found == keywords.end() ? TokenKind::Identifier : found->second;
}

std::string position_message(std::string message, std::size_t line,
                             std::size_t column) {
    std::ostringstream output;
    output << message << " at line " << line << ", column " << column;
    return output.str();
}

} // namespace

LexError::LexError(LexErrorCategory category, std::string message,
                   std::size_t line, std::size_t column)
    : std::runtime_error(position_message(std::move(message), line, column)),
      category_(category), line_(line), column_(column) {}

std::vector<Token> tokenize(std::string_view source) {
    std::vector<Token> tokens;
    std::size_t index = 0;
    std::size_t line = 1;
    std::size_t column = 1;

    auto advance = [&](char character) {
        ++index;
        ++column;
        if (character == '\n') {
            ++line;
            column = 1;
        }
    };

    auto add_token = [&](TokenKind kind, std::size_t start,
                         std::size_t start_line, std::size_t start_column) {
        tokens.push_back({kind,
                          std::string(source.substr(start, index - start)),
                          start_line, start_column});
    };

    while (index < source.size()) {
        const char character = source[index];

        if (character == ' ' || character == '\t' || character == '\f' ||
            character == '\v') {
            advance(character);
            continue;
        }

        if (character == '\r' || character == '\n') {
            const std::size_t start = index;
            const std::size_t start_line = line;
            const std::size_t start_column = column;
            if (character == '\r' && index + 1 < source.size() &&
                source[index + 1] == '\n') {
                advance('\r');
                advance('\n');
            } else {
                advance(character == '\r' ? '\n' : character);
            }
            tokens.push_back({TokenKind::StatementEnd,
                              std::string(source.substr(start, index - start)),
                              start_line, start_column});
            continue;
        }

        if (character == '/' && index + 1 < source.size() &&
            source[index + 1] == '/') {
            advance('/');
            advance('/');
            while (index < source.size() && source[index] != '\r' &&
                   source[index] != '\n') {
                advance(source[index]);
            }
            continue;
        }

        const std::size_t start = index;
        const std::size_t start_line = line;
        const std::size_t start_column = column;

        if (character == ';') {
            advance(character);
            add_token(TokenKind::StatementEnd, start, start_line, start_column);
            continue;
        }

        if (is_identifier_start(character)) {
            advance(character);
            while (index < source.size() && is_identifier_part(source[index])) {
                advance(source[index]);
            }
            const std::string_view word = source.substr(start, index - start);
            add_token(keyword_kind(word), start, start_line, start_column);
            continue;
        }

        if (is_digit(character)) {
            advance(character);
            while (index < source.size() && is_digit(source[index])) {
                advance(source[index]);
            }
            if (index < source.size() && source[index] == '.') {
                advance('.');
                if (index >= source.size() || !is_digit(source[index])) {
                    throw LexError(LexErrorCategory::InvalidNumber,
                                   "decimal point must be followed by digits",
                                   line, column - 1);
                }
                while (index < source.size() && is_digit(source[index])) {
                    advance(source[index]);
                }
            }
            if (index < source.size() &&
                (is_identifier_start(source[index]) || source[index] == '.')) {
                throw LexError(LexErrorCategory::InvalidNumber,
                               "invalid numeric literal", line, column);
            }
            add_token(TokenKind::Number, start, start_line, start_column);
            continue;
        }

        if (character == '"') {
            advance(character);
            bool closed = false;
            while (index < source.size()) {
                const char current = source[index];
                if (current == '"') {
                    advance(current);
                    closed = true;
                    break;
                }
                if (current == '\r' || current == '\n') {
                    throw LexError(LexErrorCategory::UnterminatedString,
                                   "unterminated string literal", line, column);
                }
                if (current == '\\') {
                    const std::size_t escape_line = line;
                    const std::size_t escape_column = column;
                    advance(current);
                    if (index >= source.size()) {
                        throw LexError(LexErrorCategory::UnterminatedString,
                                       "unterminated string literal",
                                       escape_line, escape_column);
                    }
                    const char escaped = source[index];
                    if (escaped != '\\' && escaped != '"' && escaped != 'n') {
                        throw LexError(LexErrorCategory::InvalidEscape,
                                       "unsupported string escape", escape_line,
                                       escape_column);
                    }
                    advance(escaped);
                    continue;
                }
                advance(current);
            }
            if (!closed) {
                throw LexError(LexErrorCategory::UnterminatedString,
                               "unterminated string literal", start_line,
                               start_column);
            }
            add_token(TokenKind::String, start, start_line, start_column);
            continue;
        }

        TokenKind kind;
        std::size_t length = 1;
        switch (character) {
        case '(':
            kind = TokenKind::LeftParen;
            break;
        case ')':
            kind = TokenKind::RightParen;
            break;
        case '[':
            kind = TokenKind::LeftBracket;
            break;
        case ']':
            kind = TokenKind::RightBracket;
            break;
        case '{':
            kind = TokenKind::LeftBrace;
            break;
        case '}':
            kind = TokenKind::RightBrace;
            break;
        case ',':
            kind = TokenKind::Comma;
            break;
        case '.':
            kind = TokenKind::Dot;
            break;
        case '=':
            kind = TokenKind::Assign;
            if (index + 1 < source.size() && source[index + 1] == '=') {
                kind = TokenKind::EqualEqual;
                length = 2;
            }
            break;
        case '-':
            kind = TokenKind::Minus;
            break;
        case '+':
            kind = TokenKind::Plus;
            break;
        case '*':
            kind = TokenKind::Star;
            break;
        case '/':
            kind = TokenKind::Slash;
            break;
        case '%':
            kind = TokenKind::Percent;
            break;
        case '<':
            kind = TokenKind::Less;
            if (index + 1 < source.size() && source[index + 1] == '=') {
                kind = TokenKind::LessEqual;
                length = 2;
            }
            break;
        case '>':
            kind = TokenKind::Greater;
            if (index + 1 < source.size() && source[index + 1] == '=') {
                kind = TokenKind::GreaterEqual;
                length = 2;
            }
            break;
        case '!':
            if (index + 1 < source.size() && source[index + 1] == '=') {
                kind = TokenKind::NotEqual;
                length = 2;
                break;
            }
            throw LexError(LexErrorCategory::InvalidCharacter,
                           "unexpected '!'; expected '!='", line, column);
        default:
            throw LexError(LexErrorCategory::InvalidCharacter,
                           "unexpected character", line, column);
        }

        for (std::size_t offset = 0; offset < length; ++offset) {
            advance(source[index]);
        }
        add_token(kind, start, start_line, start_column);
    }

    tokens.push_back({TokenKind::EndOfFile, "", line, column});
    return tokens;
}

} // namespace abel
