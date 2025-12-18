TOP_DIR=../
# Subdirectories to build if they have Makefiles
SUB_DIRS := core mem string array

include $(TOP_DIR)/Makefile.inc

# Final binary
TARGET := $(BUILD_DIR)/come

# Link all objects into final binary
# Link all objects into final binary
$(TARGET): $(SUB_MAKE_DIRS)
	$(CC) $(CFLAGS) -o $@ $(wildcard $(BUILD_DIR)/*.o)

all: $(SUB_MAKE_DIRS) $(TARGET)
