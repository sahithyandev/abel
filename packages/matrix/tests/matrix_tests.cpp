#include "abel/matrix.h"

#include <cstdlib>
#include <iostream>
#include <sstream>

namespace {

void require(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

} // namespace

int main() {
    abel::ExtensionRegistry extensions;
    abel::matrix::register_module(extensions);
    std::ostringstream output;
    abel::Evaluator evaluator(output, extensions);
    evaluator.evaluate(R"abel(
        import matrix
        A = matrix.from_rows([[1, 2], [3, 4]])
        B = matrix.zeros(2, 2)
        B[[0, 1]] = 7
        C = matrix.add(A, B)
        D = matrix.multiply(A, C)
        shape = matrix.rows(D) + matrix.columns(D)
        total = 0
        for row in D { total = total + sum(row) }
    )abel");
    require(evaluator.lookup("C").to_string() == "matrix([[1, 9], [3, 4]])",
            "matrix addition and formatting");
    require(evaluator.lookup("shape").as_number() == 4, "matrix dimensions");
    require(evaluator.lookup("total").as_number() == 82,
            "matrix iteration and multiplication");
    return EXIT_SUCCESS;
}
