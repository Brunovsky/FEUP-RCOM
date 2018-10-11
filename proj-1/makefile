.PHONY: all clean string substring mkdir

SRC_DIR := src
OBJ_DIR := bin
GVW_DIR := GraphViewer/cpp
OUT_DIR := .

SRC_CPP := $(wildcard $(SRC_DIR)/*.cpp)
SRC_OBJ := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_CPP))

GVW_CPP := $(wildcard $(GVW_DIR)/*.cpp)
GVW_OBJ := $(patsubst $(GVW_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(GVW_CPP))

OBJECTS := $(SRC_OBJ) $(GVW_OBJ)

OUT := $(OUT_DIR)/cal.exe

CXXFLAGS := -std=c++17 -Wall -Wextra -O3 -march=native -flto
CXXFLAGS += -Wno-unused-function -Wno-unused-parameter
# winsock32
LIBS := -lws2_32
INCLUDE := -I $(SRC_DIR) -I $(GVW_DIR)



all: createdir $(OBJECTS)
	g++ $(CXXFLAGS) $(INCLUDE) -o $(OUT) $(OBJECTS) $(LIBS)

createdir:
	@mkdir -p bin

$(SRC_OBJ): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	g++ $(CXXFLAGS) -c $< -o $@ $(INCLUDE)

$(GVW_OBJ): $(OBJ_DIR)/%.o: $(GVW_DIR)/%.cpp
	g++ $(CXXFLAGS) -c $< -o $@ $(INCLUDE)

clean:
	@rm -f $(OBJECTS) $(OUT)
