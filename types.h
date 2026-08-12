#ifndef WAKS_TYPES_H
#define WAKS_TYPES_H


#ifdef __cplusplus 
extern "C" { 
#endif

#if !defined(WAKS_NO_STDHEADERS)
    #include <stdint.h>
    #include <stddef.h>
    #include <stdbool.h>

    typedef uint8_t    waks_uchar;
    typedef uint16_t   waks_u16;
    typedef uint32_t   waks_u32;
    typedef uint64_t   waks_u64;

    typedef int8_t     waks_char;
    typedef int16_t    waks_i16;
    typedef int32_t    waks_i32;
    typedef int64_t    waks_i64;

    typedef size_t     waks_usize;
    typedef ptrdiff_t  waks_ssize;
    typedef uintptr_t  waks_uintptr;

    typedef bool       waks_bool;
    #define waks_true  true
    #define waks_false false
#else 

#if defined(__GNUC__) || defined(__clang__)
    typedef __UINT8_TYPE__   waks_uchar;
    typedef __UINT16_TYPE__  waks_u16;
    typedef __UINT32_TYPE__  waks_u32;
    typedef __UINT64_TYPE__  waks_u64;

    typedef __INT8_TYPE__    waks_char;
    typedef __INT16_TYPE__   waks_i16;
    typedef __INT32_TYPE__   waks_i32;
    typedef __INT64_TYPE__   waks_i64;

    typedef __SIZE_TYPE__    waks_usize;
    typedef __PTRDIFF_TYPE__ waks_ssize;
    typedef __UINTPTR_TYPE__ waks_uintptr;

/* MSVC (Microsoft Visual C++) Fallback */
#elif defined(_MSC_VER)
    typedef unsigned __int8  waks_uchar;
    typedef unsigned __int16 waks_u16;
    typedef unsigned __int32 waks_u32;
    typedef unsigned __int64 waks_u64;

    typedef signed __int8    waks_char;
    typedef signed __int16   waks_i16;
    typedef signed __int32   waks_i32;
    typedef signed __int64   waks_i64;

    #if defined(_WIN64)
        typedef unsigned __int64 waks_usize;
        typedef __int64          waks_ssize;
        typedef unsigned __int64 waks_uintptr;
    #else
        typedef unsigned int     waks_usize;
        typedef int              waks_ssize;
        typedef unsigned int     waks_uintptr;
    #endif

/* Universal Fallback (ANSI C / C89) */
#else
    typedef unsigned char      waks_uchar;
    typedef unsigned short     waks_u16;
    typedef unsigned int       waks_u32;

    typedef signed char        waks_char;
    typedef signed short       waks_i16;
    typedef signed int         waks_i32;

    #if defined(__GNUC__) || defined(__clang__)
        __extension__ typedef unsigned long long waks_u64;
        __extension__ typedef signed long long   waks_i64;
    #else
        typedef unsigned long long waks_u64;
        typedef signed long long   waks_i64;
    #endif

    typedef unsigned long      waks_usize;
    typedef long               waks_ssize;
    typedef unsigned long      waks_uintptr;
#endif

/* Freestanding Boolean Logic */
#if defined(__cplusplus)
    typedef bool waks_bool;
    #define waks_true  true
    #define waks_false false
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    typedef _Bool waks_bool;
    #define waks_true  1
    #define waks_false 0
#else
    typedef unsigned char waks_bool;
    #define waks_true  1
    #define waks_false 0
#endif

#endif // WAKS_NO_STDHEADERS

// compiler builtins (??? explain deeply) &&& WAKS_AUTO implementation
#define WAKS_COMPILER_PRAGMA(x) _Pragma(#x)

#if defined(__clang__) || defined(__GNUC__)
#	define WAKS_AUTO  _auto_type
#else 
#	error "WAKS_AUTO requires compiler support for __auto_type"
#endif

WAKS_COMPILER_PRAGMA(GCC diagnostic error "-Wswitch")
WAKS_COMPILER_PRAGMA(GCC diagnostic error "-Wimplicit-fallthrough")

typedef enum WaksResult {
#define X(code, string) code,
#include "waks_error.inc"
#undef X
	WAKS_ERR_COUNT
} WaksResult;

typedef struct WaksContext WaksContext;
struct WaksContext {
	waks_u64 sp;               // Offset 0
	waks_u64 pc;               // Offset 8
	waks_u64 rbx;              // Offset 16
	waks_u64 rbp;              // Offset 24
	waks_u64 r12;              // Offset 32
	waks_u64 r13;              // Offset 40
	waks_u64 r14;              // Offset 48
	waks_u64 r15;              // Offset 56
	waks_u64 arena_checkpoint; // Offset 64
	waks_i32 error_code;       // Offset 72
};

// we it is implemented in the assembly fie
// think of extern before
WaksContext global_panic_env;

#if defined(__GNUC__) || defined (__clang__)
	__attribute__((returns_twice)) extern waks_i64 waks_save_state(void);
#else 
	extern waks_i64 waks_save_state(void);
#endif 

// @TODO(waks-work): extern before to be thought
void waks_load_state(void);

enum any_tag {
	_tag_none = 0,
	_tag_i64,
	_tag_u64,
	_tag_char,
	_tag_bool,
	_tag_waks_err,
	_tag_ptr  = 0x7
};

// supports a 16 byte any_type
typedef struct Any Any;
struct Any {
    waks_u32 tag;     // 4 bytes
    waks_u32 _pad;    // 4 bytes padding
    union {
        waks_i64    i64;
        waks_u64    u64;
        waks_uchar  character;
        waks_bool   boolean;
        void       *ptr;
        WaksResult  waks_err;
    } payload;       // 8 bytes
};


// pattern matching macros 

#define match(x) switch((x).tag)
#define with  break

#define MatchInt(v, name)                                                   \
    case _tag_i64:;                                                         \
        waks_i64 name = (v).payload.i64;

#define MatchUint(v, name)                                                  \
    case _tag_u64:;                                                         \
        waks_u64 name = (v).payload.u64;

#define MatchChar(v, name)                                                  \
    case _tag_char:;                                                        \
        waks_uchar name = (v).payload.character;

#define MatchBool(v, name)                                                  \
    case _tag_bool:;                                                        \
        waks_bool name = (v).payload.boolean;

#define MatchNone(v) case _tag_none:;

#define MatchWaks(v, name)                                                  \
    case _tag_waks_err:;                                                    \
        WaksResult name = (v).payload.waks_err;

#define MatchPtr(v, name)                                                   \
    case _tag_ptr:;                                                         \
        void *name = (v).payload.ptr;

// basic string type and its implementation should follow
typedef struct waks_string waks_string;  
struct waks_string {
    waks_uchar *data;
    waks_usize  length;
};

#define WAKS_STR(literal) ((waks_string){(waks_uchar *)(literal), sizeof(literal) - 1})

// turns the raw c string provided to a string slice with the string and length
// waks_string string_slice = waks_str_from_cstr("hello there guys");
waks_string waks_str_from_cstr(const char *str);

// generates a substring from a string_slice from the string, start_pos, lenght of substring   
waks_string waks_str_sub(waks_string s, waks_usize start, waks_usize length);

// checks the equality between two string slice
waks_bool   waks_str_eq(waks_string a, waks_string b);

// checks the equality a string slice and a c string 
waks_bool   waks_str_eq_cstr(waks_string a, const char *b);

// checks for the prefix that the string starts with
waks_bool   waks_str_starts_with(waks_string s, waks_string prefix);

// checks for the suffix that the string ends with
waks_bool   waks_str_ends_with(waks_string s, waks_string suffix);

// they do search of the string
waks_ssize  waks_str_find_char(waks_string s, waks_uchar ch);
waks_ssize  waks_str_find(waks_string haystack, waks_string needle);

// trim whitespace from the string
waks_string waks_str_trim_left(waks_string s);
waks_string waks_str_trim_right(waks_string s);
waks_string waks_str_trim(waks_string s);


// Helpers
Any AnyStr(waks_string str);
Any AnyInt( waks_i64 value);
Any AnyUint(waks_u64 value);
Any AnyChar(waks_uchar value);
Any AnyBool(waks_bool strict);
Any AnyWaks(WaksResult result);
Any AnyNone(void);
Any AnyPtr(void *ptr);

waks_string from_cstr(const char *str);
const char *waks_strerror(WaksResult error);

#ifdef __cplusplus
}
#endif

#endif // !WAKS_TYPES_H

#ifdef WAKS_TYPE_IMPLEMENTATION 
#undef WAKS_TYPE_IMPLEMENTATION

/// Vector v = vector_init(arena, 10, sizeof(Any), user_id);
/// vector_push(&v, AnyInt(42));
/// vector_push(&v, AnyStr(from_cstr("Hello Waks")));
/// vector_push(&v, AnyBool(true));
Any AnyUint(waks_u64 value) {
    Any a;
    a.tag = _tag_u64;
    a._pad = 0;
    a.payload.u64 = value;
    return a;
}

Any AnyInt(waks_i64 value) {
    Any a;
    a.tag = _tag_i64;
    a._pad = 0;
    a.payload.i64 = value;
    return a;
}

Any AnyChar(waks_uchar value) {
    Any a;
    a.tag = _tag_char;
    a._pad = 0;
    a.payload.character = value;
    return a;
}

Any AnyBool(waks_bool value) {
    Any a;
    a.tag = _tag_bool;
    a._pad = 0;
    a.payload.boolean = value;
    return a;
}

Any AnyNone(void) {
    Any a;
    a.tag = _tag_none;
    a._pad = 0;
    a.payload.u64 = 0;
    return a;
}

Any AnyWaks(WaksResult result) {
    Any a;
    a.tag = _tag_waks_err;
    a._pad = 0;
    a.payload.waks_err = result;
    return a;
}

Any AnyPtr(void *ptr) {
    Any a;
    a.tag = _tag_ptr;
    a._pad = 0;
    a.payload.ptr = ptr;
    return a;
}

waks_string from_cstr(const char *str) {
    waks_usize len = 0;
    if (str) {
        while (str[len]) len++;
    }
    return (waks_string){(waks_uchar *)str, len};
}

const char *waks_strerror(WaksResult error) {
    switch (error) {
#define X(code, string) case code: return string;
    #include "waks_error.inc"
#undef X
    default:
        return "UNKNOWN WAKS_STRING";
    }
}

///
// string implementation
///

/** Create a waks_string from a null-terminated C string */
waks_string waks_str_from_cstr(const char *str) {
    waks_usize len = 0;
    if (str) {
        while (str[len]) len++;
    }
    return (waks_string){(waks_uchar *)str, len};
}

/** Create a sub-slice (substring). Bounds-checked to prevent buffer overruns */
waks_string waks_str_sub(waks_string s, waks_usize start, waks_usize length) {
    if (start >= s.length) {
        return (waks_string){s.data + s.length, 0};
    }
    waks_usize max_len = s.length - start;
    if (length > max_len) {
        length = max_len;
    }
    return (waks_string){s.data + start, length};
}

/** Check equality between two waks_strings */
waks_bool waks_str_eq(waks_string a, waks_string b) {
    if (a.length != b.length) return waks_false;
    if (a.data == b.data)     return waks_true;

    for (waks_usize i = 0; i < a.length; i++) {
        if (a.data[i] != b.data[i]) return waks_false;
    }
    return waks_true;
}

/** Check equality between a waks_string and a null-terminated C string */
waks_bool waks_str_eq_cstr(waks_string a, const char *b) {
    if (!b) return (a.length == 0);
    
    waks_usize i = 0;
    for (; i < a.length; i++) {
        if (b[i] == '\0' || a.data[i] != (waks_uchar)b[i]) {
            return waks_false;
        }
    }
    return (b[i] == '\0');
}

/** Returns true if string starts with given prefix */
waks_bool waks_str_starts_with(waks_string s, waks_string prefix) {
    if (prefix.length > s.length) return waks_false;
    return waks_str_eq(waks_str_sub(s, 0, prefix.length), prefix);
}

/** Returns true if string ends with given suffix */
waks_bool waks_str_ends_with(waks_string s, waks_string suffix) {
    if (suffix.length > s.length) return waks_false;
    return waks_str_eq(waks_str_sub(s, s.length - suffix.length, suffix.length), suffix);
}

/** Finds first occurrence of character in string. Returns index or -1 if not found */
waks_ssize waks_str_find_char(waks_string s, waks_uchar ch) {
    for (waks_usize i = 0; i < s.length; i++) {
        if (s.data[i] == ch) return (waks_ssize)i;
    }
    return -1;
}

/** Substring search using standard search loop. Returns index or -1 if not found */
waks_ssize waks_str_find(waks_string haystack, waks_string needle) {
    if (needle.length == 0)               return 0;
    if (needle.length > haystack.length) return -1;

    waks_usize max_idx = haystack.length - needle.length;
    for (waks_usize i = 0; i <= max_idx; i++) {
        waks_bool found = waks_true;
        for (waks_usize j = 0; j < needle.length; j++) {
            if (haystack.data[i + j] != needle.data[j]) {
                found = waks_false;
                break;
            }
        }
        if (found) return (waks_ssize)i;
    }
    return -1;
}

/** Trim leading whitespace */
waks_string waks_str_trim_left(waks_string s) {
    waks_usize start = 0;
    while (start < s.length) {
        waks_uchar c = s.data[start];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\v' && c != '\f') {
            break;
        }
        start++;
    }
    return waks_str_sub(s, start, s.length - start);
}

/** Trim trailing whitespace */
waks_string waks_str_trim_right(waks_string s) {
    waks_usize len = s.length;
    while (len > 0) {
        waks_uchar c = s.data[len - 1];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\v' && c != '\f') {
            break;
        }
        len--;
    }
    return waks_str_sub(s, 0, len);
}

/** Trim both leading and trailing whitespace */
waks_string waks_str_trim(waks_string s) {
    return waks_str_trim_right(waks_str_trim_left(s));
}

#endif // WAKS_TYPE_IMPLEMENTATION

