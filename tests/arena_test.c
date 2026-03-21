#include "../include/wt_io.h"

void test_with_defer()
{
    Arena *arena = ArenaAlloc();
    u32 user = 100;

    // Allocate
    Handle h1 = BoxAlloc(arena, 64, user);

    // Borrow and immediately DEFER the release
    void *ptr1 = HandleBorrow(arena, h1, user);
    HandleDefer(arena, h1); // Registered for cleanup

    ASSERT(ptr1 != NULL);

    // You can borrow multiple times
    HandleBorrow(arena, h1, user);
    HandleDefer(arena, h1); // Registered again

    // No need to call HandleRelease() manually anymore!
    dbg_print("Doing work without manual release...\n");

    // Clean up EVERYTHING at once
    ArenaReset(arena);

    dbg_print("Arena reset and all borrows cleared.\n");
    ArenaRelease(arena);
}

void test_handle_invalidation()
{
    // 1. Setup Arena
    dbg_print("START\n");
    Arena *arena = ArenaAlloc();
    ASSERT(arena != NULL);
    dbg_print("ALLOCK_OK\n");

    u32 user1 = 100;
    u32 user2 = 200;
    // 2. Alloc and Borrow
    Handle h1 = BoxAlloc(arena, 64, user1);
    ASSERT(h1.version == 1);
    dbg_print("BOX_OK\n");

    void *ptr1 = HandleBorrow(arena, h1, user1);
    ASSERT(ptr1 != NULL);
    dbg_print("BORROW_OK\n");

    // 3. Move ownership (Bumps version)
    Handle h2 = HandleMove(arena, h1, user1, user2);
    ASSERT(h2.version == 2);
    ASSERT(h2.offset == h1.offset);
    dbg_print("MOVE_OK\n");

    // 4. Verify Stale Handle Fails (Old version/Old owner)
    void *ptr_stale = HandleBorrow(arena, h1, user1);
    ASSERT(ptr_stale == NULL);
    dbg_print("STALE_HANDLE_FAILS_ok\n");

    // 5. Verify New Handle Works
    void *ptr2 = HandleBorrow(arena, h2, user2);
    ASSERT(ptr2 != NULL);
    dbg_print("NEW_HANDLE_WORKS_OK\n");

    // YOU MUST RELEASE ptr2 BEFORE trying to borrow Mutably
    HandleRelease(arena, h2);

    // dbg_print("BORROWS_VAL_IS: ");
    // BoxHeader *h = (BoxHeader *)(arena->memory + h2.offset);
    // dbg_print_int(h->borrows);

    // 6. Test Mutex/Borrow conflict
    HandleRelease(arena, h2); // Releases ptr2
    HandleRelease(arena, h2); // Releases ptr1 (or whichever one is still hanging)

    void *ptr_mut = HandleBorrowMut(arena, h2, user2);
    ASSERT(ptr_mut != NULL);
    dbg_print("SINGLE_MUT_BORROW_OK\n");

    // Try to get a shared borrow while a Mut borrow is active (Should fail)
    void *ptr_shared_fail = HandleBorrow(arena, h2, user2);
    ASSERT(ptr_shared_fail == NULL);
    dbg_print("SHARED_MUT_BORROW_FAILS_OK\n");

    ArenaRelease(arena);
    dbg_print("ARENA_RELEASE_OK\n");
    dbg_print("TEST_END\n");
}

// Since we are -nostdlib, we define our entry point
#if defined(__linux__)
void _start()
{
    test_handle_invalidation();

    // If we reached here, all ASSERTs passed.
    // Exit with 0.
    syscall6(SYS_exit, 0, 0, 0, 0, 0, 0);
}
#elif defined(_WIN32) || defined(_WIN64)
void mainCRTStartup()
{
    test_handle_invalidation();
    ExitProcess(0);
}
#endif
