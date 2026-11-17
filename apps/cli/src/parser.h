#ifndef ABEL_PARSER_H
#define ABEL_PARSER_H

#include "ast.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace abel {

enum class ParseErrorCategory { UnexpectedToken, InvalidAssignmentTarget };

class ParseError : public std::runtime_error {
  public:
    ParseError(ParseErrorCategory category, std::string message,
               std::size_t line, std::size_t column, bool incomplete = false);
    ParseErrorCategory category() const noexcept { return category_; }
    std::size_t line() const noexcept { return line_; }
    std::size_t column() const noexcept { return column_; }
    bool incomplete() const noexcept { return incomplete_; }

  private:
    ParseErrorCategory category_;
    std::size_t line_;
    std::size_t column_;
    bool incomplete_;
};

Program parse(const std::vector<Token> &tokens);
Program parse(std::string_view source);

} // namespace abel

#endif // ABEL_PARSER_H
