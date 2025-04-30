#include "math.h"
#include "print.h"
#include "string_builder.h"

#define TEST_ERROR_LIST(X)                                                                                             \
  X(CLAMP, "Input must be clamped")                                                                                    \
  X(IS_POWER_OF_TWO_EXPECTED_TRUE, "Input must be power of two")                                                       \
  X(IS_POWER_OF_TWO_EXPECTED_FALSE, "Input must NOT be power of two")                                                  \
  X(V2_ADD, "adding two v2 must be true")                                                                              \
  X(V2_SUB, "subtracting two v2 must be true")                                                                         \
  X(V2_SCALE, "scaling v2 must be true")                                                                               \
  X(V2_DOT, "dot product of two v2 must be true")                                                                      \
  X(V2_HADAMARD, "hadamard product of two v2 must be true")                                                            \
  X(V2_PERP, "perpendicular of v2 must be true")                                                                       \
  X(V2_LENGTH_SQUARE, "v2 length squared must be true")                                                                \
  X(V2_LENGTH, "v2 length must be true")                                                                               \
  X(V2_NORMALIZE, "normalizing v2 must be true")                                                                       \
  X(V2_NEG, "negative of v2 must be true")                                                                             \
  X(IS_POINT_INSIDE_RECT_EXPECTED_TRUE, "point must be inside rectangle")                                              \
  X(IS_POINT_INSIDE_RECT_EXPECTED_FALSE, "point must NOT be inside rectangle")                                         \
  X(IS_AABB_OVERLAPPING_EXPECTED_TRUE, "AABB must intersect with another AABB")                                        \
  X(IS_AABB_OVERLAPPING_EXPECTED_FALSE, "AABB must NOT intersect with another AABB")

enum math_test_error {
  MATH_TEST_ERROR_NONE = 0,
#define X(code, msg) MATH_TEST_ERROR_##code,
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
StringBuilderAppendErrorMessage(string_builder *sb, enum math_test_error errorCode)
{
  struct error {
    enum math_test_error code;
    struct string message;
  } errors[] = {
#define X(c, description) {.code = MATH_TEST_ERROR_##c, .message = StringFromLiteral(description)},
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

internalfn void
StringBuilderAppendBool(string_builder *sb, b8 value)
{
  struct string message = value ? StringFromLiteral("true") : StringFromLiteral("false");
  StringBuilderAppendString(sb, &message);
}

internalfn void
StringBuilderAppendV2(string_builder *sb, struct v2 value)
{
  StringBuilderAppendF32(sb, value.x, 2);
  StringBuilderAppendStringLiteral(sb, ", ");
  StringBuilderAppendF32(sb, value.y, 2);
}

internalfn void
StringBuilderAppendRect(string_builder *sb, struct rect value)
{
  StringBuilderAppendStringLiteral(sb, "[ (");
  StringBuilderAppendV2(sb, value.min);
  StringBuilderAppendStringLiteral(sb, "), (");
  StringBuilderAppendV2(sb, value.max);
  StringBuilderAppendStringLiteral(sb, ") ]");
}

int
main(void)
{
  enum math_test_error errorCode = MATH_TEST_ERROR_NONE;

  // setup
  enum { KILOBYTES = (1 << 10) };
  u8 stackBuffer[8 * KILOBYTES];
  memory_arena stackMemory = {
      .block = stackBuffer,
      .total = ARRAY_COUNT(stackBuffer),
  };

  string_builder *sb = MakeStringBuilder(&stackMemory, 1024, 32);

  // ClampU32(u32 value, u32 min, u32 max)
  {
    struct test_case {
      u32 input;
      u32 min;
      u32 max;
      u32 expected;
    } testCases[] = {
        {
            .input = 4,
            .min = 3,
            .max = 5,
            .expected = 4,
        },
        {
            .input = 3,
            .min = 3,
            .max = 5,
            .expected = 3,
        },
        {
            .input = 0,
            .min = 3,
            .max = 5,
            .expected = 3,
        },
        {
            .input = 5,
            .min = 3,
            .max = 5,
            .expected = 5,
        },
        {
            .input = 10,
            .min = 3,
            .max = 5,
            .expected = 5,
        },
    };
    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      u32 expected = testCase->expected;
      u32 input = testCase->input;
      u32 min = testCase->min;
      u32 max = testCase->max;
      u32 got = ClampU32(input, min, max);
      if (got != expected) {
        errorCode = MATH_TEST_ERROR_CLAMP;
        StringBuilderAppendErrorMessage(sb, errorCode);
        StringBuilderAppendStringLiteral(sb, "\n  min: ");
        StringBuilderAppendU32(sb, min);
        StringBuilderAppendStringLiteral(sb, " max: ");
        StringBuilderAppendU32(sb, max);
        StringBuilderAppendStringLiteral(sb, " input: ");
        StringBuilderAppendU32(sb, input);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendU32(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendU32(sb, got);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string message = StringBuilderFlush(sb);
        PrintString(&message);
      }
    }
  }

  // IsPowerOfTwo(u64 value)
  {
    struct test_case {
      u64 input;
      b8 expected;
    } testCases[] = {
        {
            .input = 3,
            .expected = 0,
        },
        {
            .input = 4,
            .expected = 1,
        },
        {
            .input = 5,
            .expected = 0,
        },
        {
            .input = 31,
            .expected = 0,
        },
        {
            .input = 32,
            .expected = 1,
        },
        {
            .input = 33,
            .expected = 0,
        },
    };
    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      b8 expected = testCase->expected;
      u64 input = testCase->input;
      b8 got = IsPowerOfTwo(input);
      if (got != expected) {
        errorCode =
            expected ? MATH_TEST_ERROR_IS_POWER_OF_TWO_EXPECTED_TRUE : MATH_TEST_ERROR_IS_POWER_OF_TWO_EXPECTED_FALSE;
        StringBuilderAppendErrorMessage(sb, errorCode);
        StringBuilderAppendStringLiteral(sb, "\n     input: ");
        StringBuilderAppendU64(sb, input);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendBool(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendBool(sb, got);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string message = StringBuilderFlush(sb);
        PrintString(&message);
      }
    }
  }

  // v2_add(v2 a, v2 b)
  {
    struct test_case {
      struct v2 a;
      struct v2 b;
      struct v2 expected;
    } testCases[] = {
        {
            .a = V2(3.0f, 4.0f),
            .b = V2(9.0f, 12.0f),
            .expected = V2(12.0f, 16.0f),
        },
        {
            .a = V2(3.0f, 4.0f),
            .b = V2(-3.0f, -4.0f),
            .expected = V2(0.0f, 0.0f),
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct v2 expected = testCase->expected;
      struct v2 a = testCase->a;
      struct v2 b = testCase->b;
      struct v2 got = v2_add(a, b);
      if (got.x != expected.x || got.y != expected.y) {
        errorCode = MATH_TEST_ERROR_V2_ADD;
        StringBuilderAppendErrorMessage(sb, errorCode);
        StringBuilderAppendStringLiteral(sb, "\n         a: ");
        StringBuilderAppendV2(sb, a);
        StringBuilderAppendStringLiteral(sb, "\n         b: ");
        StringBuilderAppendV2(sb, b);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendV2(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendV2(sb, got);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string message = StringBuilderFlush(sb);
        PrintString(&message);
      }
    }
  }

  // v2_sub(v2 a, v2 b)
  {
    struct test_case {
      struct v2 a;
      struct v2 b;
      struct v2 expected;
    } testCases[] = {
        {
            .a = V2(9.0f, 12.0f),
            .b = V2(3.0f, 4.0f),
            .expected = V2(6.0f, 8.0f),
        },
        {
            .a = V2(3.0f, 4.0f),
            .b = V2(-3.0f, -4.0f),
            .expected = V2(6.0f, 8.0f),
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct v2 expected = testCase->expected;
      struct v2 a = testCase->a;
      struct v2 b = testCase->b;
      struct v2 got = v2_sub(a, b);
      if (got.x != expected.x || got.y != expected.y) {
        errorCode = MATH_TEST_ERROR_V2_SUB;
        StringBuilderAppendErrorMessage(sb, errorCode);
        StringBuilderAppendStringLiteral(sb, "\n         a: ");
        StringBuilderAppendV2(sb, a);
        StringBuilderAppendStringLiteral(sb, "\n         b: ");
        StringBuilderAppendV2(sb, b);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendV2(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendV2(sb, got);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string message = StringBuilderFlush(sb);
        PrintString(&message);
      }
    }
  }

  // v2_scale(v2 a, f32 scaler)
  {
    struct test_case {
      struct v2 a;
      f32 scaler;
      struct v2 expected;
    } testCases[] = {
        {
            .a = V2(3.0f, 4.0f),
            .scaler = 5.0f,
            .expected = V2(15.0f, 20.0f),
        },
        {
            .a = V2(1.0f, -1.0f),
            .scaler = 5.0f,
            .expected = V2(5.0f, -5.0f),
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct v2 expected = testCase->expected;
      struct v2 a = testCase->a;
      f32 scaler = testCase->scaler;
      struct v2 got = v2_scale(a, scaler);
      if (got.x != expected.x || got.y != expected.y) {
        errorCode = MATH_TEST_ERROR_V2_SCALE;
        StringBuilderAppendErrorMessage(sb, errorCode);
        StringBuilderAppendStringLiteral(sb, "\n         a: ");
        StringBuilderAppendV2(sb, a);
        StringBuilderAppendStringLiteral(sb, "\n    scaler: ");
        StringBuilderAppendF32(sb, scaler, 2);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendV2(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendV2(sb, got);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string message = StringBuilderFlush(sb);
        PrintString(&message);
      }
    }
  }

  // v2_dot(v2 a, v2 b)
  {
    struct test_case {
      struct v2 a;
      struct v2 b;
      f32 expected;
    } testCases[] = {
        {
            .a = V2(3.0f, 4.0f),
            .b = V2(9.0f, 12.0f),
            .expected = 3.0f * 9.0f + 4.0f * 12.0f,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      f32 expected = testCase->expected;
      struct v2 a = testCase->a;
      struct v2 b = testCase->b;
      f32 got = v2_dot(a, b);
      if (got != expected) {
        errorCode = MATH_TEST_ERROR_V2_DOT;
        StringBuilderAppendErrorMessage(sb, errorCode);
        StringBuilderAppendStringLiteral(sb, "\n         a: ");
        StringBuilderAppendV2(sb, a);
        StringBuilderAppendStringLiteral(sb, "\n         b: ");
        StringBuilderAppendV2(sb, b);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendF32(sb, expected, 2);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendF32(sb, got, 2);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string message = StringBuilderFlush(sb);
        PrintString(&message);
      }
    }
  }

  // v2_hadamard(v2 a, v2 b)
  {
    struct test_case {
      struct v2 a;
      struct v2 b;
      struct v2 expected;
    } testCases[] = {
        {
            .a = V2(3.0f, 4.0f),
            .b = V2(9.0f, 12.0f),
            .expected = V2(27.0f, 48.0f),
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct v2 expected = testCase->expected;
      struct v2 a = testCase->a;
      struct v2 b = testCase->b;
      struct v2 got = v2_hadamard(a, b);
      if (got.x != expected.x || got.y != expected.y) {
        errorCode = MATH_TEST_ERROR_V2_HADAMARD;
        StringBuilderAppendErrorMessage(sb, errorCode);
        StringBuilderAppendStringLiteral(sb, "\n         a: ");
        StringBuilderAppendV2(sb, a);
        StringBuilderAppendStringLiteral(sb, "\n         b: ");
        StringBuilderAppendV2(sb, b);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendV2(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendV2(sb, got);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string message = StringBuilderFlush(sb);
        PrintString(&message);
      }
    }
  }

  // v2_perp(v2 a)
  {
    struct test_case {
      struct v2 a;
      struct v2 expected;
    } testCases[] = {
        {
            .a = V2(1.0f, 2.0f),
            .expected = V2(-2.0f, 1.0f),
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct v2 expected = testCase->expected;
      struct v2 a = testCase->a;
      struct v2 got = v2_perp(a);
      if (got.x != expected.x || got.y != expected.y) {
        errorCode = MATH_TEST_ERROR_V2_PERP;
        StringBuilderAppendErrorMessage(sb, errorCode);
        StringBuilderAppendStringLiteral(sb, "\n         a: ");
        StringBuilderAppendV2(sb, a);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendV2(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendV2(sb, got);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string message = StringBuilderFlush(sb);
        PrintString(&message);
      }
    }
  }

  // v2_length_square(v2 a)
  {
    struct test_case {
      struct v2 a;
      f32 expected;
    } testCases[] = {
        {
            .a = V2(3.0f, 4.0f),
            .expected = 5.0f * 5.0f,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      f32 expected = testCase->expected;
      struct v2 a = testCase->a;
      f32 got = v2_length_square(a);
      if (got != expected) {
        errorCode = MATH_TEST_ERROR_V2_LENGTH_SQUARE;
        StringBuilderAppendErrorMessage(sb, errorCode);
        StringBuilderAppendStringLiteral(sb, "\n         a: ");
        StringBuilderAppendV2(sb, a);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendF32(sb, expected, 2);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendF32(sb, got, 2);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string message = StringBuilderFlush(sb);
        PrintString(&message);
      }
    }
  }

  // v2_length(v2 a)
  {
    struct test_case {
      struct v2 a;
      f32 expected;
    } testCases[] = {
        {
            .a = V2(3.0f, 4.0f),
            .expected = 5.0f,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      f32 expected = testCase->expected;
      struct v2 a = testCase->a;
      f32 got = v2_length(a);
      if (got != expected) {
        errorCode = MATH_TEST_ERROR_V2_LENGTH;
        StringBuilderAppendErrorMessage(sb, errorCode);
        StringBuilderAppendStringLiteral(sb, "\n         a: ");
        StringBuilderAppendV2(sb, a);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendF32(sb, expected, 2);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendF32(sb, got, 2);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string message = StringBuilderFlush(sb);
        PrintString(&message);
      }
    }
  }

  // v2_normalize(v2 a)
  {
    struct test_case {
      struct v2 a;
      struct v2 expected;
    } testCases[] = {
        {
            .a = V2(3.0f, 4.0f),
            .expected = V2(3.0f / 5.0f, 4.0f / 5.0f),
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct v2 expected = testCase->expected;
      struct v2 a = testCase->a;
      struct v2 got = v2_normalize(a);
      if (got.x != expected.x || got.y != expected.y) {
        errorCode = MATH_TEST_ERROR_V2_NORMALIZE;
        StringBuilderAppendErrorMessage(sb, errorCode);
        StringBuilderAppendStringLiteral(sb, "\n         a: ");
        StringBuilderAppendV2(sb, a);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendV2(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendV2(sb, got);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string message = StringBuilderFlush(sb);
        PrintString(&message);
      }
    }
  }

  // v2_neg(v2 a)
  {
    struct test_case {
      struct v2 a;
      struct v2 expected;
    } testCases[] = {
        {
            .a = V2(3.0f, 4.0f),
            .expected = V2(-3.0f, -4.0f),
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct v2 expected = testCase->expected;
      struct v2 a = testCase->a;
      struct v2 got = v2_neg(a);
      if (got.x != expected.x || got.y != expected.y) {
        errorCode = MATH_TEST_ERROR_V2_NEG;
        StringBuilderAppendErrorMessage(sb, errorCode);
        StringBuilderAppendStringLiteral(sb, "\n         a: ");
        StringBuilderAppendV2(sb, a);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendV2(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendV2(sb, got);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string message = StringBuilderFlush(sb);
        PrintString(&message);
      }
    }
  }

  // IsPointInsideRect(struct rect rect, v2 point)
  {
    struct test_case {
      struct v2 point;
      struct rect rect;
      b8 expected;
    } testCases[] = {
        {
            .point = V2(0.0f, 0.0f),
            .rect =
                {
                    .min = V2(0.0f, 0.0f),
                    .max = V2(10.0f, 10.0f),
                },
            .expected = 1,
        },
        {
            .point = V2(3.0f, 4.0f),
            .rect =
                {
                    .min = V2(0.0f, 0.0f),
                    .max = V2(10.0f, 10.0f),
                },
            .expected = 1,
        },
        {
            .point = V2(10.0f, 10.0f),
            .rect =
                {
                    .min = V2(0.0f, 0.0f),
                    .max = V2(10.0f, 10.0f),
                },
            .expected = 0,
        },
        {
            .point = V2(-0.1f, -0.1f),
            .rect =
                {
                    .min = V2(0.0f, 0.0f),
                    .max = V2(10.0f, 10.0f),
                },
            .expected = 0,
        },
        {
            .point = V2(50.0f, 50.0f),
            .rect =
                {
                    .min = V2(0.0f, 0.0f),
                    .max = V2(10.0f, 10.0f),
                },
            .expected = 0,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      b8 expected = testCase->expected;
      struct rect rect = testCase->rect;
      struct v2 point = testCase->point;
      b8 got = IsPointInsideRect(point, rect);
      if (got != expected) {
        errorCode = expected ? MATH_TEST_ERROR_IS_POINT_INSIDE_RECT_EXPECTED_TRUE
                             : MATH_TEST_ERROR_IS_POINT_INSIDE_RECT_EXPECTED_FALSE;
        StringBuilderAppendErrorMessage(sb, errorCode);
        StringBuilderAppendStringLiteral(sb, "\n      rect: ");
        StringBuilderAppendRect(sb, rect);
        StringBuilderAppendStringLiteral(sb, "\n     point: ");
        StringBuilderAppendV2(sb, point);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendBool(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendBool(sb, got);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string message = StringBuilderFlush(sb);
        PrintString(&message);
      }
    }
  }

  // IsAABBOverlapping(struct rect a, struct rect b)
  {
    struct test_case {
      struct rect a;
      struct rect b;
      b8 expected;
    } testCases[] = {
        {
            .a =
                {
                    .min = V2(0.0f, 0.0f),
                    .max = V2(10.0f, 10.0f),
                },
            .b =
                {
                    .min = V2(0.0f, 0.0f),
                    .max = V2(5.0f, 5.0f),
                },
            .expected = 1,
        },
        {
            .a =
                {
                    .min = V2(0.0f, 0.0f),
                    .max = V2(10.0f, 10.0f),
                },
            .b =
                {
                    .min = V2(10.0f, 10.0f),
                    .max = V2(100.0f, 100.0f),
                },
            .expected = 0,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      b8 expected = testCase->expected;
      struct rect a = testCase->a;
      struct rect b = testCase->b;
      b8 got = IsAABBOverlapping(a, b);
      if (got != expected) {
        errorCode = expected ? MATH_TEST_ERROR_IS_AABB_OVERLAPPING_EXPECTED_TRUE
                             : MATH_TEST_ERROR_IS_AABB_OVERLAPPING_EXPECTED_FALSE;
        StringBuilderAppendErrorMessage(sb, errorCode);
        StringBuilderAppendStringLiteral(sb, "\n         a: ");
        StringBuilderAppendRect(sb, a);

        StringBuilderAppendStringLiteral(sb, "\n         b: ");
        StringBuilderAppendRect(sb, b);

        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendBool(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n       got: ");
        StringBuilderAppendBool(sb, got);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string message = StringBuilderFlush(sb);
        PrintString(&message);
      }
    }
  }

  return (int)errorCode;
}
