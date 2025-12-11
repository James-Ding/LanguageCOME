# Root Makefile: build tests and examples using compiler built in src/

CFLAGS=-Wall -g -Isrc/include/ -Isrc/core/include/

# Directories
SRC_DIR = src
BUILD_DIR = build
EXAMPLES_DIR = examples
TESTS_DIR = tests
TARGET = $(BUILD_DIR)/come

# Test sources (optional C tests)
# TEST_SRC = $(wildcard $(TESTS_DIR)/*.c)
# TEST_BIN = $(patsubst $(TESTS_DIR)/%.c,$(BUILD_DIR)/tests/%,$(TEST_SRC))

# Default: build compiler then examples
all: $(TARGET) examples

# Build compiler by invoking src Makefile
$(TARGET):
	cd $(SRC_DIR) && $(MAKE)

# Build all examples by invoking examples Makefile
examples: $(TARGET)
	@$(MAKE) -C $(EXAMPLES_DIR)

# Run all examples
run-examples: examples
	@$(MAKE) -C $(EXAMPLES_DIR) run

# Run tests
## Build all test binaries (C tests in tests/)
tests: 
	@echo "Building tests via script..."

# $(BUILD_DIR)/tests/%: $(TESTS_DIR)/%.c
# 	@mkdir -p $(BUILD_DIR)/tests
# 	$(CC) $(CFLAGS) $< -o $@

test: tests test-e2e test-come
	@echo "Running unit tests..."
	@chmod +x $(TESTS_DIR)/run_tests.sh
	@$(TESTS_DIR)/run_tests.sh

test-e2e: $(TARGET)
	@echo "Running end-to-end tests..."
	@python3 $(TESTS_DIR)/test_runner.py

# Run COME language tests (*.co files in t/ directories)
test-come: $(TARGET)
	@echo "Running COME language tests..."
	@passed=0; failed=0; \
	for tdir in $$(find src -type d -name 't'); do \
		for test in $$tdir/*.co; do \
			[ -f "$$test" ] || continue; \
			testname=$$(basename $$test .co); \
			printf "  %-40s " "$$testname"; \
			if $(TARGET) build $$test -o /tmp/come_test_$$$$ 2>/dev/null; then \
				if /tmp/come_test_$$$$ >/dev/null 2>&1; then \
					echo "✓ PASS"; \
					passed=$$((passed + 1)); \
				else \
					echo "✗ FAIL (runtime)"; \
					failed=$$((failed + 1)); \
				fi; \
				rm -f /tmp/come_test_$$$$; \
			else \
				echo "✗ FAIL (compile)"; \
				failed=$$((failed + 1)); \
			fi; \
		done; \
	done; \
	echo ""; \
	echo "Results: $$passed passed, $$failed failed"; \
	[ $$failed -eq 0 ]

# Clean build artifacts
clean:
	@$(MAKE) -C $(SRC_DIR) clean
	@$(MAKE) -C $(EXAMPLES_DIR) clean

.PHONY: all examples run-examples test test-come clean

