SRC_DIR = src
BUILD_DIR = build

CC = mpicc
CFLAGS = -Wall -O3 -lm -I$(SRC_DIR)

SRC_HDR = $(shell find $(SRC_DIR) -mindepth 2 -type f -name "*.h")
SRC_DEF = $(shell find $(SRC_DIR) -mindepth 2 -type f -name "*.c")

.PHONY: all clean single mpi_blocking mpi_non_blocking

all: single mpi_blocking mpi_non_blocking

clean:
	rm -rf $(BUILD_DIR)

single mpi_blocking mpi_non_blocking: %: $(BUILD_DIR)/%

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BUILD_DIR)/%: $(SRC_HDR) $(SRC_DEF) $(SRC_DIR)/$(notdir %).c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)
