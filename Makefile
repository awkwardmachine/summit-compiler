CXX       = g++
SRC_DIR   = src
BUILD_DIR = build
BIN_DIR   = $(BUILD_DIR)/bin
TARGET    = $(BIN_DIR)/summit

# Detect number of CPU cores for parallel LTO
NUM_CORES := $(shell nproc)

CXXFLAGS  = -std=c++17 -Os -flto=$(NUM_CORES) -fdata-sections -ffunction-sections \
            -fno-unwind-tables -fno-asynchronous-unwind-tables -DNDEBUG \
            -Isrc -I/usr/include/llvm-18 -I/usr/include/llvm \
            -fmerge-all-constants -fno-stack-protector \
            -fno-math-errno -fno-ident -w

LDFLAGS   = -L/usr/lib/llvm-18/lib \
            -lLLVM-18 -ltommath \
            -Wl,--gc-sections,--as-needed,--strip-all \
            -flto=$(NUM_CORES) -Wl,-O3

SOURCES   = $(shell find $(SRC_DIR) -name '*.cpp')
OBJS      = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

.PHONY: all clean strip-upx

all: $(TARGET)
	@echo "Stripping and compressing binary..."
	@$(MAKE) strip-upx -s

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN_DIR):
	@mkdir -p $@

strip-upx:
	@strip --strip-unneeded $(TARGET) || true
	@upx --best --lzma --no-progress $(TARGET) >/dev/null 2>&1 || true

clean:
	rm -rf $(BUILD_DIR)
