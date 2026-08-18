SRC_DIR := src
BUILD_DIR := build

CC := clang
CFLAFS := -Wall -Wextra -Wpedantic

SRCS := $(SRC_DIR)/main.c
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
TARGET := $(BUILD_DIR)/ni

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAFS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAFS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR)
