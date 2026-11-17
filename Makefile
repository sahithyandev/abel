BUILD_DIR ?= build
CMAKE ?= cmake
BUN ?= bun
CLANG_FORMAT ?= clang-format
RUSTFMT ?= rustfmt

.DEFAULT_GOAL := help

.PHONY: help configure cli build test test-all coverage wasm docs docs-dev desktop fmt run clean setup-hooks

help:
	@printf '%s\n' \
		'Abel development commands:' \
		'  make cli       Configure and compile the native CLI' \
		'  make test      Compile and run the C++ test suite' \
		'  make test-all  Run native, JavaScript, and Rust tests' \
		'  make coverage  Generate native gcov and JavaScript LCOV reports' \
		'  make fmt       Format all tracked source files' \
		'  make wasm      Build the WebAssembly runtime' \
		'  make docs      Build the documentation site' \
		'  make docs-dev  Start the documentation site locally' \
		'  make desktop   Start the Tauri desktop app locally' \
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

test-all: test
	$(BUN) run --cwd apps/desktop test
	$(BUN) run --cwd packages/wasm test
	$(BUN) run --cwd apps/docs test
	cargo test --manifest-path apps/desktop/src-tauri/Cargo.toml

coverage:
	@mkdir -p coverage/native coverage/desktop coverage/wasm coverage/docs
	$(CMAKE) -S . -B coverage/build -DBUILD_TESTING=ON -DABEL_ENABLE_COVERAGE=ON
	$(CMAKE) --build coverage/build
	ctest --test-dir coverage/build --output-on-failure
	@cd coverage/native && \
	  gcov -p -o ../build/apps/cli/CMakeFiles/abel_tokenizer.dir/src/tokenizer.cpp.gcda ../../apps/cli/src/tokenizer.cpp > tokenizer.txt && \
	  gcov -p -o ../build/apps/cli/CMakeFiles/abel_parser.dir/src/parser.cpp.gcda ../../apps/cli/src/parser.cpp > parser.txt && \
	  gcov -p -o ../build/apps/cli/CMakeFiles/abel_evaluator.dir/src/evaluator.cpp.gcda ../../apps/cli/src/evaluator.cpp > evaluator.txt && \
	  gcov -p -o ../build/apps/cli/CMakeFiles/abel_cli.dir/src/cli.cpp.gcda ../../apps/cli/src/cli.cpp > cli.txt
	$(BUN) run --cwd apps/desktop test --coverage --coverage-reporter=lcov --coverage-dir ../../coverage/desktop
	$(BUN) run --cwd packages/wasm test --coverage --coverage-reporter=lcov --coverage-dir ../../coverage/wasm
	$(BUN) run --cwd apps/docs test --coverage --coverage-reporter=lcov --coverage-dir ../../coverage/docs
	@if cargo llvm-cov --version >/dev/null 2>&1; then \
		cargo llvm-cov --manifest-path apps/desktop/src-tauri/Cargo.toml --lcov --output-path coverage/tauri.lcov; \
	else \
		echo 'Rust coverage skipped: install cargo-llvm-cov to generate coverage/tauri.lcov'; \
	fi

wasm:
	$(BUN) run --cwd packages/wasm build

docs:
	$(BUN) run --cwd apps/docs build

docs-dev:
	$(BUN) run --cwd apps/docs dev

desktop:
	$(BUN) run --cwd apps/desktop tauri dev

run: cli
	$(BUILD_DIR)/apps/cli/abel $(ARGS)

clean:
	$(CMAKE) --build $(BUILD_DIR) --target clean
