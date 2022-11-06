SOURCE_DIR=src
BUILD_DIR=build
include $(N64_INST)/include/n64.mk

all: n64-systemcrash.z64
.PHONY: all

OBJS = $(BUILD_DIR)/n64-systemcrash.o

n64-systemcrash.z64: N64_ROM_SAVETYPE=sram256k
n64-systemcrash.z64: N64_ROM_TITLE="SysCrash"
n64-systemcrash.z64: $(BUILD_DIR)/n64-systemcrash.dfs

$(BUILD_DIR)/n64-systemcrash.dfs: $(wildcard filesystem/*)
$(BUILD_DIR)/n64-systemcrash.elf: $(OBJS)

clean:
	rm -f $(BUILD_DIR)/* *.z64
.PHONY: clean

-include $(wildcard $(BUILD_DIR)/*.d)
