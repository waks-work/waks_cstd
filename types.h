#ifndef WAKS_TYPE_H
#define WAKS_TYPE_H


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


#if !defined(WAKS_NO_STDHEADERS)
	typedef float  waks_f32;
	typedef double waks_f64;
#else 
	typedef float  waks_f32;
	typedef double waks_f64;

#endif

#define WAKS_2CSTR_CAST(waks_cstring) (waks_char *)(waks_cstring)
#define WAKS_2UCSTR_CAST(waks_cstring) (waks_uchar *)(waks_cstring)

// compiler builtins (??? explain deeply) &&& WAKS_AUTO implementation
#define WAKS_COMPILER_PRAGMA(x) _Pragma(#x)

#if defined(__clang__) || defined(__GNUC__)
#	define WAKS_AUTO  __auto_type
#else 
#	error "WAKS_AUTO requires compiler support for __auto_type"
#endif

WAKS_COMPILER_PRAGMA(GCC diagnostic error "-Wswitch")
WAKS_COMPILER_PRAGMA(GCC diagnostic error "-Wimplicit-fallthrough")

// Centralized X-Macro definition replacing waks_error.inc
#define WAKS_ERROR_LIST(X) \
    X(WAKS_OK,                 "Operation successful") \
    X(WAKS_ERR_GENERIC,         "An unknown error occurred") \
    X(WAKS_ERR_NOMEM,           "Arena allocation failed: Out of memory") \
    X(WAKS_ERR_BOUNDS,          "Pointer out of bounds during iteration") \
    X(WAKS_ERR_ALIGN,           "Memory alignment violation") \
    X(WAKS_ERR_NULL,            "Null pointer dereference prevented") \
    X(WAKS_ERR_OVERFLOW,        "Integer or buffer overflow detected") \
    X(WAKS_ERR_STALE_HANDLE,    "Handle version mismatch: stale reference") \
    X(WAKS_ERR_BORROW_CONFLICT, "Borrow violation: resource busy") \
    X(WAKS_ERR_OS_REJECTED,     "OS-level memory request failed") \
    X(WAKS_ERR_NOT_FOUND,       "Requested item does not exist") \
    X(WAKS_ERR_INVALID_STATE,   "Logic error: system in wrong state for operation") \
    X(WAKS_ERR_HANDLE_CORRUPT,  "Invalid handle magic or offset") \
    X(WAKS_ERR_UNAUTHORIZED,    "Owner ID mismatch: unauthorized memory access") \
    X(WAKS_ERR_PAGE_FAULT,      "Hardware/Software page protection violation") \
    X(WAKS_ERR_UNSUPPORTED,     "Feature not available in this environment") \
    X(WAKS_ERR_EMPTY,           "Operation failed: Collection is empty") \
    X(WAKS_ERR_FULL,            "Operation failed: Fixed-size buffer is full") \
    X(WAKS_ERR_TIMEOUT,         "Synchronization primitive timed out") \
    X(WAKS_ERR_DEADLOCK,        "Potential deadlock detected") \
    X(WAKS_ERR_IO,              "IO failed: Incorrect or corrupt user input and output")

// Enum expansion
typedef enum waks_result {
#define X(code, string) code,
    WAKS_ERROR_LIST(X)
#undef X
    WAKS_ERR_COUNT
} waks_result;

typedef struct waks_context waks_context;
struct waks_context {
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

// Global context definition
extern waks_context global_panic_env;

// Function prototype annotations for the compiler
#if defined(__GNUC__) || defined(__clang__)
__attribute__((returns_twice)) waks_i32 waks_save_state(void);
#else
waks_i32 waks_save_state(void);
#endif

void waks_load_state(void);

typedef void (*waks_cleanup_func)(void *user_data, waks_u64 checkpoint);

waks_result waks_custom_pcall(
		void (*unsafe_func)(void*),   // the function we are trying to call whichis unsafe 
		void *arg,                    // the arguements of the function called
		waks_u64 checkpoint,          // location of the memory pointer 
		waks_cleanup_func cleanup_fn, 
		void *cleanup_arg); 

void waks_panic(waks_result error);


enum any_tag {
	_tag_none = 0,
	_tag_i64,
	_tag_u64,
	_tag_f32,
	_tag_f64,
	_tag_char,
	_tag_bool,
	_tag_waks_err,
	_tag_ptr,
};

// supports a 16 byte any_type
typedef struct Any Any;
struct Any {
    waks_u32 tag;     // 4 bytes
    waks_u32 _pad;    // 4 bytes padding
    union {
        waks_i64     i64;
        waks_u64     u64;
		waks_f32     f32;
		waks_f64     f64;
        waks_uchar   character;
        waks_bool    boolean;
        void        *ptr;
        waks_result  waks_err;
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

#define MatchF32(v, name) \
    case _tag_f32:;       \
	    waks_f32 name = (v).payload.f32;

#define MatchF64(v, name) \
    case _tag_f64:;       \
	    waks_f64 name = (v).payload.f64;

#define MatchChar(v, name)                                                  \
    case _tag_char:;                                                        \
        waks_uchar name = (v).payload.character;

#define MatchBool(v, name)                                                  \
    case _tag_bool:;                                                        \
        waks_bool name = (v).payload.boolean;

#define MatchNone(v) case _tag_none:;

#define MatchWaks(v, name)                                                  \
    case _tag_waks_err:;                                                    \
        waks_result name = (v).payload.waks_err;

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
waks_string waks_str_from_cstr(const waks_char *str);

waks_usize waks_f32_to_cstr(waks_f32 f, waks_char *buf, waks_ssize size);
waks_usize waks_f64_to_cstr(waks_f64 d, waks_char *buf, waks_ssize size);

// generates a substring from a string_slice from the string, start_pos, lenght of substring   
waks_string waks_str_sub(waks_string s, waks_usize start, waks_usize length);

// checks the equality between two string slice
waks_bool   waks_str_eq(waks_string a, waks_string b);

// checks the equality a string slice and a c string 
waks_bool   waks_str_eq_cstr(waks_string a, const waks_char *b);

// gets the length of a cstr 
waks_usize  waks_str_cstr_len(const waks_char *cstr);

// checks the equality between the two cstr
waks_bool   waks_str_cstr_eq_cstr(const waks_char *a, const waks_char *b);

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
Any AnyInt( waks_i64 value);
Any AnyUint(waks_u64 value);
Any AnyF32(waks_f32  value);
Any AnyF64(waks_f64  value);
Any AnyChar(waks_uchar value);
Any AnyBool(waks_bool strict);
Any AnyWaks(waks_result result);
Any AnyNone(void);
Any AnyPtr(void *ptr);

waks_string      waks_from_cstr(const waks_char *str);
const waks_char *waks_strerror(waks_result error);

#ifdef __cplusplus
}
#endif

#endif // !WAKS_TYPE_H

#ifdef WAKS_TYPE_IMPLEMENTATION 
#undef WAKS_TYPE_IMPLEMENTATION

waks_context global_panic_env;

__attribute__((naked)) waks_i32 waks_save_state(void) {
    __asm__ volatile (
        // Fetch Return Address (PC) from top of stack
        "movq (%rsp), %rax\n\t"
        "leaq global_panic_env(%rip), %rdx\n\t"
        "movq %rax, 8(%rdx)\n\t"

        // Save Stack Pointer (SP) adjusted for the call frame
        "leaq 8(%rsp), %rax\n\t"
        "movq %rax, 0(%rdx)\n\t"

        // Save Callee-Saved Registers
        "movq %rbx, 16(%rdx)\n\t"
        "movq %rbp, 24(%rdx)\n\t"
        "movq %r12, 32(%rdx)\n\t"
        "movq %r13, 40(%rdx)\n\t"
        "movq %r14, 48(%rdx)\n\t"
        "movq %r15, 56(%rdx)\n\t"

        // Return 0 on initial save
        "xorq %rax, %rax\n\t"
        "ret\n\t"
    );
}

__attribute__((naked, noreturn)) void waks_load_state(void) {
    __asm__ volatile (
        "leaq global_panic_env(%rip), %rdx\n\t"

        // Restore Stack Pointer & Frame Pointer
        "movq 0(%rdx), %rsp\n\t"
        "movq 24(%rdx), %rbp\n\t"

        // Restore Callee-Saved Registers
        "movq 16(%rdx), %rbx\n\t"
        "movq 32(%rdx), %r12\n\t"
        "movq 40(%rdx), %r13\n\t"
        "movq 48(%rdx), %r14\n\t"
        "movq 56(%rdx), %r15\n\t"

        // Return error_code (Offset 72) in EAX/RAX
        "movl 72(%rdx), %eax\n\t"

        // Jump back to saved PC (Offset 8)
        "jmp *8(%rdx)\n\t"
    );
}

waks_result waks_custom_pcall(void (*unsafe_func)(void*), void *arg, waks_u64 checkpoint,
		waks_cleanup_func cleanup_fn, void *cleanup_arg)
{
	global_panic_env.arena_checkpoint = checkpoint;
	waks_i32 status = waks_save_state();

	if (status == 0) {
         unsafe_func(arg);
		 return WAKS_OK;
	} else {
		// run a generic cleanup if function is passed
		if (cleanup_fn != ((void*)0)) {
			cleanup_fn(cleanup_arg, global_panic_env.arena_checkpoint);
		}
        return (waks_result)status;
	}
}

void waks_panic(waks_result error)
{
    *((volatile int *)&global_panic_env.error_code) = (int)error;
    waks_load_state();
}

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

Any AnyF32(waks_f32 value) {
    Any a;
    a.tag = _tag_f32;
    a._pad = 0;
    a.payload.f32 = value;
    return a;
}

Any AnyF64(waks_f64 value) {
    Any a;
    a.tag = _tag_f64;
    a._pad = 0;
    a.payload.f64 = value;
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

Any AnyWaks(waks_result result) {
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

waks_string waks_from_cstr(const waks_char *str) {
    waks_usize len = 0;
    if (str) {
        while (str[len]) len++;
    }
    return (waks_string){(waks_uchar *)str, len};
}

const waks_char *waks_strerror(waks_result error) {
    switch (error) {
#define X(code, string) case code: return WAKS_2CSTR_CAST(string);
		WAKS_ERROR_LIST(X)
#undef X
    default:
        return WAKS_2CSTR_CAST("UNKNOWN WAKS_STRING");
    }
}

///
// string implementation
///

/** Create a waks_string from a null-terminated C string */
waks_string waks_str_from_cstr(const waks_char *str) {
    waks_usize len = 0;
    if (str) {
        while (str[len]) len++;
    }
    return (waks_string){(waks_uchar *)str, len};
}

static waks_usize waks_internal_write_u64_rev(waks_u64 val, waks_char *buf) 
{
	waks_usize len = 0;
	if (val == 0) {
        buf[len++] = '0';
		return len;
	}
	while (val > 0) {
		buf[len++] = (waks_char)('0' + (val % 10));
		val /= 10;
	}
	return len;
}

// Helper function to handle raw integer to string writing in reverse
waks_usize waks_f32_to_cstr(waks_f32 f, waks_char *buf, waks_ssize size) 
{
    return waks_f64_to_cstr((waks_f64)f, buf, size);
}

waks_usize waks_f64_to_cstr(waks_f64 d, waks_char *buf, waks_ssize size) 
{
    if (!buf || size <= 0) return 0;
	if (size == 1) {
		buf[0] = '\0';
		return 0;
	}

    // bitwise inspection for IEEE-754 special values (NaN / Inf)
	union { 
		waks_f64 f; 
		waks_u64 u;
	} bits; 
	bits.f = d;

	waks_u64 sign     = (bits.u >> 63) & 1 ;
	waks_u64 exponent = (bits.u >> 52) & 0x7FF;
	waks_u64 mantissa = bits.u & 0x000FFFFFFFFFFFFFULL;

    // Handle NaN / Infinity
    if (exponent == 0x7FF) {
		// check or convert to waks_cstr
        const char *str = (mantissa != 0) ? "NaN" : (sign ? "-Inf" : "Inf");
        waks_usize i = 0;
        while (str[i] != '\0' && (waks_ssize)i < size - 1) {
            buf[i] = str[i];
            i++;
        }
        buf[i] = '\0';
        return i;
    }

    waks_usize idx = 0;

    // Handle Sign
    if (sign) {
        buf[idx++] = '-';
        d          = -d;
    }

    // Extract Integer and Fractional Parts
    waks_u64 int_part  = (waks_u64)d;
    waks_f64 frac_part = d - (waks_f64)int_part;

    // Format Integer Part (write reversed, then flip)
    waks_char rev_int[24];
    waks_usize int_len = waks_internal_write_u64_rev(int_part, rev_int);
    
    for (waks_ssize i = (waks_ssize)int_len - 1; i >= 0; i--) {
        if ((waks_ssize)idx < size - 1) buf[idx++] = rev_int[i];
    }

    // Format Decimal Point
    if ((waks_ssize)idx < size - 1) buf[idx++] = '.';

    // Format Fractional Part (Fixed to 6 decimal places)
    waks_usize precision = 6;
    for (waks_usize p = 0; p < precision; p++) {
        frac_part     *= 10.0;
        waks_u32 digit = (waks_u32)frac_part;
        if ((waks_ssize)idx < size - 1) buf[idx++] = (char)('0' + digit);
        frac_part -= digit;
    }

    buf[idx] = '\0';
    return idx;
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
waks_bool waks_str_eq_cstr(waks_string a, const waks_char *b) {
    if (!b) return (a.length == 0);
    
    waks_usize i = 0;
    for (; i < a.length; i++) {
        if (b[i] == '\0' || a.data[i] != (waks_uchar)b[i]) {
            return waks_false;
        }
    }
    return (b[i] == '\0');
}

waks_usize  waks_str_cstr_len(const waks_char *cstr) {
	waks_usize len = 0;
	while (cstr[len] != '\0') len++;
	return len;
}

waks_bool   waks_str_cstr_eq_cstr(const waks_char *a, const waks_char *b) {
	waks_usize a_len = waks_str_cstr_len(a);
	waks_usize b_len = waks_str_cstr_len(b);
	if (a_len != b_len) return waks_false;

	for (waks_usize i = 0;i < a_len; i++) {
		if (a[i] != b[i]) return waks_false;
	}
	return waks_true;
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

