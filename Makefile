# Root Makefile: build tests and examples using compiler built in src/

CFLAGS=-Wall -g -Isrc/include/

# Directories
SRC_DIR = src
BUILD_DIR = build
EXAMPLES_DIR = examples
TESTS_DIR = tests
COMPILER = $(BUILD_DIR)/come

# Example sources
EXAMPLE_CO = $(wildcard $(EXAMPLES_DIR)/*.co)
EXAMPLE_BIN = $(patsubst $(EXAMPLES_DIR)/%.co,$(BUILD_DIR)/examples/%,$(EXAMPLE_CO))

# Test sources (optional C tests)
TEST_SRC = $(wildcard $(TESTS_DIR)/*.c)
TEST_BIN = $(patsubst $(TESTS_DIR)/%.c,$(BUILD_DIR)/tests/%,$(TEST_SRC))

# Default target: build compiler
all: $(COMPILER)

# Build compiler by delegating to src/Makefile
$(COMPILER):
	@mkdir -p $(BUILD_DIR)
	@$(MAKE) -C $(SRC_DIR)

# Build all examples
examples: $(EXAMPLE_BIN)

$(BUILD_DIR)/examples/%: $(EXAMPLES_DIR)/%.co $(COMPILER)
	@mkdir -p $(BUILD_DIR)/examples
	@echo "Building example $<"
	@$(COMPILER) build $< -o $@

# Run all examples
run-examples: examples
	@for bin in $(EXAMPLE_BIN); do \
		echo "Running $$bin"; \
		./$$bin; \
	done

# Run tests
## Build all test binaries (C tests in tests/)
tests: $(TEST_BIN)

$(BUILD_DIR)/tests/%: $(TESTS_DIR)/%.c
	@mkdir -p $(BUILD_DIR)/tests
	$(CC) $(CFLAGS) $< -o $@

test: tests
	@echo "Running tests..."
	@chmod +x $(TESTS_DIR)/run_tests.sh
	@$(TESTS_DIR)/run_tests.sh

# Clean build artifacts
clean:
	@rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/examples $(BUILD_DIR)/tests
	@$(MAKE) -C $(SRC_DIR) clean

.PHONY: all examples run-examples test clean

