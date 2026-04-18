CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -O2 -g -MMD -MP
LDFLAGS =

SRC_DIR = src
OBJ_DIR = build
BIN_DIR = bin

ifeq ($(OS),Windows_NT)
    TARGET = $(BIN_DIR)/zkp_db.exe
    FIX_PATH = $(subst /,\,$1)
    RM = del /Q /S
    MKDIR = mkdir
else
    TARGET = $(BIN_DIR)/zkp_db
    FIX_PATH = $1
    RM = rm -rf
    MKDIR = mkdir -p
endif

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

all: directories $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking $@"
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(MKDIR) $(call FIX_PATH,$(dir $@))
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

directories:
	@$(MKDIR) $(call FIX_PATH,$(BIN_DIR))
	@$(MKDIR) $(call FIX_PATH,$(OBJ_DIR))

-include $(DEPS)

clean:
	@echo "Cleaning build files..."
	$(RM) $(call FIX_PATH,$(OBJ_DIR))
	$(RM) $(call FIX_PATH,$(BIN_DIR))

rebuild: clean all

.PHONY: all clean rebuild directories
