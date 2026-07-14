#!/usr/bin/env bash
set -e 

CC=clang
AR=ar

BUILD=build

mkdir -p "$BUILD"

# compile src/*.c files  in build/*.o
for file in *.h
do
    obj="$BUILD/$(basename "${file%.c}.o")"

    $CC \
        -std=c11 \
        -Wall \
        -Wextra \
        -pedantic \
        -Iinclude \
        -c "$file" \
        -o "$obj"
done

# Compile Assembly files
for file in *.s
do
    obj="$BUILD/$(basename "${file%.s}.o")"

    $CC \
        -c "$file" \
        -o "$obj"
done

# make them into one libwaks.a
$AR rcs \
    "$BUILD/libwaks_cstd.a" \
    "$BUILD"/*.o
