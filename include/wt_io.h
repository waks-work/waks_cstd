#ifndef IO_H
#define IO_H

#include "arena.h"

#define STR(s) (String){(u8 *)s, sizeof(s) - 1}
#define LOG(level, msg) log_msg(level, STR(msg))

/// usage: void waks_error_handle(const char *context, i32 err_code) {
/// usage:     io_print_fmt("[WAKS ERROR] %s: %s (Code: %d) \n", context,
/// usage:     waks_strerror(err_code), err_code);
/// usage:     LOG_FMT(LOG_ERROR,"ERROR", "Failed in %s with code %d", context,err_code)
/// usage: }
#define LOG_FMT(level, msg, fmt, ...)                                                              \
    do                                                                                             \
    {                                                                                              \
        log_msg(level, msg);                                                                       \
        io_print_fmt(fmt, __VA_ARGS__);                                                            \
        io_print(STR("\n"));                                                                       \
    } while (0)

/// Puts the variadic arguements on the stack or in registers __builtin uses the compiler
/// knowledge
typedef __builtin_va_list variadic_list;

/// Acts as a pointer or an iterator to find those arguements in memory
#define variadic_start(iterator, last_arg) __builtin_va_start(iterator, last_arg)

/// Grabs the data and moves forward
#define variadic_args(iterator, type) __builtin_va_arg(iterator, type)

/// Resets the stack or cleans up the compiler internal state
#define variadic_end(iterator) __builtin_va_end(iterator)

typedef enum
{
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
} LogLevel;

static inline void log_msg(LogLevel level, String msg);
static inline void dbg_print_(String str);
static inline void io_print(String);
static void io_print_hex(u64 value);
static inline void io_print_u64(u64 value);
static inline void io_print_fmt(const char *fmt, ...);
static inline void any_print(Any val);

static inline void any_print(Any value)
{
    match(value)
    {
        with MatchStr(value, str)
        {
            io_print(str);
        }

        with MatchChar(value, c)
        {
            io_print((String){&c, 1});
        }
        with MatchBool(value, b)
        {
            io_print(b ? from_cstr("true") : from_cstr("false"));
        }
        with MatchInt(value, i)
        {
            if (i == 0)
            {
                io_print(from_cstr("0"));
            }
            else
            {
                char buf[20]; // Big enough for i64
                int pos = 0;
                u64 num = (i < 0) ? (u64)-i : (u64)i;
                if (i < 0)
                    io_print(from_cstr("-"));
                while (num > 0)
                {
                    buf[pos++] = (char)((num % 10) + '0');
                    num /= 10;
                }
                while (pos > 0)
                    io_print((String){(u8 *)&buf[--pos], 1});
            }
        }
        with MatchUint(value, u)
        {
            if (u == 0)
            {
                io_print(from_cstr("0"));
            }
            else
            {
                char buf[20];
                int pos = 0;
                while (u > 0)
                {
                    buf[pos++] = (char)((u % 10) + '0');
                    u /= 10;
                }
                while (pos > 0)
                    io_print((String){(u8 *)&buf[--pos], 1});
            }
        }
        with MatchNone(value, n)
        {
            io_print(from_cstr("none"));
        }
        with MatchWaks(value, err)
        {
            io_print(from_cstr((char *)waks_strerror(err)));
        }
    }
}

static inline void io_print_fmt(const char *fmt, ...)
{
    variadic_list arguements;
    variadic_start(arguements, fmt);

    for (const char *pointer = fmt; *pointer != '\0'; pointer++)
    {
        if (*pointer != '%')
        {
            io_print((String){.data = (u8 *)pointer, .length = 1});
            continue;
        }

        pointer++;
        switch (*pointer)
        {
            case 's':
            {
                char *raw = variadic_args(arguements, char *);
                any_print(AnyStr(from_cstr(raw)));
                break;
            }
            case 'd':
            {
                i32 val = variadic_args(arguements, i32);
                any_print(AnyInt(val));
                break;
            }
            case 'u':
            {
                u64 val = variadic_args(arguements, u64);
                any_print(AnyUint(val));
                break;
            }
            case 'c':
            {
                // variadic_args promotes char to int
                u8 c = (u8)variadic_args(arguements, int);
                any_print(AnyChar(c));
                break;
            }
            case 'x':
            {
                u64 val = variadic_args(arguements, u64);
                io_print_hex(val); // Reuse your existing hex function
                break;
            }
            case '%':
            {
                any_print(AnyChar('%'));
                break;
            }
            default:
            {
                io_print(STR("?"));
                break;
            }
        }
    }
    variadic_end(arguements);
}

static inline void log_msg(LogLevel level, String msg)
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
    static int cursor_pos = 0;
    for (usize i = 0; i < str.length; i++)
        vga_buffer[cursor_pos++] = (u16)str.data[i] | (0x07 << 8);
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

// static void io_print_u64(u64 val) {
//    u8 buf[20]; // Max digits for a 64-bit uint
//    int i = 19;
//    do { buf[i--] = (val % 10) + '0'; val /= 10; } while (val > 0);
//    io_print((String){&buf[i + 1], (usize)(19 - i)});
// }

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
