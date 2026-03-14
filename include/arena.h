#ifndef ARENA_H
#define ARENA_H

typedef unsigned long size_t;
typedef unsigned long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef long uintptr_t;
typedef long ssize_t;

/// IN ARENA
#define NULL ((void *)0)
#define ALIGN_16(n) (((n) + 15) & ~15)
#define ALIGN_UP(n, align) (((n) + (align - 1)) & ~((align) - 1))
#define ALIGN_DOWN(n, align) ((n) & ~((align) - 1))
#define BOX_MAGIC 0xBAAD
#define PAGESIZE 4096

#define ATOMIC_RELAXED 0
#define ATOMIC_SEQ_CST 5

// x86_64 syscall no.
#if defined(__linux__)
#if defined(__x86_64)
#define SYS_mmap 9
#define SYS_munmap 11
#define SYS_write 1
#define SYS_exit 60
#define SYS_mprotect 10
#define SYS_madvise 28

#elif defined(__aarch64__) || defined(__riscv)
#define SYS_mmap 222
#define SYS_munmap 215
#define SYS_exit 93
#define SYS_write 64
#define SYS_mprotect 226
#define SYS_madvise 233

#endif

#define PROT_NONE 0x0
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_PRIVATE 0x02
#define MAP_ANONYMOUS 0x20
#define MADV_DONTNEED 4
#define MAP_FAILED ((void *)-1)
#define _SC_PAGESIZE 30

static inline long syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6)
{
    long ret;
#if defined(__x86_64)
    __asm__ volatile("movq %5,%%r10; movq %6,%%r8; movq %7,%%r9; syscall"
                     : "=a"(ret)
                     : "a"(n), "D"(a1), "S"(a2), "d"(a3), "g"(a4), "g"(a5), "g"(a6)
                     : "rcx", "r11", "memory");
#elif defined(__aarch64__)
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

#elif defined(__riscv)
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

#endif
    return ret;
}

#elif defined(_WIN32) || defined(_WIN64)

#define MEM_COMMIT 0x00001000
#define MEM_RESERVE 0x00002000
#define MEM_DECOMMIT 0x00004000
#define MEM_RELEASE 0x00008000
#define PAGE_NOACCESS 0x01
#define PAGE_READWRITE 0x04

// Manually declare Kernel functions to avoid windows.h
#ifdef __cpluscplus
extern "C"
{
#endif
    void *__stdcall VirtualAlloc(void *lpAddress, size_t dwSize, uint32_t flAllocationType,
                                 uint32_t flProtect);
    int __stdcall VirtualFree(void *lpAddress, size_t dwSize, uint32_t dwFreeType);
    void __stdcall ExitProcess(uint32_t uExitCode);
#ifdef __cpluscplus
}
#endif

#endif

static void os_panic()
{
#if defined(__linux)
    syscall6(SYS_exit, 1, 0, 0, 0, 0, 0);
#elif defined(_WIN32) || defined(_WIN64)
    ExitProcess(1);
#endif
}

static inline void dbg_print(const char *str);
static inline void dbg_print_int(int16_t n);

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
    for (uint64_t _pos = ArenaGetPos(arena), _once = 1; _once;                                     \
         _once = 0, ArenaSetPosBack(arena, _pos))

/// ASSERT:  ASSERT(needed_space <= arena->capacity);
#define PANIC_MSG(msg)                                                                             \
    do                                                                                             \
    {                                                                                              \
        dbg_print("[PANIC] ");                                                                     \
        dbg_print(msg);                                                                            \
        dbg_print(" at " __FILE__ ":");                                                            \
        dbg_print_int(__LINE__);                                                                   \
        os_panic();                                                                                \
    } while (0)

#undef ASSERT
#define ASSERT(cond)                                                                               \
    do                                                                                             \
    {                                                                                              \
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
#define BIT(n) (1ULL << (n))
#define SET_BIT(reg, n) ((reg) |= BIT(n))
#define CLR_BIT(reg, n) ((reg) &= ~BIT(n))
#define GET_BIT(reg, n) ((reg) & BIT(n))
#define GB(x) ((uint64_t)(x) << 30)

/// Automatic clean up RAAI
///  Arena *arena = ArenaAlloc();
///  current_arena = arena; // set global context for scoped macros
///  {
///      ScopedHandle handle = BoxNew(current_arena, uint32_t, 0);
///      ScopedBorrow(uint32_t,  handle, 0);
///      if (ptr) { *ptr = 1337; dbg_print("Data set successfully");}
///  }  <--- HandleRelease(current_arena, handle) and HandleRelease(current_arena,ptr) called;
///  ArenaRelease(arena);
#define ScopedHandle __attribute__((cleanup(_auto_release_handle))) __attribute__((unused)) Handle

// Use this when you want the borrow to end EXACTLY at the '}'
#define ScopedBorrow(type, handle, caller)                                                         \
    __attribute__((cleanup(_raii_release_now))) type *ptr =                                        \
        (type *)HandleBorrow(current_arena, handle, caller)

// Use this when you want to use the data now, but let the Arena clean it up later
#define DeferBorrow(type, handle, caller)                                                          \
    __attribute__((cleanup(_raii_release_deferred))) type *ptr =                                   \
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
        HandleMove((arena), _old, (old_owner), (new_owner))                                        \
    })

/// uint32_t my_id = 1;
/// Handle hd2 = MOVE_TO(arena, hd1, 2);
#define MOVE_TO(arena, handle, new_owner) MOVE(arena, handle, my_id, new_owner)

typedef struct Arena Arena;
struct Arena
{
    uint8_t *memory;
    uint64_t capacity;
    uint64_t position;
    uint64_t commited;
    uint64_t pagesize;
    uint32_t defer_count;
};

typedef struct BoxHeader BoxHeader;
struct BoxHeader
{
    uint32_t size;     // 0-3: Bytes of user data
    uint32_t owner_id; // 4-7: Standard uint
    uint16_t version;  // 8-9: Standard uint
    int16_t borrows;   // 10-11: Standard int
    uint16_t magic;    // 12-13: 0xBAAD
    uint16_t padding;  // 14-15: Explicit padding for 16B alignment
};

typedef struct Handle Handle;
struct Handle
{
    uint64_t offset;
    uint16_t version;
};

Arena *ArenaAlloc(void);
uint64_t ArenaGetPos(Arena *arena);
static inline void *ArenaPush(Arena *arena, uint64_t size);
static inline void ArenaSetPosBack(Arena *arena, uint64_t pos);
static inline void ArenaRelease(Arena *arena);
static inline void ArenaReset(Arena *arena);
static inline void _auto_release_handle(Handle *handle);

Handle BoxAlloc(Arena *arena, uint32_t size, uint32_t owner);
static inline void *HandleBorrow(Arena *arena, Handle handle, uint64_t caller);
static inline void *HandleBorrowMut(Arena *arena, Handle handle, uint32_t caller);
Handle HandleMove(Arena *arena, Handle handle, uint32_t old_owner, uint32_t new_owner);
static inline void HandleRelease(Arena *arena, Handle handle);
static inline void HandleReleaseMut(Arena *arena, Handle handle);
static inline void HandleDefer(Arena *arena, Handle handle);

static inline void _auto_release_handle(Handle *handle);
static inline void _raii_release_now(void *pointer);
static inline void _raii_release_deferred(void *pointer);

/// Context Management
extern _Thread_local Arena *current_arena;
static inline void _auto_release_handle(Handle *handle)
{
    if (!current_arena)
    {
        dbg_print("KERNEL PANIC: ScopeHandle used without active Arena!\n");
        return;
    }
    if (handle && handle->version > 0)
        HandleRelease(current_arena, *handle);
}

static inline void _raii_release_now(void *pointer)
{
    void **pointer_to_ptr = (void **)pointer;
    if (*pointer_to_ptr && current_arena)
    {
        BoxHeader *header = ((BoxHeader *)*pointer_to_ptr) - 1;
        Handle handle = {.offset = (uint8_t *)header - current_arena->memory,
                         .version = header->version};
        HandleRelease(current_arena, handle);
    }
}

static inline void _raii_release_deferred(void *pointer)
{
    void **pointer_to_ptr = (void **)pointer;
    if (*pointer_to_ptr && current_arena)
    {
        BoxHeader *header = ((BoxHeader *)*pointer_to_ptr) - 1;
        Handle handle = {.offset = (uint8_t *)header - current_arena->memory,
                         .version = header->version};
        HandleDefer(current_arena, handle);
    }
}

Arena *ArenaAlloc(void)
{
    uint64_t capacity = GB(64);

#if defined(__linux)
    void *base =
        (void *)syscall6(SYS_mmap, 0, capacity, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (base == MAP_FAILED || base == NULL)
        return NULL;

    Arena *arena = (Arena *)base;
    syscall6(SYS_mprotect, (long)base, PAGESIZE, PROT_READ | PROT_WRITE, 0, 0, 0);

#elif defined(_WIN32) || defined(_WIN64)
    void *base = VirtualAlloc(NULL, (size_t)capacity, MEM_RESERVE, PAGE_NOACCESS);
    if (!base)
        return NULL;

    if (!VirtualAlloc(base, PAGESIZE, MEM_COMMIT, PAGE_READWRITE))
        return NULL;

    Arena *arena = (Arena *)base;
#endif

    arena->memory = (uint8_t *)base;
    arena->capacity = capacity;
    arena->position = ALIGN_16(sizeof(Arena)); // starts after the header
    arena->commited = PAGESIZE;
    arena->pagesize = PAGESIZE;
    return arena;
}

static inline void *ArenaPush(Arena *arena, uint64_t size)
{
    uint64_t aligned_size = ALIGN_16(size);
    uint64_t next_pos = arena->position + aligned_size;
    // 16 handles for safety
    uint64_t needed_space = next_pos + (arena->defer_count * sizeof(Handle)) + 256;
    ASSERT(needed_space <= arena->capacity);

    if (needed_space > arena->commited)
    {
        uint64_t commit_needed = needed_space - arena->commited;
        uint64_t commit_aligned =
            ALIGN_16(commit_needed + arena->pagesize - 1) & ~(arena->pagesize - 1);

#if defined(__linux)
        uintptr_t ret = syscall6(SYS_mprotect, (long)(arena->memory + arena->commited),
                                 commit_aligned, PROT_READ | PROT_WRITE, 0, 0, 0);
        if (ret != 0)
            return NULL;
#elif defined(_WIN32) || defined(_WIN64)
        if (!VirtualAlloc(arena->memory + arena->commited, (size_t)commit_aligned, MEM_COMMIT,
                          PAGE_READWRITE))
            return NULL;
#endif

        arena->commited += commit_aligned;
    }

    void *ptr = arena->memory + arena->position;
    arena->position = next_pos;
    return ptr;
}

uint64_t ArenaGetPos(Arena *arena)
{
    return arena->position;
}

static inline void ArenaSetPosBack(Arena *arena, uint64_t position)
{
    ASSERT(position <= arena->position);
    uint64_t rounded_position = ALIGN_16(position + arena->pagesize - 1) & ~(arena->pagesize - 1);

    if (rounded_position < arena->commited)
    {
#if defined(__linux)
        syscall6(SYS_madvise, (long)(arena->memory + rounded_position),
                 arena->commited - rounded_position, MADV_DONTNEED, 0, 0, 0);
#elif defined(_WIN32) || defined(_WIN64)
        VirtualFree(arena->memory + rounded_position, (size_t)(arena->commited - rounded_position),
                    MEM_DECOMMIT);
#endif
    }

    arena->position = position;
}

static inline void ArenaRelease(Arena *arena)
{
    if (arena)
    {
#if defined(__linux)
        syscall6(SYS_munmap, (long)arena->memory, arena->capacity, 0, 0, 0, 0);
#elif defined(_WIN32) || defined(_WIN64)
        VirtualFree(arena->memory, 0, MEM_RELEASE);
#endif
    }
}

static inline void ArenaReset(Arena *arena)
{
    // process based on pagewide boundary
    for (uint32_t i = 1; i <= arena->defer_count; i++)
    {
        uint64_t defer_offset = arena->commited - (i * sizeof(Handle));
        Handle *handle_ptr = (Handle *)(arena->memory + defer_offset);
        BoxHeader *header = (BoxHeader *)(arena->memory + handle_ptr->offset);
#ifdef CONCURRENT_MODE
        // If it was a Mut borrow (-1), set to 0. If shared (>0), decrement.
        int16_t b = __atomic_load_n(&header->borrows, ATOMIC_SEQ_CST);
        if (b == -1)
            __atomic_store_n(&header->borrows, 0, ATOMIC_SEQ_CST);
        else if (b > 0)
            __atomic_fetch_sub(&header->borrows, 1, ATOMIC_SEQ_CST);
#else
        if (header->borrows == -1)
            header->borrows = 0;
        else if (header->borrows > 0)
            header->borrows--;
#endif
    }
    // Clear the defer counter.
    arena->defer_count = 0;

    // Reset the bump pointer to after the Arena struct
    // This makes all memory available again.
    // Old handles will fail because their version won't match
    // whatever is newly allocated in this space.
    uint64_t start_position = ALIGN_16(sizeof(Arena));

    if (arena->commited > start_position)
    {
        uint64_t size_to_reclaim = arena->commited - start_position;
#if defined(__linux)
        syscall6(SYS_madvise, (long)(arena->memory + start_position), size_to_reclaim,
                 MADV_DONTNEED, 0, 0, 0);
#elif defined(_WIN32) || defined(_WIN64)
        VirtualFree(arena->memory + start_position, (size_t)size_to_reclaim, 0x4000);
#endif
    }
    arena->position = start_position;
}

Handle BoxAlloc(Arena *arena, uint32_t size, uint32_t owner)
{
    uint32_t total_size = ALIGN_16(sizeof(BoxHeader) + size);
    uint8_t *raw_ptr = (uint8_t *)ArenaPush(arena, total_size);
    if (!raw_ptr)
        return (Handle){0, 0};

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

    return (Handle){.offset = (uint64_t)(raw_ptr - arena->memory), .version = 1};
}

static inline void *HandleBorrow(Arena *arena, Handle handle, uint64_t caller)
{
    if (handle.version == 0)
        return NULL;

    BoxHeader *header = (BoxHeader *)(arena->memory + handle.offset);

#ifdef CONCURRENT_MODE
    if (__atomic_load_n(&header->version, ATOMIC_SEQ_CST) != handle.version ||
        __atomic_load_n(&header->owner_id, ATOMIC_SEQ_CST) != caller || header->magic != BOX_MAGIC)
        return NULL;

    int16_t borrow = __atomic_load_n(&header->borrows, ATOMIC_SEQ_CST);
    if (borrow < 0)
        return NULL;
    __atomic_fetch_add(&header->borrows, 1, ATOMIC_SEQ_CST);
#else
    if (header->version != handle.version || header->owner_id != caller ||
        header->magic != BOX_MAGIC || header->borrows < 0)
        return NULL;
    header->borrows++;
#endif
    return (void *)(header + 1);
}

static inline void *HandleBorrowMut(Arena *arena, Handle handle, uint32_t caller)
{
    if (handle.version == 0)
        return NULL;
    BoxHeader *header = (BoxHeader *)(arena->memory + handle.offset);
    if (header->borrows != 0)
        return NULL;

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

Handle HandleMove(Arena *arena, Handle handle, uint32_t old_owner, uint32_t new_owner)
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

    return (Handle){.offset = handle.offset, .version = (uint16_t)(handle.version + 1)};
#else
    if ((header->owner_id != old_owner || header->version != handle.version) &&
        header->borrows != 0)
        return (Handle){0, 0};

    header->version++;
    header->owner_id = new_owner;
    return (Handle){.offset = handle.offset, .version = header->version};
#endif
}

static inline void HandleRelease(Arena *arena, Handle handle)
{
    BoxHeader *header = (BoxHeader *)(arena->memory + handle.offset);
    if (header->borrows == -1)
    {
        header->borrows = 0;
    }
    else if (header->borrows > 0)
    {
#ifdef CONCURRENT_MODE
        __atomic_fetch_sub(&header->borrows, 1, ATOMIC_SEQ_CST);
#else
        header->borrows--;
#endif
    }
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
    if (handle.version == 0)
        return;

    if (arena->position + (arena->defer_count + 1) * sizeof(Handle) >= arena->commited)
    {
        ArenaPush(arena, arena->pagesize);
    }

    // we store our defer list at the end of the arena growing backwards.
    arena->defer_count++;
    uint64_t defer_offset = arena->commited - (arena->defer_count * sizeof(Handle));

    Handle *defer_ptr = (Handle *)(arena->memory + defer_offset);
    *defer_ptr = handle;
}

static inline void dbg_print(const char *str)
{
#if defined(__linux__)
    const char *pointer = str;
    while (*pointer)
        pointer++;
    syscall6(SYS_write, 2, (long)str, (long)(pointer - str), 0, 0, 0);
#endif
}

static inline void dbg_print_int(int16_t n)
{
    char buf[16];
    int i = 0;
    if (n == 0)
    {
        buf[i++] = '0';
    }
    else
    {
        if (n < 0)
        {
            buf[i++] = '-';
            n = -n;
        }
        while (n > 0)
        {
            buf[i++] = (n % 10) + '0';
            n /= 10;
        }
    }
    buf[i] = '\0';
    // Simple reverse for display
    for (int j = 0; j < i / 2; j++)
    {
        char t = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = t;
    }
    dbg_print(buf);
    dbg_print("\n");
}

#endif
