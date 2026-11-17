BUILD_DIR ?= build
CMAKE ?= cmake
BUN ?= bun
CLANG_FORMAT ?= clang-format
RUSTFMT ?= rustfmt

.DEFAULT_GOAL := help

.PHONY: help configure cli build test wasm docs docs-dev fmt run clean setup-hooks

help:
	@printf '%s\n' \
		'Abel development commands:' \
		'  make cli       Configure and compile the native CLI' \
		'  make test      Compile and run the C++ test suite' \
		'  make fmt       Format all tracked source files' \
		'  make wasm      Build the WebAssembly runtime' \
		'  make docs      Build the documentation site' \
		'  make docs-dev  Start the documentation site locally' \
		'  make run ARGS="file.abel"  Run the compiled CLI' \
		'  make setup-hooks Enable the repository Git hooks' \
		'  make clean     Remove native build outputs'

setup-hooks:
	./scripts/setup-git-hooks.sh

fmt:
	@if ! command -v "$(CLANG_FORMAT)" >/dev/null 2>&1; then \
		echo "clang-format is required to format C/C++ files" >&2; \
		exit 1; \
	fi
	@if ! command -v "$(RUSTFMT)" >/dev/null 2>&1; then \
		echo "rustfmt is required to format Rust files" >&2; \
		exit 1; \
	fi
	$(BUN) fmt .
	@git ls-files -- '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' '*.hxx' | \
	while IFS= read -r file; do \
		[ -n "$$file" ] || continue; \
		"$(CLANG_FORMAT)" -i --style=file -- "$$file"; \
	done
	@git ls-files -- '*.rs' | \
	while IFS= read -r file; do \
		[ -n "$$file" ] || continue; \
		"$(RUSTFMT)" "$$file"; \
	done

configure:
	$(CMAKE) -S . -B $(BUILD_DIR)

cli: configure
	$(CMAKE) --build $(BUILD_DIR) --target abel

build: cli

test: configure
	$(CMAKE) --build $(BUILD_DIR)
	ctest --test-dir $(BUILD_DIR) --output-on-failure

wasm:
	$(BUN) --cwd packages/wasm run build

docs:
	$(BUN) run --cwd apps/docs build

docs-dev:
	$(BUN) run --cwd apps/docs dev

run: cli
	$(BUILD_DIR)/apps/cli/abel $(ARGS)

clean:
	$(CMAKE) --build $(BUILD_DIR) --target clean
