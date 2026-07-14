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
struct String {
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

// Detect architecture
#if defined(__x86_64__) || defined(_M_X64)
// x86-64: Use top 16 bits (48–63) for tagging
#define _PTR_MASK 0x0000FFFFFFFFFFFFULL
#define _EXTRACT_PTR(slot) (u8 *)(((i64)((slot) & _PTR_MASK) << 16) >> 16)
#define _TAG_SHIFT 48

#elif defined(__aarch64__) && defined(__ARM_64BIT_STATE)
// ARM64: Use Top Byte Ignore (TBI), bits 56–63
#define _PTR_MASK 0x00FFFFFFFFFFFFFFULL // Mask bits 56–63
#define _EXTRACT_PTR(slot)                                                     \
  (u8 *)((slot) & _PTR_MASK) // TBI: bits ignored on deref
#define _TAG_SHIFT 56

#elif defined(__riscv) && (__riscv_xlen == 64)
// RISC-V 64: Assume 48-bit addressing (SV48) if no pointer masking
#if defined(__riscv_zicbom) || defined(__riscv_zicboz)
// Use compressed instructions or cache hints (optional)
#endif
#define _PTR_MASK 0x0000FFFFFFFFFFFFULL
#define _EXTRACT_PTR(slot)                                                     \
  (u8 *)((slot) & _PTR_MASK) // No sign-extension needed
#define _TAG_SHIFT 48

#else
// Fallback: Use lower bits (if aligned)
#warning "Using lower-bit tagging for portability"
#define _PTR_MASK 0xFFFFFFFFFFFFFFF0ULL // Mask low 4 bits
#define _EXTRACT_PTR(slot) (u8 *)((slot) & _PTR_MASK)
#define _TAG_SHIFT 0 // Tags in bits 0–3
#endif

#define match(x) switch (__builtin_expect((x)._slot0 >> _TAG_SHIFT, _tag_i64))
#define with break;

typedef enum WaksResult {
#define X(code, string) code,
#include "waks_error.inc"
#undef X
  WAKS_ERR_COUNT
} WaksResult;

typedef struct WaksContext WaksContext;
struct WaksContext {
  u64 sp;               // Offset 0
  u64 pc;               // Offset 8
  u64 rbx;              // Offset 16
  u64 rbp;              // Offset 24
  u64 r12;              // Offset 32
  u64 r13;              // Offset 40
  u64 r14;              // Offset 48
  u64 r15;              // Offset 56
  u64 arena_checkpoint; // Offset 64
  i32 error_code;       // Offset 72
};

extern WaksContext global_panic_env;

__attribute__((returns_twice)) extern int waks_save_state(void);
extern void waks_load_state(void);

/**
 * AnyType: Boxes a Type  into an Any type to be used.
 * Bottom 48 bits: Pointer address
 * Top 16 bits: _tag_ptr
 */

enum any_tag {
  _tag_none,
  _tag_str,
  _tag_i64,
  _tag_u64,
  _tag_char,
  _tag_bool,
  _tag_waks_err,
  _tag_ptr = 0x7
};

union any_union {
  String _str;
  i64 _i64;
  u64 _u64;
  u8 _char;
  b8 _bool;
  b8 _none;
  void *_ptr;
  WaksResult _waks_err;
};

typedef struct {
  u64 _slot0; // Tag (Top 16) + Pointer/Value (Bottom 48)
  u64 _slot1; // String.length && secondary data
} Any;

/// for (usize i = 0; i < v.length; i++) {
///     Any item = *vector_get(arena, &v, i);
///     match(item) {
///         MatchStr(item, s) { io_print(s); io_print(STR("\n")); } with;
///         default: break;
///     }
/// }
#define MatchStr(v, name)                                                      \
  case _tag_str:;                                                              \
    String name = {.data = _EXTRACT_PTR((v)._slot0),                           \
                   .length = (usize)(v)._slot1};

/// Any item = vector_get(arena, &v, i);
/// match(item) { MatchInt(item, val) { dbg_print_int((i16)val); } with;
/// default: break; }
#define MatchInt(v, name)                                                      \
  case _tag_i64:;                                                              \
    i64 name = (i64)(((i64)((v)._slot0 & _PTR_MASK) << 16) >> 16);

#define MatchUint(v, name)                                                     \
  case _tag_u64:;                                                              \
    u64 name = (u64)((v)._slot0 & _PTR_MASK) | ((u64)(v)._slot1 << 48);

#define MatchChar(v, name)                                                     \
  case _tag_char:;                                                             \
    u8 name = (u8)(((v)._slot0 & 0xFF));

#define MatchBool(v, name)                                                     \
  case _tag_bool:;                                                             \
    b8 name = (b8)((v)._slot0 & 0x1);

#define MatchNone(v) case _tag_none:;

/// Any result = some_logic();
/// match(result) { MatchWaks(result, err) { return err; } with; }
#define MatchWaks(v, name)                                                     \
  case _tag_waks_err:;                                                         \
    WaksResult name = (WaksResult)((v)._slot0 & _PTR_MASK);

/*
 * Extract pointer back to the match statement
 * */
#define MatchPtr(v, name)                                                      \
  case _tag_ptr:;                                                              \
    void *name = (void *)((v)._slot0 & _PTR_MASK);

static inline Any AnyStr(String str);
static inline Any AnyInt(i64 value);
static inline Any AnyUint(u64 value);
static inline Any AnyChar(u8 value);
static inline Any AnyBool(b8 strict);
static inline Any AnyWaks(WaksResult result);
static inline Any AnyNone(void);
static inline Any AnyPtr(void *ptr);

/// Vector v = vector_init(arena, 10, sizeof(Any), user_id);
/// vector_push(&v, AnyInt(42));
/// vector_push(&v, AnyStr(from_cstr("Hello Waks")));
/// vector_push(&v, AnyBool(true));
static inline Any AnyStr(String str) {
  return (Any){
      // Mask the pointer and OR the tag into the top 16 bits
      ._slot0 = ((u64)str.data & _PTR_MASK) | ((u64)_tag_str << _TAG_SHIFT),
      ._slot1 = (u64)str.length,
  };
}

static inline Any AnyUint(u64 value) {
  return (Any){
      ._slot0 = (value & _PTR_MASK) | ((u64)_tag_u64 << _TAG_SHIFT),
      ._slot1 = (value >> 48),
  };
}

static inline Any AnyInt(i64 value) {
  return (Any){
      ._slot0 = ((u64)value & _PTR_MASK) | ((u64)_tag_i64 << _TAG_SHIFT),
      ._slot1 = 0,
  };
}

static inline Any AnyChar(u8 value) {
  return (Any){
      ._slot0 = ((u64)value & _PTR_MASK) | ((u64)_tag_char << _TAG_SHIFT),
      ._slot1 = 0,
  };
}

static inline Any AnyBool(b8 value) {
  return (Any){
      ._slot0 = ((u64)value & _PTR_MASK) | ((u64)_tag_bool << _TAG_SHIFT),
      ._slot1 = 0,
  };
}

/** AnyNone: Boxes a none type that returns nothing **/
static inline Any AnyNone(void) {
  return (Any){._slot0 = (u64)_tag_none << _TAG_SHIFT, ._slot1 = 0};
}

/**  AnyWaks: Boxes an error type into an Any type. */
static inline Any AnyWaks(WaksResult result) {
  return (Any){
      ._slot0 = ((u64)result & _PTR_MASK) | ((u64)_tag_waks_err << _TAG_SHIFT),
      ._slot1 = 0,
  };
}

/** AnyPtr: Boxes a raw pointer into an Any type. */
static inline Any AnyPtr(void *ptr) {
  return (Any){
      ._slot0 = ((u64)ptr & _PTR_MASK) | ((u64)_tag_ptr << _TAG_SHIFT),
      ._slot1 = 0,
  };
}
#pragma GCC poison _internal_tag _internal_field _str _i64 _u64 _char _bool

static inline String from_cstr(char *str);
/// Error Logic
static inline const char *waks_strerror(WaksResult error);

static inline String from_cstr(char *str) {
  usize len = 0;
  while (str[len])
    len++;
  return (String){(u8 *)str, len};
}

static inline const char *waks_strerror(WaksResult error) {
  switch (error) {
#define X(code, string)                                                        \
  case code:                                                                   \
    return string;
#include "waks_error.inc"
#undef X
  default:
    return "UNKNOWN STRING";
  }
}

#endif // !TYPES_H
