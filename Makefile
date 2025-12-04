# Root Makefile: build tests and examples using compiler built in src/

CFLAGS=-Wall -g -Isrc/include/

# Directories
SRC_DIR = src
BUILD_DIR = build
EXAMPLES_DIR = examples
TESTS_DIR = tests
TARGET = $(BUILD_DIR)/come

# Example sources
EXAMPLE_CO = $(wildcard $(EXAMPLES_DIR)/*.co)
EXAMPLE_BIN = $(patsubst $(EXAMPLES_DIR)/%.co,$(EXAMPLES_DIR)/%,$(EXAMPLE_CO))

# Test sources (optional C tests)
TEST_SRC = $(wildcard $(TESTS_DIR)/*.c)
TEST_BIN = $(patsubst $(TESTS_DIR)/%.c,$(BUILD_DIR)/tests/%,$(TEST_SRC))

# Default: build compiler then examples
all: $(TARGET) examples

# Build compiler by invoking src Makefile
$(TARGET):
	cd $(SRC_DIR) && $(MAKE)

# Build all examples
examples: $(EXAMPLE_BIN)

$(EXAMPLES_DIR)/%: $(EXAMPLES_DIR)/%.co $(TARGET)
	@echo "Building example $<"
	@$(TARGET) build $< -o $@

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
	@$(MAKE) -C $(SRC_DIR) clean
	-rm -f $(EXAMPLE_BIN)
	-rm -f $(EXAMPLES_DIR)/*.c

.PHONY: all examples run-examples test clean

