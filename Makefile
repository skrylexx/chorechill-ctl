CC      = gcc
CFLAGS  = -Wall -Wextra -O2
SRC_DIR = backend/src
OUT_DIR = backend/compiled

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/hardware.c \
       $(SRC_DIR)/ipc.c \
       $(SRC_DIR)/profiles.c

TARGET = $(OUT_DIR)/chorechill-ctl

all: $(TARGET)

$(TARGET): $(SRCS)
	@mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)
	@echo "[OK] Built → $(TARGET)"

clean:
	rm -f $(TARGET)
	@echo "[OK] Cleaned."

.PHONY: all clean
