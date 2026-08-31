#include "test.h"

#define WAKS_TYPE_IMPLEMENTATION
#include "../types.h"

#define WAKS_ALLOCATOR_IMPLEMENTATION
#include "../allocator.h"

#define WAKS_CONTAINER_IMPLEMENTATION
#include "../container.h"

#define WAKS_IO_IMPLEMENTATION
#include "../io.h"

void test_vector() {
    waks_arena *arena = waks_arena_alloc((waks_u64)(WAKS_GB((1))));

    TEST("Verifying that the vector initialisation, push and copy") {
        WAKS_TEMP_ARENA(arena) {
            waks_array_list vector = waks_array_list_init(arena, 2, sizeof(Any), 0);
            waks_array_list_push(arena, &vector, AnyInt(100));
            waks_array_list_push(arena, &vector, AnyInt(300));
            waks_array_list_insert(arena, &vector, 1, AnyInt(200));

            for (waks_usize i = 0; i < vector.length; i++) {
                Any item = waks_array_list_get_copy(arena, &vector, i);
                match(item) {
                    MatchInt(item, val) {
                        waks_dbg_print_int((waks_i64)val); 
                    } with;
                    default: break;
                }
            }
        }
    }

    waks_arena_release(arena);
}

void linked_list_test() {
    waks_arena *arena = waks_arena_alloc(WAKS_GB(1));
    typedef struct {
        waks_u32       id;
        waks_char     *content;
        waks_list_node node;
    } EditorLine;

    TEST("List initialisation ,push_back and remove") {
        waks_list_node *line_list = WAKS_NOVALUE;

        EditorLine *line_1 = waks_arena_push(arena, sizeof(EditorLine));
        line_1->id = 1;
        waks_list_init(&line_1->node);

        EditorLine *line_2 = waks_arena_push(arena, sizeof(EditorLine));
        line_2->id = 2;
        waks_list_init(&line_2->node);

        TEST_ASSERT(waks_list_is_empty(line_list),
                    "New list handle should be WAKS_NOVALUE (empty)");

        waks_list_push_back(&line_list, &line_1->node);
        TEST_ASSERT(!waks_list_is_empty(line_list),
                    "List should be populated after push_back");
        TEST_ASSERT(line_list == &line_1->node,
                    "List head should point to the first pushed node");
        TEST_ASSERT(line_1->node.next == WAKS_NOVALUE,
                    "Single-node list tail 'next' must be WAKS_NOVALUE");

        waks_list_push_back(&line_list, &line_2->node);
        TEST_ASSERT(line_1->node.next == &line_2->node,
                    "Line 1 'next' should link to Line 2");
        TEST_ASSERT(line_2->node.prev == &line_1->node,
                    "Line 2 'prev' should link back to Line 1");
        TEST_ASSERT(line_2->node.next == WAKS_NOVALUE,
                    "New tail end of the list must point to WAKS_NOVALUE");

        waks_list_remove(&line_list, &line_1->node);
        TEST_ASSERT(line_list == &line_2->node,
                    "Head should have advanced to Line 2 after removing Line 1");
        TEST_ASSERT(line_2->node.prev == WAKS_NOVALUE,
                    "New head 'prev' must be WAKS_NOVALUE after removal");

        waks_list_remove(&line_list, &line_2->node);
        TEST_ASSERT(waks_list_is_empty(line_list),
                    "List should be empty after removing all nodes");
    }

    waks_arena_release(arena);
}


// PART OF THE ERROR_TEST SUITE
// void run_parser(void *arg) {
//     // waks_arena *temp = 
// 	(waks_arena *)arg;
//     waks_bool file_exists = false;
// 
//     if (!file_exists) waks_panic(WAKS_ERR_IO);
// }
// 
// WaksResult parse_config_transaction(waks_arena *arena, const char *path) {
//     return waks_pcall(arena, run_parser, (void *)arena);
// }

void run_error_handling_suite() {
    waks_arena *arena = waks_arena_alloc(WAKS_GB(1));

    // TEST("Waks error and panic test") {
    //     WaksResult res = parse_config_transaction(arena, "config.toml");
    //     TEST_ASSERT(res == WAKS_ERR_IO,
    //                 "pcall should catch the panic and return WAKS_ERR_IO");

    //     if (res != WAKS_OK)
    //         waks_io_print_fmt(WAKS_2CSTR_CAST("Transaction failed safely: %s"), waks_strerror(res));
    // }

    TEST("waks_option test method functionality") {
        waks_handle handle   = waks_box_alloc(arena, 64, 100);
        waks_option opt_some = waks_option_some(handle);
        waks_option opt_none = waks_option_none();

        TEST_ASSERT(opt_some.has_value,
                    "waks_option(Some) should have has_value set to true");
        TEST_ASSERT(!opt_none.has_value,
                    "waks_option(None) should have has_value set to false");

        waks_handle fallback = waks_box_alloc(arena, 64, 101);
        waks_handle result   = waks_option_unwrap_or(&opt_none, fallback);

        TEST_ASSERT(result.offset == fallback.offset,
                    "Unwrap_or must return fallback handle's offset");
        TEST_ASSERT(result.version == fallback.version,
                    "Unwrap_or must return fallback handle's version");

        waks_handle retrieved = waks_option_unwrap(&opt_some);
        TEST_ASSERT(retrieved.offset == handle.offset,
                    "Unwrapped offset should match waks_box_alloc result");
        TEST_ASSERT(retrieved.version == handle.version,
                    "Unwrapped version should match waks_box_alloc result");
    }

    waks_arena_release(arena);
}

void run_arena_suite() {
    waks_arena *arena = waks_arena_alloc((waks_u64)WAKS_GB(1));
    TEST_ASSERT(arena != WAKS_NOVALUE, "Allocation was not successful");

    TEST("Verifying 16-byte alignment of waks_box_alloc") {
        waks_handle h1 = waks_box_alloc(arena, 1, 100);
        waks_handle h2 = waks_box_alloc(arena, 1, 100);
        // Ensure the distance between boxes respects your ALIGN_16 macro
        ASSERT_EQ_INT(((h2.offset - h1.offset) % 16), 0);
    }

    TEST("Example of the use of the arena") {
        waks_handle handle = waks_box_alloc(arena, sizeof(int) * 10000, 1);
        waks_io_print(WAKS_2STR("Allocated 10000 items at: "));
        waks_io_print_hex(handle.offset);
        waks_io_print(WAKS_2STR("\n"));
    }

    TEST("Handling Out-of-Memory gracefully") {
        waks_handle big = waks_box_alloc(arena, arena->commited + 1, 100);
        TEST_ASSERT(big.version == 1, "waks_arena should return null handle on OOM");
    }

    TEST("Stale handles must fail to borrow") {
        waks_u32 user = 123;
        waks_handle h1 = waks_box_alloc(arena, 64, user);
        TEST_ASSERT(h1.version == 1, "Expected version one for new allocation!");

        waks_handle_release(arena, h1);
        void *ptr = waks_handle_borrow(arena, h1, user);
        TEST_ASSERT(ptr == WAKS_NOVALUE,
                    "Borrowing a released handle succeeded (Security Risk)");
    }

    TEST("Out of memory returns invalid handle") {
        // Attempt to allocate more than the arena capacity
        waks_handle huge = waks_box_alloc(arena, arena->commited + 1024, 0);
        ASSERT_EQ_INT(huge.version, 1);
    }  

    TEST("Testing box allocation and borrow (CORRECTED)") {
        waks_u32 user1 = 100;
        waks_u32 user2 = 200;
        waks_handle h1 = waks_box_alloc(arena, 64, user1);
        TEST_ASSERT(h1.version == 1, "Initial version should be 1");

        // Move ownership from user1 to user2
        waks_handle h2 = waks_handle_move(arena, h1, user1, user2);
        TEST_ASSERT(h2.version == 2, "Version should increment on move");

        // TEST: Old handle (h1) must now fail
        void *ptr_stale = waks_handle_borrow(arena, h1, user1);
        TEST_ASSERT(ptr_stale == WAKS_NOVALUE, "Old handle MUST fail after move");

        // TEST: New handle (h2) must succeed
        void *ptr2 = waks_handle_borrow(arena, h2, user2);
        TEST_ASSERT(ptr2 != WAKS_NOVALUE, "New handle MUST provide a valid pointer");

        // TEST: Mutable conflict
        // We already have 'ptr2' (shared borrow). A Mutable borrow must fail now.
        void *ptr_mut_fail = waks_handle_borrow_mut(arena, h2, user2);
        TEST_ASSERT(ptr_mut_fail == WAKS_NOVALUE,
                    "Mutable borrow must fail while shared borrow exists");

        // Cleanup
        waks_handle_release(arena, h2);
    }

    waks_arena_release(arena);
}

void test_handle_safety() {
    waks_arena *arena = waks_arena_alloc((waks_u64)WAKS_GB(1));
    waks_u32 user = 777;

    TEST("Version increment after Release 2") {
        waks_handle h1 = waks_box_alloc(arena, 64, user);

        waks_handle_release(arena, h1);
        waks_io_print_fmt(WAKS_2CSTR_CAST("\n -> handle version: %d \n"), h1.version);

        void *ptr = waks_handle_borrow(arena, h1, user);
        waks_io_print_fmt(WAKS_2CSTR_CAST("\n -> handle version: %d \n"), h1.version);
        TEST_ASSERT(ptr == WAKS_NOVALUE, "Borrowing with old version should fail");
    }

    TEST("Testing handle defer safety") {
        waks_handle h1 = waks_box_alloc(arena, 64, user);

        void *ptr1 = waks_handle_borrow(arena, h1, user);
        TEST_ASSERT(ptr1 != WAKS_NOVALUE, "Initial borrow should work");
        waks_handle_defer(arena, h1); // Queue for cleanup

        void *ptr2 = waks_handle_borrow(arena, h1, user);
        TEST_ASSERT(ptr2 != WAKS_NOVALUE, "Should still be borrowable before reset");
        waks_arena_reset(arena);

        void *ptr3 = waks_handle_borrow(arena, h1, user);
        waks_io_print_fmt(WAKS_2CSTR_CAST("\n --> handle version: %d \n"), h1.version);
        TEST_ASSERT(ptr3 == WAKS_NOVALUE,
                    "waks_handle must be invalid after waks_arenaReset processes defers");
    }

    waks_arena_release(arena);
}

void hash_map_test(void) 
{
	waks_arena *arena = waks_arena_alloc(WAKS_MB(1));
	if (!arena) return;

	TEST("@TODO: provide a name for this test") {
        waks_hash_map map;

	    // initialise the hashmap.
        waks_hm_init_arena(arena, &map, 16);

	    //   we can implement cast macros
        waks_hm_insert(&map, WAKS_2CSTR_CAST("apple"),  WAKS_TO_PTR(3));
        waks_hm_insert(&map, WAKS_2CSTR_CAST("banana"), WAKS_TO_PTR(5));
        waks_hm_insert(&map, WAKS_2CSTR_CAST("orange"), WAKS_TO_PTR(7));
        waks_hm_insert(&map, WAKS_2CSTR_CAST("banana"), WAKS_TO_PTR(10));

        if (waks_hm_contains(&map, WAKS_2CSTR_CAST("banana"))){
            waks_u32 value = WAKS_HM_GET_INT(waks_u32, &map, WAKS_2CSTR_CAST("banana"));
	    	waks_io_print_fmt(WAKS_2CSTR_CAST("banana = %d\n"), value);
	    }

        waks_hm_remove(&map, WAKS_2CSTR_CAST("apple"));

        if (!waks_hm_contains(&map, WAKS_2CSTR_CAST("apple"))){
	    	waks_io_print_fmt(WAKS_2CSTR_CAST("apple not found\n"));
	    }

        waks_hm_free(&map);
	}
	waks_arena_release(arena);
}

// Since we are -nostdlib, we define our entry point
#if defined(__linux__)
void _start() {
    test_vector();
    run_arena_suite();
    test_handle_safety();
    linked_list_test();
    run_error_handling_suite();
	hash_map_test();

    TEST_REPORT();

    // Exit with code 0 on success, 1 if any tests failed
    waks_syscall6(SYS_exit, (long)(tests_failed > 0 ? 1 : 0), 0, 0, 0, 0, 0);
}
#elif defined(_WIN32) || defined(_WIN64)
void mainCRTStartup() {
    test_vector();
    run_arena_suite();
    test_handle_safety();
    linked_list_test();
    run_error_handling_suite();

    TEST_REPORT();

    ExitProcess(tests_failed > 0 ? 1 : 0);
}
#endif
