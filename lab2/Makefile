SRC_DIR = src
BUILD_DIR = build

CC = g++
CFLAGS = -std=c++17 -Wall -I$(SRC_DIR) $(shell wx-config-gtk3 --cxxflags --libs std,richtext)

SRC_HDR = $(shell find $(SRC_DIR) -type f -name "*.h")
SRC_DEF = $(shell find $(SRC_DIR) -type f -name "*.cpp")

.PHONY: all clean

all: chat

clean:
	rm -rf $(BUILD_DIR)

chat: $(BUILD_DIR)/chat

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BUILD_DIR)/chat: $(SRC_HDR) $(SRC_DEF) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DEF)
