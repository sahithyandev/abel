#include "cli.h"

#include <iostream>

int main(int argc, char *argv[]) {
    return abel::run_cli(argc, argv, std::cin, std::cout, std::cerr);
}
