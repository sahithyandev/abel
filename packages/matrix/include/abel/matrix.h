#ifndef ABEL_MATRIX_H
#define ABEL_MATRIX_H

#include "evaluator.h"

namespace abel::matrix {

void register_module(ExtensionRegistry &extensions,
                     std::string module_name = "matrix");

} // namespace abel::matrix

#endif // ABEL_MATRIX_H
