.PHONY: all clean createbin debug

CC := gcc

SRC_DIR := src
OBJ_DIR := bin
OUT_DIR := .

OUT := $(OUT_DIR)/download

SRC_FIL := $(wildcard $(SRC_DIR)/*.c)
SRC_OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FIL))

OBJECTS := $(SRC_OBJ)

CFLAGS := -std=gnu11 -Wall -Wextra -march=native -g
CFLAGS += 
LIBS := 
INCLUDE := -I $(SRC_DIR)



all: clean createbin $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDE) -o $(OUT) $(OBJECTS) $(LIBS)

createbin:
	@mkdir -p bin

$(SRC_OBJ): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE)

clean:
	@rm -f $(OBJECTS) $(OUT)
