SRC_DIR = src
BUILD_DIR = build

SHARED_SRC_DIR = $(SRC_DIR)/shared
PING_SRC_DIR = $(SRC_DIR)/ping
TRACEROUTE_SRC_DIR = $(SRC_DIR)/traceroute
SMURF_SRC_DIR = $(SRC_DIR)/smurf

CC = gcc
CFLAGS = -Wall -pthread -I$(SRC_DIR)

.PHONY: all clean ping traceroute

all: ping traceroute smurf

clean:
	rm -rf $(BUILD_DIR)

ping: $(BUILD_DIR)/ping

traceroute: $(BUILD_DIR)/traceroute

smurf: $(BUILD_DIR)/smurf

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BUILD_DIR)/ping: $(SHARED_SRC_DIR)/* $(PING_SRC_DIR)/* | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

$(BUILD_DIR)/traceroute: $(SHARED_SRC_DIR)/* $(TRACEROUTE_SRC_DIR)/* | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

$(BUILD_DIR)/smurf: $(SHARED_SRC_DIR)/* $(SMURF_SRC_DIR)/* | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)
