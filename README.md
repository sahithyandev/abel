# Abel

Abel is an early-stage project exploring a simpler, lightweight alternative to
MATLAB and Mathematica for numerical computing.

The first language contract is documented in the [Abel Language Specification](docs/abel-language-spec.md).

## Current decision

- **C++** will be used to build Abel.

The rest of the product scope, architecture, language design, libraries, user
interface, and development tools are still under consideration.

## Project status

This project is currently in the exploration and planning stage.

## Building

The project currently uses CMake for its minimal build setup.

```sh
cmake -S . -B build
cmake --build build
./build/abel
```
