#!/usr/bin/env bash
set -euo pipefail

c=g++
s=src
b=build
bin="$b/bin"
t="$bin/summit"
n=$(nproc)

cf="-std=c++17 -Os -flto=$n -fdata-sections -ffunction-sections \
-fno-unwind-tables -fno-asynchronous-unwind-tables -DNDEBUG \
-Isrc -I/usr/include/llvm-18 -I/usr/include/llvm \
-fmerge-all-constants -fno-stack-protector -fno-math-errno -fno-ident \
-w"

lf="-L/usr/lib/llvm-18/lib -lLLVM-18 -ltommath \
-Wl,--gc-sections,--as-needed,--strip-all,-s -flto=$n -Wl,-O3"

run(){ echo "+ $*"; "$@"; }

build(){
    run mkdir -p "$b" "$bin"
    
    srcs=$(find "$s" -name '*.cpp')
    for f in $srcs; do
        o="$b/${f#$s/}"; o="${o%.cpp}.o"
        run mkdir -p "$(dirname "$o")"
        run $c $cf -c "$f" -o "$o"
    done

    objs=$(find "$b" -name '*.o')
    run $c $objs $lf -o "$t"

    # use sstrip if available for smaller binary
    if command -v sstrip >/dev/null 2>&1; then
        run sstrip "$t" || true
    else
        run strip --strip-all "$t" || true
    fi

    # remove unnecessary elf sections
    run objcopy --remove-section=.comment --remove-section=.note --remove-section=.eh_frame "$t" || true

    # compress with upx ultra-brute lzma
    run upx --ultra-brute --lzma --no-progress "$t" >/dev/null 2>&1 || true

    echo "done: $t"
}

clean(){ run rm -rf "$b"; }

[ $# -eq 0 ] && build || case $1 in
    build) build ;;
    clean) clean ;;
    *) echo "usage: $0 [build|clean]"; exit 1 ;;
esac
