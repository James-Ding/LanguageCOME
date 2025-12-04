TOP_DIR=../
# Subdirectories to build if they have Makefiles
SUB_DIRS := modules

include $(TOP_DIR)/Makefile.inc

# Final binary
TARGET := $(BUILD_DIR)/come

# Link all objects into final binary
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

all: $(SUB_MAKE_DIRS) $(TARGET)
