#include "memory.h"
#include "platform.h"
#include "string_builder.h"
#include "teju.h"

#define TEST_ERROR_LIST(X)                                                                                             \
  X(FORMATF32_EXPECTED_SUCCESS, "Converting f32 to string failed")                                                     \
  X(FORMATF32_EXPECTED_FAILURE, "Converting f32 to string must fail")

enum teju_test_error {
  TEJU_TEST_ERROR_NONE = 0,
#define X(name, message) TEJU_TEST_ERROR_##name,
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
StringBuilderAppendPrintableString(string_builder *sb, struct string *string)
{
  if (IsStringNull(string))
    StringBuilderAppendStringLiteral(sb, "(NULL)");
  else if (IsStringNull(string))
    StringBuilderAppendStringLiteral(sb, "(EMPTY)");
  else if (IsStringEqual(string, &StringFromLiteral(" ")))
    StringBuilderAppendStringLiteral(sb, "(SPACE)");
  else
    StringBuilderAppendString(sb, string);
}

internalfn void
StringBuilderAppendErrorMessage(string_builder *sb, enum teju_test_error errorCode)
{

  struct error {
    enum teju_test_error code;
    struct string message;
  } errors[] = {
#define X(c, description) {.code = TEJU_TEST_ERROR_##c, .message = StringFromLiteral(description)},
      TEST_ERROR_LIST(X)
#undef X
  };

  struct string *message = &StringFromLiteral("Unknown error");
  for (u32 errorIndex = 0; errorIndex < ARRAY_COUNT(errors); errorIndex++) {
    struct error *error = errors + errorIndex;
    if (errorCode == error->code) {
      message = &error->message;
      break;
    }
  }
  StringBuilderAppendString(sb, message);
}

int
main(void)
{
  enum teju_test_error errorCode = TEJU_TEST_ERROR_NONE;

  // setup
  enum { KILOBYTES = (1 << 10) };
  u8 stackBuffer[8 * KILOBYTES];
  memory_arena stackMemory = {
      .block = stackBuffer,
      .total = ARRAY_COUNT(stackBuffer),
  };

  string_builder *sb = MakeStringBuilder(&stackMemory, 1024, 32);

  // FormatF32(struct string *stringBuffer, f32 value, u32 fractionCount)
  {
    struct test_case {
      u32 bufferSize;
      f32 value;
      u32 fractionCount;
      struct string expected;
    } testCases[] = {
        {
            .bufferSize = 3,
            .value = 0.0f,
            .fractionCount = 1,
            .expected = StringFromLiteral("0.0"),
        },
        {
            .bufferSize = 3,
            .value = 0.99f,
            .fractionCount = 1,
            .expected = StringFromLiteral("0.9"),
        },
        {
            .bufferSize = 3,
            .value = 1.0f,
            .fractionCount = 1,
            .expected = StringFromLiteral("1.0"),
        },
        {
            .bufferSize = 4,
            .value = 0.1f,
            .fractionCount = 2,
            .expected = StringFromLiteral("0.10"),
        },
        {
            .bufferSize = 4,
            .value = 9.05f,
            .fractionCount = 2,
            .expected = StringFromLiteral("9.05"),
        },
        {
            .bufferSize = 4,
            .value = 2.50f,
            .fractionCount = 2,
            .expected = StringFromLiteral("2.50"),
        },
        {
            .bufferSize = 4,
            .value = 2.55999f,
            .fractionCount = 2,
            .expected = StringFromLiteral("2.55"),
        },
        {
            .bufferSize = 4,
            .value = 2.55999f,
            .fractionCount = 2,
            .expected = StringFromLiteral("2.55"),
        },
        {
            .bufferSize = 4,
            .value = 4.99966526f,
            .fractionCount = 2,
            .expected = StringFromLiteral("4.99"),
        },
        {
            .bufferSize = 9,
            .value = 10234.293f,
            .fractionCount = 3,
            .expected = StringFromLiteral("10234.293"),
        },
        {
            .bufferSize = 4,
            .value = -0.99f,
            .fractionCount = 1,
            .expected = StringFromLiteral("-0.9"),
        },
        {
            .bufferSize = 4,
            .value = -1.0f,
            .fractionCount = 1,
            .expected = StringFromLiteral("-1.0"),
        },
        {
            .bufferSize = 5,
            .value = -1.0f,
            .fractionCount = 2,
            .expected = StringFromLiteral("-1.00"),
        },
        {
            .bufferSize = 5,
            .value = -0.1f,
            .fractionCount = 2,
            .expected = StringFromLiteral("-0.10"),
        },
        {
            .bufferSize = 5,
            .value = -2.50f,
            .fractionCount = 2,
            .expected = StringFromLiteral("-2.50"),
        },
        {
            .bufferSize = 5,
            .value = -2.55999f,
            .fractionCount = 2,
            .expected = StringFromLiteral("-2.55"),
        },
        {
            .bufferSize = 4,
            .value = 3.76991844e-25f,
            .fractionCount = 2,
            .expected = StringFromLiteral("0.00"),
        },
        {
            .bufferSize = 41,
            .value = F32_MAX,
            .fractionCount = 1,
            .expected = StringFromLiteral("340282350000000000000000000000000000000.0"),
        },
        {
            .bufferSize = 53,
            .value = F32_MIN,
            .fractionCount = 51,
            .expected = StringFromLiteral("0.000000000000000000000000000000000000000000011754944"),
        },
        {
            .bufferSize = 42,
            .value = F32_LOWEST,
            .fractionCount = 1,
            .expected = StringFromLiteral("-340282350000000000000000000000000000000.0"),
        },
        {
            .bufferSize = 1,
            .value = 0.0f,
            .fractionCount = 1,
            .expected = StringNull(),
        },
        {
            .bufferSize = 2,
            .value = 0.0f,
            .fractionCount = 1,
            .expected = StringNull(),
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;
      f32 value = testCase->value;
      u32 fractionCount = testCase->fractionCount;
      struct string *expected = &testCase->expected;
      u32 bufferSize = testCase->bufferSize;

      memory_temp tempMemory = MemoryTempBegin(&stackMemory);
      struct string *buffer = MakeString(tempMemory.arena, bufferSize);
      struct string *randomString =
          &StringFromLiteral("PTmTivmdRIxMFroaLtsVIWooGFTfTlEKueBimsPIzMGKRczMJvDCdwyWiNEYKCoU");
      debug_assert(buffer->length <= randomString->length && "could not fill the buffer with invalid data");
      struct string randomStringSlice = StringSlice(randomString, 0, buffer->length);
      memcpy(buffer->value, randomStringSlice.value, randomStringSlice.length);

      struct string got = FormatF32(buffer, value, fractionCount);

      if (IsStringNotEqual(&got, expected)) {
        errorCode = expected->length != 0 ? TEJU_TEST_ERROR_FORMATF32_EXPECTED_SUCCESS
                                          : TEJU_TEST_ERROR_FORMATF32_EXPECTED_FAILURE;

        StringBuilderAppendErrorMessage(sb, errorCode);
        StringBuilderAppendStringLiteral(sb, "\n expected: ");
        StringBuilderAppendPrintableString(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n      got: ");
        StringBuilderAppendPrintableString(sb, &got);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string message = StringBuilderFlush(sb);
        PrintString(&message);
      }

      MemoryTempEnd(&tempMemory);
    }
  }

  return (int)errorCode;
}
