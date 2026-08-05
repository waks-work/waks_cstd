#ifndef WAKS_TEST
#define WAKS_TEST

#include "../waks.h"

static waks_i32 tests_run = 0;
static waks_i32 tests_failed = 0;
static waks_bool _test_current_failed = false;

#define TEST_ASSERT(condition, message)                                        \
    do {                                                                         \
        tests_run++;                                                               \
        if (!(condition)) {                                                        \
            io_print_fmt("[FAIL] %s:%d: { %s }\n", __FILE__, __LINE__, message);     \
            tests_failed++;                                                          \
            _test_current_failed = true;                                             \
        }                                                                          \
    } while (0)

#define ASSERT_EQ_INT(a, b)                                                    \
    do {                                                                         \
        waks_i64 _a = (waks_i64)(a);                                                         \
        waks_i64 _b = (waks_i64)(b);                                                         \
        if (_a != _b) {                                                            \
            io_print_fmt("\n [FAIL] Actual: %d ,Expected: %d\n", _a, _b);            \
            TEST_ASSERT(false, "Integer equality failed");                           \
        } else {                                                                   \
            tests_run++;                                                             \
        }                                                                          \
    } while (0)

#define TEST_REPORT()                                                          \
    do {                                                                         \
        if (tests_failed == 0) {                                                   \
            io_print_fmt("SUCCESS: %d/%d tests passed.\n", tests_run, tests_run);    \
        } else {                                                                   \
            io_print_fmt("FAILURE: %d/%d tests failed.\n", tests_failed, tests_run); \
        }                                                                          \
    } while (0)

/// void test_arena_basics() {
///    Arena *arena = ArenaAlloc((u64)GB(1));
///    TEST("Verifying 16-byte alignment of BoxAlloc") {
///        Handle h1 = BoxAlloc(arena, 1, 100);
///        Handle h2 = BoxAlloc(arena, 1, 100);
///        ASSERT_EQ_INT((h2.offset - h1.offset) % 16, 0);
///    }
///    TEST("Handling Out-of-Memory gracefully") {
///        Handle big = BoxAlloc(arena, arena->commited + 1, 100);
///        TEST_ASSERT(big.version == 0, "Arena should return null handle on
///        OOM");
///    }
///    ArenaRelease(arena);
///}
#define TEST(description) \
    for (int _i = (io_print_fmt("\n [TEST] %s...", description), \
         _test_current_failed = false, 0); _i < 1; _i++, \
         io_print(_test_current_failed ? STR("\n") : STR(" OK \n")))

#endif
