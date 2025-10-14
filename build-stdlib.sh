#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ "$SCRIPT_DIR" == */stdlib ]]; then
    cd "$SCRIPT_DIR/.."
fi

STDLIB_DIR="stdlib"
BUILD_DIR="build-stdlib/stdlib"
LIB_DIR="lib"
INCLUDE_DIR="include"

mkdir -p "$BUILD_DIR"
mkdir -p "$LIB_DIR"
mkdir -p "$INCLUDE_DIR"

echo "Building Summit standard library..."
echo "Working directory: $(pwd)"

if [ ! -f "$STDLIB_DIR/io.c" ]; then
    echo "Error: Cannot find $STDLIB_DIR/io.c"
    echo "Make sure you're running this from the project root, or that stdlib files exist"
    exit 1
fi

C_SOURCES=($(find "$STDLIB_DIR" -name "*.c"))
if [ ${#C_SOURCES[@]} -eq 0 ]; then
    echo "Error: No C source files found in $STDLIB_DIR/"
    exit 1
fi

echo "Found ${#C_SOURCES[@]} C source files:"
for source in "${C_SOURCES[@]}"; do
    echo "  - $source"
done

OBJECT_FILES=()
for source_file in "${C_SOURCES[@]}"; do
    base_name=$(basename "$source_file" .c)
    object_file="$BUILD_DIR/$base_name.o"
    
    echo "Compiling $source_file..."
    clang -c -O2 -fPIC -I"$INCLUDE_DIR" "$source_file" -o "$object_file"
    if [ $? -ne 0 ]; then
        echo "Failed to compile $source_file"
        exit 1
    fi
    OBJECT_FILES+=("$object_file")
done

echo "Creating static library from ${#OBJECT_FILES[@]} object files..."
ar rcs "$LIB_DIR/libsummit.a" "${OBJECT_FILES[@]}"
if [ $? -ne 0 ]; then
    echo "Failed to create static library"
    exit 1
fi

echo "Creating shared library..."
clang -shared -fPIC "${OBJECT_FILES[@]}" -o "$LIB_DIR/libsummit.so"
if [ $? -ne 0 ]; then
    echo "Failed to create shared library (continuing anyway)"
fi

echo "Standard library built successfully:"
echo "  Static: $LIB_DIR/libsummit.a"
echo "  Shared: $LIB_DIR/libsummit.so"
echo "Object files: $BUILD_DIR/*.o"

echo "Library contents:"
ar t "$LIB_DIR/libsummit.a"