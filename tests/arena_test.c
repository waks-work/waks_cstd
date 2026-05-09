#include "../include/wt_io.h"
#include "test.h"

void test_vector()
{
    Arena *arena = ArenaAlloc((u64)(GB((1))));

    TEST ("Verifying that the vector initialisation, push and copy") {
        WITH_ARENA (arena) {
            Vector vector = vector_init(arena, 2, sizeof(Any), 0);
            vector_push(arena, &vector, AnyInt(100));
            vector_push(arena, &vector, AnyInt(300));
            vector_insert(arena, &vector, 1, AnyInt(200));

            for (usize i = 0; i < vector.length; i++) {
                Any item = vector_get_copy(arena, &vector, i);
                match(item)
                {
                    MatchInt(item, val)
                    {
                        dbg_print_int((i64)val);
                    }
                    with default : break;
                }
            }
        }
    }

    ArenaRelease(arena);
}

void run_arena_suite()
{
    Arena *arena = ArenaAlloc((u64)GB(1));
    TEST_ASSERT(arena != NULL, "Allocation was not successful");

    TEST ("Verifying 16-byte alignment of BoxAlloc") {
        Handle h1 = BoxAlloc(arena, 1, 100);
        Handle h2 = BoxAlloc(arena, 1, 100);
        // Ensure the distance between boxes respects your ALIGN_16 macro
        ASSERT_EQ_INT(((h2.offset - h1.offset) % 16), 0);
    }

    TEST ("Example of the use of the arena") {
        Handle handle = BoxAlloc(arena, sizeof(int) * 10000, 1);
        io_print(STR("Allocated 10000 items at: "));
        io_print_hex(handle.offset);
        io_print(STR("\n"));
    }

    TEST ("Handling Out-of-Memory gracefully") {
        Handle big = BoxAlloc(arena, arena->commited + 1, 100);
        TEST_ASSERT(big.version == 1, "Arena should return null handle on OOM");
    }

    TEST ("Stale handles must fail to borrow") {
        u32 user = 123;
        Handle h1 = BoxAlloc(arena, 64, user);
        TEST_ASSERT(h1.version == 1, "Expected version one for new allocation!");

        HandleRelease(arena, h1);
        void *ptr = HandleBorrow(arena, h1, user);
        TEST_ASSERT(ptr == NULL, "Borrowing a released handle succeeded (Security Risk)");
    }

    TEST ("Out of memory returns invalid handle") {
        // Attempt to allocate more than the arena capacity
        Handle huge = BoxAlloc(arena, arena->commited + 1024, 0);
        ASSERT_EQ_INT(huge.version, 1);
    }

    TEST ("Testing box allocation and borrow (CORRECTED)") {
        u32 user1 = 100;
        u32 user2 = 200;
        Handle h1 = BoxAlloc(arena, 64, user1);
        TEST_ASSERT(h1.version == 1, "Initial version should be 1");

        // Move ownership from user1 to user2
        Handle h2 = HandleMove(arena, h1, user1, user2);
        TEST_ASSERT(h2.version == 2, "Version should increment on move");

        // TEST: Old handle (h1) must now fail
        void *ptr_stale = HandleBorrow(arena, h1, user1);
        TEST_ASSERT(ptr_stale == NULL, "Old handle MUST fail after move");

        // TEST: New handle (h2) must succeed
        void *ptr2 = HandleBorrow(arena, h2, user2);
        TEST_ASSERT(ptr2 != NULL, "New handle MUST provide a valid pointer");

        // TEST: Mutable conflict
        // We already have 'ptr2' (shared borrow). A Mutable borrow must fail now.
        void *ptr_mut_fail = HandleBorrowMut(arena, h2, user2);
        TEST_ASSERT(ptr_mut_fail == NULL, "Mutable borrow must fail while shared borrow exists");

        // Cleanup
        HandleRelease(arena, h2);
    }

    ArenaRelease(arena);
}

void test_handle_safety()
{
    Arena *arena = ArenaAlloc((u64)GB(1));
    u32 user = 777;

    TEST ("Version increment after Release 2") {
        u32 user = 123;
        Handle h1 = BoxAlloc(arena, 64, user);

        HandleRelease(arena, h1);
        io_print_fmt("\n --> handle version: %d \n", h1.version);

        void *ptr = HandleBorrow(arena, h1, user);
        io_print_fmt("\n --> handle version: %d \n", h1.version);
        TEST_ASSERT(ptr == NULL, "Borrowing with old version should fail");
    }

    TEST ("Testing handle defer safety") {
        Handle h1 = BoxAlloc(arena, 64, user);

        void *ptr1 = HandleBorrow(arena, h1, user);
        TEST_ASSERT(ptr1 != NULL, "Initial borrow should work");
        HandleDefer(arena, h1); // Queue for cleanup

        void *ptr2 = HandleBorrow(arena, h1, user);
        TEST_ASSERT(ptr2 != NULL, "Should still be borrowable before reset");
        ArenaReset(arena);

        void *ptr3 = HandleBorrow(arena, h1, user);
        io_print_fmt("\n --> handle version: %d \n", h1.version);
        TEST_ASSERT(ptr3 == NULL, "Handle must be invalid after ArenaReset processes defers");
    }

    ArenaRelease(arena);
}

// Since we are -nostdlib, we define our entry point
#if defined(__linux__)
void _start()
{
    test_vector();
    run_arena_suite();
    test_handle_safety();
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
