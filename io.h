
#ifndef WAKS_IO_H
#define WAKS_IO_H

#include "container.h"

#ifdef __cplusplus 
extern "C" { 
#endif

// This macro allows us to convert a string to a waks_string slice
#define WAKS_2STR(s) (waks_string){(waks_uchar *)(s), sizeof((s)) - 1}

// This macro allows us to log an error messages 
#define WAKS_LOG(level, msg) waks_log_msg((level), WAKS_2STR((msg)))

// This macro allows us to log formatted error messages allowing 
// for easier debugging.
#define WAKS_LOG_FMT(level, msg, fmt, ...)     \
    do {                                       \
        waks_log_msg((level), (msg));          \
        waks_io_print_fmt((fmt), __VA_ARGS__); \
        waks_io_print(WAKS_2STR("\n"));              \
    } while (0)

// This is the variadic arguements on the stack or in registers
typedef __builtin_va_list waks_variadic_list;

// This acts as a pointer or an iterator to find those arguements in memory
#define WAKS_variadic_start(iterator, last_arg) __builtin_va_start((iterator), (last_arg))

// This Grabs the data and moves forward
#define WAKS_variadic_args(iterator, type) __builtin_va_arg(iterator, type)

// This resets the stack or cleans up the compiler internal state
#define WAKS_variadic_end(iterator) __builtin_va_end((iterator))

// This categorises the severity level for system application and logs.
typedef enum 
{ 
	LOG_INFORMATION, // Informational messages for general operational tracking.
	LOG_WARNINGS,    // Warning events that indicate non-critical abnormalies.
	LOG_ERRORS,      // Recoverable runtime error conditions 
	LOG_FATALS       // Critical system failure that typically precedes process panic/exit.
} waks_log_level;

// This outputs a structured log entry prefixed with its severity tag.
void waks_log_msg(waks_log_level level, waks_string msg);

// This is a lowlevel debug print function routing directly to stderr.
void waks_dbg_print_(waks_string str);

// This is the primary standard output stream printer 
// (fd 1 or VGA buffer fallback).
void waks_io_print(waks_string str);

// This prints a 64 bit unsigned interger as a 16-digit 
// hexadecimal string with the prefix 0x
void waks_io_print_hex(waks_u64 value);

// This prints a 64-bit unsigned integer in decimal format
// to standard output.
void waks_io_print_u64(waks_u64 value);

// This is a formatted I/O print function supporting custom 
// format specifiers.
// Supported specifiers are: 
//   - `%s`: String pointer
//   - `%d`: Signed 32-bit integer
//   - `%u`: Unsigned 64-bit integer
//   - `%c`: Character
//   - `%x`: Hexadecimal 64-bit integer
//   - `%%`: Literal percent symbol
void waks_io_print_fmt(const waks_char *fmt, ...);

// Dynamic polymorphic print function that inspects an `Any` 
// type container and renders its inner value accordingly.
void waks_any_print(Any val);

#ifdef __cplusplus 
} 
#endif

#endif // WAKS_IO_H


#ifdef WAKS_IO_IMPLEMENTATION 

/* Caters for io arguements matched from any type */
void waks_any_print(Any value)
{
    match(value)
    {
        MatchChar(value, c) {
            waks_io_print((waks_string){&c, 1});
        } break;

        MatchBool(value, b) {
            waks_io_print(b ? WAKS_STR("true") : WAKS_STR("false"));
        } break;

        MatchInt(value, i) {
            if (i == 0) {
                waks_io_print(WAKS_2STR("0"));
            } else {
                waks_char buf[32];
                waks_i32  pos = 0;
                
                // Safe negation handling for INT64_MIN
                waks_u64 num;
                if (i < 0) {
                    waks_io_print(WAKS_2STR("-"));
                    num = (waks_u64)(-(i + 1)) + 1;
                } else num = (waks_u64)i;

                while (num > 0) {
                    buf[pos++] = (waks_char)((num % 10) + '0');
                    num /= 10;
                }

                while (pos > 0) waks_io_print((waks_string){(waks_uchar *)&buf[--pos], 1});
            }
        } break;

        MatchUint(value, u) {
            if (u == 0) {
                waks_io_print(WAKS_STR("0"));
            } else {
                waks_char buf[20];
                waks_i32  pos = 0;
                while (u > 0) {
                    buf[pos++] = (char)((u % 10) + '0');
                    u /= 10;
                }
                while (pos > 0) waks_io_print((waks_string){(waks_uchar *)&buf[--pos], 1});
            }
        } break;

        MatchNone(value) {
            waks_io_print(WAKS_STR("none"));
        } break;

        MatchWaks(value, err) {
            waks_io_print(WAKS_STR((char *)waks_strerror(err)));
        } break;

        MatchPtr(value, ptr) {
            if (ptr == WAKS_NOVALUE || ptr == NULL) {
                waks_io_print(WAKS_2STR("(null)"));
            } else {
                waks_io_print(WAKS_2STR("0x"));
                waks_io_print_hex((waks_u64)(waks_uintptr)ptr);
			}
        } break;
    }
}

void waks_io_print_fmt(const waks_char *fmt, ...)
{
    waks_variadic_list arguements;
    WAKS_variadic_start(arguements, fmt);

    for (const waks_char *pointer = fmt; *pointer != '\0'; pointer++) {
        if (*pointer != '%') {
            waks_io_print((waks_string){.data = (waks_uchar *)pointer, .length = 1});
            continue;
        }

        pointer++;
        switch (*pointer) {
            case 's': {
                waks_char *raw = WAKS_variadic_args(arguements, waks_char *);
				if (raw) {
					waks_string str = WAKS_STR(raw);
                    waks_io_print(str);
				}
                break;
            }
            case 'd': {
                waks_i32 val = WAKS_variadic_args(arguements, waks_i32);
                waks_any_print(AnyInt(val));
                break;
            }
            case 'u': {
                waks_u64 val = WAKS_variadic_args(arguements, waks_u64);
                waks_any_print(AnyUint(val));
                break;
            }
            case 'c': {
                // variadic_args promotes char to int
                waks_uchar c = (waks_uchar)WAKS_variadic_args(arguements, waks_i32);
                waks_any_print(AnyChar(c));
                break;
            }
            case 'x': {
                waks_u64 val = WAKS_variadic_args(arguements, waks_u64);
                waks_io_print_hex(val); // Reuse your existing hex function
                break;
            }
            case '%': {
                waks_any_print(AnyChar('%'));
                break;
            }
            default: {
                waks_io_print(WAKS_2STR("?"));
                break;
            }
        }
    }
    WAKS_variadic_end(arguements);
}

void waks_log_msg(waks_log_level level, waks_string msg)
{
    switch (level) {
        case LOG_INFORMATION: {
            waks_io_print(WAKS_2STR("[INFO] "));
            break;
        }
        case LOG_WARNINGS: {
            waks_io_print(WAKS_2STR("[WARN] "));
            break;
        }
        case LOG_ERRORS: {
            waks_io_print(WAKS_2STR("[ERROR] "));
            break;
        }
        case LOG_FATALS: {
            waks_io_print(WAKS_2STR("[FATAL] "));
            break;
        }
    }
    waks_io_print(msg);
    waks_io_print(WAKS_2STR("\n"));
}

void waks_dbg_print_(waks_string str)
{
#if defined(__linux__)
    waks_syscall6(SYS_write, 2, (long)str.data, (long)str.length, 0, 0, 0);
#endif
}

void waks_io_print(waks_string str)
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
void waks_io_print_hex(waks_u64 value)
{
    waks_uchar buf[18];
    static const waks_uchar hex_chars[] = "0123456789ABCDEF";
    buf[0] = '0';
    buf[1] = 'x';

    for (int i = 15; i >= 0; i--) {
        buf[17 - i] = hex_chars[(value >> (i * 4)) & 0xF];
    }
    waks_io_print((waks_string){buf, 18});
}

void waks_io_print_u64(waks_u64 value)
{
    if (value == 0) {
        waks_io_print(WAKS_2STR("0"));
        return;
    }

    waks_char buf[20];
    waks_i32 pos = 0;
    while (value > 0) {
        buf[pos++] = (waks_char)((value % 10) + '0');
        value /= 10;
    }

    while (pos > 0) {
        waks_io_print((waks_string){(waks_uchar *)&buf[--pos], 1});
    }
}

#endif // WAKS_IO_IMPLEMENTATION
