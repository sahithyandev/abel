# Abel

Abel is an early-stage, lightweight scripting language for numerical
computing. This repository is a monorepo containing the language CLI and its
documentation website.

## Repository layout

```text
apps/
  cli/       C++ interpreter, CLI, and tests
  docs/      static documentation site and language spec source
```

The [Abel Language Specification](apps/docs/content/abel-language-spec.md) is
the source of truth for the v0.1 language contract.

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
