BUILD_DIR ?= build
CMAKE ?= cmake
BUN ?= bun

.DEFAULT_GOAL := help

.PHONY: help configure cli build test wasm docs docs-dev run clean

help:
	@printf '%s\n' \
		'Abel development commands:' \
		'  make cli       Configure and compile the native CLI' \
		'  make test      Compile and run the C++ test suite' \
		'  make wasm      Build the WebAssembly runtime' \
		'  make docs      Build the documentation site' \
		'  make docs-dev  Start the documentation site locally' \
		'  make run ARGS="file.abel"  Run the compiled CLI' \
		'  make clean     Remove native build outputs'

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
	$(BUN) --cwd apps/docs run build

docs-dev:
	$(BUN) --cwd apps/docs run dev

run: cli
	$(BUILD_DIR)/apps/cli/abel $(ARGS)

clean:
	$(CMAKE) --build $(BUILD_DIR) --target clean
