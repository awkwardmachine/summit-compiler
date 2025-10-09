CXX = g++
CXXFLAGS = -std=c++17 -Isrc -I/usr/include/llvm-18 -I/usr/include/llvm -w
LDFLAGS = -L/usr/lib/llvm-18/lib -lLLVM-18 -ltommath

SRC_DIR = src
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin

SOURCES = $(wildcard $(SRC_DIR)/*.cpp) \
          $(wildcard $(SRC_DIR)/*/*.cpp)
OBJS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

TARGET = $(BIN_DIR)/summit

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(OBJS) $(LDFLAGS) -o $(TARGET)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BUILD_DIR)

test: $(TARGET)
	# Add your test commands here

$(BUILD_DIR)/ast: | $(BUILD_DIR)
	@mkdir -p $@

$(BUILD_DIR)/codegen: | $(BUILD_DIR)
	@mkdir -p $@

$(BUILD_DIR)/lexer: | $(BUILD_DIR)
	@mkdir -p $@

$(BUILD_DIR)/parser: | $(BUILD_DIR)
	@mkdir -p $@

$(BUILD_DIR)/utils: | $(BUILD_DIR)
	@mkdir -p $@