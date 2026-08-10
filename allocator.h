
//
// waks_type name to the types 
//   - each file must be able to survive with itself without any dependency to any 
//     other external file ie containers.h, allocators.h, 
//   - have a malloc implementation also so it can survive without the need of arenas 
//   - make sure it works, easy to use and works as expected.
//   - ensure it doesn't conflict with the stdlib in cases where stblib may be used.
// 

#ifndef WAKS_ALLOCATOR_H
#define WAKS_ALLOCATOR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/// IN ARENA
#define WAKS_NOVALUE ((void *)0)               // waks_null

// This macro handle the alignment either by 16,up or down
#define WAKS_ALIGN_16(n)  (((n) + 15) & ~15) // waks_align16
#define WAKS_ALIGN_UP(n, align) (((n) + (align - 1)) & ~((align) - 1))
#define WAKS_ALIGN_DOWN(n, align) ((n) & ~((align) - 1))


// LINUX/MAC SYSCALL NUMBER MACRO DEFINITION
#if defined(__linux__) || defined(__APPLE__)
    // syscall numbers for different architectures
 	#if defined(__x86_64)
 		#define SYS_mmap     9    // reserves/maps vritual memory pages.
 		#define SYS_munmap   11   // unmaps virtual memory pages.
 		#define SYS_write    1    // raw i/o to file descriptors.
 		#define SYS_exit     60   // terminate process immidiately.
 		#define SYS_mprotect 10   // change memory page permision (READ/WRITE/NONE).
 		#define SYS_madvise  28   // decommit/release physical RAM back to OS
 	#elif defined(__aarch64__) || defined(__riscv)
    	#define SYS_mmap     222
    	#define SYS_munmap   215
    	#define SYS_exit     93
    	#define SYS_write    64
    	#define SYS_mprotect 226
    	#define SYS_madvise  233
 	#endif
#endif // LINUX AND MAC SYSCALL NO

// This function when called allows us to communicate with the OS 
// kernel directly without linking with c library wrappers like libc/glibc.
// By passes glibc when requesting OS services by putting services 
// directly in CPU registers and triggering a hardware trap instruction.
long waks_syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6);

// This when called prints null terminated strings directly to 
// STDERR. It performs zero heap allocations and by passes the 
// stdio buffering, ensuring logs are emmited during critical  
// crashes or allocator panics.
void waks_dbg_print(const waks_uchar *str);

// This function when called converts/fmts a 16 bit signed integer into 
// ASCII using a small stack-allocated buffer and modulo operations.
void waks_dbg_print_int(waks_i16 n);

// This function when called aborts execution immidiately by terminating 
// the host process through making a system call directly to the OS
// in linux and Exiting the process in windows.
void waks_os_panic(void);

// This macro is used when we want a part of the program to panic  and return a 
// panic message and the file location and line where the panic occured
#define WAKS_PANIC_MSG(msg)                   \
    do {                                      \
        waks_dbg_print("[PANIC] ");           \
        waks_dbg_print((msg));                \
        waks_dbg_print(" at " __FILE__ ":");  \
        waks_dbg_print_int(__LINE__);         \
        waks_os_panic();                      \
    } while (0)

#undef WAKS_ASSERT

// This macro takes a  condition  and panics returning a message and line where the 
// program failed.
#define WAKS_ASSERT(cond)   				\
    do {                                    \
        if (!(cond)) WAKS_PANIC_MSG(#cond); \
    } while (0)

/// BIT MANIPULATION
// This macro handle bit manipuaation 
#define WAKS_BIT(n) (1ULL << (n))   // waks_bit
#define SET_BIT(reg, n) ((reg) |= WAKS_BIT(n))
#define CLR_BIT(reg, n) ((reg) &= ~WAKS_BIT(n))
#define GET_BIT(reg, n) ((reg) & WAKS_BIT(n))

// This macro is used to shift bits and is used in our arena_allocator function to 
// allocate a gigabyte of memory space 
#define GB(x) ((waks_u64)(x) << 30)

// This macro allows for automatic cleanup of the memory borrowed once we get 
// out of scope. This happens by calling the cleanup function  and rendering 
// the memory allocated unusefull once we get out of scope
#define WAKS_scoped_handle __attribute__((cleanup(_auto_release_handle))) __attribute__((unused)) waks_handle

// This macro allows us to borrow memory from a handle and the cleanup will happen 
// automatically when we go out of scope. It also returns a pointer that needs to 
// be checked if valid if we use it: 
// if (!name) return; 
// we also need to assign the pointer a value:  *name = 43;
#define WAKS_scoped_borrow(type, name, handle, caller) \
    __attribute__((cleanup(_raii_release_now)))  \
    type *name =  (type *)waks_handle_borrow(current_arena, handle, caller)

// Much safer version of Scoped borrow where we check validity of the pointer 
// @TODO(waks-work): we have to find a better way to handle when there is no 
//                   pointer.
#define WAKS_safe_scoped_borrow(type, name, handle, caller) \
        WAKS_scoped_borrow(type, name, handle, caller);     \
        if (!name) return;

// This macro allows us to borrow memory and have it defered or released at 
// the end when we call ArenaRelease. May be used in cases where you may want 
// an object or something to live up to some time and  not be removed automatically 
// at the end of the scope.
#define WAKS_defer_borrow(type, name, handle, caller) \
    __attribute__((cleanup(_raii_release_deferred)))  \
    type *name = (type *)waks_handle_borrow(current_arena, handle, caller)


// This macro automatically creates a context where we can set the current_arena 
// ie player_arena, game_arena , that allows us to use it for a specific part where 
// it lives for the specific  part and  does not exist outside the context of the 
// block
#define WAKS_TEMP_ARENA(context_arena) \
    for (waks_arena *_old = current_arena, *_once = (current_arena = (context_arena),(waks_arena *)1); \
        _once;_once = 0, current_arena = _old)

// This  macro allows us to move ownership of a handle from the
// current owner to the new owner and derefences the handle of
// our current owner making it unusefull
// eg: WAKS_MOVE(current_arena,handle, current_owner, new_owner);
// now here the current_owner cannot have ownership of the handle
#define WAKS_MOVE(arena, handle, old_owner, new_owner)             \
    ({                                                             \
        WAKS_ASSERT((handle).version != 0);                        \
        waks_handle _old = (handle);                               \
        (handle) = (waks_handle){0, 0};                            \
        waks_handle_move((arena), _old, (old_owner), (new_owner)); \
    })

// This macro like the one above but we just provide the only the address 
// of the new owner. 
// @TODO(waks-work): check how we can implement my_id or how it is implemented 
// or how it can be infered.
#define WAKS_MOVE_TO(arena, handle, new_owner) WAKS_MOVE(arena, handle, my_id, new_owner)

// This is an iterator macro that allows us to iterate over a slice which has 
// a pointer to the memory location and the length of the slice. 
#define WAKS_foreach_slice(type, item, slice) \
    for (type *item = (type *)(slice).ptr, *_end = ((item) + (slice).len); \
        (item)< _end; (item)++)

// This is an iterator macro that allows us to iterate over an array 
#define WAKS_foreach_arr(type, item, array) \
    for (type *item = (type *)(array), *_end = (array) + sizeof(array)/sizeof(array[0]); \
        (item) < _end; (item)++)

// This is an iterator slice macro that allows for automatically infered types. 
#define WAKS_foreach_slice_auto(item, slice) \
    for (__typeof__((slice).ptr) item = (slice).ptr, _end = (item) + (slice).len; \
         (item) < _end; item++)

// This is an iterator slice macro that allows for allows to put a conditon 
// and an action which can occur incase the condition fails
#define WAKS_foreach_slice_if(type, item, slice, condition, action) \
    WAKS_foreach_slice(type, item, slice){ \
        if (!(condition)) action           \
    }

// This is an iterator macro that filters a slice given a specific condition
// @TODO(waks_work): Check on the explanation of this macro  and its imporvement.
#define WAKS_foreach_slice_filter(type, item, slice, condition) \
    WAKS_foreach_slice(type, item, slice) if (!(condition)) continue; \
    else /// condition will be executed here

// This is an iterator macro that maps a slice given a specific condition
// @TODO(waks_work): Check on the explanation of this macro  and its imporvement.
#define foreach_map(type, item, slice, expression) \
    WAKS_foreach_slice(type, item, slice) { expression; }

// This is an iterator macro that iterates over a given node given 
// the node head. It allows us to iterates over linked lists and list objects.
#define WAKS_foreach_node(type, item, head) \
    for (type *item = (head); item != WAKS_NOVALUE; item = item->next)

// This iterator is a much safer implementation of the WAKS_foreach_node
#define WAKS_foreach_node_safe(type, item, next_item, head)  \
    for (type *item = (head), *next_item = item ? item->next : WAKS_NOVALUE; \
         item != WAKS_NOVALUE; \
         item = next_item, next_item = item ? item->next : WAKS_NOVALUE)

// This is an iterator macro that is used to find a member in a node  
// it can be used in tree to find elements. 
#define WAKS_foreach_find(type, item, head, condition, result_assign) \
    WAKS_foreach_node(type, item, head) { \
        if (condition) {             \
            result_assign;           \
            break;                   \
        }                            \
    }

// This is a Reverse Iterator (LIFO / Cleanup) that iterates through a slice 
// in the reverse order.
#define WAKS_foreach_rev(type, item, slice) \
    for (type *item = (slice).len ? ((type *)((slice).ptr) + (slice).len - 1) : WAKS_NOVALUE, \
        *_start = (type *)(slice).ptr; item >= _start; item--)

// This is a strided/Walking memory pages or fixed-size blocks (4KB / Stride)
// iterator that iterates over memory spaces
#define WAKS_foreach_step(type, item, slice, step) \
    for (type *item = (type *)(slice).ptr, *_end = (type *)((waks_uchar *)(slice).ptr + (slice).len); \
         (waks_uchar *)item < (waks_uchar *)_end; item = (type *)((waks_uchar *)item + (step)))

// This is a sentinel/ Null terminated strings/streams iterator that 
// iterates over pointers 
// @TODO(waks-work): make a better description of this macro.
#define WAKS_foreach_ptr(type, item, start_ptr) \
    for (type *item = (type *)(start_ptr); item && *item != (type)0; item++)

// This macro is an iterator that allows us to borrow memory from  a handle 
// use it proving something like a context but for the HandleAllocator then 
// later at the end automatically release the handle when we finish with it.
#define WAKS_foreach_borrow(type, item, handle, owner) \
    for (type *item = (type *)waks_handle_borrow(current_arena, handle, owner), *_once = item; \
        _once; _once = WAKS_NOVALUE, waks_handle_release(current_arena, handle))

// Allows us to defer using a macro
#define WAKS_defer(code)                                \
    void _waks_internal_defer_##__LINE__() { code; }    \
    void (*_waks_internal_ptr_##__LINE__)(void)         \
         __attribute__((cleanup(_waks_defer_cleanup))) = _waks_internal_defer_##__LINE__;


/// ARENA STRUCTS

typedef enum {
    WAKS_ARENA_FLAG_NONE       = 0,
    WAKS_ARENA_FLAG_READONLY   = (1 << 0), // Prevents new pushes
    WAKS_ARENA_FLAG_ALLOW_GROW = (1 << 1), // Allows page reallocation
} waks_arena_flags;

typedef struct waks_arena waks_arena;
struct waks_arena
{
    // This member points to the start of the virtual memory region assigned 
    // to this memory arena. We changed from waks_uchar * to void * to make it 
    // more generic and allow for us to use any type.
    void       *memory;

    // This shows the total virtual memory capacity in bytes reserved by the OS 
    // ie a huge memory chunk like 1GB, 64 GB this is the initial memory allocated 
    // once at the begining
    waks_u64    capacity;

    // This shows the current bumb-allocation pointer relative to the  
    // start of our virtual memory allocated by the operating system. 
    // The pointer moves forward the more we push the given memory.
    waks_u64    position;

    // This represents the total physical RAM in bytes currently committed using 
    // VirtualAlloc or waks_syscall by the OS, which was reserved in virtual  
    // memory. The memory allocated is the size of a PAGESIZE (4KB).
    waks_u64    commited;

    // This defines the allocation granularity when allocating additional memory. 
    // Indicates the size we can allocate at each point of additional allocation. 
    waks_u64    pagesize;

    // Carries the total number of active handles defered before we reset.
    waks_u32    defer_count;

    // @NOTE(waks-work): we can add this member to tell us what the current state(flag)
    // and also acts as an explicit padding of 4 bytes.
    waks_u32    flags;
};

// This holds the metadata for waks_handle allowing it to be 16 bytes.
// It is stored at the begining of the memory arena so it is placed 
// only once.
typedef struct waks_box_header waks_box_header;
struct waks_box_header
{
    waks_u32 size;     // 0-3:   User payload size requested during allocation
    waks_u32 owner_id; // 4-7:   Unique owner id assigned to the memory allocated

    // 8-9: Version tag that is incremented on release to invalidate old handles.
    waks_u16 version;  // 8-9:  
    waks_i16 borrows;  // 10-11: Active borrow cournt. >0 shared, -1 for mut, 0 free.
    waks_u16 magic;    // 12-13: Integrity check magic number (0xBAAD).
    waks_u16 padding;  // 14-15: Explicit padding for 16B alignment
};

// This struct acts like a smart pointer that tracks the offset, allocations 
// and helps us avoid use after free.
typedef struct waks_handle waks_handle;
struct waks_handle
{
    waks_u64 offset;  // byte offset from arena->memory start to waks_box_header
    waks_u16 version; // expected version tag, helps to protect against use after free.

    // @TODO(waks-work) Come with a way to add this to our current code base.
    waks_u16 flags;   // reserved capability/access flag 
    waks_u32 padding; // a 4 byte padding ensuring total handle size
};

/// ARENA API/METHOD IMPLEMENTATION

// This  requests a large chunk of virtual address memory from the OS 
// depending with the provided capacity and returns a pointer to the 
// memory location.
// When the memory is reserved it is granted read and write capability.
waks_arena *waks_arena_alloc(waks_u64 capacity);

// This function when called bumps the allocation pointer in the arena 
// forward while it allocates spaces  a contigous memory block. 
// When called also automatically commits physical RAM from the OS 
// as needed.  
// Returns a pointer to memory if allocated 
// @TODO(waks-work): check what it returns if the capacity is exceeded 
//       as i am sure it reallocates but of a larger size or allocates 
//       another page.
void       *waks_arena_push(waks_arena *arena,waks_u64 size);

// This function when called gets the current position of the bump 
// allocation pointer in the memory arena.
waks_u64    waks_arena_get_pos(waks_arena *arena);

// This function rewinds the bumb pointer to a previous saved 
// checkpoint. Here unused memory pages are decommited  to save 
// RAM.
void        waks_arena_set_pos_back(waks_arena *arena, waks_u64 pos);

// This function when called unmaps and frees virtual memory and 
// physical RAM.
void        waks_arena_release(waks_arena *arena);

// This function when called resets the arena position to zero 
// and clears all the defered handle borrows.
// Also invalidates all existing handles by bumpin up header 
// versions while memory is retained.
void        waks_arena_reset(waks_arena *arena);

// @TODO(waks-work): Try to explain this better. 
// Allows it to automatically release it.
void        _auto_release_handle(waks_handle *handle);


/// ALLOCATOR STRUCTS 
//

// This is an allocator struct that allows us to have a 
// more generic allocator struct to be used with any kind 
// of allocator ie arena, pool, buffer, handle etc
typedef struct WaksAllocator WaksAllocator;
struct WaksAllocator {
 
    // This points to the function that allocates memory
    void *(*alloc)(void *context, waks_ssize size, waks_ssize alignment);

    // This points to the function that frees the allocated memory
    void (*free)(void *context, void *ptr);
    void *context;
};

// This is an internal vtable allocation function for WaksAllocator wrapper.
void *arena_alloc_w(void *context, waks_ssize size, waks_ssize alignment);

// This is a no operation free adapter for arena allocation as arenas 
// do not support the free operation. 
void arena_free_w(void *context, void *ptr);

// This wraps the waks_arena into a generic vtable allocator
WaksAllocator waks_arena_as_allocator(waks_arena *arena);

/// HANDLE ALLOCATORS
//

// @TODO add api usage and documentation directly to their declaration

// This allocates a versioned memory box with the metadata inside the arena.
// We can indicate the size of the allocation we want, and the initial owner 
// of the memory box, where we can track the borrowing and move operation on it.
waks_handle  waks_box_alloc(waks_arena *arena, waks_u32 size, waks_u32 owner);

// This request for an immutable/shared borrow of a handle's payload. 
// In which it increments the internal borrow count. Fails if already  
// borrowed mutably or if the version or owner mismatches.
void        *waks_handle_borrow(waks_arena *arena, waks_handle handle,    waks_u32 caller);

// This request for a mutable borrow of a handle payload but can 
// only be done once not more than twice. It agains sets the borrow 
// count to -1 which invalidates when they try to borrow again.
void   		*waks_handle_borrow_mut(waks_arena *arena, waks_handle handle, waks_u32 caller);

// This transfers ownership of a box handle to a new owner id. Bumps the 
// handle version tag to invalidate all existing borrows held by the  
// previous owner.
waks_handle  waks_handle_move(waks_arena *arena, waks_handle handle, waks_u32 old_owner, waks_u32 new_owner);

// This releases a shared or mutable borrow to a handle's payload. 
// Decrements a borrow count or clears a mutable lock flag.
void         waks_handle_release(waks_arena *arena, waks_handle handle);

// This clears the mutable borrows flag.
void         waks_handle_release_mut(waks_arena *arena, waks_handle handle);

// This pushes the arena's to the handles defer list to automatically 
// release borrows upon scope reset.
void         waks_handle_defer(waks_arena *arena, waks_handle handle);

/// AUTO RELEASE FEATURES

void _auto_release_handle(waks_handle *handle);
void _raii_release_now(void *pointer);
void _raii_release_deferred(void *pointer);

/// AUTO RELEASE IMPLEMENTATION

// This allows to create a scope/thread global context for the 
// RAII macros ie WAKS_TEMP_ARENA, ... etc
extern waks_arena *current_arena;

/// ERROR HANDLING

// This function allows to safely call function allowing us to catch 
// it at function call 
// @TODO(waks-work): Go down and deeply explain the code  runs this 
// two functions in the background and logic behind it
WaksResult waks_pcall(waks_arena *arena, void (*fn)(void *), void *arg);
void waks_panic(WaksResult err);

__attribute__((aligned(16))) WaksContext global_panic_env;

waks_arena *current_arena = WAKS_NOVALUE;

#ifdef __cplusplus 
}
#endif

#endif // WAKS_ALLOCATOR_H


#ifdef WAKS_ALLOCATOR_IMPLEMENTATION

#define BOX_MAGIC    0xBAAD
#define PAGESIZE     4096

#define ATOMIC_RELAXED 0
#define ATOMIC_SEQ_CST 5

// LINUX/MAC MEMORY MACRO DEFINITION
#if defined(__linux__) || defined(__APPLE__)
 	#define PROT_NONE     0x0
 	#define PROT_READ     0x1
 	#define PROT_WRITE    0x2
 	#define MAP_PRIVATE   0x02
 	#define MAP_ANONYMOUS 0x20
 	#define MADV_DONTNEED 4
 	#define MAP_FAILED    ((void *)-1)
 	#define _SC_PAGESIZE  30

long waks_syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6)
{
	#if defined(__x86_64)
    long ret;
    register long r10 __asm__("r10") = a4;
    register long r8 __asm__("r8")   = a5;
    register long r9 __asm__("r9")   = a6;

    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9)
                     : "rcx", "r11", "memory");
	#elif defined(__aarch64__)
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

    #elif defined(__riscv)
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

    #endif
    return ret;
}
#endif // LINUX MEMORY MACRO DEFINITION

// WINDOW'S MEMORY MACRO DEFINITION
#if defined(_WIN32) || defined(_WIN64)
	#define MEM_COMMIT     0x00001000   // waks_mem_commit
	#define MEM_RESERVE    0x00002000
	#define MEM_DECOMMIT   0x00004000
	#define MEM_RELEASE    0x00008000
	#define PAGE_NOACCESS  0x01
	#define PAGE_READWRITE 0x04     // waks_page_readwrite

// Manually declare Kernel functions to avoid windows.h
// This function when called allocates memory ... in windows based system
void     *__stdcall VirtualAlloc(void *lpAddress, waks_usize dwSize, waks_u32 flAllocationType, waks_u32 flProtect);

// This function when called frees the memory by ... 
waks_i32  __stdcall VirtualFree(void *lpAddress, waks_usize dwSize, waks_u32 dwFreeType);

// This function ...
void      __stdcall ExitProcess(waks_u32 ExitCode);

#endif // WINDOWS MEMORY DEFINITION

// what it does if we use a stdlib instead of windows or linux specifics
#if defined(USE_STD_LIB)
	#include <stdlib.h>
	#define OS_COMMIT(ptr, size) (1)
	#define OS_DECOMMIT(ptr, size)
#endif

void waks_os_panic(void)
{
#if defined(__linux)
    waks_syscall6(SYS_exit, 1, 0, 0, 0, 0, 0);
#elif defined(_WIN32) || defined(_WIN64)
    ExitProcess(1);
#endif
}


///
/// ARENA IMPLEMENTATION
//  @TODO this section should be moved to the #ifdef WAKS_ALLOCATOR_IMPLEMENTATION

waks_arena *waks_arena_alloc(waks_u64 capacity)
{
    void *base = WAKS_NOVALUE;
#if defined(__linux)
    // Ask the OS for a large chunk of the of the virtual memory but tells
    // the hardware not to allocate physical RAM yet
    base = (void *)waks_syscall6(SYS_mmap, 0, capacity, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (base == MAP_FAILED || base == WAKS_NOVALUE) return WAKS_NOVALUE;

    waks_arena *arena = (waks_arena *)base;
    /* Changes access from NOACCESS to READ && WRITE */
    // @TODO check the use of the types long... to ensure proper compatibility
    waks_syscall6(SYS_mprotect, (long)base, PAGESIZE, PROT_READ | PROT_WRITE, 0, 0, 0);

#elif defined(_WIN32) || defined(_WIN64)
    base = VirtualAlloc(WAKS_NOVALUE, (waks_ssize)capacity, MEM_RESERVE, PAGE_NOACCESS);
    if (!base) return WAKS_NOVALUE;

    if (!VirtualAlloc(base, PAGESIZE, MEM_COMMIT, PAGE_READWRITE))
        return WAKS_NOVALUE;

    waks_arena *arena = (waks_arena *)base;
#elif defined(USE_STD_LIB)
    waks_u64 total_size = capacity + sizeof(waks_arena);
    base = malloc(total_size);
    if (!base) return WAKS_NOVALUE;
    if (!OS_COMMIT(base, PAGESIZE)) {
        free(base);
        return WAKS_NOVALUE;
    }
#endif

    arena->memory      = (void *)base;
    arena->capacity    = capacity;
    arena->position    = WAKS_ALIGN_16(sizeof(waks_arena)); // starts after the header
    arena->commited    = PAGESIZE;
    arena->pagesize    = PAGESIZE;
    arena->defer_count = 0;
    return arena;
}

void *waks_arena_push(waks_arena *arena, waks_u64 size)
{
    waks_u64 aligned_size = WAKS_ALIGN_16(size);
    waks_u64 next_pos     = arena->position + aligned_size;

    /* Safety Buffer: allocate last 256 bytes  of the page to the handle */
    // Do a better explanation of this specific parts and complex code to prevent 
    // bugs that may arise due lack of information
    waks_u64 needed_space = next_pos + (arena->defer_count * sizeof(waks_handle)) + 256;
    WAKS_ASSERT(needed_space <= arena->capacity);

    // We will allocate an extra memory page if the (needed_space + defer_count)
    // is greater than a single pagesize(4KB)
    if (needed_space > arena->commited) {
        waks_u64 commit_needed  = needed_space - arena->commited;
        waks_u64 commit_aligned = WAKS_ALIGN_16(commit_needed + arena->pagesize - 1) & ~(arena->pagesize - 1);

#if defined(__linux)
        waks_uintptr ret = waks_syscall6(SYS_mprotect, (long)(waks_uchar *)(arena->memory + arena->commited),
                                 commit_aligned, PROT_READ | PROT_WRITE, 0, 0, 0);
        if (ret != 0) return WAKS_NOVALUE;
#elif defined(_WIN32) || defined(_WIN64)
        if (!VirtualAlloc(arena->memory + arena->commited, (waks_usize)commit_aligned, MEM_COMMIT,
                          PAGE_READWRITE))
            return WAKS_NOVALUE;
#elif defined(USE_STD_LIB)
        if (!OS_COMMIT(arena->memory + arena->commited, commit_aligned))
            return WAKS_NOVALUE;
#endif
        arena->commited += commit_aligned;
    }

    void *ptr = arena->memory + arena->position;
    arena->position = next_pos;
    return ptr;
}

waks_u64 waks_arena_get_pos(waks_arena *arena)
{
    return arena->position;
}


// TODO(waks-work): try to figure out how we can use defer_count either
// to increment or decrement or just leave it as it is after reset
void waks_arena_set_pos_back(waks_arena *arena, waks_u64 position)
{
    // @TODO  make this more explicit to avoid some hidden operation
    WAKS_ASSERT(position <= arena->position);
    waks_u64 rounded_position = WAKS_ALIGN_16(position + arena->pagesize - 1) & ~(arena->pagesize - 1);

    if (rounded_position < arena->commited) {
#if defined(__linux)
        waks_syscall6(SYS_madvise, (long)(arena->memory + rounded_position),
                 arena->commited - rounded_position, MADV_DONTNEED, 0, 0, 0);
#elif defined(_WIN32) || defined(_WIN64)
        VirtualFree(arena->memory + rounded_position, (waks_usize)(arena->commited - rounded_position),
                    MEM_DECOMMIT);
#elif defined(USE_STD_LIB)
        //  This in context does nothing as seen in macro
        //  TODO(waks-work); may be check if there is any need of having this block of Code
        waks_u64 size_to_free = arena->commited - rounded_position;
        OS_DECOMMIT(arena->memory + rounded_position, size_to_free);
        arena->commited = rounded_position;

#endif
    }

    arena->position = position;
}

void waks_arena_release(waks_arena *arena)
{
    if (arena) {
#if defined(__linux)
        waks_syscall6(SYS_munmap, (long)arena->memory, arena->capacity, 0, 0, 0, 0);
#elif defined(_WIN32) || defined(_WIN64)
        VirtualFree(arena->memory, 0, MEM_RELEASE);
#elif defined(USE_STD_LIB)
        free(arena);
#endif
    }
}


// Used to reset all the borrow count together for all the borrow
// we called HandleDefer on for a specific scope.
void waks_arena_reset(waks_arena *arena)
{
    // process based on pagewide boundary
    for (waks_u32 i = 1; i <= arena->defer_count; i++) {
        waks_u64  defer_offset = arena->commited - (i * sizeof(waks_handle));
        waks_handle     *handle_ptr  = (waks_handle *)(arena->memory + defer_offset);
        waks_box_header *header      = (waks_box_header *)(arena->memory + handle_ptr->offset);

// @TODO for concurency ensure that there is more emphasis on the safety
#ifdef CONCURRENT_MODE
        // If it was a Mut borrow (-1), set to 0. If shared (>0), decrement.
        waks_i16 b = __atomic_load_n(&header->borrows, ATOMIC_SEQ_CST);
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
        // @TODO(waks-work): we need to invalidate the handle after this point 
        // rendering them unusable
#endif
    }
    // Clear the defer counter.
    arena->defer_count = 0;

    // Reset the bump pointer to after the Arena struct
    // This makes all memory available again.
    // Old handles will fail because their version won't match
    // whatever is newly allocated in this space.
    waks_u64 start_position = WAKS_ALIGN_16(sizeof(waks_arena));

    if (arena->commited > start_position) {
        waks_u64 size_to_reclaim = arena->commited - start_position;
#if defined(__linux)
        waks_syscall6(SYS_madvise, (long)(arena->memory + start_position), size_to_reclaim,
                 MADV_DONTNEED, 0, 0, 0);
#elif defined(_WIN32) || defined(_WIN64)
        VirtualFree(arena->memory + start_position, (waks_ssize)size_to_reclaim, 0x4000);
#elif defined(USE_STD_LIB)
        OS_DECOMMIT(arena->memory + arena->pagesize, size_to_reclaim);
        arena->commited = arena->pagesize;
#endif
    }
    arena->position = start_position;
}

/// WAKS_ALLOCATOR_IMPLEMENTATION
//

void *arena_alloc_w(void *context, waks_ssize size, waks_ssize alignment)
{
	waks_arena *arena = (waks_arena*)context;
    (void)alignment;
    return ArenaPush(arena,(waks_u64)size);
}

void arena_free_w(void *context, void *ptr) 
{
    (void)context;
    (void)ptr;
}

WaksAllocator waks_arena_as_allocator(waks_arena *arena){
    return (WaksAllocator){
 	  .alloc = arena_alloc_w, 
 	  .free = arena_free_w, 
      .context =arena};
}

/// HANDLE ALLOCATORS IMPLEMENTATION
//

waks_handle waks_box_alloc(waks_arena *arena, waks_u32 size, waks_u32 owner)
{
    waks_u32 total_size = WAKS_ALIGN_16(sizeof(waks_box_header) + size);
    waks_uchar *raw_ptr = (waks_uchar *)waks_arena_push(arena, total_size);
    if (!raw_ptr) return (waks_handle){0, 0};

    waks_box_header *handle = (waks_box_header *)raw_ptr;
    handle->size  = size;
    handle->magic = BOX_MAGIC;

#ifdef CONCURRENT_MODE
    __atomic_store_n(&handle->owner_id, owner, ATOMIC_SEQ_CST);
    __atomic_store_n(&handle->version, 1, ATOMIC_SEQ_CST);
    __atomic_store_n(&handle->borrows, 0, ATOMIC_SEQ_CST);
#else
    handle->owner_id = owner;
    handle->version  = 1;
    handle->borrows  = 0;
#endif

    return (waks_handle){.offset = (waks_u64)(raw_ptr - arena->memory), .version = 1};
}

void *waks_handle_borrow(waks_arena *arena, waks_handle handle, waks_u32 caller)
{
    if (handle.version == 0) return WAKS_NOVALUE;

    waks_box_header *header = (waks_box_header *)(arena->memory + handle.offset);

#ifdef CONCURRENT_MODE
    if (__atomic_load_n(&header->version, ATOMIC_SEQ_CST) != handle.version ||
        __atomic_load_n(&header->owner_id, ATOMIC_SEQ_CST) != caller || header->magic != BOX_MAGIC)
        return WAKS_NOVALUE;

    waks_i16 borrow = __atomic_load_n(&header->borrows, ATOMIC_SEQ_CST);
    if (borrow < 0) return WAKS_NOVALUE;

    __atomic_fetch_add(&header->borrows, 1, ATOMIC_SEQ_CST);
#else
    if (header->version != handle.version || header->owner_id != caller ||
        header->magic != BOX_MAGIC || header->borrows < 0)
        return WAKS_NOVALUE;

    header->borrows++;
#endif
    return (void *)(header + 1);
}

void *waks_handle_borrow_mut(waks_arena *arena, waks_handle handle, waks_u32 caller)
{
    if (handle.version == 0) return WAKS_NOVALUE;

    waks_box_header *header = (waks_box_header *)(arena->memory + handle.offset);
    if (header->borrows != 0) return WAKS_NOVALUE;

#ifdef CONCURRENT_MODE
    // Atomic Compare and Swap (CAS) to ensure borrows is exactly 0
    waks_i16 expected = 0;
    if (__atomic_load_n(&header->version, ATOMIC_SEQ_CST) != handle.version ||
        __atomic_load_n(&header->owner_id, ATOMIC_SEQ_CST) != caller)
        return WAKS_NOVALUE;

    if (!__atomic_compare_exchange_strong_n(&header->borrows, &expected, -1, ATOMIC_SEQ_CST))
        return WAKS_NOVALUE;
#else
    if (header->version != handle.version || header->owner_id != caller || header->borrows != 0)
        return WAKS_NOVALUE;

    header->borrows = -1;
#endif
    return (void *)(header + 1);
}

waks_handle waks_handle_move(waks_arena *arena, waks_handle handle, waks_u32 old_owner, waks_u32 new_owner)
{
     waks_box_header *header = (waks_box_header *)(arena->memory + handle.offset);

#ifdef CONCURRENT_MODE
    // Ensure caller is current owner and version matches
    if (__atomic_load_n(&header->owner_id, ATOMIC_SEQ_CST) != old_owner ||
        __atomic_load_n(&header->version, ATOMIC_SEQ_CST) != handle.version)
        return (waks_handle){0, 0};

    // Kill all existing borrows/handles by bumping version
    __atomic_fetch_add(&header->version, 1, ATOMIC_SEQ_CST);
    __atomic_store_n(&header->owner_id, new_owner, ATOMIC_SEQ_CST);

    return (waks_handle){.offset = handle.offset, .version = (waks_u16)(handle.version + 1)};
#else
    if (header->owner_id != old_owner || header->version != handle.version ||
        header->borrows != 0) {
        return (waks_handle){0, 0};
    }

    header->version++;
    header->owner_id = new_owner;

    return (waks_handle){.offset = handle.offset, .version = header->version};
#endif
}

void waks_handle_release(waks_arena *arena, waks_handle handle)
{
    waks_box_header *header = (waks_box_header *)(arena->memory + handle.offset);
    if (header->magic != BOX_MAGIC || header->version != handle.version) return;
#ifdef CONCURRENT_MODE
    __atomic_fetch_add(&header->version, 1, ATOMIC_SEQ_CST);
    __atomic_store_n(&header->owner_id, 0, ATOMIC_SEQ_CST);
    __atomic_store_n(&header->borrows, 0, ATOMIC_SEQ_CST);
#else
    header->version++;
    header->owner_id = 0;
    header->borrows  = 0;
#endif
}

void waks_handle_release_mut(waks_arena *arena, waks_handle handle)
{
    waks_box_header *header = (waks_box_header *)(arena->memory + handle.offset);
#ifdef CONCURRENT_MODE
    __atomic_store_n(&header->borrows, 0, ATOMIC_SEQ_CST);
#else
    header->borrows = 0;
#endif
}

void waks_handle_defer(waks_arena *arena, waks_handle handle)
{
    if (handle.version == 0) return;

    if (arena->position + (arena->defer_count + 1) * sizeof(waks_handle) >= arena->commited) {
        waks_arena_push(arena, arena->pagesize);
    }

    // we store our defer list at the end of the arena growing backwards.
    arena->defer_count++;
    waks_u64 defer_offset = arena->commited - (arena->defer_count * sizeof(waks_handle));

    waks_handle *defer_ptr = (waks_handle *)(arena->memory + defer_offset);
    *defer_ptr = handle;
}

/* allows for automatic release of handle i.e ScopedHandle (RAII like) */
void _auto_release_handle(waks_handle *handle)
{
    if (!current_arena) {
        waks_dbg_print("KERNEL PANIC: ScopeHandle used without active Arena!\n");
        return;
    }
    if (handle && handle->version > 0)
        waks_handle_release(current_arena, *handle);
}

/* allows for automatic release of borrow i.e ScopedBorrow */
void _raii_release_now(void *pointer)
{
    void **pointer_to_ptr = (void **)pointer;
    if (*pointer_to_ptr && current_arena) {
        waks_box_header *header = ((waks_box_header *)*pointer_to_ptr) - 1;
        waks_handle handle = {.offset = (waks_uchar *)header - current_arena->memory,
                         .version = header->version};
        waks_handle_release(current_arena, handle);
    }
}

void _raii_release_deferred(void *pointer)
{
    void **pointer_to_ptr = (void **)pointer;
    if (*pointer_to_ptr && current_arena) {
        waks_box_header *header = ((waks_box_header *)*pointer_to_ptr) - 1;
        waks_handle handle = {.offset = (waks_uchar *)header - current_arena->memory,
                         .version = header->version};
        waks_handle_defer(current_arena, handle);
    }
}


WaksResult waks_pcall(waks_arena *arena, void (*unsafe_func)(void *), void *arg)
{
    waks_u64 checkpoint = waks_arena_get_pos(arena);
    global_panic_env.arena_checkpoint = checkpoint;

    int status = waks_save_state();
    if (status == 0) {
        /// SUCCESS PATH:
        unsafe_func(arg);
        return WAKS_OK;
    } else {
        /// RECOVERY PATH:
        waks_arena_set_pos_back(arena, global_panic_env.arena_checkpoint);
        return (WaksResult)status;
    }
}

void waks_panic(WaksResult error)
{
    *((volatile int *)&global_panic_env.error_code) = (int)error;
    waks_load_state();
}

void waks_dbg_print(const waks_uchar *str)
{
#if defined(__linux__)
    const char *pointer = str;
    while (*pointer) pointer++;
 
    // @TODO: we can look at using waks_uchar * instead of const char *
    waks_syscall6(SYS_write, 2, (long)str, (long)(pointer - (const char *)str), 0, 0, 0);
#endif
}

void waks_dbg_print_int(waks_i16 n)
{
    waks_uchar buf[16];
    waks_i64 i = 0;
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
    waks_dbg_print(buf);
    waks_dbg_print("\n");
}


#endif // WAKS_ALLOCATOR_IMPLEMENTATION


