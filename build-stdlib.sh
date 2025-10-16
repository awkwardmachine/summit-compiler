#!/bin/bash
OS_TYPE="unknown"
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" || "$OSTYPE" == "cygwin" ]]; then
    OS_TYPE="windows"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS_TYPE="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS_TYPE="macos"
fi

echo "Detected OS: $OS_TYPE"

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
   
    if [ "$OS_TYPE" = "windows" ]; then
        object_file="$BUILD_DIR/$base_name.obj"
    else
        object_file="$BUILD_DIR/$base_name.o"
    fi
   
    echo "Compiling $source_file..."
   
    if [ "$OS_TYPE" = "windows" ]; then
        clang -c -O2 -I"$INCLUDE_DIR" "$source_file" -o "$object_file"
    else
        clang -c -O2 -fPIC -I"$INCLUDE_DIR" "$source_file" -o "$object_file"
    fi
   
    if [ $? -ne 0 ]; then
        echo "Failed to compile $source_file"
        exit 1
    fi
    OBJECT_FILES+=("$object_file")
done

echo "Creating static library from ${#OBJECT_FILES[@]} object files..."

if [ "$OS_TYPE" = "windows" ]; then
    lib.exe /OUT:"$LIB_DIR/libsummit.lib" "${OBJECT_FILES[@]}" 2>/dev/null
   
    if [ $? -ne 0 ]; then
        echo "lib.exe not found, trying llvm-lib..."
        llvm-lib /OUT:"$LIB_DIR/libsummit.lib" "${OBJECT_FILES[@]}"
       
        if [ $? -ne 0 ]; then
            echo "llvm-lib not found, using ar as fallback..."
            ar rcs "$LIB_DIR/libsummit.a" "${OBJECT_FILES[@]}"
            if [ $? -ne 0 ]; then
                echo "Failed to create static library"
                exit 1
            fi
            echo "Standard library built successfully (fallback mode):"
            echo "  Static: $LIB_DIR/libsummit.a"
        else
            echo "Standard library built successfully:"
            echo "  Static: $LIB_DIR/libsummit.lib"
        fi
    else
        echo "Standard library built successfully:"
        echo "  Static: $LIB_DIR/libsummit.lib"
    fi
    
    echo "Creating dynamic library..."
    clang -shared "${OBJECT_FILES[@]}" -o "$LIB_DIR/libsummit.dll"
    if [ $? -eq 0 ]; then
        echo "  Dynamic: $LIB_DIR/libsummit.dll"
        
        FULL_LIB_PATH="$(cd "$LIB_DIR" && pwd)"
        
        if command -v cygpath &> /dev/null; then
            WINDOWS_LIB_PATH=$(cygpath -w "$FULL_LIB_PATH")
        else
            WINDOWS_LIB_PATH=$(echo "$FULL_LIB_PATH" | sed 's|^/\([a-z]\)/|\1:/|' | sed 's|/|\\|g')
        fi
        
        echo ""
        echo "==================================================================="
        echo "Setting up SUMMIT_LIB environment variable..."
        echo "==================================================================="
        echo "Library Location: $WINDOWS_LIB_PATH"

        echo "Setting SUMMIT_LIB permanently..."
        setx SUMMIT_LIB "$WINDOWS_LIB_PATH" > /dev/null 2>&1
        
        export SUMMIT_LIB="$FULL_LIB_PATH"
        
        echo ""
        echo "SUMMIT_LIB environment variable set permanently!"
        echo "  SUMMIT_LIB=$WINDOWS_LIB_PATH"
        echo ""
        echo "==================================================================="
        echo "IMPORTANT: Please restart your terminal for changes to take effect"
        echo "==================================================================="

    else
        echo "Failed to create dynamic library (continuing anyway)"
    fi
else
    ar rcs "$LIB_DIR/libsummit.a" "${OBJECT_FILES[@]}"
    if [ $? -ne 0 ]; then
        echo "Failed to create static library"
        exit 1
    fi
   
    echo "Standard library built successfully:"
    echo "  Static: $LIB_DIR/libsummit.a"
   
    echo "Creating shared library..."
    if [ "$OS_TYPE" = "macos" ]; then
        clang -shared -fPIC "${OBJECT_FILES[@]}" -o "$LIB_DIR/libsummit.dylib"
        if [ $? -eq 0 ]; then
            echo "  Shared: $LIB_DIR/libsummit.dylib"

            FULL_LIB_PATH="$(cd "$LIB_DIR" && pwd)"

            SHELL_PROFILE=""
            if [ -f "$HOME/.zshrc" ]; then
                SHELL_PROFILE="$HOME/.zshrc"
            elif [ -f "$HOME/.bashrc" ]; then
                SHELL_PROFILE="$HOME/.bashrc"
            elif [ -f "$HOME/.bash_profile" ]; then
                SHELL_PROFILE="$HOME/.bash_profile"
            fi
            
            if [ -n "$SHELL_PROFILE" ]; then
                if ! grep -q "SUMMIT_LIB" "$SHELL_PROFILE" 2>/dev/null; then
                    echo "" >> "$SHELL_PROFILE"
                    echo "# Summit standard library" >> "$SHELL_PROFILE"
                    echo "export SUMMIT_LIB=\"$FULL_LIB_PATH\"" >> "$SHELL_PROFILE"
                    echo ""
                    echo "SUMMIT_LIB environment variable added to $SHELL_PROFILE"
                else
                    echo ""
                    echo "SUMMIT_LIB environment variable already in $SHELL_PROFILE"
                fi
            fi
            
            export SUMMIT_LIB="$FULL_LIB_PATH"
            
            echo "  SUMMIT_LIB=$SUMMIT_LIB"
            echo ""
            echo "Please restart your terminal or run: source $SHELL_PROFILE"
        else
            echo "Failed to create shared library (continuing anyway)"
        fi
    else
        clang -shared -fPIC "${OBJECT_FILES[@]}" -o "$LIB_DIR/libsummit.so"
        if [ $? -eq 0 ]; then
            echo "  Shared: $LIB_DIR/libsummit.so"
            
            FULL_LIB_PATH="$(cd "$LIB_DIR" && pwd)"
            
            SHELL_PROFILE=""
            if [ -f "$HOME/.bashrc" ]; then
                SHELL_PROFILE="$HOME/.bashrc"
            elif [ -f "$HOME/.bash_profile" ]; then
                SHELL_PROFILE="$HOME/.bash_profile"
            elif [ -f "$HOME/.profile" ]; then
                SHELL_PROFILE="$HOME/.profile"
            fi
            
            if [ -n "$SHELL_PROFILE" ]; then
                if ! grep -q "SUMMIT_LIB" "$SHELL_PROFILE" 2>/dev/null; then
                    echo "" >> "$SHELL_PROFILE"
                    echo "# Summit standard library" >> "$SHELL_PROFILE"
                    echo "export SUMMIT_LIB=\"$FULL_LIB_PATH\"" >> "$SHELL_PROFILE"
                    echo ""
                    echo "SUMMIT_LIB environment variable added to $SHELL_PROFILE"
                else
                    echo ""
                    echo "SUMMIT_LIB environment variable already in $SHELL_PROFILE"
                fi
            fi
            
            export SUMMIT_LIB="$FULL_LIB_PATH"
            
            echo "  SUMMIT_LIB=$SUMMIT_LIB"
            echo ""
            echo "Please restart your terminal or run: source $SHELL_PROFILE"
        else
            echo "Failed to create shared library (continuing anyway)"
        fi
    fi
   
    echo ""
    echo "Library contents:"
    ar t "$LIB_DIR/libsummit.a"
fi

echo ""
echo "Object files: $BUILD_DIR/*"
echo "Build complete!"