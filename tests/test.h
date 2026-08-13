#ifndef WAKS_TEST_H
#define WAKS_TEST_H

#include "../waks.h"

static waks_i32  tests_run           = 0;
static waks_i32  tests_failed        = 0;
static waks_bool _test_current_failed = false;

#define TEST_ASSERT(condition, message)                                         \
    do {                                                                        \
        tests_run++;                                                            \
        if (!(condition)) {                                                     \
            waks_io_print_fmt(WAKS_2CSTR_CAST("[FAIL] %s:%d: { %s }\n"),                         \
                              __FILE__, __LINE__, message);                     \
            tests_failed++;                                                     \
            _test_current_failed = true;                                        \
        }                                                                       \
    } while (0)

#define ASSERT_EQ_INT(a, b)                                                     \
    do {                                                                        \
        waks_i64 _a = (waks_i64)(a);                                            \
        waks_i64 _b = (waks_i64)(b);                                            \
        if (_a != _b) {                                                         \
            waks_io_print_fmt(WAKS_2CSTR_CAST("\n [FAIL] Actual: %d ,Expected: %d\n"), _a, _b);  \
            TEST_ASSERT(false, "Integer equality failed");                      \
        } else {                                                                \
            tests_run++;                                                        \
        }                                                                       \
    } while (0)

#define TEST_REPORT()                                                           \
    do {                                                                        \
        waks_io_print(WAKS_2STR("\n========================================\n")); \
        if (tests_failed == 0) {                                                \
            waks_io_print_fmt(WAKS_2CSTR_CAST("SUCCESS: %d/%d assertions passed.\n"),            \
                              tests_run, tests_run);                            \
        } else {                                                                \
            waks_io_print_fmt(WAKS_2CSTR_CAST("FAILURE: %d failed out of %d assertions.\n"),    \
                              tests_failed, tests_run);                         \
        }                                                                       \
        waks_io_print(WAKS_2STR("========================================\n"));   \
    } while (0)

#define TEST(description)                                                       \
    for (int _i = (waks_io_print_fmt(WAKS_2CSTR_CAST("\n [TEST] %s..."), (waks_char *)description),           \
                   _test_current_failed = false, 0);                             \
         _i < 1;                                                                \
         _i++, waks_io_print(_test_current_failed ? WAKS_2STR("\n") : WAKS_2STR(" OK \n")))

#endif // WAKS_TEST_H
