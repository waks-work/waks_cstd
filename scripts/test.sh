#!/usr/bin/env bash
set -e

./scripts/build.sh

mkdir -p build

clang \
    tests/arena_test.c \
    build/libwaks_cstd.a \
    -Iinclude \
    -o build/arena_test

./build/arena_test
