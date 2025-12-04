
CC=gcc
CFLAGS += -Wall -g -I$(TOP_DIR)src/include/ -I$(TOP_DIR)src/core/include/

# Source and build dirs
SRC_DIR := .
BUILD_DIR := $(TOP_DIR)/build

# Subdirectories to build if they have Makefiles
# Define only if SUB_DIRS exists and is non-empty
ifdef SUB_DIRS
SUB_MAKE_DIRS := $(if $(wildcard $(SUB_DIRS)),$(shell find $(SUB_DIRS) -type f -name Makefile -exec dirname {} \;),)
else
SUB_MAKE_DIRS :=
endif

# Collect all .c files in SRC_DIR
SRCS := $(wildcard $(SRC_DIR)/*.c)

# Replace .c with .o and move them into BUILD_DIR
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

all: $(SUB_MAKE_DIRS) $(OBJS)

# Rule to recurse into subdirs if they have a Makefile
$(SUB_MAKE_DIRS):
	echo ">>> Building in $@"; \
	$(MAKE) -C $@; \


# Rule to build .o from .c into BUILD_DIR
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure build dir exists
$(BUILD_DIR):
	mkdir -p $@

# Clean up
.PHONY: clean $(SUB_DIRS)
clean:
	rm -f $(OBJS) $(TARGET)
	@for d in $(SUB_DIRS); do \
		if [ -f $$d/Makefile ]; then \
			echo ">>> Cleaning in $$d"; \
			$(MAKE) -C $$d clean; \
		fi; \
	done
