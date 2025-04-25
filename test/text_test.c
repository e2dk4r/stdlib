#include "print.h"
#include "string_builder.h"
#include "text.h"

#define TEST_ERROR_LIST(XX)                                                                                            \
  XX(TEXT_TEST_ERROR_STRING_FROM_ZERO_TERMINATED, "Failed to create string from zero terminated c-string")             \
  XX(TEXT_TEST_ERROR_IS_STRING_EQUAL_MUST_BE_TRUE, "Strings must be equal")                                            \
  XX(TEXT_TEST_ERROR_IS_STRING_EQUAL_MUST_BE_FALSE, "Strings must NOT be equal")                                       \
  XX(TEXT_TEST_ERROR_IS_STRING_EQUAL_IGNORE_CASE_MUST_BE_TRUE, "Strings that are case ignored must be equal")          \
  XX(TEXT_TEST_ERROR_IS_STRING_EQUAL_IGNORE_CASE_MUST_BE_FALSE, "Strings that are case ignored must NOT be equal")     \
  XX(TEXT_TEST_ERROR_IS_STRING_CONTAINS_EXPECTED_TRUE, "String must contain search string")                            \
  XX(TEXT_TEST_ERROR_IS_STRING_CONTAINS_EXPECTED_FALSE, "String must NOT contain search string")                       \
  XX(TEXT_TEST_ERROR_IS_STRING_STARTS_WITH_EXPECTED_TRUE, "String must start with search string")                      \
  XX(TEXT_TEST_ERROR_IS_STRING_STARTS_WITH_EXPECTED_FALSE, "String must NOT start with search string")                 \
  XX(TEXT_TEST_ERROR_IS_STRING_ENDS_WITH_EXPECTED_TRUE, "String must end with search string")                          \
  XX(TEXT_TEST_ERROR_IS_STRING_ENDS_WITH_EXPECTED_FALSE, "String must NOT end with search string")                     \
  XX(TEXT_TEST_ERROR_STRIP_WHITESPACE_EXPECTED_STRING, "Stripping string from whitespace must return new string")      \
  XX(TEXT_TEST_ERROR_STRIP_WHITESPACE_EXPECTED_NULL, "Stripping string from whitespace must return null")              \
  XX(TEXT_TEST_ERROR_PARSE_DURATION_EXPECTED_TRUE, "Parsing duration string must be successful")                       \
  XX(TEXT_TEST_ERROR_PARSE_DURATION_EXPECTED_FALSE, "Parsing duration string must fail")                               \
  XX(TEXT_TEST_ERROR_IS_DURATION_LESS_THAN_EXPECTED_TRUE, "lhs duration must be less then rhs")                        \
  XX(TEXT_TEST_ERROR_IS_DURATION_LESS_THAN_EXPECTED_FALSE, "lhs duration must NOT be less then rhs")                   \
  XX(TEXT_TEST_ERROR_IS_DURATION_GREATER_THAN_EXPECTED_TRUE, "lhs duration must be greater then rhs")                  \
  XX(TEXT_TEST_ERROR_IS_DURATION_GREATER_THAN_EXPECTED_FALSE, "lhs duration must NOT be greater then rhs")             \
  XX(TEXT_TEST_ERROR_PARSE_U64_EXPECTED_TRUE, "Parsing unsigned value must be successful")                             \
  XX(TEXT_TEST_ERROR_PARSE_U64_EXPECTED_FALSE, "Parsing unsigned value must fail")                                     \
  XX(TEXT_TEST_ERROR_PARSE_HEX_EXPECTED_TRUE, "Parsing hexadecimal value must be successful")                          \
  XX(TEXT_TEST_ERROR_PARSE_HEX_EXPECTED_FALSE, "Parsing hexadecimal value must fail")                                  \
  XX(TEXT_TEST_ERROR_FORMATU64_EXPECTED, "Formatting u64 value must be successful")                                    \
  XX(TEXT_TEST_ERROR_FORMATF32SLOW_EXPECTED, "Formatting f32 value must be successful")                                \
  XX(TEXT_TEST_ERROR_FORMATHEX_EXPECTED, "Formatting value to hex must be successful")                                 \
  XX(TEXT_TEST_ERROR_PATHGETDIRECTORY, "Extracting path's parent directory must be successful")                        \
  XX(TEXT_TEST_ERROR_STRINGSPLIT_EXPECTED_TRUE, "Splitting string into parts must be successful")                      \
  XX(TEXT_TEST_ERROR_STRINGSPLIT_EXPECTED_FALSE, "Splitting string into parts must be fail")

enum text_test_error {
  TEXT_TEST_ERROR_NONE = 0,
#define XX(tag, message) tag,
  TEST_ERROR_LIST(XX)
#undef XX

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

internalfn string *
GetTextTestErrorMessage(enum text_test_error errorCode)
{
  struct text_test_error_info {
    enum text_test_error code;
    struct string message;
  } errors[] = {
#define XX(tag, msg) {.code = tag, .message = StringFromLiteral(msg)},
      TEST_ERROR_LIST(XX)
#undef XX
  };

  for (u32 index = 0; index < ARRAY_COUNT(errors); index++) {
    struct text_test_error_info *info = errors + index;
    if (info->code == errorCode)
      return (struct string *)&info->message;
  }
  return 0;
}

internalfn void
StringBuilderAppendBool(string_builder *sb, b8 value)
{
  StringBuilderAppendString(sb, value ? &StringFromLiteral("true") : &StringFromLiteral("false"));
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
  enum text_test_error errorCode = TEXT_TEST_ERROR_NONE;

  // setup
  enum { KILOBYTES = (1 << 10) };
  u8 stackBuffer[8 * KILOBYTES];
  memory_arena stackMemory = {
      .block = stackBuffer,
      .total = ARRAY_COUNT(stackBuffer),
  };

  string_builder *sb = MakeStringBuilder(&stackMemory, 1024, 32);

  // struct string StringFromZeroTerminated(u8 *src, u64 max)
  {
    struct test_case {
      char *input;
      u64 length;
      u64 expected;
    } testCases[] = {
        {
            .input = "abc",
            .length = 1024,
            .expected = 3,
        },
        {
            .input = "abcdefghijklm",
            .length = 3,
            .expected = 3,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      u64 expected = testCase->expected;
      char *input = testCase->input;
      u64 length = testCase->length;

      struct string got = StringFromZeroTerminated((u8 *)input, length);
      if (got.value != (u8 *)input || got.length != expected) {
        errorCode = TEXT_TEST_ERROR_STRING_FROM_ZERO_TERMINATED;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendU64(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendU64(sb, got.length);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // b8 IsStringEqual(struct string *left, struct string *right)
  b8 IsStringEqualOK = 1;
  {
    struct test_case {
      struct string *left;
      struct string *right;
      b8 expected;
    } testCases[] = {
        {
            .left = &StringFromLiteral("abc"),
            .right = &StringFromLiteral("abc"),
            .expected = 1,
        },
        {
            .left = &StringFromLiteral("abc"),
            .right = &StringFromLiteral("ABC"),
            .expected = 0,
        },
        {
            .left = &StringFromLiteral("abc"),
            .right = &StringFromLiteral("abc def ghi"),
            .expected = 0,
        },
        // NULL
        {
            .left = &(struct string){.value = 0},
            .right = &StringFromLiteral("foo"),
            .expected = 0,
        },
        {
            .left = &StringFromLiteral("foo"),
            .right = &(struct string){.value = 0},
            .expected = 0,
        },
        {
            .left = &(struct string){.value = 0},
            .right = &(struct string){.value = 0},
            .expected = 1,
        },
        // EMPTY
        {
            .left = &StringFromLiteral(""),
            .right = &StringFromLiteral(""),
            .expected = 1,
        },
        {
            .left = &(struct string){.value = 0},
            .right = &StringFromLiteral(""),
            .expected = 0,
        },
        {
            .left = &StringFromLiteral(""),
            .right = &(struct string){.value = 0},
            .expected = 0,
        },
        // SPACE
        {
            .left = &StringFromLiteral(" "),
            .right = &StringFromLiteral(" "),
            .expected = 1,
        },
        {
            .left = &(struct string){.value = 0},
            .right = &StringFromLiteral(" "),
            .expected = 0,
        },
        {
            .left = &StringFromLiteral(" "),
            .right = &(struct string){.value = 0},
            .expected = 0,
        },
        {
            .left = &StringFromLiteral(""),
            .right = &StringFromLiteral(" "),
            .expected = 0,
        },
        {
            .left = &StringFromLiteral(" "),
            .right = &StringFromLiteral(""),
            .expected = 0,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *left = testCase->left;
      struct string *right = testCase->right;
      b8 expected = testCase->expected;
      b8 value = IsStringEqual(left, right);
      if (value != expected) {
        IsStringEqualOK = 0;
        errorCode =
            expected ? TEXT_TEST_ERROR_IS_STRING_EQUAL_MUST_BE_TRUE : TEXT_TEST_ERROR_IS_STRING_EQUAL_MUST_BE_FALSE;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendStringLiteral(sb, "\n  left:     ");
        StringBuilderAppendPrintableString(sb, left);
        StringBuilderAppendStringLiteral(sb, "\n  right:    ");
        StringBuilderAppendPrintableString(sb, right);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendBool(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendBool(sb, value);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // b8 IsStringEqualIgnoreCase(struct string *left, struct string *right)
  {
    struct test_case {
      struct string *left;
      struct string *right;
      b8 expected;
    } testCases[] = {
        {
            .left = &StringFromLiteral("abc"),
            .right = &StringFromLiteral("ABC"),
            .expected = 1,
        },
        {
            .left = &StringFromLiteral("ABC"),
            .right = &StringFromLiteral("abc"),
            .expected = 1,
        },
        {
            .left = &StringFromLiteral("abc"),
            .right = &StringFromLiteral("abc"),
            .expected = 1,
        },
        {
            .left = &StringFromLiteral("abc"),
            .right = &StringFromLiteral("abc def ghi"),
            .expected = 0,
        },
        // NULL
        {
            .left = &(struct string){.value = 0},
            .right = &StringFromLiteral("foo"),
            .expected = 0,
        },
        {
            .left = &StringFromLiteral("foo"),
            .right = &(struct string){.value = 0},
            .expected = 0,
        },
        {
            .left = &(struct string){.value = 0},
            .right = &(struct string){.value = 0},
            .expected = 1,
        },
        // EMPTY
        {
            .left = &StringFromLiteral(""),
            .right = &StringFromLiteral(""),
            .expected = 1,
        },
        {
            .left = &(struct string){.value = 0},
            .right = &StringFromLiteral(""),
            .expected = 0,
        },
        {
            .left = &StringFromLiteral(""),
            .right = &(struct string){.value = 0},
            .expected = 0,
        },
        // SPACE
        {
            .left = &StringFromLiteral(" "),
            .right = &StringFromLiteral(" "),
            .expected = 1,
        },
        {
            .left = &(struct string){.value = 0},
            .right = &StringFromLiteral(" "),
            .expected = 0,
        },
        {
            .left = &StringFromLiteral(" "),
            .right = &(struct string){.value = 0},
            .expected = 0,
        },
        {
            .left = &StringFromLiteral(""),
            .right = &StringFromLiteral(" "),
            .expected = 0,
        },
        {
            .left = &StringFromLiteral(" "),
            .right = &StringFromLiteral(""),
            .expected = 0,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *left = testCase->left;
      struct string *right = testCase->right;
      b8 expected = testCase->expected;
      b8 value = IsStringEqualIgnoreCase(left, right);
      if (value != expected) {
        IsStringEqualOK = 0;
        errorCode = expected ? TEXT_TEST_ERROR_IS_STRING_EQUAL_IGNORE_CASE_MUST_BE_TRUE
                             : TEXT_TEST_ERROR_IS_STRING_EQUAL_IGNORE_CASE_MUST_BE_FALSE;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendStringLiteral(sb, "\n  left:     ");
        StringBuilderAppendPrintableString(sb, left);
        StringBuilderAppendStringLiteral(sb, "\n  right:    ");
        StringBuilderAppendPrintableString(sb, right);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendBool(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendBool(sb, value);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // IsStringContains(struct string *string, struct string *search)
  {
    struct test_case {
      struct string string;
      struct string search;
      b8 expected;
    } testCases[] = {
        {
            .string = StringFromLiteral("abc def ghi"),
            .search = StringFromLiteral("abc"),
            .expected = 1,
        },
        {
            .string = StringFromLiteral("abc def ghi"),
            .search = StringFromLiteral("def"),
            .expected = 1,
        },
        {
            .string = StringFromLiteral("abc def ghi"),
            .search = StringFromLiteral("ghi"),
            .expected = 1,
        },
        {
            .string = StringFromLiteral("abc def ghi"),
            .search = StringFromLiteral("ghijkl"),
            .expected = 0,
        },
        {
            .string = StringFromLiteral("abc def ghi"),
            .search = StringFromLiteral("jkl"),
            .expected = 0,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *string = &testCase->string;
      struct string *search = &testCase->search;
      b8 expected = testCase->expected;
      b8 value = IsStringContains(string, search);
      if (value != expected) {
        errorCode = expected ? TEXT_TEST_ERROR_IS_STRING_CONTAINS_EXPECTED_TRUE
                             : TEXT_TEST_ERROR_IS_STRING_CONTAINS_EXPECTED_FALSE;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendStringLiteral(sb, "\n  string:   ");
        StringBuilderAppendString(sb, string);
        StringBuilderAppendStringLiteral(sb, "\n  search:   ");
        StringBuilderAppendString(sb, search);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendBool(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendBool(sb, value);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // IsStringStartsWith(struct string *string, struct string *search)
  {
    struct test_case {
      struct string string;
      struct string search;
      b8 expected;
      enum text_test_error error;
    } testCases[] = {
        {
            .string = StringFromLiteral("abc def ghi"),
            .search = StringFromLiteral("abc"),
            .expected = 1,
        },
        {
            .string = StringFromLiteral("abc def ghi"),
            .search = StringFromLiteral("def"),
            .expected = 0,
        },
        {
            .string = StringFromLiteral("abc def ghi"),
            .search = StringFromLiteral("ghi"),
            .expected = 0,
        },
        {
            .string = StringFromLiteral("abc def ghi"),
            .search = StringFromLiteral("ghijkl"),
            .expected = 0,
        },
        {
            .string = StringFromLiteral("abc def ghi"),
            .search = StringFromLiteral("jkl"),
            .expected = 0,
        },
        {
            .string = StringFromLiteral("abc def ghi"),
            .search = (struct string){.value = 0, .length = 0},
            .expected = 0,
        },
        {
            .string = StringFromLiteral("abc def ghi"),
            .search = StringFromLiteral(""),
            .expected = 0,
        },
        {
            .string = (struct string){.value = 0, .length = 0},
            .search = StringFromLiteral(""),
            .expected = 0,
        },
        {
            .string = StringFromLiteral(""),
            .search = (struct string){.value = 0, .length = 0},
            .expected = 0,
        },
        {
            .string = (struct string){.value = 0, .length = 0},
            .search = (struct string){.value = 0, .length = 0},
            .expected = 0,
        },
        {
            .string = StringFromLiteral(""),
            .search = StringFromLiteral(""),
            .expected = 0,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *string = &testCase->string;
      struct string *search = &testCase->search;
      b8 expected = testCase->expected;
      b8 value = IsStringStartsWith(string, search);
      if (value != expected) {
        errorCode = expected ? TEXT_TEST_ERROR_IS_STRING_STARTS_WITH_EXPECTED_TRUE
                             : TEXT_TEST_ERROR_IS_STRING_STARTS_WITH_EXPECTED_FALSE;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendStringLiteral(sb, "\n  string:   ");
        StringBuilderAppendString(sb, string);
        StringBuilderAppendStringLiteral(sb, "\n  search:   ");
        StringBuilderAppendString(sb, search);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendBool(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendBool(sb, value);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // b8 IsStringEndsWith(struct string *string, struct string *search)
  {
    struct test_case {
      struct string string;
      struct string search;
      b8 expected;
      enum text_test_error error;
    } testCases[] = {
        {
            .string = StringFromLiteral("abc def ghi"),
            .search = StringFromLiteral("ghi"),
            .expected = 1,
        },
        {
            .string = StringFromLiteral("abc def ghi"),
            .search = StringFromLiteral("abc"),
            .expected = 0,
        },
        {
            .string = StringFromLiteral("abc def ghi"),
            .search = StringFromLiteral("def"),
            .expected = 0,
        },
        {
            .string = StringFromLiteral("abc def ghi"),
            .search = StringFromLiteral("abc def"),
            .expected = 0,
        },
        {
            .string = StringFromLiteral("abc def ghi"),
            .search = StringFromLiteral("jkl"),
            .expected = 0,
        },
        {
            .string = (struct string){.value = 0, .length = 0},
            .search = StringFromLiteral(""),
            .expected = 0,
        },
        {
            .string = StringFromLiteral(""),
            .search = (struct string){.value = 0, .length = 0},
            .expected = 0,
        },
        {
            .string = (struct string){.value = 0, .length = 0},
            .search = (struct string){.value = 0, .length = 0},
            .expected = 0,
        },
        {
            .string = StringFromLiteral(""),
            .search = StringFromLiteral(""),
            .expected = 0,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *string = &testCase->string;
      struct string *search = &testCase->search;
      b8 expected = testCase->expected;
      b8 value = IsStringEndsWith(string, search);
      if (value != expected) {
        errorCode = expected ? TEXT_TEST_ERROR_IS_STRING_ENDS_WITH_EXPECTED_TRUE
                             : TEXT_TEST_ERROR_IS_STRING_ENDS_WITH_EXPECTED_FALSE;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendStringLiteral(sb, "\n  string:   ");
        StringBuilderAppendString(sb, string);
        StringBuilderAppendStringLiteral(sb, "\n  search:   ");
        StringBuilderAppendString(sb, search);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendBool(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendBool(sb, value);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // struct string StringStripWhitespace(struct string *string)
  // Dependencies: IsStringEqual()
  if (IsStringEqualOK) {
    struct test_case {
      struct string string;
      struct string expected;
    } testCases[] = {
        {
            .string = StringFromLiteral(" abc \n"),
            .expected = StringFromLiteral("abc"),
        },
        {
            .string = StringFromLiteral("\t123"),
            .expected = StringFromLiteral("123"),
        },
        {
            .string = StringFromLiteral("123\t\r\n"),
            .expected = StringFromLiteral("123"),
        },
        {
            .string = (struct string){.value = 0, .length = 0},
            .expected = (struct string){.value = 0, .length = 0},
        },
        {
            .string = StringFromLiteral(""),
            .expected = (struct string){.value = 0, .length = 0},
        },
        {
            .string = StringFromLiteral("\n\t\v\f"),
            .expected = (struct string){.value = 0, .length = 0},
        },
        {
            .string = StringFromLiteral("abc"),
            .expected = StringFromLiteral("abc"),
        },
        {
            .string = StringFromLiteral("123 456"),
            .expected = StringFromLiteral("123 456"),
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *string = &testCase->string;
      struct string *expected = &testCase->expected;
      struct string got = StringStripWhitespace(string);
      if (!IsStringEqual(&got, expected)) {
        errorCode = expected->length > 0 ? TEXT_TEST_ERROR_STRIP_WHITESPACE_EXPECTED_STRING
                                         : TEXT_TEST_ERROR_STRIP_WHITESPACE_EXPECTED_NULL;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendStringLiteral(sb, "\n  string:   ");
        StringBuilderAppendString(sb, string);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendPrintableString(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendPrintableString(sb, &got);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  } else {
    struct string errorMessage =
        StringFromLiteral("Warning: StringStripWhitespace() test skipped because IsStringEqual() failed\n.");
    PrintString(&errorMessage);
  }

  // ParseDuration(struct string *string, struct duration *duration)
  {
    struct test_case {
      struct string string;
      struct {
        b8 value;
        struct duration duration;
      } expected;
    } testCases[] = {
        {
            .string = StringFromLiteral("0ns"),
            .expected =
                {
                    .value = 1,
                    .duration = DurationInNanoseconds(0),
                },
        },
        {
            .string = StringFromLiteral("1ns"),
            .expected =
                {
                    .value = 1,
                    .duration = DurationInNanoseconds(1),
                },
        },
        {
            .string = StringFromLiteral("1sec"),
            .expected =
                {
                    .value = 1,
                    .duration = DurationInSeconds(1),
                },
        },
        {
            .string = StringFromLiteral("5sec"),
            .expected =
                {
                    .value = 1,
                    .duration = DurationInSeconds(5),
                },
        },
        {
            .string = StringFromLiteral("7min"),
            .expected =
                {
                    .value = 1,
                    .duration = DurationInMinutes(7),
                },
        },
        {
            .string = StringFromLiteral("39day"),
            .expected =
                {
                    .value = 1,
                    .duration = DurationInDays(39),
                },
        },
        {
            .string = StringFromLiteral("1hr5min"),
            .expected =
                {
                    .value = 1,
                    .duration = DurationAddMultiple(DurationInHours(1), DurationInMinutes(5)),
                },
        },
        {
            .string = StringFromLiteral("73day384sec"),
            .expected =
                {
                    .value = 1,
                    .duration = DurationAddMultiple(DurationInDays(73), DurationInSeconds(384)),
                },
        },
        {
            .string = (struct string){.value = 0, .length = 0}, // NULL
            .expected =
                {
                    .value = 0,
                },
        },
        {
            .string = StringFromLiteral(""), // EMPTY
            .expected =
                {
                    .value = 0,
                },
        },
        {
            .string = StringFromLiteral(" "), // SPACE
            .expected =
                {
                    .value = 0,
                },
        },
        {
            .string = StringFromLiteral("abc"),
            .expected =
                {
                    .value = 0,
                },
        },
        {
            .string = StringFromLiteral("5m5s"),
            .expected =
                {
                    .value = 0,
                },
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;
      struct string *string = &testCase->string;
      struct duration duration;

      struct duration *expectedDuration = &testCase->expected.duration;
      b8 expected = testCase->expected.value;
      b8 got = ParseDuration(string, &duration);
      if (got != expected || (expected && !IsDurationEqual(&duration, expectedDuration))) {
        errorCode =
            expected ? TEXT_TEST_ERROR_PARSE_DURATION_EXPECTED_TRUE : TEXT_TEST_ERROR_PARSE_DURATION_EXPECTED_FALSE;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendStringLiteral(sb, "\n  string:   ");
        StringBuilderAppendPrintableString(sb, string);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendBool(sb, expected);
        if (expected) {
          StringBuilderAppendStringLiteral(sb, " duration: ");
          StringBuilderAppendU64(sb, expectedDuration->ns);
          StringBuilderAppendStringLiteral(sb, "ns");
        }
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendBool(sb, got);
        if (expected) {
          StringBuilderAppendStringLiteral(sb, " duration: ");
          StringBuilderAppendU64(sb, duration.ns);
          StringBuilderAppendStringLiteral(sb, "ns");
        }
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // IsDurationLessThan(struct duration *left, struct duration *right)
  // IsDurationGraterThan(struct duration *left, struct duration *right)
  {
    struct test_case {
      struct duration left;
      struct duration right;
      b8 isLeftLess;
      b8 isLeftGreater;
    } testCases[] = {
        {
            .left = (struct duration){.ns = 1000000000ull /* 1e9 */ * 1},
            .right = (struct duration){.ns = 1000000000ull /* 1e9 */ * 5},
            .isLeftLess = 1,
            .isLeftGreater = 0,
        },
        {
            .left = (struct duration){.ns = 1000000000ull /* 1e9 */ * 1},
            .right = (struct duration){.ns = 1000000000ull /* 1e9 */ * 1},
            .isLeftLess = 0,
            .isLeftGreater = 0,
        },
        {
            .left = (struct duration){.ns = 1000000000ull /* 1e9 */ * 5},
            .right = (struct duration){.ns = 1000000000ull /* 1e9 */ * 1},
            .isLeftLess = 0,
            .isLeftGreater = 1,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct duration *left = &testCase->left;
      struct duration *right = &testCase->right;
      b8 expectedGreaterThan = testCase->isLeftGreater;
      b8 expectedLessThan = testCase->isLeftLess;

      b8 gotGreaterThan = IsDurationGreaterThan(left, right);
      b8 gotLessThan = IsDurationLessThan(left, right);

      if (gotGreaterThan != expectedGreaterThan) {
        errorCode = expectedGreaterThan ? TEXT_TEST_ERROR_IS_DURATION_GREATER_THAN_EXPECTED_TRUE
                                        : TEXT_TEST_ERROR_IS_DURATION_GREATER_THAN_EXPECTED_FALSE;
        goto end;
      }

      if (gotLessThan != expectedLessThan) {
        errorCode = expectedLessThan ? TEXT_TEST_ERROR_IS_DURATION_LESS_THAN_EXPECTED_TRUE
                                     : TEXT_TEST_ERROR_IS_DURATION_LESS_THAN_EXPECTED_FALSE;
        goto end;
      }
    }
  }

  // b8 ParseU64(struct string *string, u64 *value)
  {
    struct test_case {
      struct string input;
      struct {
        b8 result;
        u64 value;
      } expected;
    } testCases[] = {
        {
            .input = StringFromLiteral("0"),
            .expected =
                {
                    .result = 1,
                    .value = 0,
                },
        },
        {
            .input = StringFromLiteral("7"),
            .expected =
                {
                    .result = 1,
                    .value = 7,
                },
        },
        {
            .input = StringFromLiteral("18446744073709551615"),
            .expected =
                {
                    .result = 1,
                    .value = 18446744073709551615ul,
                },
        },
        {
            .input = StringFromLiteral("123456789ABCDEF"),
            .expected =
                {
                    .result = 0,
                },
        },
        {
            .input = StringFromLiteral("ffffffffffffffff"),
            .expected =
                {
                    .result = 0,
                },
        },
        {
            .input = (struct string){.value = 0, .length = 0},
            .expected =
                {
                    .result = 0,
                },
        },
        {
            .input = StringFromLiteral(""),
            .expected =
                {
                    .result = 0,
                },
        },
        {
            .input = StringFromLiteral("not a unsigned value 1340"),
            .expected =
                {
                    .result = 0,
                },
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *input = &testCase->input;
      b8 expectedResult = testCase->expected.result;
      u64 expectedValue = testCase->expected.value;
      u64 value;
      b8 result = ParseU64(input, &value);
      if (result != expectedResult || (expectedResult && value != expectedValue)) {
        errorCode = expectedResult ? TEXT_TEST_ERROR_PARSE_U64_EXPECTED_TRUE : TEXT_TEST_ERROR_PARSE_U64_EXPECTED_FALSE;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendStringLiteral(sb, "\n  input:    ");
        StringBuilderAppendPrintableString(sb, input);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        if (result != expectedResult)
          StringBuilderAppendBool(sb, expectedResult);
        else
          StringBuilderAppendU64(sb, expectedValue);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        if (result != expectedResult)
          StringBuilderAppendBool(sb, result);
        else
          StringBuilderAppendU64(sb, value);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // b8 ParseHex(struct string *string, u64 *value)
  {
    struct test_case {
      struct string input;
      struct {
        b8 result;
        u64 value;
      } expected;
    } testCases[] = {
        {
            .input = StringFromLiteral("0"),
            .expected =
                {
                    .result = 1,
                    .value = 0x0,
                },
        },
        {
            .input = StringFromLiteral("fa"),
            .expected =
                {
                    .result = 1,
                    .value = 0xfa,
                },
        },
        {
            .input = StringFromLiteral("123456789abcdef"),
            .expected =
                {
                    .result = 1,
                    .value = 0x123456789abcdef,
                },
        },
        {
            .input = StringFromLiteral("123456789ABCDEF"),
            .expected =
                {
                    .result = 1,
                    .value = 0x123456789ABCDEF,
                },
        },
        {
            .input = StringFromLiteral("ffffffffffffffff"),
            .expected =
                {
                    .result = 1,
                    .value = 0xffffffffffffffff,
                },
        },
        {
            .input = (struct string){.value = 0, .length = 0},
            .expected =
                {
                    .result = 0,
                },
        },
        {
            .input = StringFromLiteral(""),
            .expected =
                {
                    .result = 0,
                },
        },
        {
            .input = StringFromLiteral("not a hexadecimal 1340"),
            .expected =
                {
                    .result = 0,
                },
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *input = &testCase->input;
      b8 expectedResult = testCase->expected.result;
      u64 expectedValue = testCase->expected.value;
      u64 value;
      b8 result = ParseHex(input, &value);
      if (result != expectedResult || (expectedResult && value != expectedValue)) {
        errorCode = expectedResult ? TEXT_TEST_ERROR_PARSE_HEX_EXPECTED_TRUE : TEXT_TEST_ERROR_PARSE_HEX_EXPECTED_FALSE;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendStringLiteral(sb, "\n  input:    ");
        StringBuilderAppendPrintableString(sb, input);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        if (result != expectedResult)
          StringBuilderAppendBool(sb, expectedResult);
        else
          StringBuilderAppendU64(sb, expectedValue);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        if (result != expectedResult)
          StringBuilderAppendBool(sb, result);
        else
          StringBuilderAppendU64(sb, value);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // struct string FormatU64(struct string *stringBuffer, u64 value)
  // Dependencies: IsStringEqual()
  if (IsStringEqualOK) {
    struct test_case {
      u64 input;
      struct string expected;
    } testCases[] = {
        {
            .input = 0,
            .expected = StringFromLiteral("0"),
        },
        {
            .input = 1,
            .expected = StringFromLiteral("1"),
        },
        {
            .input = 10,
            .expected = StringFromLiteral("10"),
        },
        {
            .input = 3912,
            .expected = StringFromLiteral("3912"),
        },
        {
            .input = 18446744073709551615UL,
            .expected = StringFromLiteral("18446744073709551615"),
        },
        // no fail cases
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      u8 buf[20];
      struct string stringBuffer = {.value = buf, .length = sizeof(buf)};

      u64 input = testCase->input;
      struct string *expected = &testCase->expected;
      struct string value = FormatU64(&stringBuffer, input);
      if (!IsStringEqual(&value, expected)) {
        errorCode = TEXT_TEST_ERROR_FORMATU64_EXPECTED;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendStringLiteral(sb, "\n  input:    ");
        StringBuilderAppendU64(sb, input);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendString(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendString(sb, &value);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // struct string FormatF32Slow(struct string *stringBuffer, f32 value, u32 fractionCount)
  // Dependencies: IsStringEqual()
  if (IsStringEqualOK) {
    struct test_case {
      f32 input;
      u32 fractionCount;
      struct string expected;
    } testCases[] = {
        {
            .input = 0.99f,
            .fractionCount = 1,
            .expected = StringFromLiteral("0.9"),
        },
        {
            .input = 0.99f,
            .fractionCount = 1,
            .expected = StringFromLiteral("0.9"),
        },
        {
            .input = 1.0f,
            .fractionCount = 1,
            .expected = StringFromLiteral("1.0"),
        },
        {
            .input = 1.0f,
            .fractionCount = 2,
            .expected = StringFromLiteral("1.00"),
        },
        {
            .input = 9.05f,
            .fractionCount = 2,
            .expected = StringFromLiteral("9.05"),
        },
        {
            .input = 2.50f,
            .fractionCount = 2,
            .expected = StringFromLiteral("2.50"),
        },
        {
            .input = 2.55999f,
            .fractionCount = 2,
            .expected = StringFromLiteral("2.56"),
        },
        {
            .input = 4.99966526f,
            .fractionCount = 2,
            .expected = StringFromLiteral("4.99"),
        },
        {
            .input = 10234.293f,
            .fractionCount = 3,
            .expected = StringFromLiteral("10234.293"),
        },
        {
            .input = -0.99f,
            .fractionCount = 1,
            .expected = StringFromLiteral("-0.9"),
        },
        {
            .input = -1.0f,
            .fractionCount = 1,
            .expected = StringFromLiteral("-1.0"),
        },
        {
            .input = -1.0f,
            .fractionCount = 2,
            .expected = StringFromLiteral("-1.00"),
        },
        {
            .input = -2.50f,
            .fractionCount = 2,
            .expected = StringFromLiteral("-2.50"),
        },
        {
            .input = -2.55999f,
            .fractionCount = 2,
            .expected = StringFromLiteral("-2.56"),
        },
        // no fail cases
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      u8 buf[20];
      struct string stringBuffer = {.value = buf, .length = sizeof(buf)};

      f32 input = testCase->input;
      u32 fractionCount = testCase->fractionCount;
      struct string *expected = &testCase->expected;
      struct string value = FormatF32Slow(&stringBuffer, input, fractionCount);
      if (!IsStringEqual(&value, expected)) {
        errorCode = TEXT_TEST_ERROR_FORMATF32SLOW_EXPECTED;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendStringLiteral(sb, "\n  input (with 12 fraction count): ");
        StringBuilderAppendF32(sb, input, 12);
        StringBuilderAppendStringLiteral(sb, "\n                   fractionCount: ");
        StringBuilderAppendU64(sb, fractionCount);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendString(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendString(sb, &value);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // struct string FormatHex(struct string *stringBuffer, u64 value)
  // Dependencies: IsStringEqual()
  if (IsStringEqualOK) {
    struct test_case {
      u64 input;
      struct string expected;
    } testCases[] = {
        {
            .input = 0x0,
            .expected = StringFromLiteral("00"),
        },
        {
            .input = 0x4,
            .expected = StringFromLiteral("04"),
        },
        {
            .input = 0x0abc,
            .expected = StringFromLiteral("0abc"),
        },
        {
            .input = 0x00f2aa499b9028eaUL,
            .expected = StringFromLiteral("00f2aa499b9028ea"),
        },
        // no fail cases
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      u8 buf[18];
      struct string stringBuffer = {.value = buf, .length = sizeof(buf)};

      u64 input = testCase->input;
      struct string *expected = &testCase->expected;
      struct string value = FormatHex(&stringBuffer, input);
      if (!IsStringEqual(&value, expected)) {
        errorCode = TEXT_TEST_ERROR_FORMATHEX_EXPECTED;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendStringLiteral(sb, "\n  input:    ");
        StringBuilderAppendU64(sb, input);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendString(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendString(sb, &value);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // struct string PathGetDirectory(struct string *path)
  // Dependencies: IsStringEqual()
  if (IsStringEqualOK) {
    struct test_case {
      struct string input;
      struct string expected;
    } testCases[] = {
        {
            .input = StringFromLiteral("/usr/bin/ls"),
            .expected = StringFromLiteral("/usr/bin"),
        },
        {
            .input = StringFromLiteral("/usr"),
            .expected = StringFromLiteral("/"),
        },
        {
            .input = (struct string){.value = 0, .length = 0},
            .expected = (struct string){.value = 0, .length = 0},
        },
        {
            .input = StringFromLiteral(""),
            .expected = (struct string){.value = 0, .length = 0},
        },
        {
            .input = StringFromLiteral(" "),
            .expected = (struct string){.value = 0, .length = 0},
        },
        {
            .input = StringFromLiteral("no directory"),
            .expected = (struct string){.value = 0, .length = 0},
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *input = &testCase->input;
      struct string *expected = &testCase->expected;
      struct string value = PathGetDirectory(input);
      if (!IsStringEqual(&value, expected)) {
        errorCode = TEXT_TEST_ERROR_PATHGETDIRECTORY;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendStringLiteral(sb, "\n  input:    ");
        StringBuilderAppendPrintableString(sb, input);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendPrintableString(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendPrintableString(sb, &value);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // b8 StringSplit(struct string *string, struct string *separator, u64 *splitCount, struct string *splits)
  // Dependencies: IsStringEqual()
  if (IsStringEqualOK) {
    __cleanup_memory_temp__ memory_temp tempMemory = MemoryTempBegin(&stackMemory);

    struct test_case {
      struct string input;
      struct string separator;
      struct {
        b8 value;
        u64 splitCount;
        struct string *splits;
      } expected;
    } testCases[] = {
        {
            .input = StringFromLiteral("1 2 3"),
            .separator = StringFromLiteral(" "),
            .expected =
                {
                    .value = 1,
                    .splitCount = 3,
                    .splits =
                        (struct string[]){
                            StringFromLiteral("1"),
                            StringFromLiteral("2"),
                            StringFromLiteral("3"),
                        },
                },
        },
        {
            .input = StringFromLiteral("1xx2xx3"),
            .separator = StringFromLiteral("xx"),
            .expected =
                {
                    .value = 1,
                    .splitCount = 3,
                    .splits =
                        (struct string[]){
                            StringFromLiteral("1"),
                            StringFromLiteral("2"),
                            StringFromLiteral("3"),
                        },
                },
        },
        {
            .input = StringFromLiteral("1xoxo2xo3"),
            .separator = StringFromLiteral("xo"),
            .expected =
                {
                    .value = 1,
                    .splitCount = 4,
                    .splits =
                        (struct string[]){
                            StringFromLiteral("1"),
                            (struct string){.value = 0, .length = 0},
                            StringFromLiteral("2"),
                            StringFromLiteral("3"),
                        },
                },
        },
        {
            .input = StringFromLiteral("1xo2xo3xo"),
            .separator = StringFromLiteral("xo"),
            .expected =
                {
                    .value = 1,
                    .splitCount = 4,
                    .splits =
                        (struct string[]){
                            StringFromLiteral("1"),
                            StringFromLiteral("2"),
                            StringFromLiteral("3"),
                            (struct string){.value = 0, .length = 0},
                        },
                },
        },
        {
            .input = StringFromLiteral("Lorem ipsum dolor sit amet, consectetur adipiscing elit"),
            .separator = StringFromLiteral(" "),
            .expected =
                {
                    .value = 1,
                    .splitCount = 8,
                    .splits =
                        (struct string[]){
                            StringFromLiteral("Lorem"),
                            StringFromLiteral("ipsum"),
                            StringFromLiteral("dolor"),
                            StringFromLiteral("sit"),
                            StringFromLiteral("amet,"),
                            StringFromLiteral("consectetur"),
                            StringFromLiteral("adipiscing"),
                            StringFromLiteral("elit"),
                        },
                },
        },
        {
            .input = StringFromLiteral("Lorem ipsum dolor sit amet, consectetur adipiscing elit"),
            .separator = StringFromLiteral("no separator"),
            .expected =
                {
                    .value = 0,
                },
        },
        {
            .input = StringFromLiteral("Lorem"),
            .separator = StringFromLiteral("no separator"),
            .expected =
                {
                    .value = 0,
                },
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct string *input = &testCase->input;
      struct string *separator = &testCase->separator;

      u64 expectedSplitCount = testCase->expected.splitCount;
      b8 expected = testCase->expected.value;
      u64 splitCount;
      b8 value = StringSplit(input, separator, &splitCount, 0);
      if (value != expected || (expected && splitCount != expectedSplitCount)) {
        errorCode = expected ? TEXT_TEST_ERROR_STRINGSPLIT_EXPECTED_TRUE : TEXT_TEST_ERROR_STRINGSPLIT_EXPECTED_FALSE;

        StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
        StringBuilderAppendStringLiteral(sb, "\n      input: '");
        StringBuilderAppendString(sb, input);
        StringBuilderAppendStringLiteral(sb, "'");
        StringBuilderAppendStringLiteral(sb, "\n  separator: '");
        StringBuilderAppendString(sb, separator);
        StringBuilderAppendStringLiteral(sb, "'");
        if (value != expected) {
          StringBuilderAppendStringLiteral(sb, "\n   expected: ");
          StringBuilderAppendBool(sb, expected);
          StringBuilderAppendStringLiteral(sb, "\n        got: ");
          StringBuilderAppendBool(sb, value);
        } else {
          StringBuilderAppendStringLiteral(sb, "\n  expected split count: ");
          StringBuilderAppendU64(sb, expectedSplitCount);
          StringBuilderAppendStringLiteral(sb, "\n                   got: ");
          StringBuilderAppendU64(sb, splitCount);
        }
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      } else if (expected) {
        struct string *expectedSplits = testCase->expected.splits;
        struct string *splits = MemoryArenaPush(tempMemory.arena, sizeof(*splits) * splitCount);
        StringSplit(input, separator, &splitCount, splits);

        u32 splitFailCount = 0;

        for (u32 splitIndex = 0; splitIndex < splitCount; splitIndex++) {
          struct string *expectedSplit = expectedSplits + splitIndex;
          struct string *split = splits + splitIndex;
          if (!IsStringEqual(split, expectedSplit)) {
            errorCode =
                expected ? TEXT_TEST_ERROR_STRINGSPLIT_EXPECTED_TRUE : TEXT_TEST_ERROR_STRINGSPLIT_EXPECTED_FALSE;

            if (splitFailCount == 0) {
              StringBuilderAppendString(sb, GetTextTestErrorMessage(errorCode));
              StringBuilderAppendStringLiteral(sb, "\n        input: '");
              StringBuilderAppendString(sb, input);
              StringBuilderAppendStringLiteral(sb, "'");
              StringBuilderAppendStringLiteral(sb, "\n    separator: '");
              StringBuilderAppendString(sb, separator);
              StringBuilderAppendStringLiteral(sb, "'");
              StringBuilderAppendStringLiteral(sb, "\n  split count: ");
              StringBuilderAppendU64(sb, splitCount);
            }

            StringBuilderAppendStringLiteral(sb, "\n  index ");
            StringBuilderAppendU64(sb, splitIndex);
            StringBuilderAppendStringLiteral(sb, " is wrong");
            StringBuilderAppendStringLiteral(sb, "\n     expected: ");
            StringBuilderAppendPrintableString(sb, expectedSplit);
            StringBuilderAppendStringLiteral(sb, "\n          got: ");
            StringBuilderAppendPrintableString(sb, split);
            StringBuilderAppendStringLiteral(sb, "\n");
            struct string errorMessage = StringBuilderFlush(sb);
            PrintString(&errorMessage);

            splitFailCount++;
          }
        }
      }
    }
  }

end:
  return (int)errorCode;
}
