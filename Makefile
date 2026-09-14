# ghost-hoock — Makefile for aarch64 Android (Samsung A17 BZA5)
#
# On-device build (clang from $PATH):
#   make
#   make clean
#   make strip
#
# Output: ghost-hoock (PIE aarch64)

CC      ?= clang
STRIP   ?= llvm-strip

TARGET  := ghost-hoock
BUILD   := build

# --- flags ---
CFLAGS  := -O2 -Wall \
           -Wno-unused-parameter \
           -Wno-sign-compare \
           -Wno-unused-function \
           -Isrc/kernelsnitch \
           -Isrc \
           -Iinclude \
           -D_GNU_SOURCE -D__ARM=1 \
           -DTARGET_CONFIG_H='"target.h"'

LDFLAGS := -fPIE -pie -pthread

# --- sources ---
SRCS := \
  src/main.c \
  src/spray.c \
  src/route.c

OBJS := $(SRCS:src/%.c=$(BUILD)/%.o)

# --- rules ---
.PHONY: all clean strip info

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@
	@echo "[+] built $(TARGET)"
	@ls -la $(TARGET)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	@mkdir -p $(BUILD)

strip: $(TARGET)
	$(STRIP) --strip-all $(TARGET)

clean:
	rm -rf $(BUILD) $(TARGET)

info:
	@echo "CC      = $(CC)"
	@echo "TARGET  = $(TARGET)"
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "SRCS    = $(SRCS)"
	@echo "OBJS    = $(OBJS)"
