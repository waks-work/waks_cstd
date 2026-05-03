#include "../include/wt_io.h"
#include "test.h"

void test_vector() {
  Arena *arena = ArenaAlloc();
  WITH_ARENA(arena) {
    Vector vector = vector_init(arena, 2, sizeof(Any), 0);
    vector_push(arena, &vector, AnyInt(100));
    vector_push(arena, &vector, AnyInt(300));
    vector_insert(arena, &vector, 1, AnyInt(200));

    for (usize i = 0; i < vector.length; i++) {
      Any item = vector_get_copy(arena, &vector, i);
      match(item) {
        MatchInt(item, val) { dbg_print_int((i64)val); }
        with default : break;
      }
    }
  }
  ArenaRelease(arena);
}

void test_arena_basics() {
  Arena *arena = ArenaAlloc();

  TEST("Verifying 16-byte alignment of BoxAlloc") {
    Handle h1 = BoxAlloc(arena, 1, 100);
    Handle h2 = BoxAlloc(arena, 1, 100);
    // Ensure the distance between boxes respects your ALIGN_16 macro
    ASSERT_EQ_INT(((h2.offset - h1.offset) % 16), 0);
  }

  TEST("Handling Out-of-Memory gracefully") {
    // Try to allocate a box larger than the remaining committed space
    Handle big = BoxAlloc(arena, arena->commited + 1, 100);
    TEST_ASSERT(big.version == 0, "Arena should return null handle on OOM");
  }

  ArenaRelease(arena);
}

void run_arena_suite() {
  Arena *arena = ArenaAlloc();

  TEST("Stale handles must fail to borrow") {
    u32 user = 123;
    Handle h1 = BoxAlloc(arena, 64, user);

    HandleRelease(arena, h1);
    void *ptr = HandleBorrow(arena, h1, user);

    TEST_ASSERT(ptr == NULL,
                "Borrowing a released handle succeeded (Security Risk)");
  }

  TEST("Out of memory returns invalid handle") {
    // Attempt to allocate more than the arena capacity
    Handle huge = BoxAlloc(arena, arena->commited + 1024, 0);
    ASSERT_EQ_INT(huge.version, 0);
  }

  ArenaRelease(arena);
}

void test_handle_safety() {
  Arena *arena = ArenaAlloc();
  u32 user = 777;

  TEST("Version increment after Release") {
    Handle h1 = BoxAlloc(arena, 64, user);
    u16 v1 = h1.version;
    HandleRelease(arena, h1);
    // Version should remain the same in the handle, but be invalid in the
    // header
    void *ptr = HandleBorrow(arena, h1, user);
    TEST_ASSERT(ptr == NULL, "Stale handle should not be borrowable");
  }

  ArenaRelease(arena);
}

void test_with_defer() {
  Arena *arena = ArenaAlloc();
  u32 user = 100;

  // Allocate
  Handle h1 = BoxAlloc(arena, 64, user);

  // Borrow and immediately DEFER the release

  {
    void *ptr1 = HandleBorrow(arena, h1, user);
    HandleDefer(arena, h1); // Registered for cleanup
    ASSERT(ptr1 != NULL);
  }

  // You can borrow multiple times
  {
    HandleBorrow(arena, h1, user);
    HandleDefer(arena, h1); // Registered again
  }

  // No need to call HandleRelease() manually anymore!
  dbg_print("Doing work without manual release...\n");

  // Clean up EVERYTHING at once
  ArenaReset(arena);

  dbg_print("Arena reset and all borrows cleared.\n");
  ArenaRelease(arena);
}

void test_handle_invalidation() {
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
void _start() {
  /// test_handle_invalidation();
  test_vector();
  // If we reached here, all ASSERTs passed.
  // Exit with 0.
  syscall6(SYS_exit, 0, 0, 0, 0, 0, 0);
}
#elif defined(_WIN32) || defined(_WIN64)
void mainCRTStartup() {
  test_handle_invalidation();
  ExitProcess(0);
}
#endif
