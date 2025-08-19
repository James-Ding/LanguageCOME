CC=gcc
CFLAGS=-Wall -g -Iinclude/
BUILD=../build

# Source and build dirs
SRC_DIR := .
BUILD_DIR := ../build

# Collect all .c files in SRC_DIR
SRCS := $(wildcard $(SRC_DIR)/*.c)

# Replace .c with .o and move them into BUILD_DIR
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Final binary
TARGET := $(BUILD_DIR)/come

# Default target
all: $(TARGET)

# Link all objects into final binary
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Rule to build .o from .c into BUILD_DIR
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure build dir exists
$(BUILD_DIR):
	mkdir -p $@

# Clean up
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
