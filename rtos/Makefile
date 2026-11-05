CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
BUILD_DIR := build
TARGET := $(BUILD_DIR)/vehicle-simulator
GOIL ?= goil
TRAMPOLINE_PATH ?=
TRAMPOLINE_APP := vehicle-dashboard_exe

.PHONY: all run test clean trampoline-generate trampoline-build trampoline-run

all: $(TARGET)

$(TARGET): vehicle_simulator.c cluster_client.c cluster_client.h
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) vehicle_simulator.c cluster_client.c -o $(TARGET)

run: $(TARGET)
	@test -n "$(API_KEY)" || (echo "Set API_KEY to the cluster server key" >&2; exit 2)
	API_KEY="$(API_KEY)" CLUSTER_PORT="$(or $(CLUSTER_PORT),3000)" ./$(TARGET)

test:
	node test_integration.mjs

trampoline-generate:
	@test -n "$(TRAMPOLINE_PATH)" || (echo "Set TRAMPOLINE_PATH to the Trampoline checkout" >&2; exit 2)
	$(GOIL) --target=posix/linux --templates="$(TRAMPOLINE_PATH)/goil/templates/" vehicle_dashboard.oil

trampoline-build: trampoline-generate
	./make.py

trampoline-run: trampoline-build
	@test -n "$(API_KEY)" || (echo "Set API_KEY to the cluster server key" >&2; exit 2)
	API_KEY="$(API_KEY)" CLUSTER_PORT="$(or $(CLUSTER_PORT),3000)" ./$(TRAMPOLINE_APP)

clean:
	rm -rf $(BUILD_DIR)
