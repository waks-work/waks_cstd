#ifndef TYPES_H
#define TYPES_H

typedef __UINT8_TYPE__ u8; // char
typedef __UINT16_TYPE__ u16;
typedef __UINT32_TYPE__ u32;
typedef __UINT64_TYPE__ u64;

typedef __INT8_TYPE__ i8;
typedef __INT16_TYPE__ i16;
typedef __INT32_TYPE__ i32;
typedef __INT64_TYPE__ i64;

typedef float f32;
typedef double f64;

typedef __SIZE_TYPE__ usize;
typedef __PTRDIFF_TYPE__ ssize;
typedef _Bool b8;

#ifndef __cplusplus
#define true 1
#define false 0
#endif

typedef struct String String;
struct String
{
    u8 *data;
    usize length;
};

#if defined(__clang__)
#define COMPILER_PRAGMA(x) _Pragma(#x)
#define AUTO __auto_type
#elif defined(__GNUC__)
#define COMPILER_PRAGMA(x) _Pragma(#x)
#define AUTO auto
#else
#define COMPILER_PRAGMA(x) _Pragma(#x)
#define AUTO ____auto_type
#endif

COMPILER_PRAGMA(GCC diagnostic error "-Wswitch")
COMPILER_PRAGMA(GCC diagnostic error "-Wimplicit-fallthrough")

#define tagged_union(_tag, _union)                                                                 \
    struct __attribute__((designated_init))                                                        \
    {                                                                                              \
        enum _tag _internal_tag;                                                                   \
        union _union _internal_field;                                                              \
    }

#define tunion_constructor(_T, _tag, _field, _value)                                               \
    (_T)                                                                                           \
    {                                                                                              \
        ._internal_tag = _tag, ._internal_field._field = (_value)                                  \
    }

#define tunion_eliminator(_value, _tag, _field, _name)                                             \
    _tag:                                                                                          \
    AUTO _name = ((_value)._internal_field._field);

#define match(x) switch ((x)._internal_tag)
#define with                                                                                       \
    break;                                                                                         \
    case

typedef enum WaksResult WaksResult;
enum WaksResult
{
#define X(code, string) code,
#include "waks_error.inc"
#undef X
    WAKS_ERR_COUNT
};

enum any_tag
{
    _tag_none,
    _tag_str,
    _tag_i64,
    _tag_u64,
    _tag_char,
    _tag_bool,
    _tag_waks_err
};

union any_union
{
    String _str;
    i64 _i64;
    u64 _u64;
    u8 _char;
    b8 _bool;
    b8 _none;
    WaksResult _waks_err;
};

typedef tagged_union(any_tag, any_union) Any;

#define AnyStr(val) tunion_constructor(Any, _tag_str, _str, val)
#define AnyInt(val) tunion_constructor(Any, _tag_i64, _i64, (i64)val)
#define AnyUint(val) tunion_constructor(Any, _tag_u64, _u64, (u64)val)
#define AnyChar(val) tunion_constructor(Any, _tag_char, _char, (u8)val)
#define AnyBool(val) tunion_constructor(Any, _tag_bool, _bool, val)
#define AnyNone() tunion_constructor(Any, _tag_none, _none, false)
#define AnyWaks(val) tunion_constructor(Any, _tag_waks_err, _waks_err, val)

#define MatchStr(v, name) tunion_eliminator(v, _tag_str, _str, name)
#define MatchInt(v, name) tunion_eliminator(v, _tag_i64, _i64, name)
#define MatchUint(v, name) tunion_eliminator(v, _tag_u64, _u64, name)
#define MatchChar(v, name) tunion_eliminator(v, _tag_char, _char, name)
#define MatchBool(v, name) tunion_eliminator(v, _tag_bool, _bool, name)
#define MatchNone(v, n) tunion_eliminator(v, _tag_none, _none, n)
#define MatchWaks(v, name) tunion_eliminator(v, _tag_waks_err, _waks_err, name)

#pragma GCC poison _internal_tag _internal_field _str _i64 _u64 _char _bool

static inline String from_cstr(char *str);
/// Error Logic
static inline const char *waks_strerror(WaksResult error);

static inline String from_cstr(char *str)
{
    usize len = 0;
    while (str[len])
        len++;
    return (String){(u8 *)str, len};
}

static inline const char *waks_strerror(WaksResult error)
{
    switch (error)
    {
#define X(code, string)                                                                            \
    case code:                                                                                     \
        return string;
#include "waks_error.inc"
#undef X
        default:
            return "UNKNOWN STRING";
    }
}

#endif // !TYPES_H
