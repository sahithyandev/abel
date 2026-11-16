# Abel

Abel is an early-stage, lightweight scripting language for numerical
computing. It is designed to be small, readable, and useful for experimenting
with numbers, arrays, and simple programs.

This repository is a monorepo containing the language CLI and its
documentation website.

## Try Abel

Create a file named `mean.abel`:

```abel
fn mean(values) {
    return sum(values) / length(values)
}

scores = [72, 81, 95]
average = mean(scores)
print(average)
```

Run it with the CLI:

```sh
./build/apps/cli/abel mean.abel
```

Output:

```text
82.6666666666667
```

The interpreter also supports an interactive REPL:

```text
$ ./build/apps/cli/abel
abel> values = [4, 8, 15]
abel> sum(values)
27
abel> sqrt(81)
9
```

## Built-in functions

The initial runtime includes these built-ins:

| Function | Description |
| --- | --- |
| `print(value)` | Writes a readable value and returns `null`. |
| `length(value)` | Returns the length of a string or array. |
| `sum(array)` | Returns the numeric sum of an array. |
| `min(array)` | Returns the smallest numeric element. |
| `max(array)` | Returns the largest numeric element. |
| `abs(number)` | Returns the absolute value of a number. |
| `sqrt(number)` | Returns the square root of a non-negative number. |

## Run source in other ways

Use `-` to read Abel source from standard input:

```sh
printf 'x = 6 * 7\nprint(x)\n' | ./build/apps/cli/abel -
```

Show the available CLI options:

```sh
./build/apps/cli/abel --help
./build/apps/cli/abel --version
```

Newlines end statements. Semicolons can be used when multiple statements need
to share a line, and `//` starts a comment. See the [language specification](apps/docs/src/pages/specification.md)
for the complete syntax reference.

## Repository layout

```text
apps/
  cli/       C++ interpreter, CLI, and tests
  wasm/      Emscripten browser adapter
  docs/      static documentation site and language spec source
packages/
  wasm/      private browser-runtime package consumed by the docs site
```

The [Abel Language Specification](apps/docs/src/pages/specification.md) is
the source of truth for the v0.1 language contract.

## Build the WebAssembly runtime

The browser runtime uses the same tokenizer, parser, and evaluator as the CLI.
Install the Emscripten SDK and activate its environment, then run:

```sh
bun --cwd packages/wasm run build
```

This writes `abel.js` and `abel.wasm` to `packages/wasm/dist/`. The docs build
copies those files into its static assets:

```sh
cd apps/docs
bun install
bun run build
```

The online playground is available at `/playground/` in the generated site.

## Build the CLI

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Run Abel with:

```sh
./build/apps/cli/abel program.abel
```

With no file argument, Abel starts an interactive REPL. Use `-` to read source
from standard input.

## Run the documentation site

The documentation site is an Astro static site and is ready for Vercel. From
the docs workspace:

```sh
cd apps/docs
bun install
bun run dev
```

Build the same static output Vercel deploys with `bun run build`; Astro writes
it to `apps/docs/dist/`. The included `vercel.json` configures this workspace
for Vercel's Astro build detection. When creating the Vercel project, set its
Root Directory to `apps/docs`.
