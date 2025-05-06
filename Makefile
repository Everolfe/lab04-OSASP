CC = gcc
DEBUG_FLAGS = -g -ggdb -std=c11 -pedantic -W -Wall -Wextra
RELEASE_FLAGS = -std=c11 -pedantic -W -Wall -Wextra -Werror

ifeq ($(MODE),debug)
    CFLAGS = $(DEBUG_FLAGS)
    BUILD_DIR = build/debug
else
    CFLAGS = $(RELEASE_FLAGS)
    BUILD_DIR = build/release
endif

.PHONY: all clean debug release

all: build_dir ipc

build_dir:
	mkdir -p $(BUILD_DIR)

ipc: $(BUILD_DIR)/main.o $(BUILD_DIR)/ring.o $(BUILD_DIR)/stack.o 
	$(CC) $^ -o $(BUILD_DIR)/$@

$(BUILD_DIR)/main.o: src/main.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ring.o: src/ring.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/stack.o: src/stack.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o $(BUILD_DIR)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

release: CFLAGS += $(RELEASE_FLAGS)
release: all