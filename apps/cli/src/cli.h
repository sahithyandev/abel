#ifndef ABEL_CLI_H
#define ABEL_CLI_H

#include <iosfwd>

namespace abel {

int run_cli(int argc, char* argv[], std::istream& input, std::ostream& output,
            std::ostream& error);

}  // namespace abel

#endif  // ABEL_CLI_H
