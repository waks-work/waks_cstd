cmake_minimum_required(VERSION 3.15)
project(waks_cstd C)

# standard C and Perfomance flags 
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # for neovim lsp(clangd)

# project paths
set(WAKS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(WAKS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Build option
message(STATUS "Source Dir: ${WAKS_SRC_DIR}")
option(WAKS_BUILD_TESTS "Build test suite" ON)
option(WAKS_FREESTANDING "Build without standard library (for mini_os)" OFF )

# Compiler Flags - Strict Setup
add_compile_options(
  -Wall -Wextra -Wpedantic
  -Wshadow -Wpointer-arith -Wcast-align
  -Wmissing-prototypes -Wstrict-prototypes
)

if (WAKS_FREESTANDING)
  # essential for mini_os progress
  add_compile_options(-ffreestanding -nostdlib -fno-stack-protector -fno-pie)
  add_definitions(-DWAKS_FREESTANDING)
endif()

# Core Library Target: a static library used for easy linking with mini_os kernel
file(GLOB_RECURSE WAKS_SOURCES "${WAKS_SRC_DIR}/*.c")

if(WAKS_SOURCES)
    # We only filter main.c out of the LIBRARY if there are other files.
    # If main.c is the only file, we'll let it stay for now just to fix the error,
    # but ideally, you should create a dummy file like src/waks_init.c
    list(LENGTH WAKS_SOURCES SRC_COUNT)
    if(${SRC_COUNT} GREATER 1)
        list(FILTER WAKS_SOURCES EXCLUDE REGEX "main.c$")
    endif()

    add_library(waks_cstd STATIC ${WAKS_SOURCES})
    target_include_directories(waks_cstd PUBLIC ${WAKS_INCLUDE_DIR})
else()
    message(FATAL_ERROR "No source files found in ${WAKS_SRC_DIR}")
endif()
# public headers are include and private in src
target_include_directories(waks_cstd PUBLIC ${WAKS_INCLUDE_DIR})

# executable 
if (NOT WAKS_FREESTANDING)
  if(EXISTS "${WAKS_SRC_DIR}/main.c")
        add_executable(waks_test "${WAKS_SRC_DIR}/main.c")
        target_link_libraries(waks_test waks_cstd)
    endif()
endif()
