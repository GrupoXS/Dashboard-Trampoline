CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
BUILD_DIR := build
TARGET := $(BUILD_DIR)/vehicle-simulator

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): vehicle_simulator.c cluster_client.c cluster_client.h
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) vehicle_simulator.c cluster_client.c -o $(TARGET)

run: $(TARGET)
	@test -n "$(API_KEY)" || (echo "Set API_KEY to the cluster server key" >&2; exit 2)
	API_KEY="$(API_KEY)" CLUSTER_PORT="$(or $(CLUSTER_PORT),3000)" ./$(TARGET)

test:
	node test_integration.mjs

clean:
	rm -rf $(BUILD_DIR)
