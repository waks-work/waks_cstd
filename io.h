#ifndef WAKS_IO_H
#define WAKS_IO_H

#include "allocator.h"
#include "types.h"

#define WAKS_2STR(s)    (waks_string){(waks_uchar *)s, sizeof(s) - 1}
#define LOG(level, msg) log_msg(level, STR(msg))

/// usage: void waks_error_handle(const char *context, i32 err_code) {
/// usage:     io_print_fmt("[WAKS ERROR] %s: %s (Code: %d) \n", context,
/// usage:     waks_strerror(err_code), err_code);
/// usage:     LOG_FMT(LOG_ERROR,"ERROR", "Failed in %s with code %d",
/// usage:     context,err_code) usage: }
#define LOG_FMT(level, msg, fmt, ...)                                                              \
    do {                                                                                           \
        log_msg(level, msg);                                                                       \
        io_print_fmt(fmt, __VA_ARGS__);                                                            \
        io_print(STR("\n"));                                                                       \
    } while (0)

/// Puts the variadic arguements on the stack or in registers
/// The __builtin uses the compiler knowledge
typedef __builtin_va_list variadic_list;

/// Acts as a pointer or an iterator to find those arguements in memory
#define variadic_start(iterator, last_arg) __builtin_va_start(iterator, last_arg)

/// Grabs the data and moves forward
#define variadic_args(iterator, type) __builtin_va_arg(iterator, type)

/// Resets the stack or cleans up the compiler internal state
#define variadic_end(iterator) __builtin_va_end(iterator)

typedef enum 
{ 
	LOG_INFORMATION, 
	LOG_WARNINGS,
	LOG_ERRORS,
	LOG_FATALS 
} LogLevel;

static inline void log_msg(LogLevel level, waks_string msg);
static inline void dbg_print_(waks_string str);
static inline void io_print(waks_string);
static        void io_print_hex(waks_u64 value);
static inline void io_print_u64(waks_u64 value);
static inline void io_print_fmt(const waks_char *fmt, ...);
static inline void any_print(Any val);

/* Caters for io arguements matched from any type */
static inline void any_print(Any value)
{
    match(value)
    {
        // MatchStr(value, str)
        // {
        //     io_print(str);
        // }
        // break;

        MatchChar(value, c)
        {
            io_print((waks_string){&c, 1});
        }
        break;
        MatchBool(value, b)
        {
            io_print(b ? from_cstr("true") : from_cstr("false"));
        }
        break;
        MatchInt(value, i)
        {
            if (i == 0) {
                io_print(from_cstr("0"));
            } else {
                waks_char buf[20]; // Big enough for i64
                waks_i32  pos = 0;
                waks_u64  num = (i < 0) ? (waks_u64)-i : (waks_u64)i;

                if (i < 0) io_print(from_cstr("-"));
                while (num > 0) {
                    buf[pos++] = (char)((num % 10) + '0');
                    num /= 10;
                }

                while (pos > 0) io_print((waks_string){(waks_uchar *)&buf[--pos], 1});
            }
        }
        break;
        MatchUint(value, u)
        {
            if (u == 0) {
                io_print(from_cstr("0"));
            } else {
                waks_char buf[20];
                waks_i32  pos = 0;
                while (u > 0) {
                    buf[pos++] = (char)((u % 10) + '0');
                    u /= 10;
                }
                while (pos > 0) io_print((waks_string){(waks_uchar *)&buf[--pos], 1});
            }
        }
        break;
        MatchNone(value)
        {
            io_print(from_cstr("none"));
        }
        break;
        MatchWaks(value, err)
        {
            io_print(from_cstr((char *)waks_strerror(err)));
        }
        break;
        MatchPtr(value, ptr)
        {
            if (ptr == WAKS_NOVALUE) io_print(from_cstr("Invalid memory not allocated: ptr (null)"));
            io_print(from_cstr("Memory allocated successfully: ptr (Ox)"));

            waks_uintptr addr = (waks_uintptr)ptr;
            waks_char buf[16];
            waks_u64  pos = 0;

            while (addr > 0) {
                waks_u64 rem = addr % 16;
                buf[pos++] = (char)((rem < 10) ? (rem + '0') : (rem - 10 + 'a'));
                addr /= 16;
            }
            while (pos > 0) io_print((waks_string){(waks_uchar *)&buf[--pos], 1});
        }
        break;
    }
}

static inline void io_print_fmt(const waks_char *fmt, ...)
{
    variadic_list arguements;
    variadic_start(arguements, fmt);

    for (const char *pointer = fmt; *pointer != '\0'; pointer++) {
        if (*pointer != '%') {
            io_print((waks_string){.data = (waks_uchar *)pointer, .length = 1});
            continue;
        }

        pointer++;
        switch (*pointer) {
            case 's': {
                char *raw = variadic_args(arguements, waks_char *);
                any_print(AnyStr(from_cstr(raw)));
                break;
            }
            case 'd': {
                waks_i32 val = variadic_args(arguements, waks_i32);
                any_print(AnyInt(val));
                break;
            }
            case 'u': {
                waks_u64 val = variadic_args(arguements, waks_u64);
                any_print(AnyUint(val));
                break;
            }
            case 'c': {
                // variadic_args promotes char to int
                waks_uchar c = (waks_uchar)variadic_args(arguements, waks_i32);
                any_print(AnyChar(c));
                break;
            }
            case 'x': {
                waks_u64 val = variadic_args(arguements, waks_u64);
                io_print_hex(val); // Reuse your existing hex function
                break;
            }
            case '%': {
                any_print(AnyChar('%'));
                break;
            }
            default: {
                io_print(STR("?"));
                break;
            }
        }
    }
    variadic_end(arguements);
}

static inline void log_msg(LogLevel level, waks_string msg)
{
    switch (level) {
        case LOG_INFORMATION: {
            io_print(STR("[INFO] "));
            break;
        }
        case LOG_WARNINGS: {
            io_print(STR("[WARN] "));
            break;
        }
        case LOG_ERRORS: {
            io_print(STR("[ERROR] "));
            break;
        }
        case LOG_FATALS: {
            io_print(STR("[FATAL] "));
            break;
        }
    }
    io_print(msg);
    io_print(STR("\n"));
}

static inline void dbg_print_(waks_string str)
{
#if defined(__linux__)
    waks_syscall6(SYS_write, (long)str.data, (long)str.length, 0, 0, 0, 0);
#endif
}

static inline void io_print(waks_string str)
{
#if defined(__linux__)
    // FD 1 is stdout. Your current code was passing data as the FD!
    waks_syscall6(SYS_write, 1, (long)str.data, (long)str.length, 0, 0, 0);
#elif defined(_WIN32) || defined(_WIN64)
    // Windows uses WriteFile or WriteConsole
#else
    waks_u16 *vga_buffer = (waks_u16 *)0x8000;
    static waks_i32 cursor_pos = 0;
    for (waks_usize i = 0; i < str.length; i++)
        vga_buffer[cursor_pos++] = (waks_u16)str.data[i] | (0x07 << 8);
#endif
}

static inline void io_print_hex(waks_u64 value)
{
    waks_uchar buf[18];
    static const waks_uchar hex_chars[] = "0123456789ABCDEF";
    buf[0] = '0';
    buf[1] = 'x';

    // Print all 16 digits for consistent memory address debugging
    for (int i = 15; i >= 0; i--) {
        buf[i + 2] = hex_chars[(value >> (i * 4)) & 0xF];
    }

    io_print((waks_string){buf, 18});
}

#endif
