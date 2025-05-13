#include "print.h"
#include "string_builder.h"
#include "string_cursor.h"

#define TEST_ERROR_LIST(X)                                                                                             \
  X(STRING_CURSOR_TEST_ERROR_STARTS_WITH_EXPECTED_TRUE,                                                                \
    "IsStartsWith: Expected string to start with the given prefix")                                                    \
  X(STRING_CURSOR_TEST_ERROR_STARTS_WITH_EXPECTED_FALSE,                                                               \
    "IsStartsWith: Expected string NOT to start with the given prefix")                                                \
  X(STRING_CURSOR_TEST_ERROR_ADVANCE_AFTER_EXPECTED,                                                                   \
    "AdvanceAfter: Cursor must advance after first occorence of search text or go to end")                             \
  X(STRING_CURSOR_TEST_ERROR_ADVANCE_AFTER_LAST_EXPECTED,                                                              \
    "AdvanceAfterLast: Cursor must advance after last occorence of search text or go to end")                          \
  X(STRING_CURSOR_TEST_ERROR_IS_REMAINING_EQUAL_EXPECTED_TRUE,                                                         \
    "IsRemainingEqual: Remaining text must match the search string")                                                   \
  X(STRING_CURSOR_TEST_ERROR_IS_REMAINING_EQUAL_EXPECTED_FALSE,                                                        \
    "IsRemainingEqual: Remaining text must NOT match the search string")                                               \
  X(STRING_CURSOR_TEST_ERROR_CONSUME_UNTIL_EXPECTED, "ConsumeUntil: Text consumed must match the expected")            \
  X(STRING_CURSOR_TEST_ERROR_CONSUME_UNTIL_LAST_EXPECTED, "ConsumeUntilLast: Text consumed must match the expected")   \
  X(STRING_CURSOR_TEST_ERROR_EXTRACT_THROUGH_EXPECTED,                                                                 \
    "ExtractThrough: Text extracted through search must match the expected")                                           \
  X(STRING_CURSOR_TEST_ERROR_EXTRACT_NUMBER_EXPECTED_TRUE,                                                             \
    "ExtractNumber: Number must be extracted from cursor position")                                                    \
  X(STRING_CURSOR_TEST_ERROR_EXTRACT_NUMBER_EXPECTED_FALSE,                                                            \
    "ExtractNumber: Number must NOT be extracted from cursor position")                                                \
  X(STRING_CURSOR_TEST_ERROR_EXTRACT_CONSUMED, "ExtractConsumed: Consumed string not matching expected")

enum string_cursor_test_error {
  STRING_CURSOR_TEST_ERROR_NONE = 0,
#define X(tag, message) tag,
  TEST_ERROR_LIST(X)
#undef X

  // src: https://mesonbuild.com/Unit-tests.html#skipped-tests-and-hard-errors
  // For the default exitcode testing protocol, the GNU standard approach in
  // this case is to exit the program with error code 77. Meson will detect this
  // and report these tests as skipped rather than failed. This behavior was
  // added in version 0.37.0.
  MESON_TEST_SKIP = 77,
  // In addition, sometimes a test fails set up so that it should fail even if
  // it is marked as an expected failure. The GNU standard approach in this case
  // is to exit the program with error code 99. Again, Meson will detect this
  // and report these tests as ERROR, ignoring the setting of should_fail. This
  // behavior was added in version 0.50.0.
  MESON_TEST_FAILED_TO_SET_UP = 99,
};

internalfn void
StringBuilderAppendErrorMessage(struct string_builder *stringBuilder, enum string_cursor_test_error errorCode)
{
  struct error {
    enum string_cursor_test_error code;
    struct string message;
  } errors[] = {
#define XX(tag, msg) {.code = tag, .message = StringFromLiteral(msg)},
      TEST_ERROR_LIST(XX)
#undef XX
  };

  struct string message = StringFromLiteral("Unknown error");
  for (u32 index = 0; index < ARRAY_COUNT(errors); index++) {
    struct error *error = errors + index;
    if (error->code == errorCode) {
      message = error->message;
      break;
    }
  }

  StringBuilderAppendString(stringBuilder, &message);
}

internalfn void
StringBuilderAppendBool(string_builder *sb, b8 value)
{
  struct string *boolString = value ? &StringFromLiteral("true") : &StringFromLiteral("false");
  StringBuilderAppendString(sb, boolString);
}

internalfn void
StringBuilderAppendPrintableString(string_builder *sb, struct string *string)
{
  if (string->value == 0)
    StringBuilderAppendStringLiteral(sb, "(NULL)");
  else if (string->value != 0 && string->length == 0)
    StringBuilderAppendStringLiteral(sb, "(EMPTY)");
  else if (string->length == 1 && string->value[0] == ' ')
    StringBuilderAppendStringLiteral(sb, "(SPACE)");
  else
    StringBuilderAppendString(sb, string);
}

int
main(void)
{
  enum string_cursor_test_error errorCode = STRING_CURSOR_TEST_ERROR_NONE;

  // setup
  enum { KILOBYTES = (1 << 10) };
  u8 stackBuffer[8 * KILOBYTES];
  memory_arena stackMemory = {
      .block = stackBuffer,
      .total = ARRAY_COUNT(stackBuffer),
  };

  string_builder *sb = MakeStringBuilder(&stackMemory, 1024, 32);

  // b8 IsStringCursorStartsWith(struct string_cursor *cursor, struct string *search)
  {
    struct test_case {
      struct string_cursor cursor;
      struct string *search;
      b8 expected;
    } testCases[] = {
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Lorem Ipsum"),
                    .position = 0,
                },
            .search = &StringFromLiteral("Lorem Ipsum"),
            .expected = 1,
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral(" Lorem Ipsum"),
                    .position = 1,
                },
            .search = &StringFromLiteral("Lorem Ipsum"),
            .expected = 1,
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral(" Lorem Ipsum "),
                    .position = 1,
                },
            .search = &StringFromLiteral("Lorem Ipsum"),
            .expected = 1,
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Lorem Ipsum"),
                    .position = 1,
                },
            .search = &StringFromLiteral("Lorem Ipsum"),
            .expected = 0,
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Lorem Ipsum"),
                    .position = 0,
                },
            .search = &StringFromLiteral("abc"),
            .expected = 0,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      b8 expected = testCase->expected;
      struct string_cursor *cursor = &testCase->cursor;
      struct string *search = testCase->search;

      b8 got = IsStringCursorStartsWith(cursor, search);
      if (got != expected) {
        errorCode = expected ? STRING_CURSOR_TEST_ERROR_STARTS_WITH_EXPECTED_TRUE
                             : STRING_CURSOR_TEST_ERROR_STARTS_WITH_EXPECTED_FALSE;

        StringBuilderAppendErrorMessage(sb, errorCode);

        StringBuilderAppendStringLiteral(sb, "\n  cursor: '");
        StringBuilderAppendString(sb, cursor->source);
        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n           ");
        for (u64 pos = 0; pos < cursor->position; pos++)
          StringBuilderAppendStringLiteral(sb, " ");
        StringBuilderAppendStringLiteral(sb, "↓");

        StringBuilderAppendStringLiteral(sb, "\n  search: '");
        StringBuilderAppendString(sb, search);
        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendBool(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendBool(sb, got);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // b8 IsStringCursorRemainingEqual(struct string_cursor *cursor, struct string *search)
  {
    struct test_case {
      struct string_cursor cursor;
      struct string *search;
      b8 expected;
    } testCases[] = {
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Lorem Ipsum"),
                    .position = 0,
                },
            .search = &StringFromLiteral("Lorem Ipsum"),
            .expected = 1,
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral(" Lorem Ipsum"),
                    .position = 1,
                },
            .search = &StringFromLiteral("Lorem Ipsum"),
            .expected = 1,
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral(" Lorem Ipsum "),
                    .position = 1,
                },
            .search = &StringFromLiteral("Lorem Ipsum"),
            .expected = 0,
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Lorem Ipsum"),
                    .position = 1,
                },
            .search = &StringFromLiteral("Lorem Ipsum"),
            .expected = 0,
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Lorem Ipsum"),
                    .position = 0,
                },
            .search = &StringFromLiteral("abc"),
            .expected = 0,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      b8 expected = testCase->expected;
      struct string_cursor *cursor = &testCase->cursor;
      struct string *search = testCase->search;

      b8 got = IsStringCursorRemainingEqual(cursor, search);
      if (got != expected) {
        errorCode = expected ? STRING_CURSOR_TEST_ERROR_IS_REMAINING_EQUAL_EXPECTED_TRUE
                             : STRING_CURSOR_TEST_ERROR_IS_REMAINING_EQUAL_EXPECTED_FALSE;

        StringBuilderAppendErrorMessage(sb, errorCode);

        StringBuilderAppendStringLiteral(sb, "\n  cursor: '");
        StringBuilderAppendString(sb, cursor->source);
        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n           ");
        for (u64 pos = 0; pos < cursor->position; pos++)
          StringBuilderAppendStringLiteral(sb, " ");
        StringBuilderAppendStringLiteral(sb, "↓");

        StringBuilderAppendStringLiteral(sb, "\n  search: '");
        StringBuilderAppendString(sb, search);
        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendBool(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendBool(sb, got);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // b8 StringCursorAdvanceAfter(struct string_cursor *cursor, struct string *search)
  {
    struct test_case {
      struct string_cursor cursor;
      struct string *search;
      struct {
        b8 value;
        struct string *remaining;
        u64 position;
      } expected;
    } testCases[] = {
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Lorem ipsum dolor sit amet, consectetur adipiscing elit."),
                    .position = 0,
                },
            .search = &StringFromLiteral("Lorem"),
            .expected =
                {
                    .value = 1,
                    .remaining = &StringFromLiteral(" ipsum dolor sit amet, consectetur adipiscing elit."),
                },
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Fusce tempor feugiat purus, quis scelerisque dui accumsan sodales."),
                    .position = 0,
                },
            .search = &StringFromLiteral("purus,"),
            .expected =
                {
                    .value = 1,
                    .remaining = &StringFromLiteral(" quis scelerisque dui accumsan sodales."),
                },
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Mauris sed rutrum risus, sit amet blandit velit."),
                    .position = 0,
                },
            .search = &StringFromLiteral("velit."),
            .expected =
                {
                    .value = 1,
                    .remaining = &StringFromLiteral(""),
                },
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Nam quis aliquet augue."),
                    .position = 0,
                },
            .search = &StringFromLiteral("dignissim"),
            .expected =
                {
                    .value = 0,
                    .position = StringFromLiteral("Nam quis aliquet augue.").length,
                },
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral(
                        "Praesent nec nibh ut arcu semper pharetra. Praesent volutpat ut metus vitae ultrices."),
                    .position = StringFromLiteral("Praesent nec nibh ut arcu semper pharetra.").length,
                },
            .search = &StringFromLiteral("Nullam"),
            .expected =
                {
                    .value = 0,
                    .position =
                        StringFromLiteral(
                            "Praesent nec nibh ut arcu semper pharetra. Praesent volutpat ut metus vitae ultrices.")
                            .length,
                },
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Duis id augue nec eros faucibus ultrices."),
                    .position = StringFromLiteral("Duis id augue nec eros faucibus ultrices.").length,
                },
            .search = &StringFromLiteral("Etiam"),
            .expected =
                {
                    .value = 0,
                    .position = StringFromLiteral("Duis id augue nec eros faucibus ultrices.").length,
                },
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      b8 expectedToFindString = testCase->expected.value;
      u64 expectedPosition = testCase->expected.position;
      struct string *expectedRemaining = testCase->expected.remaining;
      struct string_cursor *cursor = &testCase->cursor;
      struct string *search = testCase->search;

      b8 got = StringCursorAdvanceAfter(cursor, search);
      struct string remaining = StringCursorExtractRemaining(cursor);
      if (got != expectedToFindString || (expectedToFindString && !IsStringEqual(&remaining, expectedRemaining)) ||
          (!expectedToFindString && cursor->position != expectedPosition)) {
        errorCode = STRING_CURSOR_TEST_ERROR_ADVANCE_AFTER_EXPECTED;

        StringBuilderAppendErrorMessage(sb, errorCode);

        StringBuilderAppendStringLiteral(sb, "\n  cursor: '");
        StringBuilderAppendString(sb, cursor->source);
        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n           ");
        for (u64 pos = 0; pos < cursor->position; pos++)
          StringBuilderAppendStringLiteral(sb, " ");
        StringBuilderAppendStringLiteral(sb, "↓");

        StringBuilderAppendStringLiteral(sb, "\n  search: '");
        StringBuilderAppendString(sb, search);
        StringBuilderAppendStringLiteral(sb, "'");

        if (got != expectedToFindString) {
          StringBuilderAppendStringLiteral(sb, "\n  expected to find search: ");
          StringBuilderAppendBool(sb, expectedToFindString);
          StringBuilderAppendStringLiteral(sb, "\n                      got: ");
          StringBuilderAppendBool(sb, got);
        } else if (!expectedToFindString && cursor->position != expectedPosition) {
          StringBuilderAppendStringLiteral(sb, "\n  expected cursor position: ");
          StringBuilderAppendU64(sb, expectedPosition);
          StringBuilderAppendStringLiteral(sb, "\n                       got: ");
          StringBuilderAppendU64(sb, cursor->position);
        } else {
          StringBuilderAppendStringLiteral(sb, "\n   expected: '");
          StringBuilderAppendString(sb, expectedRemaining);
          StringBuilderAppendStringLiteral(sb, "'");
          StringBuilderAppendStringLiteral(sb, "\n        got: '");
          StringBuilderAppendString(sb, &remaining);
          StringBuilderAppendStringLiteral(sb, "'");
        }
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // struct string StringCursorConsumeUntil(struct string_cursor *cursor, struct string *search)
  {
    struct test_case {
      struct string_cursor cursor;
      struct string *search;
      struct string expected;
    } testCases[] = {
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Lorem ipsum dolor sit amet, consectetur adipiscing elit."),
                    .position = 0,
                },
            .search = &StringFromLiteral("Lorem"),
            .expected = StringNull(),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Fusce tempor feugiat purus, quis scelerisque dui accumsan sodales."),
                    .position = 0,
                },
            .search = &StringFromLiteral("purus,"),
            .expected = StringFromLiteral("Fusce tempor feugiat "),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Mauris sed rutrum risus, sit amet blandit velit."),
                    .position = 0,
                },
            .search = &StringFromLiteral("velit."),
            .expected = StringFromLiteral("Mauris sed rutrum risus, sit amet blandit "),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Nam quis aliquet augue."),
                    .position = 0,
                },
            .search = &StringFromLiteral("Praesent"),
            .expected = StringFromLiteral("Nam quis aliquet augue."),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Praesent nec nibh ut arcu semper pharetra."),
                    .position = 0,
                },
            .search = &StringFromLiteral("volutpat"),
            .expected = StringFromLiteral("Praesent nec nibh ut arcu semper pharetra."),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Praesent volutpat ut metus vitae ultrices. Nullam ultrices ipsum mi, "
                                                 "id ultrices urna accumsan in."),
                    .position = StringFromLiteral("Praesent volutpat ut metus vitae ultrices. ").length,
                },
            .search = &StringFromLiteral("Nullam"),
            .expected = StringNull(),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Morbi maximus elit libero, at blandit purus facilisis vitae. Donec "
                                                 "sed dignissim est, quis vestibulum elit."),
                    .position =
                        StringFromLiteral("Morbi maximus elit libero, at blandit purus facilisis vitae. ").length,
                },
            .search = &StringFromLiteral("est,"),
            .expected = StringFromLiteral("Donec sed dignissim "),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral(
                        "Ut congue pulvinar aliquam. Donec volutpat viverra ex, eget convallis neque bibendum id."),
                    .position = StringFromLiteral("Ut congue pulvinar aliquam. ").length,
                },
            .search = &StringFromLiteral("id."),
            .expected = StringFromLiteral("Donec volutpat viverra ex, eget convallis neque bibendum "),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral(
                        "Cras gravida, nunc vitae interdum sagittis, leo ex sollicitudin libero, at pretium nulla "
                        "neque vel nibh. Morbi eu aliquet neque, eget imperdiet augue."),
                    .position = StringFromLiteral("Cras gravida, nunc vitae interdum sagittis, leo ex sollicitudin "
                                                  "libero, at pretium nulla neque vel nibh. ")
                                    .length,
                },
            .search = &StringFromLiteral("Fusce"),
            .expected = StringFromLiteral("Morbi eu aliquet neque, eget imperdiet augue."),
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *expected = &testCase->expected;
      struct string_cursor *cursor = &testCase->cursor;
      struct string *search = testCase->search;

      struct string got = StringCursorConsumeUntil(cursor, search);
      if (!IsStringEqual(&got, expected)) {
        errorCode = STRING_CURSOR_TEST_ERROR_CONSUME_UNTIL_EXPECTED;

        StringBuilderAppendErrorMessage(sb, errorCode);

        StringBuilderAppendStringLiteral(sb, "\n  cursor: '");
        StringBuilderAppendString(sb, cursor->source);
        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n           ");
        for (u64 pos = 0; pos < cursor->position; pos++)
          StringBuilderAppendStringLiteral(sb, " ");
        StringBuilderAppendStringLiteral(sb, "↓");

        StringBuilderAppendStringLiteral(sb, "\n  search: '");
        StringBuilderAppendString(sb, search);
        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendPrintableString(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendPrintableString(sb, &got);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // struct string StringCursorConsumeUntilLast(struct string_cursor *cursor, struct string *search)
  {
    struct test_case {
      struct string_cursor cursor;
      struct string search;
      struct string expected;
    } testCases[] = {
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Lorem Ipsum"),
                },
            .search = StringFromLiteral("Lorem"),
            .expected = StringNull(),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral(" Lorem Ipsum"),
                },
            .search = StringFromLiteral("Lorem"),
            .expected = StringFromLiteral(" "),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Lorem ipsum dolor sit amet, consectetur adipiscing elit."),
                },
            .search = StringFromLiteral(" "),
            .expected = StringFromLiteral("Lorem ipsum dolor sit amet, consectetur adipiscing"),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Praesent nec consectetur orci."),
                },
            .search = StringFromLiteral(" "),
            .expected = StringFromLiteral("Praesent nec consectetur"),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("abcdefgh"),
                    .position = 0,
                },
            .search = StringFromLiteral("012345"),
            .expected = StringFromLiteral("abcdefgh"),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Lorem"),
                    .position = 0,
                },
            .search = StringFromLiteral("no fucking way"),
            .expected = StringFromLiteral("Lorem"),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("no fucking way"),
                },
            .search = StringNull(),
            .expected = StringNull(),
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *expected = &testCase->expected;
      struct string_cursor *cursor = &testCase->cursor;
      struct string *search = &testCase->search;

      struct string got = StringCursorConsumeUntilLast(cursor, search);
      if (!IsStringEqual(&got, expected)) {
        errorCode = STRING_CURSOR_TEST_ERROR_CONSUME_UNTIL_LAST_EXPECTED;

        StringBuilderAppendErrorMessage(sb, errorCode);

        StringBuilderAppendStringLiteral(sb, "\n    cursor: '");
        StringBuilderAppendString(sb, cursor->source);
        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n           ");
        for (u64 pos = 0; pos < cursor->position; pos++)
          StringBuilderAppendStringLiteral(sb, " ");
        StringBuilderAppendStringLiteral(sb, "↓");

        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n    search: '");
        StringBuilderAppendString(sb, search);
        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n  expected: '");
        StringBuilderAppendPrintableString(sb, expected);
        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n       got: '");
        StringBuilderAppendPrintableString(sb, &got);
        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // struct string StringCursorExtractThrough(struct string_cursor *cursor, struct string *search)
  {
    struct test_case {
      struct string_cursor cursor;
      struct string *search;
      struct string expected;
    } testCases[] = {
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Lorem ipsum dolor sit amet, consectetur adipiscing elit."),
                    .position = 0,
                },
            .search = &StringFromLiteral("Lorem"),
            .expected = StringFromLiteral("Lorem"),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Fusce tempor feugiat purus, quis scelerisque dui accumsan sodales."),
                    .position = 0,
                },
            .search = &StringFromLiteral("purus,"),
            .expected = StringFromLiteral("Fusce tempor feugiat purus,"),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Mauris sed rutrum risus, sit amet blandit velit."),
                    .position = 0,
                },
            .search = &StringFromLiteral("velit."),
            .expected = StringFromLiteral("Mauris sed rutrum risus, sit amet blandit velit."),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Nam quis aliquet augue."),
                    .position = 0,
                },
            .search = &StringFromLiteral("Praesent"),
            .expected = StringNull(),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Praesent nec nibh ut arcu semper pharetra."),
                    .position = 0,
                },
            .search = &StringFromLiteral("volutpat"),
            .expected = StringNull(),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Praesent volutpat ut metus vitae ultrices."),
                    .position = StringFromLiteral("Praesent volutpat ut metus vitae ultrices.").length,
                },
            .search = &StringFromLiteral("Nullam"),
            .expected = StringNull(),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("123"),
                    .position = 0,
                },
            .search = &StringFromLiteral("a"),
            .expected = StringNull(),
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *expected = &testCase->expected;
      struct string_cursor *cursor = &testCase->cursor;
      struct string *search = testCase->search;
      struct string remaining = StringCursorExtractRemaining(cursor);

      struct string got = StringCursorExtractThrough(cursor, search);
      if (!IsStringEqual(&got, expected)) {
        errorCode = STRING_CURSOR_TEST_ERROR_EXTRACT_THROUGH_EXPECTED;

        StringBuilderAppendErrorMessage(sb, errorCode);

        StringBuilderAppendStringLiteral(sb, "\n  cursor: '");
        StringBuilderAppendString(sb, cursor->source);
        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n           ");
        for (u64 pos = 0; pos < cursor->position; pos++)
          StringBuilderAppendStringLiteral(sb, " ");
        StringBuilderAppendStringLiteral(sb, "↓");

        StringBuilderAppendStringLiteral(sb, "\n  search: '");
        StringBuilderAppendString(sb, search);
        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n  expected: '");
        StringBuilderAppendPrintableString(sb, expected);
        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n       got: '");
        StringBuilderAppendPrintableString(sb, &got);
        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // struct string StringCursorExtractNumber(struct string_cursor *cursor)
  {
    struct test_case {
      struct string_cursor cursor;
      struct string expected;
    } testCases[] = {
        {
            .cursor =
                {
                    .source = &StringFromLiteral("90876"),
                    .position = 0,
                },
            .expected = StringFromLiteral("90876"),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("5933 abcdef"),
                    .position = 0,
                },
            .expected = StringFromLiteral("5933"),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("-10203 fool"),
                    .position = 0,
                },
            .expected = StringFromLiteral("-10203"),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("-54.3023 fool"),
                    .position = 0,
                },
            .expected = StringFromLiteral("-54.3023"),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("86774.60272.062713"),
                    .position = 0,
                },
            .expected = StringFromLiteral("86774.60272"),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("54.-3023 fool"),
                    .position = 0,
                },
            .expected = StringFromLiteral("54."),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("5933 abcdef"),
                    .position = 1,
                },
            .expected = StringFromLiteral("933"),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("abcdef"),
                    .position = 0,
                },
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral(""),
                    .position = 0,
                },
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *expected = &testCase->expected;
      struct string_cursor *cursor = &testCase->cursor;

      struct string got = StringCursorExtractNumber(cursor);
      if (!IsStringEqual(&got, expected)) {
        errorCode = expected->length ? STRING_CURSOR_TEST_ERROR_EXTRACT_NUMBER_EXPECTED_TRUE
                                     : STRING_CURSOR_TEST_ERROR_EXTRACT_NUMBER_EXPECTED_FALSE;

        StringBuilderAppendErrorMessage(sb, errorCode);

        StringBuilderAppendStringLiteral(sb, "\n  cursor: '");
        StringBuilderAppendString(sb, cursor->source);
        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n           ");
        for (u64 pos = 0; pos < cursor->position; pos++)
          StringBuilderAppendStringLiteral(sb, " ");
        StringBuilderAppendStringLiteral(sb, "↓");

        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendPrintableString(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendPrintableString(sb, &got);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // struct string StringCursorExtractConsumed(struct string_cursor *cursor)
  {
    struct test_case {
      struct string_cursor cursor;
      struct string expected;
    } testCases[] = {
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Lorem Ipsum"),
                    .position = 6,
                },
            .expected = StringFromLiteral("Lorem "),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Lorem Ipsum"),
                    .position = 11,
                },
            .expected = StringFromLiteral("Lorem Ipsum"),
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral("Lorem Ipsum"),
                    .position = 0,
                },
        },
        {
            .cursor =
                {
                    .source = &StringFromLiteral(""),
                    .position = 0,
                },
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *expected = &testCase->expected;

      struct string_cursor *cursor = &testCase->cursor;
      struct string got = StringCursorExtractConsumed(cursor);
      if (!IsStringEqual(&got, expected)) {
        errorCode = STRING_CURSOR_TEST_ERROR_EXTRACT_CONSUMED;

        StringBuilderAppendErrorMessage(sb, errorCode);

        StringBuilderAppendStringLiteral(sb, "\n  cursor: '");
        StringBuilderAppendString(sb, cursor->source);
        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n           ");
        for (u64 pos = 0; pos < cursor->position; pos++)
          StringBuilderAppendStringLiteral(sb, " ");
        StringBuilderAppendStringLiteral(sb, "↓");

        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendString(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendString(sb, &got);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  return (int)errorCode;
}
