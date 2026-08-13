#!/usr/bin/env bash
set -e 

CC=clang
AR=ar

BUILD=build
SRC_DIR="src" 
TEST_DIR="tests"

mkdir -p "$BUILD"

#Create a dynamic unity implementation wrapper if src/waks.c does not exist
UNITY_C="$BUILD/waks_unity.c"
cat << 'EOF' > "$UNITY_C"
#define WAKS_IMPLEMENTATION
#define WAKS_IO_IMPLEMENTATION
#include "../waks.h"
EOF

# Compile Waks Core
$CC \
    -std=c11 \
    -Wall \
    -Wextra \
    -pedantic \
    -ffreestanding \
    -fno-stack-protector \
    -I. \
    -c "$UNITY_C" \
    -o "$BUILD/waks.o"

# Assemble any assembly files if present
for file in *.s
do
    [ -f "$file" ] || continue
    obj="$BUILD/$(basename "${file%.s}.o")"
    $CC -c "$file" -o "$obj"
done

# Pack into static library
$AR rcs "$BUILD/libwaks.a" "$BUILD"/*.o
echo "--> Created $BUILD/libwaks.a"

echo ""
echo "=== Building & Running Test Suite ==="

# 5. Compile and link test runner with nostdlib flags
TEST_SRC="$TEST_DIR/test.c"

if [ -f "$TEST_SRC" ]; then
    $CC \
        -std=c11 \
        -Wall \
        -Wextra \
        -ffreestanding \
        -fno-stack-protector \
        -nostdlib \
        -I. \
        "$TEST_SRC" \
        "$BUILD/libwaks.a" \
        -o "$BUILD/test_runner"

    echo "--> Built $BUILD/test_runner successfully."
    echo "--> Running Tests:"
    echo "----------------------------------------"
    
    # Run the test binary directly
    ./"$BUILD/test_runner"
else
    echo "Warning: $TEST_SRC not found, skipping test run."
fi
