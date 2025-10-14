set -euo pipefail

if [[ -z "${MSYSTEM:-}" ]]; then
    echo "Please run this script inside an MSYS2 shell (UCRT64, CLANG64, or MINGW64)."
    exit 1
fi

if command -v clang++ >/dev/null 2>&1; then
    CXX=$(command -v clang++)
    echo "Using compiler: $CXX"
elif command -v g++ >/dev/null 2>&1; then
    CXX=$(command -v g++)
    echo "Using compiler: $CXX"
else
    echo "No C++ compiler found (clang++ or g++)."
    exit 1
fi

if command -v llvm-config >/dev/null 2>&1; then
    LLVM_CXXFLAGS=$(llvm-config --cxxflags | sed 's/-fno-exceptions//g')
    LLVM_LDFLAGS=$(llvm-config --ldflags --libs core orcjit native all)
else
    echo "llvm-config not found. Install MSYS2 package 'mingw-w64-ucrt-x86_64-llvm'."
    exit 1
fi

SRC_DIR=src
BUILD_DIR=build-windows
BIN_DIR="$BUILD_DIR/bin"
TARGET="$BIN_DIR/summit.exe"

CXXFLAGS="-std=c++17 -O2 \
-fdata-sections -ffunction-sections -DNDEBUG \
-Isrc -Iinclude -Iinclude/codegen -Iinclude/ast -Iinclude/utils -Iinclude/lexer -Iinclude/parser \
-I/ucrt64/include \
$LLVM_CXXFLAGS \
-fmerge-all-constants -fno-stack-protector -fno-math-errno -fno-ident -w"

LDFLAGS="$LLVM_LDFLAGS -ltommath -lstdc++ -Wl,--gc-sections,--as-needed,--strip-all,-s"

run() { echo "+ $*"; "$@"; }

build() {
    run mkdir -p "$BIN_DIR"
    echo "Starting compilation..."
    START_TIME=$(date +%s)
    
    while IFS= read -r -d '' src; do
        obj="$BUILD_DIR/${src#$SRC_DIR/}"
        obj="${obj%.cpp}.o"
        run mkdir -p "$(dirname "$obj")"
        echo "Compiling $src -> $obj"
        run "$CXX" $CXXFLAGS -c "$src" -o "$obj"
    done < <(find "$SRC_DIR" -name '*.cpp' -print0)
    
    OBJS=$(find "$BUILD_DIR" -name '*.o')
    if [[ -z "$OBJS" ]]; then
        echo "No object files found to link."
        exit 1
    fi
    
    echo "Linking..."
    run "$CXX" $OBJS $LDFLAGS -o "$TARGET"
    
    if command -v strip >/dev/null 2>&1; then
        run strip --strip-all "$TARGET" 2>/dev/null || true
    fi
    if command -v upx >/dev/null 2>&1; then
        run upx --lzma --no-progress "$TARGET" >/dev/null 2>&1 || true
    fi

    echo "Copying required DLLs..."
    for dll in libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll libLLVM-20.dll; do
        if [ -f "/ucrt64/bin/$dll" ]; then
            cp "/ucrt64/bin/$dll" "$BIN_DIR/" 2>/dev/null || true
        fi
    done
    
    END_TIME=$(date +%s)
    echo "Done: $TARGET"
    echo "Compilation time: $((END_TIME - START_TIME))s"
}

clean() {
    run rm -rf "$BUILD_DIR"
    echo "🧹 Cleaned build directory."
}

case "${1:-build}" in
    build) build ;;
    clean) clean ;;
    *) echo "Usage: $0 [build|clean]"; exit 1 ;;
esac