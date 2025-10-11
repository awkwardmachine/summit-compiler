#!/usr/bin/env bash
set -euo pipefail

c=clang++
s=src
b=build
bin="$b/bin"
t="$bin/summit"
n=$(nproc)

# Compilation flags
cf="-std=c++17 -O2 \
-fdata-sections -ffunction-sections -DNDEBUG \
-Isrc -Iinclude -Iinclude/codegen -Iinclude/ast -Iinclude/utils -Iinclude/lexer -Iinclude/parser \
-I/usr/include/llvm-18 -I/usr/include/llvm \
-fmerge-all-constants -fno-stack-protector -fno-math-errno -fno-ident -w"

# Linker flags
lf="-L/usr/lib/llvm-18/lib -lLLVM-18 -ltommath \
-Wl,--gc-sections,--as-needed,--strip-all,-s -flto -Wl,-O3"

run(){ echo "+ $*"; "$@"; }

build(){
    run mkdir -p "$b" "$bin"

    # compile in parallel and show each file
    srcs=$(find "$s" -name '*.cpp')
    
    echo "Starting compilation..."
    start_time=$(date +%s)

    # faster parallel compilation using xargs directly
    echo "$srcs" | xargs -n 1 -P $n -I {} bash -c '
        o="'"$b"'/${1#'"$s"'/}"; o="${o%.cpp}.o"
        mkdir -p "$(dirname "$o")"
        echo "Compiling $1"
        '"$c"' '"$cf"' -c "$1" -o "$o"
    ' _ {}

    objs=$(find "$b" -name '*.o')
    run $c $objs $lf -o "$t"

    # strip and compress
    if command -v sstrip >/dev/null 2>&1; then
        run sstrip "$t" || true
    else
        run strip --strip-all "$t" || true
    fi

    run objcopy --remove-section=.comment --remove-section=.note --remove-section=.eh_frame "$t" || true
    run upx --ultra-brute --lzma --no-progress "$t" >/dev/null 2>&1 || true

    end_time=$(date +%s)
    elapsed=$((end_time - start_time))
    echo "done: $t"
    echo "Compilation time: ${elapsed}s"
}

clean(){ run rm -rf "$b"; }

[ $# -eq 0 ] && build || case $1 in
    build) build ;;
    clean) clean ;;
    *) echo "usage: $0 [build|clean]"; exit 1 ;;
esac