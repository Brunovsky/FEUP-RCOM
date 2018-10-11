.PHONY: obj all clean createbin

CC := gcc

SRC_DIR := src
OBJ_DIR := bin
OUT_DIR := .

SRC_FIL := $(wildcard $(SRC_DIR)/*.c)
SRC_OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FIL))

OBJECTS := $(SRC_OBJ)

OUT := $(OUT_DIR)/ll

CFLAGS := -std=gnu11 -Wall -Wextra -O2 -march=native
CFLAGS += -Wno-switch
LIBS := 
INCLUDE := -I $(SRC_DIR)



all: obj
	$(CC) $(CFLAGS) $(INCLUDE) -o $(OUT) $(OBJECTS) $(LIBS)

obj: createbin $(OBJECTS)

createbin:
	@mkdir -p bin

$(SRC_OBJ): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE)

$(GVW_OBJ): $(OBJ_DIR)/%.o: $(GVW_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE)

clean:
	@rm -f $(OBJECTS) $(OUT)
