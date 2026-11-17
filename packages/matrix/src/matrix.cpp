#include "abel/matrix.h"

#include <cmath>
#include <sstream>
#include <utility>

namespace abel::matrix {
namespace {

class MatrixValue final : public ExtensionValue {
  public:
    MatrixValue(std::size_t rows, std::size_t columns, std::vector<double> data)
        : rows_(rows), columns_(columns), data_(std::move(data)) {}

    std::string type_name() const override { return "matrix"; }

    std::string to_string() const override {
        std::ostringstream output;
        output << "matrix([";
        for (std::size_t row = 0; row < rows_; ++row) {
            if (row)
                output << ", ";
            output << "[";
            for (std::size_t column = 0; column < columns_; ++column) {
                if (column)
                    output << ", ";
                output << data_[offset(row, column)];
            }
            output << "]";
        }
        output << "])";
        return output.str();
    }

    bool equals(const ExtensionValue &other) const override {
        const auto *matrix = dynamic_cast<const MatrixValue *>(&other);
        return matrix && rows_ == matrix->rows_ &&
               columns_ == matrix->columns_ && data_ == matrix->data_;
    }

    bool supports_indexing() const override { return true; }

    Value get_index(const Value &index,
                    SourceLocation location) const override {
        const auto coordinates = coordinates_from(index, location);
        return Value(data_[offset(coordinates.first, coordinates.second)]);
    }

    void set_index(const Value &index, const Value &value,
                   SourceLocation location) override {
        if (value.kind() != Value::Kind::Number)
            fail(RuntimeErrorCategory::TypeError,
                 "matrix assignment requires a number", location);
        const auto coordinates = coordinates_from(index, location);
        data_[offset(coordinates.first, coordinates.second)] =
            value.as_number();
    }

    bool supports_length() const override { return true; }
    Value length(SourceLocation) const override {
        return Value(static_cast<double>(rows_));
    }

    bool supports_iteration() const override { return true; }
    std::vector<Value> iterate(SourceLocation) const override {
        std::vector<Value> result;
        result.reserve(rows_);
        for (std::size_t row = 0; row < rows_; ++row) {
            Array values;
            values.reserve(columns_);
            for (std::size_t column = 0; column < columns_; ++column)
                values.emplace_back(data_[offset(row, column)]);
            result.emplace_back(std::move(values));
        }
        return result;
    }

    std::size_t rows() const { return rows_; }
    std::size_t columns() const { return columns_; }
    double at(std::size_t row, std::size_t column) const {
        return data_[offset(row, column)];
    }

  private:
    static void fail(RuntimeErrorCategory category, const std::string &message,
                     SourceLocation location) {
        throw RuntimeError(category, message, location.line, location.column);
    }

    static std::size_t coordinate(const Value &value, SourceLocation location) {
        if (value.kind() != Value::Kind::Number ||
            !std::isfinite(value.as_number()) || value.as_number() < 0 ||
            std::floor(value.as_number()) != value.as_number())
            fail(RuntimeErrorCategory::InvalidIndex,
                 "matrix coordinates must be non-negative integers", location);
        return static_cast<std::size_t>(value.as_number());
    }

    std::pair<std::size_t, std::size_t>
    coordinates_from(const Value &value, SourceLocation location) const {
        if (value.kind() != Value::Kind::Array || value.as_array().size() != 2)
            fail(RuntimeErrorCategory::InvalidIndex,
                 "matrix index must be [row, column]", location);
        const auto row = coordinate(value.as_array()[0], location);
        const auto column = coordinate(value.as_array()[1], location);
        if (row >= rows_ || column >= columns_)
            fail(RuntimeErrorCategory::InvalidIndex,
                 "matrix index out of range", location);
        return {row, column};
    }

    std::size_t offset(std::size_t row, std::size_t column) const {
        return row * columns_ + column;
    }

    std::size_t rows_;
    std::size_t columns_;
    std::vector<double> data_;
};

const MatrixValue &matrix(const Value &value, SourceLocation location) {
    if (value.kind() != Value::Kind::Extension ||
        value.as_extension()->type_name() != "matrix")
        throw RuntimeError(RuntimeErrorCategory::TypeError, "expected a matrix",
                           location.line, location.column);
    return static_cast<const MatrixValue &>(*value.as_extension());
}

std::pair<std::size_t, std::size_t> dimensions(const Value &value,
                                               SourceLocation location) {
    const auto &value_matrix = matrix(value, location);
    return {value_matrix.rows(), value_matrix.columns()};
}

Value from_rows(const std::vector<Value> &arguments, SourceLocation location) {
    if (arguments[0].kind() != Value::Kind::Array)
        throw RuntimeError(RuntimeErrorCategory::TypeError,
                           "matrix.from_rows requires an array", location.line,
                           location.column);
    const auto &rows = arguments[0].as_array();
    const std::size_t columns =
        rows.empty()
            ? 0
            : (rows[0].kind() == Value::Kind::Array ? rows[0].as_array().size()
                                                    : 0);
    std::vector<double> data;
    data.reserve(rows.size() * columns);
    for (const auto &row : rows) {
        if (row.kind() != Value::Kind::Array ||
            row.as_array().size() != columns)
            throw RuntimeError(RuntimeErrorCategory::InvalidArgument,
                               "matrix rows must be rectangular", location.line,
                               location.column);
        for (const auto &value : row.as_array()) {
            if (value.kind() != Value::Kind::Number)
                throw RuntimeError(RuntimeErrorCategory::TypeError,
                                   "matrix elements must be numbers",
                                   location.line, location.column);
            data.push_back(value.as_number());
        }
    }
    return Value(
        std::make_shared<MatrixValue>(rows.size(), columns, std::move(data)));
}

std::size_t dimension_argument(const Value &value, SourceLocation location) {
    if (value.kind() != Value::Kind::Number || value.as_number() < 0 ||
        !std::isfinite(value.as_number()) ||
        std::floor(value.as_number()) != value.as_number())
        throw RuntimeError(RuntimeErrorCategory::InvalidArgument,
                           "matrix dimensions must be non-negative integers",
                           location.line, location.column);
    return static_cast<std::size_t>(value.as_number());
}

Value zeros(const std::vector<Value> &arguments, SourceLocation location) {
    const auto rows = dimension_argument(arguments[0], location);
    const auto columns = dimension_argument(arguments[1], location);
    return Value(std::make_shared<MatrixValue>(
        rows, columns, std::vector<double>(rows * columns, 0)));
}

Value rows(const std::vector<Value> &arguments, SourceLocation location) {
    return Value(static_cast<double>(dimensions(arguments[0], location).first));
}

Value columns(const std::vector<Value> &arguments, SourceLocation location) {
    return Value(
        static_cast<double>(dimensions(arguments[0], location).second));
}

Value add(const std::vector<Value> &arguments, SourceLocation location) {
    const auto &left = matrix(arguments[0], location);
    const auto &right = matrix(arguments[1], location);
    if (left.rows() != right.rows() || left.columns() != right.columns())
        throw RuntimeError(RuntimeErrorCategory::InvalidArgument,
                           "matrix.add requires equal dimensions",
                           location.line, location.column);
    std::vector<double> data;
    data.reserve(left.rows() * left.columns());
    for (std::size_t row = 0; row < left.rows(); ++row)
        for (std::size_t column = 0; column < left.columns(); ++column)
            data.push_back(left.at(row, column) + right.at(row, column));
    return Value(std::make_shared<MatrixValue>(left.rows(), left.columns(),
                                               std::move(data)));
}

Value multiply(const std::vector<Value> &arguments, SourceLocation location) {
    const auto &left = matrix(arguments[0], location);
    const auto &right = matrix(arguments[1], location);
    if (left.columns() != right.rows())
        throw RuntimeError(RuntimeErrorCategory::InvalidArgument,
                           "matrix.multiply dimensions do not match",
                           location.line, location.column);
    std::vector<double> data(left.rows() * right.columns(), 0);
    for (std::size_t row = 0; row < left.rows(); ++row)
        for (std::size_t column = 0; column < right.columns(); ++column)
            for (std::size_t inner = 0; inner < left.columns(); ++inner)
                data[row * right.columns() + column] +=
                    left.at(row, inner) * right.at(inner, column);
    return Value(std::make_shared<MatrixValue>(left.rows(), right.columns(),
                                               std::move(data)));
}

} // namespace

void register_module(ExtensionRegistry &extensions, std::string module_name) {
    ExtensionRegistry module;
    module.register_function("from_rows", 1, from_rows);
    module.register_function("zeros", 2, zeros);
    module.register_function("rows", 1, rows);
    module.register_function("columns", 1, columns);
    module.register_function("add", 2, add);
    module.register_function("multiply", 2, multiply);
    extensions.register_module(std::move(module_name), module);
}

} // namespace abel::matrix
