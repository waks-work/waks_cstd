#ifndef IO_H
#define IO_H

#include "arena.h"
#include "types.h"

#define STR(s) (String){(u8 *)s, sizeof(s) - 1}
#define LOG(level, msg) log_msg(level, STR(msg))

typedef enum
{
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
} LogLevel;

void log_msg(LogLevel level, String msg);
static inline void dbg_print_(String str);
static inline void io_print(String);
static void io_print_hex(u64 value);
static inline void io_print_u64(u64 value);

void log_msg(LogLevel level, String msg)
{
    switch (level)
    {
        case LOG_INFO:
            io_print(STR("[INFO] "));
            break;
        case LOG_WARN:
            io_print(STR("[WARN] "));
            break;
        case LOG_ERROR:
            io_print(STR("[ERROR] "));
            break;
        case LOG_FATAL:
            io_print(STR("[FATAL] "));
            break;
    }
    io_print(msg);
    io_print(STR("\n"));
}

static inline void dbg_print_(String str)
{
#if defined(__linux__)
    syscall6(SYS_write, (long)str.data, (long)str.length, 0, 0, 0, 0);
#endif
}

static inline void io_print(String str)
{
#if defined(__linux__)
    // FD 1 is stdout. Your current code was passing data as the FD!
    syscall6(SYS_write, 1, (long)str.data, (long)str.length, 0, 0, 0);
#elif defined(_WIN32) || defined(_WIN64)
    // Windows uses WriteFile or WriteConsole
#else
    u16 *vga_buffer = (u16 *)0x8000;
    static int cursor = 0;
    for (usize i = 0; i < str.length; i++)
        vga_buffer[cursor_pos++] = (u16 *)str.data[i] | (0x07 << 8);
#endif
}

static inline void io_print_hex(u64 value)
{
    u8 buf[18];
    static const u8 hex_chars[] = "0123456789ABCDEF";
    buf[0] = '0';
    buf[1] = 'x';

    // Print all 16 digits for consistent memory address debugging
    for (int i = 15; i >= 0; i--)
    {
        buf[i + 2] = hex_chars[(value >> (i * 4)) & 0xF];
    }

    io_print((String){buf, 18});
}

static void io_print_u64(u64 val)
{
    u8 buf[20]; // Max digits for a 64-bit uint
    int i = 19;
    do
    {
        buf[i--] = (val % 10) + '0';
        val /= 10;
    } while (val > 0);

    io_print((String){&buf[i + 1], (usize)(19 - i)});
}

void example_use()
{
    Arena *arena = ArenaAlloc();
    Handle handle = BoxAlloc(arena, sizeof(int) * 10000, 1);
    io_print(STR("Allocated 10000 items at: "));
    io_print_hex(handle.offset);
    io_print(STR("\n"));
    ArenaRelease(arena);
}

#endif
