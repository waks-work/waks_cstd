
/*
*  waks_type name to the types 
*    - each file must be able to survive with itself without any dependency to any 
*      other external file ie containers.h, allocators.h, 
*    - have a malloc implementation also so it can survive without the need of arenas 
*    - make sure it works, easy to use and works as expected.
*    - ensure it doesn't conflict with the stdlib in cases where stblib may be used.
*  */

#ifndef ARENA_H
#define ARENA_H

#ifndef WAKS_TYPES_H
#define WAKS_TYPES_H

// basic unsigned types
typedef __UINT8_TYPE__  u8;  // waks_uchar
typedef __UINT16_TYPE__ u16; // waks_u16
typedef __UINT32_TYPE__ u32; // waks_u32
typedef __UINT64_TYPE__ u64; // waks_u64

// basic signed types
typedef __INT8_TYPE__  i8;   // waks_char
typedef __INT16_TYPE__ i16;  // waks_i16
typedef __INT32_TYPE__ i32;  // waks_i32
typedef __INT64_TYPE__ i64;  // waks_i64

// float types
typedef float  f32;          // waks_float
typedef double f64;          // waks_double

// size types
typedef __SIZE_TYPE__    usize; // waks_usize
typedef __PTRDIFF_TYPE__ ssize; // waks_ssize
typedef __INT64_TYPE__ uintptr_t; // waks_uintptr

// basic size types
typedef _Bool            b8;    // waks_bool
#ifndef __cplusplus
#	define true  1     // waks_true
#	define false 0     // waks_false
#endif

// basic string type and its implementation should follow
typedef struct String String; // waks_string
struct String {
    u8    *data;
    usize length;
};

// compiler builtins (??? explain deeply) &&& waks_auto implementation
#if defined(__clang__)
#	define COMPILER_PRAGMA(x) _Pragma(#x)
#	define AUTO               __auto_type
#elif defined(__GNUC__)
#	define COMPILER_PRAGMA(x) _Pragma(#x)
#	define AUTO auto                        // waks_auto
#else
#	define COMPILER_PRAGMA(x) _Pragma(#x)
#	define AUTO               ____auto_type  // waks_auto
#endif

COMPILER_PRAGMA(GCC diagnostic error "-Wswitch")
COMPILER_PRAGMA(GCC diagnostic error "-Wimplicit-fallthrough")

// Detect architecture
#if defined(__x86_64__) || defined(_M_X64)
// x86-64: Use top 16 bits (48–63) for tagging
#	define _PTR_MASK 0x0000FFFFFFFFFFFFULL
#	define _EXTRACT_PTR(slot) (u8 *)(((i64)((slot) & _PTR_MASK) << 16) >> 16)
#	define _TAG_SHIFT 48
#elif defined(__aarch64__) && defined(__ARM_64BIT_STATE)
// ARM64: Use Top Byte Ignore (TBI), bits 56–63
#	define _PTR_MASK 0x00FFFFFFFFFFFFFFULL  		     // Mask bits 56–63
#	define _EXTRACT_PTR(slot) (u8 *)((slot) & _PTR_MASK) // TBI: bits ignored on deref
#	define _TAG_SHIFT 56
#elif defined(__riscv) && (__riscv_xlen == 64)
// RISC-V 64: Assume 48-bit addressing (SV48) if no pointer masking
#	if defined(__riscv_zicbom) || defined(__riscv_zicboz)
 		// Use compressed instructions or cache hints (optional)
#	endif

#	define _PTR_MASK 0x0000FFFFFFFFFFFFULL
#	define _EXTRACT_PTR(slot) (u8 *)((slot) & _PTR_MASK) // No sign-extension needed
#	define _TAG_SHIFT 48
#else
// Fallback: Use lower bits (if aligned)
#warning "Using lower-bit tagging for portability"
#define _PTR_MASK 0xFFFFFFFFFFFFFFF0ULL // Mask low 4 bits
#define _EXTRACT_PTR(slot) (u8 *)((slot) & _PTR_MASK)
#define _TAG_SHIFT 0 // Tags in bits 0–3
#endif

#define match(x) switch (__builtin_expect((x)._slot0 >> _TAG_SHIFT, _tag_i64))
#define with break; 	// break

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

// we it is implemented in the assembly fie
extern WaksContext global_panic_env;

__attribute__((returns_twice)) extern int waks_save_state(void);
extern void waks_load_state(void);

// AnyType: Boxes a Type  into an Any type to be used.
// Bottom 48 bits: Pointer address
// Top 16 bits: _tag_ptr

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
	String 	_str;
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
///         MatchStr(item, s) { io_print(s); io_print(STR("\n")); } with / break;
///         default: break;
///     }
/// }
#define MatchStr(v, name)                                                      \
  case _tag_str:;                                                              \
    String name = {.data = _EXTRACT_PTR((v)._slot0),.length = (usize)(v)._slot1};

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

// Extract pointer back to the match statement
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
  while (str[len]) len++;
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

#endif // !WAKS_TYPES_H

#define internal     static
#define local_scope  static
#define global_scope static

/// IN ARENA
#define NULL ((void *)0)               // waks_null
#define ALIGN_16(n) (((n) + 15) & ~15) // waks_align16
#define ALIGN_UP(n, align) (((n) + (align - 1)) & ~((align) - 1))
#define ALIGN_DOWN(n, align) ((n) & ~((align) - 1))
#define BOX_MAGIC 0xBAAD
#define PAGESIZE 4096

#define ATOMIC_RELAXED 0
#define ATOMIC_SEQ_CST 5

// x86_64 syscall no.
#if defined(__linux__) || defined(__APPLE__)
#    if defined(__x86_64)
#        define SYS_mmap 9
#        define SYS_munmap 11
#        define SYS_write 1
#        define SYS_exit 60
#        define SYS_mprotect 10
#        define SYS_madvise 28

#    elif defined(__aarch64__) || defined(__riscv)
#        define SYS_mmap 222
#        define SYS_munmap 215
#        define SYS_exit 93
#        define SYS_write 64
#        define SYS_mprotect 226
#        define SYS_madvise 233

#    endif

#    define PROT_NONE 0x0
#    define PROT_READ 0x1
#    define PROT_WRITE 0x2
#    define MAP_PRIVATE 0x02
#    define MAP_ANONYMOUS 0x20
#    define MADV_DONTNEED 4
#    define MAP_FAILED ((void *)-1)
#    define _SC_PAGESIZE 30

// @TODO(waks-work): this needs to be changed  to waks_syscall6
static inline long syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6)
{
#    if defined(__x86_64)
    long ret;
    register long r10 __asm__("r10") = a4;
    register long r8 __asm__("r8") = a5;
    register long r9 __asm__("r9") = a6;

    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9)
                     : "rcx", "r11", "memory");
#    elif defined(__aarch64__)
    long ret = 0;
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a1;
    register long x1 __asm__("x1") = a2;
    register long x2 __asm__("x2") = a3;
    register long x3 __asm__("x3") = a4;
    register long x4 __asm__("x4") = a5;
    register long x5 __asm__("x5") = a6;
    __asm__ volatile("svc #0"
                     : "=r"(x0)
                     : "r"(x8), "r"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5)
                     : "memory");
    ret = x0;

#    elif defined(__riscv)
    long ret = 0;
    register long a7 __asm__("a7") = n;
    register long a0 __asm__("a0") = a1;
    register long a1_reg __asm__("a1") = a2;
    register long a2_reg __asm__("a2") = a3;
    register long a3_reg __asm__("a3") = a4;
    register long a4_reg __asm__("a4") = a5;
    register long a5_reg __asm__("a5") = a6;
    __asm__ volatile("ecall"
                     : "=r"(a0)
                     : "r"(a7), "r"(a0), "r"(a1_reg), "r"(a2_reg), "r"(a3_reg), "r"(a4_reg),
                       "r"(a5_reg)
                     : "memory");
    ret = a0;

#    endif
    return ret;
}

#elif defined(_WIN32) || defined(_WIN64)

#    define MEM_COMMIT 0x00001000   // waks_mem_commit
#    define MEM_RESERVE 0x00002000
#    define MEM_DECOMMIT 0x00004000
#    define MEM_RELEASE 0x00008000
#    define PAGE_NOACCESS 0x01
#    define PAGE_READWRITE 0x04     // waks_page_readwrite

// Manually declare Kernel functions to avoid windows.h
#    ifdef __cpluscplus
extern "C" {
#    endif
void *__stdcall VirtualAlloc(void *lpAddress, usize dwSize, u32 flAllocationType, u32 flProtect);
i32 __stdcall VirtualFree(void *lpAddress, usize dwSize, u32 dwFreeType);
void __stdcall ExitProcess(u32 ExitCode);
#    ifdef __cpluscplus
}
#    endif

#elif defined(USE_STD_LIB)
#    include <stdlib.h>
#    define OS_COMMIT(ptr, size) (1)
#    define OS_DECOMMIT(ptr, size)
#endif

static void os_panic(void)
{
#if defined(__linux)
    syscall6(SYS_exit, 1, 0, 0, 0, 0, 0);
#elif defined(_WIN32) || defined(_WIN64)
    ExitProcess(1);
#endif
}

static inline void dbg_print(const char *str);
static inline void dbg_print_int(i16 n);

/// ASSERT:  ASSERT(needed_space <= arena->capacity);
#define PANIC_MSG(msg)                                                                             \
    do {                                                                                           \
        dbg_print("[PANIC] ");                                                                     \
        dbg_print(msg);                                                                            \
        dbg_print(" at " __FILE__ ":");                                                            \
        dbg_print_int(__LINE__);                                                                   \
        os_panic();                                                                                \
    } while (0)

#undef ASSERT
#define ASSERT(cond)                                                                               \
    do {                                                                                           \
        if (!(cond))                                                                               \
            PANIC_MSG(#cond);                                                                      \
    } while (0)

/// #define ArenaInitOnce(name, cap) \
///    static Arena *name = NULL; \
///    if (!name) name = ArenaAlloc(cap)

/// #define ArenaPushString(arena, str) ({ \
///    usize _len = 0; \
///    while(str[_len]) _len++; \
///    char *_dst = (char *)ArenaPush(arena, _len + 1); \
///    for(usize _i = 0; _i <= _len; _i++) _dst[_i] = str[_i]; \
///    _dst; \
/// })

/// BIT MANIPULATION
#define BIT(n) (1ULL << (n))   // waks_bit
#define SET_BIT(reg, n) ((reg) |= BIT(n))
#define CLR_BIT(reg, n) ((reg) &= ~BIT(n))
#define GET_BIT(reg, n) ((reg) & BIT(n))
#define GB(x) ((u64)(x) << 30)

/// Allocation: Handle handle = BoxNew(arena, u32, owner_id);
#define BoxNew(arena, type, owner) BoxAlloc(arena, sizeof(type), owner)

/// Borrowing: type *ptr = BoxGet(arena, handle, caller_id);
#define BoxGet(arena, type, handle, caller) (type *)HandleBorrow(arena, handle, caller)

/// Mutable Borrowing: type *ptr = BoxGetMut(arena, handle, caller_id);
#define BoxGetMut(arena, type, handle, caller) (type *)HandleBorrowMut(arena, handle, caller)

/// Scratch macro: Usage: ArenaTempBlock(my_arena)
///     {
///          u8 * temp = ArenaPush(my_arena, 1024);
///          //Do work... no need to call ArenaSetPosBack as the macro does
///     }
#define ArenaTempBlock(arena)                                                                      \
    for (u64 _pos = ArenaGetPos(arena), _once = 1; _once; _once = 0, ArenaSetPosBack(arena, _pos))

/// Automatic clean up RAAI
///  Arena *arena = ArenaAlloc((u64)GB(1));
///  current_arena = arena; // set global context for scoped macros
///  {
///      ScopedHandle handle = BoxNew(current_arena, uint32_t, 0);
///      ScopedBorrow(uint32_t,  handle, 0);
///      if (ptr) { *ptr = 1337; dbg_print("Data set successfully");}
///  }  <--- HandleRelease(current_arena, handle) and
///  HandleRelease(current_arena,ptr) called; ArenaRelease(arena);
#define ScopedHandle __attribute__((cleanup(_auto_release_handle))) __attribute__((unused)) Handle

// Use this when you want the borrow to end EXACTLY at the '}'
#define ScopedBorrow(type, name, handle, caller)                                                   \
    __attribute__((cleanup(_raii_release_now))) type *name =                                       \
        (type *)HandleBorrow(current_arena, handle, caller)

// Use this when you want to use the data now, but let the Arena clean it up
// later
#define DeferBorrow(type, name, handle, caller)                                                    \
    __attribute__((cleanup(_raii_release_deferred))) type *name =                                  \
        (type *)HandleBorrow(current_arena, handle, caller)

/// Context Helper: Sets the current_arena for a block and restores it after
/// WITH_ARENA(disk_buffer_arena) {
///    ScopedHandle handle = BoxNew(current_arena, uint64_t, 0);
///    {
///        DeferBorrow(uint64_t, handle, 0);
///        if (ptr) *ptr = 0xDEADBEEF;
///    }  // ptr id deffered here
/// }  // handle is released here
#define WITH_ARENA(a)                                                                              \
    for (Arena *_old = current_arena, *_once = (current_arena = (a), (Arena *)1); _once;           \
         _once = 0, current_arena = _old)

/// Example of MOVE in use:
/// WITH_ARENA(arena) {
///    ScopedHandle handle = BoxNew(current_arena, uint64_t, 1);
///    {
///        ScopedBorrow(uint64_t, handle, 1);
///        *ptr = 42;
///    }  // ptr id scope ends here
///    Handle handle_2 = MOVE(current_arena, handle, 1,2);
///    ScopedBorrow(uint64_t, handle_2, 2);
///    dbg_print(*ptr);
/// }  // handle is released here
#define MOVE(arena, handle, old_owner, new_owner)                                                  \
    ({                                                                                             \
        ASSERT((handle).version != 0);                                                             \
        Handle _old = (handle);                                                                    \
        (handle) = (Handle){0, 0};                                                                 \
        HandleMove((arena), _old, (old_owner), (new_owner));                                       \
    })

/// uint32_t my_id = 1;
/// Handle hd2 = MOVE_TO(arena, hd1, 2);
#define MOVE_TO(arena, handle, new_owner) MOVE(arena, handle, my_id, new_owner)

/// Foward Iterator (slice)
/// usage: foreach(u32, item, my_slice) {*item = 0;}
#define foreach(type, item, slice)                                                                 \
    for (type *item = (type *)(slice).ptr, *_end = (item + (slice).len); item < _end; item++)

/// Automatically infered types.
/// usage: typedef struct { u32 *ptr; usize len;} slice_u32; slice_u32 s = {
/// arr, 10}; usage: foreach_auto(x, s) {*x = 5; } important: (slice).ptr ->
/// u32* && __typeof__((slice).ptr) -> u32*
#define foreach_auto(item, slice)                                                                  \
    for (__typeof__((slice).ptr) item = (slice).ptr, _end = item + (slice).len; item < _end; item++)

/// usage: foreach_if(u8, page, mem_region, is_page_free(page),
/// mark_allocated(page))
#define foreach_if(type, item, slice, condition, action)                                           \
    foreach(type, item, slice)                                                                     \
    {                                                                                              \
        if (!(condition)) {                                                                        \
            action;                                                                                \
        }                                                                                          \
    }

/// usage: foreach_filter(u32, x, slice, *x > 10) { dbg_print_int(*x);}
#define foreach_filter(type, item, slice, condition)                                               \
    foreach(type, item, slice) if (!(condition)) continue;                                         \
    else /// condition will be executed here

/// usage: foreach_map(u32, x,slice, *x *= 2);
#define foreach_map(type, item, slice, expression)                                                 \
    foreach(type, item, slice)                                                                     \
    {                                                                                              \
        expression;                                                                                \
    }

/// Linked: Intrusive lists (Kernel tasks / Resource chains)
/// usage: foreach_node(Task, t, head_task) { schedule(t); }
#define foreach_node(type, item, head) for (type *item = (head); item != NULL; item = item->next)

/// Safe Linked: Allows for deletion of 'item' during iteration
#define foreach_node_safe(type, item, next_item, head)                                             \
    for (type *item = (head), *next_item = item ? item->next : NULL; item != NULL;                 \
         item = next_item, next_item = item ? item->next : NULL)

/// usage: Task *found = NULL; foreach_find(Task, t, head, t->id == 5, found =
/// t)
#define foreach_find(type, item, head, condition, result_assign)                                   \
    foreach_node(type, item, head)                                                                 \
    {                                                                                              \
        if (condition) {                                                                           \
            result_assign;                                                                         \
            break;                                                                                 \
            -                                                                                      \
        }                                                                                          \
    }

/// Reverse Iterator (LIFO / Cleanup)
#define foreach_rev(type, item, slice)                                                             \
    for (type *item = (slice).len ? ((type *)((slice).ptr) + (slice).len - 1) : NULL,              \
              *_start = (type *)(slice).ptr;                                                       \
         item >= _start; item--)

/// Strided: Walking memory pages or fixed-size blocks (4KB / Stride)
/// usage: foreach_step(u8, page, my_arena, 4096)
#define foreach_step(type, item, slice, step)                                                      \
    for (type *item = (type *)(slice).ptr, *_end = (type *)((u8 *)(slice).ptr + (slice).len);      \
         (u8 *)item < (u8 *)_end; item = (type *)((u8 *)item + (step)))

/// Sentinel: Null terminated strings or streams
/// usage: foreach_ptr(char, c, my_string) { if(*c == 'A') ... }
#define foreach_ptr(type, item, start_ptr)                                                         \
    for (type *item = (type *)(start_ptr); item && *item != (type)0; item++)

/// This gives us : borrow -> use -> auto_release
/// usage: foreach_borrow(u32, ptr, handle, 1) { *ptr = 42; }
/// WITH_ARENA(my_arena) {
///    foreach_borrow(Any, item_ptr, my_vector.data, my_vector.user_id) {
///        for(usize i = 0; i < my_vector.length; i++) {
///            match(item_ptr[i]) { with MatchInt(item_ptr[i], val) { ... } }
///        }
///    }
/// }
#define foreach_borrow(type, item, handle, owner)                                                  \
    for (type *item = (type *)HandleBorrow(current_arena, handle, owner), *_once = item; _once;    \
         _once = NULL, HandleRelease(current_arena, handle))

/// Any val;
/// WITH_ARENA(my_arena) {
///    Any *p = vector_safe_get(my_arena, my_vec, 5);
///    if (p) val = *p; // Copy out immediately
/// }
// HandleDefer cleans up the borrow here
#define vector_safe_get(arena, vector, index)                                                      \
    ((index < vector.length) ? (HandleDefer(arena, vector.data),                                   \
                                (Any *)HandleBorrow(arena, vector.data, vector.user_id) + index)   \
                             : NULL)

/*
 * EXAMPLE: Vector<T>
 * Vector nums = vector_init(arena, 8, sizeof(u32), uid);
 * vector_push_t(arena, &nums, (u32)10);
 * vector_push_t(arena, &nums, (u32)20);
 */
#define vector_push_t(arena, vec, value)                                                           \
    do {                                                                                           \
        AUTO _tmp = (value);                                                                       \
        vector_push_raw(arena, vec, &_tmp);                                                        \
    } while (0)

/* u32 x = vector_get_t(arena, &nums, u32, 1); */
#define vector_get_t(arena, vec, T, index) (*(T *)vector_get_raw(arena, vec, index))

//  for (ListNode *curr = line_list; curr != NULL; curr = curr->next) {
//      EditorLine *line_data = container_of(curr, EditorLine, node);
//      (Do something with line_data->content...)
//  }
#define container_of(ptr, type, member) ((type *)((char *)(ptr) - (uintptr_t)&((type *)0)->member))

// void* ptr = ArenaPush(arena, 100);
// defer(ArenaReset(arena)); // This runs automatically when the function ends
// static inline void _waks_defer_cleanup(void (**)()) {
//    (*ptr)();
// }
#define defer(code)                                                                                \
    void _waks_internal_defer_##__LINE__()                                                         \
    {                                                                                              \
        code;                                                                                      \
    }                                                                                              \
    void (*_waks_internal_ptr_##__LINE__)(void) __attribute__((cleanup(_waks_defer_cleanup))) =    \
        _waks_internal_defer_##__LINE__;

/// ARENA STRUCTS

typedef struct Arena Arena;
struct Arena
{
    u8 *memory;
    u64 capacity;
    u64 position;
    u64 commited;
    u64 pagesize;
    u32 defer_count;
};

typedef struct BoxHeader BoxHeader;
struct BoxHeader
{
    u32 size;     // 0-3: Bytes of user data
    u32 owner_id; // 4-7: Standard uint
    u16 version;  // 8-9: Standard uint
    i16 borrows;  // 10-11: Standard int
    u16 magic;    // 12-13: 0xBAAD
    u16 padding;  // 14-15: Explicit padding for 16B alignment
};

typedef struct Handle Handle;
struct Handle
{
    u64 offset;
    u16 version;
};

/// ARENA API/METHOD IMPLEMENTATION

Arena *ArenaAlloc(u64 capacity);
u64 ArenaGetPos(Arena *arena);
static inline void *ArenaPush(Arena *arena, u64 size);
static inline void ArenaSetPosBack(Arena *arena, u64 pos);
static inline void ArenaRelease(Arena *arena);
static inline void ArenaReset(Arena *arena);
static inline void _auto_release_handle(Handle *handle);

/// ARENA IMPLEMENTATION

Arena *ArenaAlloc(u64 capacity)
{
    void *base = NULL;
#if defined(__linux)
    // Ask the OS for a large chunk of the of the virtual memory but tells
    // the hardware not to allocate physical RAM yet
    base = (void *)syscall6(SYS_mmap, 0, capacity, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (base == MAP_FAILED || base == NULL) return NULL;

    Arena *arena = (Arena *)base;
    /* Changes access from NOACCESS to READ && WRITE */
    syscall6(SYS_mprotect, (long)base, PAGESIZE, PROT_READ | PROT_WRITE, 0, 0, 0);

#elif defined(_WIN32) || defined(_WIN64)
    base = VirtualAlloc(NULL, (size_t)capacity, MEM_RESERVE, PAGE_NOACCESS);
    if (!base) return NULL;

    if (!VirtualAlloc(base, PAGESIZE, MEM_COMMIT, PAGE_READWRITE))
        return NULL;

    Arena *arena = (Arena *)base;
#elif defined(USE_STD_LIB)
    u64 total_size = capacity + sizeof(Arena);
    base = malloc(total_size);
    if (!base) return NULL;
    if (!OS_COMMIT(base, PAGESIZE)) {
        free(base);
        return NULL;
    }
#endif

    arena->memory = (u8 *)base;
    arena->capacity = capacity;
    arena->position = ALIGN_16(sizeof(Arena)); // starts after the header
    arena->commited = PAGESIZE;
    arena->pagesize = PAGESIZE;
    arena->defer_count = 0;
    return arena;
}

static inline void *ArenaPush(Arena *arena, u64 size)
{
    u64 aligned_size = ALIGN_16(size);
    u64 next_pos = arena->position + aligned_size;

    /* Safety Buffer: allocate last 256 bytes  of the page to the handle */
    u64 needed_space = next_pos + (arena->defer_count * sizeof(Handle)) + 256;
    ASSERT(needed_space <= arena->capacity);

    // We will allocate an extra memory page if the (needed_space + defer_count)
    // is greater than a single pagesize(4KB)
    if (needed_space > arena->commited) {
        u64 commit_needed = needed_space - arena->commited;
        u64 commit_aligned = ALIGN_16(commit_needed + arena->pagesize - 1) & ~(arena->pagesize - 1);

#if defined(__linux)
        uintptr_t ret = syscall6(SYS_mprotect, (long)(arena->memory + arena->commited),
                                 commit_aligned, PROT_READ | PROT_WRITE, 0, 0, 0);
        if (ret != 0)
            return NULL;
#elif defined(_WIN32) || defined(_WIN64)
        if (!VirtualAlloc(arena->memory + arena->commited, (usize)commit_aligned, MEM_COMMIT,
                          PAGE_READWRITE))
            return NULL;
#elif defined(USE_STD_LIB)
        if (!OS_COMMIT(arena->memory + arena->commited, commit_aligned))
            return NULL;
#endif
        arena->commited += commit_aligned;
    }

    void *ptr = arena->memory + arena->position;
    arena->position = next_pos;
    return ptr;
}

u64 ArenaGetPos(Arena *arena)
{
    return arena->position;
}

/*
 * TODO(waks-work): try to figure out how we can use defer_count either
 * to increment or decrement or just leave it as it is after reset
 * */
static inline void ArenaSetPosBack(Arena *arena, u64 position)
{
    ASSERT(position <= arena->position);
    u64 rounded_position = ALIGN_16(position + arena->pagesize - 1) & ~(arena->pagesize - 1);

    if (rounded_position < arena->commited) {
#if defined(__linux)
        syscall6(SYS_madvise, (long)(arena->memory + rounded_position),
                 arena->commited - rounded_position, MADV_DONTNEED, 0, 0, 0);
#elif defined(_WIN32) || defined(_WIN64)
        VirtualFree(arena->memory + rounded_position, (usize)(arena->commited - rounded_position),
                    MEM_DECOMMIT);
#elif defined(USE_STD_LIB)
        /*
         * This in context does nothing as seen in macro
         * TODO(waks-work); may be check if there is any need of having this block
         * of Code
         * */
        u64 size_to_free = arena->commited - rounded_position;
        OS_DECOMMIT(arena->memory + rounded_position, size_to_free);
        arena->commited = rounded_position;

#endif
    }

    arena->position = position;
}

static inline void ArenaRelease(Arena *arena)
{
    if (arena) {
#if defined(__linux)
        syscall6(SYS_munmap, (long)arena->memory, arena->capacity, 0, 0, 0, 0);
#elif defined(_WIN32) || defined(_WIN64)
        VirtualFree(arena->memory, 0, MEM_RELEASE);
#elif defined(USE_STD_LIB)
        free(arena);
#endif
    }
}


// Used to reset all the borrow count together for all the borrow
// we called HandleDefer on for a specific scope.
static inline void ArenaReset(Arena *arena)
{
    // process based on pagewide boundary
    for (u32 i = 1; i <= arena->defer_count; i++) {
        u64 defer_offset = arena->commited - (i * sizeof(Handle));
        Handle *handle_ptr = (Handle *)(arena->memory + defer_offset);
        BoxHeader *header = (BoxHeader *)(arena->memory + handle_ptr->offset);
#ifdef CONCURRENT_MODE
        // If it was a Mut borrow (-1), set to 0. If shared (>0), decrement.
        i16 b = __atomic_load_n(&header->borrows, ATOMIC_SEQ_CST);
        if (b == -1)
            __atomic_store_n(&header->borrows, 0, ATOMIC_SEQ_CST);
        else if (b > 0)
            __atomic_fetch_sub(&header->borrows, 1, ATOMIC_SEQ_CST);
#else
        if (header->magic == BOX_MAGIC) {
            header->version++;
            header->borrows = 0;
            header->magic = 0;
        }
#endif
    }
    // Clear the defer counter.
    arena->defer_count = 0;

    // Reset the bump pointer to after the Arena struct
    // This makes all memory available again.
    // Old handles will fail because their version won't match
    // whatever is newly allocated in this space.
    u64 start_position = ALIGN_16(sizeof(Arena));

    if (arena->commited > start_position) {
        u64 size_to_reclaim = arena->commited - start_position;
#if defined(__linux)
        syscall6(SYS_madvise, (long)(arena->memory + start_position), size_to_reclaim,
                 MADV_DONTNEED, 0, 0, 0);
#elif defined(_WIN32) || defined(_WIN64)
        VirtualFree(arena->memory + start_position, (size_t)size_to_reclaim, 0x4000);
#elif defined(USE_STD_LIB)
        OS_DECOMMIT(arena->memory + arena->pagesize, size_to_reclaim);
        arena->commited = arena->pagesize;
#endif
    }
    arena->position = start_position;
}

/// ALLOCATOR STRUCTS 

typedef struct WaksAllocator WaksAllocator;
struct WaksAllocator {
    void *(*alloc)(void *context, ssize size, ssize alignment);
    void (*free)(void *context, void *ptr);
    void *context;
};

static void *arena_alloc_w(void *context, ssize size, ssize alignment);
static void arena_free_w(void *context, void *ptr);
WaksAllocator waks_arena_as_allocator(Arena *arena);

static void *arena_alloc_w(void *context, ssize size, ssize alignment)
{
	Arena *arena = (Arena*)context;
    (void)alignment;
    return ArenaPush(arena,(u64)size);
}

static void arena_free_w(void *context, void *ptr) 
{
    (void)context;
    (void)ptr;
}

WaksAllocator waks_arena_as_allocator(Arena *arena){
    return (WaksAllocator){
 	  .alloc = arena_alloc_w, 
 	  .free = arena_free_w, 
      .context =arena};
}

/// HANDLE ALLOCATORS

Handle BoxAlloc(Arena *arena, u32 size, u32 owner);
static inline void *HandleBorrow(Arena *arena, Handle handle, u64 caller);
static inline void *HandleBorrowMut(Arena *arena, Handle handle, u32 caller);
Handle HandleMove(Arena *arena, Handle handle, u32 old_owner, u32 new_owner);
static inline void HandleRelease(Arena *arena, Handle handle);
static inline void HandleReleaseMut(Arena *arena, Handle handle);
static inline void HandleDefer(Arena *arena, Handle handle);

Handle BoxAlloc(Arena *arena, u32 size, u32 owner)
{
    u32 total_size = ALIGN_16(sizeof(BoxHeader) + size);
    u8 *raw_ptr = (u8 *)ArenaPush(arena, total_size);
    if (!raw_ptr) return (Handle){0, 0};

    BoxHeader *handle = (BoxHeader *)raw_ptr;
    handle->size = size;
    handle->magic = BOX_MAGIC;

#ifdef CONCURRENT_MODE
    __atomic_store_n(&handle->owner_id, owner, ATOMIC_SEQ_CST);
    __atomic_store_n(&handle->version, 1, ATOMIC_SEQ_CST);
    __atomic_store_n(&handle->borrows, 0, ATOMIC_SEQ_CST);
#else
    handle->owner_id = owner;
    handle->version = 1;
    handle->borrows = 0;
#endif

    return (Handle){.offset = (u64)(raw_ptr - arena->memory), .version = 1};
}

static inline void *HandleBorrow(Arena *arena, Handle handle, u64 caller)
{
    if (handle.version == 0) return NULL;

    BoxHeader *header = (BoxHeader *)(arena->memory + handle.offset);

#ifdef CONCURRENT_MODE
    if (__atomic_load_n(&header->version, ATOMIC_SEQ_CST) != handle.version ||
        __atomic_load_n(&header->owner_id, ATOMIC_SEQ_CST) != caller || header->magic != BOX_MAGIC)
        return NULL;

    i16 borrow = __atomic_load_n(&header->borrows, ATOMIC_SEQ_CST);
    if (borrow < 0) return NULL;

    __atomic_fetch_add(&header->borrows, 1, ATOMIC_SEQ_CST);
#else
    if (header->version != handle.version || header->owner_id != caller ||
        header->magic != BOX_MAGIC || header->borrows < 0)
        return NULL;

    header->borrows++;
#endif
    return (void *)(header + 1);
}

static inline void *HandleBorrowMut(Arena *arena, Handle handle, u32 caller)
{
    if (handle.version == 0) return NULL;

    BoxHeader *header = (BoxHeader *)(arena->memory + handle.offset);
    if (header->borrows != 0) return NULL;

#ifdef CONCURRENT_MODE
    // Atomic Compare and Swap (CAS) to ensure borrows is exactly 0
    int16_t expected = 0;
    if (__atomic_load_n(&header->version, ATOMIC_SEQ_CST) != handle.version ||
        __atomic_load_n(&header->owner_id, ATOMIC_SEQ_CST) != caller)
        return NULL;

    if (!__atomic_compare_exchange_strong_n(&header->borrows, &expected, -1, ATOMIC_SEQ_CST))
        return NULL;
#else
    if (header->version != handle.version || header->owner_id != caller || header->borrows != 0)
        return NULL;

    header->borrows = -1;
#endif
    return (void *)(header + 1);
}

Handle HandleMove(Arena *arena, Handle handle, u32 old_owner, u32 new_owner)
{
    BoxHeader *header = (BoxHeader *)(arena->memory + handle.offset);

#ifdef CONCURRENT_MODE
    // Ensure caller is current owner and version matches
    if (__atomic_load_n(&header->owner_id, ATOMIC_SEQ_CST) != old_owner ||
        __atomic_load_n(&header->version, ATOMIC_SEQ_CST) != handle.version)
        return (Handle){0, 0};

    // Kill all existing borrows/handles by bumping version
    __atomic_fetch_add(&header->version, 1, ATOMIC_SEQ_CST);
    __atomic_store_n(&header->owner_id, new_owner, ATOMIC_SEQ_CST);

    return (Handle){.offset = handle.offset, .version = (u16)(handle.version + 1)};
#else
    if (header->owner_id != old_owner || header->version != handle.version ||
        header->borrows != 0) {
        return (Handle){0, 0};
    }

    header->version++;
    header->owner_id = new_owner;

    return (Handle){.offset = handle.offset, .version = header->version};
#endif
}

static inline void HandleRelease(Arena *arena, Handle handle)
{
    BoxHeader *header = (BoxHeader *)(arena->memory + handle.offset);
    if (header->magic != BOX_MAGIC || header->version != handle.version) return;
#ifdef CONCURRENT_MODE
    __atomic_fetch_add(&header->version, 1, ATOMIC_SEQ_CST);
    __atomic_store_n(&header->owner_id, 0, ATOMIC_SEQ_CST);
    __atomic_store_n(&header->borrows, 0, ATOMIC_SEQ_CST);
#else
    header->version++;
    header->owner_id = 0;
    header->borrows = 0;
#endif
}

static inline void HandleReleaseMut(Arena *arena, Handle handle)
{
    BoxHeader *header = (BoxHeader *)(arena->memory + handle.offset);
#ifdef CONCURRENT_MODE
    __atomic_store_n(&header->borrows, 0, ATOMIC_SEQ_CST);
#else
    header->borrows = 0;
#endif
}

static inline void HandleDefer(Arena *arena, Handle handle)
{
    if (handle.version == 0) return;

    if (arena->position + (arena->defer_count + 1) * sizeof(Handle) >= arena->commited) {
        ArenaPush(arena, arena->pagesize);
    }

    // we store our defer list at the end of the arena growing backwards.
    arena->defer_count++;
    u64 defer_offset = arena->commited - (arena->defer_count * sizeof(Handle));

    Handle *defer_ptr = (Handle *)(arena->memory + defer_offset);
    *defer_ptr = handle;
}

/// AUTO RELEASE FEATURES

static inline void _auto_release_handle(Handle *handle);
static inline void _raii_release_now(void *pointer);
static inline void _raii_release_deferred(void *pointer);

/// AUTO RELEASE IMPLEMENTATION

/* Context Management */
extern Arena *current_arena;

/* allows for automatic release of handle i.e ScopedHandle (RAII like) */
static inline void _auto_release_handle(Handle *handle)
{
    if (!current_arena) {
        dbg_print("KERNEL PANIC: ScopeHandle used without active Arena!\n");
        return;
    }
    if (handle && handle->version > 0)
        HandleRelease(current_arena, *handle);
}

/* allows for automatic release of borrow i.e ScopedBorrow */
static inline void _raii_release_now(void *pointer)
{
    void **pointer_to_ptr = (void **)pointer;
    if (*pointer_to_ptr && current_arena) {
        BoxHeader *header = ((BoxHeader *)*pointer_to_ptr) - 1;
        Handle handle = {.offset = (u8 *)header - current_arena->memory,
                         .version = header->version};
        HandleRelease(current_arena, handle);
    }
}

static inline void _raii_release_deferred(void *pointer)
{
    void **pointer_to_ptr = (void **)pointer;
    if (*pointer_to_ptr && current_arena) {
        BoxHeader *header = ((BoxHeader *)*pointer_to_ptr) - 1;
        Handle handle = {.offset = (u8 *)header - current_arena->memory,
                         .version = header->version};
        HandleDefer(current_arena, handle);
    }
}

/// ERROR HANDLING

WaksResult waks_pcall(Arena *arena, void (*fn)(void *), void *arg);
void waks_panic(WaksResult err);

__attribute__((aligned(16))) WaksContext global_panic_env;

Arena *current_arena = NULL;

// Arena *arena = ArenaAlloc(GB(1));
// WaksResult res = waks_pcall(arena, test_risky_logic, arena);
// if (res != WAKS_OK){ //  -> no manual cleanup needed as the arena is already rolled up 
//	  LOG_FMT(LOG_ERROR, "CATCH", "Code failed with: %s",
//    waks_strerror(res));
// }
//
// //example two
// WaksResult parse_config transaction(Arena *arena, const char *path) {
//    // On waks_panic call, arena->position is automatically restored to point before the call. 
//    return waks_pcall(arena, (void (*)(void *))run_parser, (void*)path);
// }
// void run_parser(void *arg) {
//   Arena *temp = (Arena *)arg;
//   Config *cfg = ArenaPush(temp, sizeof(Config));
//   if (!load_file(temp, cfg)) 
//      waks_panic(WAKS_ERR_IO) // jumps out and cleans the arena instantly
// }

WaksResult waks_pcall(Arena *arena, void (*unsafe_func)(void *), void *arg)
{
    u64 checkpoint = ArenaGetPos(arena);
    global_panic_env.arena_checkpoint = checkpoint;

    int status = waks_save_state();
    if (status == 0) {
        /// SUCCESS PATH:
        unsafe_func(arg);
        return WAKS_OK;
    } else {
        /// RECOVERY PATH:
        ArenaSetPosBack(arena, global_panic_env.arena_checkpoint);
        return (WaksResult)status;
    }
}

/// void test_risky_logic(void *arg) {
///    Arena *a = (Arena *)arg;
///    u32 *data = ArenaPush(a, 1024);
///    if (some_error_condition) 
///     	waks_panic(WAKS_ERR_NOMEM);
/// }
void waks_panic(WaksResult error)
{
    *((volatile int *)&global_panic_env.error_code) = (int)error;
    waks_load_state();
}

/// OPTION API
typedef struct Option Option;
struct Option
{
    Handle value;
    b8 has_value;
};

static inline Option option_none(void);
static inline Option option_some(Handle handle);
static inline Handle option_unwrap(Option *opt);
static inline Handle option_unwrap_or(Option *opt, Handle fallback);

/// OPTION API IMPLEMENTATION

static inline Option option_none(void)
{
    return (Option){.value = {0, 0}, .has_value = false};
}

static inline Option option_some(Handle handle)
{
    return (Option){.value = handle, .has_value = true};
}

// Think about passing by reference so that we know we are passing
// by reference and not by value 
static inline Handle option_unwrap(Option *opt)
{
    if (!opt) return (Handle){0};
	 // TODO(waks-work):implement the panic issue and check the logic as it is
	 // needed and used in alot of places not just here and it may lead to issue
	 // it is the part returning segmentation fault */
    if (!opt->has_value) PANIC_MSG("Attempted to unwrap an Option(None)");

    return opt->value;
}

// Think about passing by reference so that we know we are passing
// by reference and not by value 
static inline Handle option_unwrap_or(Option *opt, Handle fallback)
{
    return opt->has_value ? opt->value : fallback;
}

/// Metadata stays inside the struct you want to link: no need for extra
/// boxalloc
typedef struct ListNode ListNode;
struct ListNode
{
    ListNode *next;
    ListNode *prev;
};

/// Linked list API
static inline void list_init(ListNode *node);
static inline void list_push_back(ListNode **head, ListNode *new_node);
static inline void list_remove(ListNode **head, ListNode *node);
static inline b8 list_is_empty(ListNode *head);

/// LINKED LIST IMPLEMENTATION

// EXAMPLES:
// typedef struct { u32 id; char *content; ListNode node; } EditorLine;
// 
//	 ListNode *line_list = NULL;
//	 EditorLine *line = ArenaPush(arena, sizeof(EditorLine));
//	 line->id = 1;


// typedef struct {
//     char *name;
//     ListNode active_node;  // For the list of open files
//     ListNode history_node; // For the "recently closed" list
// } File;
// list_push_back(&active_files, &file->active_node);
// list_push_back(&recent_history, &file->history_node);
//

// list_init(&line->node); 
static inline void list_init(ListNode *node)
{
    node->next = NULL;
    node->prev = NULL;
}

// list_push_back(&line_list, &line->node); 
static inline void list_push_back(ListNode **head, ListNode *new_node)
{
    if (!new_node) return;

    if (*head == NULL) {
        *head          = new_node;
        new_node->next = NULL;
        new_node->prev = NULL;
        return;
    }

    ListNode *curr = *head;
    while (curr->next) curr = curr->next;

    curr->next     = new_node;
    new_node->prev = curr;
    new_node->next = NULL;
}
// list_remove(&line_list, &line->node); 
static inline void list_remove(ListNode **head, ListNode *node)
{
    if (!head || !*head || !node) return;

    // prev-> points to -> next item
    if (node->prev) node->prev->next = node->next;

    // prev <- the prev item is pointed back by the next item <- next
    if (node->next) node->next->prev = node->prev;

    // move the head pointer to the next element
    if (*head == node) *head = node->next;

    node->prev = NULL;
    node->next = NULL;
}

static inline b8 list_is_empty(ListNode *head)
{
    return head == NULL;
}

/// VECTORS API

typedef struct Vector Vector;
struct Vector
{
    Handle data;    // Handle to the underlying array
    usize capacity; // Total slots
    usize length;   // Used slots
    usize item_size;
    u32 user_id; // Owner of the data
};

#define SLICE_CAST(type, string) ((type *)(string).ptr)

static inline Vector vector_init(Arena *arena, usize initial_cap, usize item_size, u32 user_id);
static inline Any *vector_get(Arena *arena, Vector *vector, usize index);
static inline Any vector_get_copy(Arena *arena, Vector *vector, usize index);
static inline Any vector_pop(Arena *arena, Vector *vector);
static inline void vector_push(Arena *arena, Vector *vector, Any value);
static inline void vector_insert(Arena *arena, Vector *vector, usize index, Any value);
static inline void vector_remove(Arena *arena, Vector *vector, usize index);
static inline void vector_clear(Vector *vector);
static inline void vector_ensure_capacity(Arena *arena, Vector *vector, usize min_cap);

static inline void vector_push_raw(Arena *arena, Vector *vector, void *item);
static inline void *vector_get_raw(Arena *arena, Vector *vector, usize index);
static inline void vector_pop_raw(Arena *arena, Vector *vector);

/// VECTORS IMPLEMENTATION

// EXAMPLE: VECTOR<Any>
// Vector vals = vector_init(arena, 8, sizeof(Any), uid);
// vector_push(arena, &vals, AnyInt(99));
// Any a = vector_get_copy(arena, &vals, 0);

static inline Vector vector_init(Arena *arena, usize initial_cap, usize item_size, u32 user_id)
{
    Vector list = {0};
    list.user_id = user_id;
    list.capacity = initial_cap;
    list.length = 0;
    list.item_size = (usize)item_size;
    list.data = BoxAlloc(arena, initial_cap * sizeof(u8), user_id);
    return list;
}

static inline Any *vector_get(Arena *arena, Vector *vector, usize index)
{
    if (index >= vector->length) return NULL;

    HandleDefer(arena, vector->data);
    Any *ptr = (Any *)HandleBorrow(arena, vector->data, vector->user_id);
    return ptr ? &ptr[index] : NULL;
}

static inline void *vector_get_raw(Arena *arena, Vector *vector, usize index)
{
    if (index >= vector->length) return NULL;

    HandleDefer(arena, vector->data);
    u8 *ptr = (u8 *)HandleBorrow(arena, vector->data, vector->user_id);
    return ptr ? &ptr[index] : NULL;
}

static inline Any vector_get_copy(Arena *arena, Vector *vector, usize index)
{
    if (index >= vector->length) return AnyNone();
    Any result = AnyNone();
    WITH_ARENA (arena) {
        ScopedBorrow(Any, data_ptr, vector->data, vector->user_id);
        if (data_ptr) result = data_ptr[index];
    }
    return result;
}

static inline void vector_push(Arena *arena, Vector *vector, Any value)
{
    vector_ensure_capacity(arena, vector, vector->length + 1);

    WITH_ARENA (arena) {
        ScopedBorrow(Any, data_ptr, vector->data, vector->user_id);
        
        // TODO(waks-work): implement proper panic or after failure case handling to
        // ensure is is more secure
        if (!data_ptr) return;
        // PANIC_MSG("Vector Push Failed: Handle Corruption or mismatch.");
        data_ptr[vector->length] = value;
        vector->length += 1;
    }
}

static inline void vector_push_raw(Arena *arena, Vector *vector, void *item)
{
    if (!item) return;

    vector_ensure_capacity(arena, vector, vector->length + 1);
    WITH_ARENA (arena) {
        ScopedBorrow(u8, data_ptr, vector->data, vector->user_id);
        if (!data_ptr) return;

        u8 *dst = data_ptr + (vector->length * vector->item_size);
        __builtin_memcpy(dst, item, vector->item_size);
        vector->length += 1;
    }
}

static inline Any vector_pop(Arena *arena, Vector *vector)
{
    if (vector->length == 0) return AnyNone();
    Any result = AnyNone();

    WITH_ARENA (arena) {
        ScopedBorrow(Any, data_ptr, vector->data, vector->user_id);
        if (!data_ptr) PANIC_MSG("Vector pop failed: Borrow denied");
        if (data_ptr) {
            vector->length -= 1;
            result = data_ptr[vector->length];
            // data_ptr[vector->length] = AnyNone();
        }
    }
    return result;
}

static inline void vector_pop_raw(Arena *arena, Vector *vector)
{
    (void)arena;
    if (vector->length == 0) return;
    vector->length -= 1;
}

static inline void vector_insert(Arena *arena, Vector *vector, usize index, Any value)
{
    if (index > vector->length) index = vector->length;
    vector_ensure_capacity(arena, vector, vector->length + 1);

    WITH_ARENA (arena) {
         // TODO(waks-work): implement proper panic or after failure case handling to
         // ensure is is more secure
        ScopedBorrow(Any, data_ptr, vector->data, vector->user_id);
        if (!data_ptr) return; /// PANIC_MSG("Couldn't Insert: Failed to Borrow");

        for (usize i = vector->length; i > index; i--)
            data_ptr[i] = data_ptr[i - 1];

        data_ptr[index] = value;
        vector->length += 1;
    }
}

static inline void vector_remove(Arena *arena, Vector *vector, usize index)
{
    if (index >= vector->length) return;

    WITH_ARENA (arena) {
        ScopedBorrow(Any, data_ptr, vector->data, vector->user_id);
        if (!data_ptr) PANIC_MSG("Couldn't Insert: Failed to Borrow");

        for (usize i = index; i < vector->length - 1; i++)
            data_ptr[i] = data_ptr[i + 1];
        vector->length -= 1;
    }
}

static inline void vector_clear(Vector *vector)
{
    vector->length = 0;
}

static inline void vector_ensure_capacity(Arena *arena, Vector *vector, usize min_cap)
{
    if (vector->capacity >= min_cap) return;

    usize new_capacity = min_cap == 0 ? 8 : vector->capacity * 2;
    if (min_cap >= new_capacity) new_capacity = min_cap;

    Handle new_handle = BoxAlloc(arena, new_capacity * sizeof(Any), vector->user_id);
    if (vector->length > 0) {
        WITH_ARENA (arena) {
            ScopedBorrow(Any, old_data, vector->data, vector->user_id);

             // We don't use ScopeBorrow instead we use the raw HandleBorrowMut to
             // prevent new_data from being cleared automatically at the end of the
             // scope so we can clean it manually when we need to clean it up as expected.
            Any *new_data = HandleBorrowMut(arena, new_handle, vector->user_id);
            if (old_data && new_data) {
                for (usize i = 0; i < vector->length; i++) {
                    new_data[i] = old_data[i];
                }
            }

            HandleReleaseMut(arena, vector->data);
        }
    }

    vector->capacity = new_capacity;
    vector->data = new_handle;
}

static inline void dbg_print(const char *str)
{
#if defined(__linux__)
    const char *pointer = str;
    while (*pointer) pointer++;
    syscall6(SYS_write, 2, (long)str, (long)(pointer - str), 0, 0, 0);
#endif
}

static inline void dbg_print_int(i16 n)
{
    char buf[16];
    int i = 0;
    if (n == 0) {
        buf[i++] = '0';
    } else {
        if (n < 0) {
            buf[i++] = '-';
            n = -n;
        }
        while (n > 0) {
            buf[i++] = (n % 10) + '0';
            n /= 10;
        }
    }
    buf[i] = '\0';
    // Simple reverse for display
    for (int j = 0; j < i / 2; j++) {
        char t = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = t;
    }
    dbg_print(buf);
    dbg_print("\n");
}

#endif
