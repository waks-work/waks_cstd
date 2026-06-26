#include "../include/waks.h"
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

void linked_list_test()
{
    Arena *arena = ArenaAlloc(GB(1));
    typedef struct
    {
        u32 id;
        char *content;
        ListNode node;
    } EditorLine;

    TEST ("List initialisation ,push_back and remove") {
        ListNode *line_list = NULL;

        EditorLine *line_1 = ArenaPush(arena, sizeof(EditorLine));
        line_1->id = 1;
        list_init(&line_1->node);

        EditorLine *line_2 = ArenaPush(arena, sizeof(EditorLine));
        line_2->id = 2;
        list_init(&line_2->node);

        TEST_ASSERT(list_is_empty(line_list), "New list handle should be NULL (empty)");

        list_push_back(&line_list, &line_1->node);
        TEST_ASSERT(!list_is_empty(line_list), "List should be populated after push_back");
        TEST_ASSERT(line_list == &line_1->node, "List head should point to the first pushed node");
        TEST_ASSERT(line_1->node.next == NULL, "Single-node list tail 'next' must be NULL");

        list_push_back(&line_list, &line_2->node);
        TEST_ASSERT(line_1->node.next == &line_2->node, "Line 1 'next' should link to Line 2");
        TEST_ASSERT(line_2->node.prev == &line_1->node, "Line 2 'prev' should link back to Line 1");
        TEST_ASSERT(line_2->node.next == NULL, "New tail end of the list must point to NULL");

        list_remove(&line_list, &line_1->node);
        TEST_ASSERT(line_list == &line_2->node,
                    "Head should have advanced to Line 2 after removing Line 1");
        TEST_ASSERT(line_2->node.prev == NULL, "New head 'prev' must be NULL after removal");

        list_remove(&line_list, &line_2->node);
        TEST_ASSERT(list_is_empty(line_list), "List should be empty after removing all nodes");
    }

    ArenaRelease(arena);
}

/*
 * PART OF THE ERROR_TEST SUITE
 * */
void run_parser(void *arg)
{
    Arena *temp = (Arena *)arg;
    b8 file_exists = false;

    if (!file_exists)
        waks_panic(WAKS_ERR_IO);
}

WaksResult parse_config_transaction(Arena *arena, const char *path)
{
    return waks_pcall(arena, run_parser, (void *)arena);
}

void run_error_handling_suite()
{
    Arena *arena = ArenaAlloc(GB(1));

    TEST ("Waks error and panic test") {
        WaksResult res = parse_config_transaction(arena, "config.toml");
        TEST_ASSERT(res == WAKS_ERR_IO, "pcall should catch the panic and return WAKS_ERR_IO");

        if (res != WAKS_OK)
            io_print_fmt("Transaction failed safely: %s", waks_strerror(res));
    }

    TEST ("Option test method functionality") {
        Handle handle = BoxAlloc(arena, 64, 100);
        Option opt_some = option_some(handle);
        Option opt_none = option_none();

        TEST_ASSERT(opt_some.has_value, "Option(Some) should have has_value set to true");
        TEST_ASSERT(!opt_none.has_value, "Option(None) should have has_value set to false");

        Handle fallback = BoxAlloc(arena, 64, 101);
        Handle result = option_unwrap_or(&opt_none, fallback);

        TEST_ASSERT(result.offset == fallback.offset,
                    "Unwrap_or must return fallback handle's offset");
        TEST_ASSERT(result.version == fallback.version,
                    "Unwrap_or must return fallback handle's version");

        Handle retrieved = option_unwrap(&opt_some);
        TEST_ASSERT(retrieved.offset == handle.offset,
                    "Unwrapped offset should match BoxAlloc result");
        TEST_ASSERT(retrieved.version == handle.version,
                    "Unwrapped version should match BoxAlloc result");
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
        Handle h1 = BoxAlloc(arena, 64, user);

        HandleRelease(arena, h1);
        io_print_fmt("\n -> handle version: %d \n", h1.version);

        void *ptr = HandleBorrow(arena, h1, user);
        io_print_fmt("\n -> handle version: %d \n", h1.version);
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
    linked_list_test();
    run_error_handling_suite();
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
