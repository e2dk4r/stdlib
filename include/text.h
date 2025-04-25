#pragma once

#include "assert.h"
#include "math.h"
#include "memory.h"
#include "type.h"

struct string {
  u8 *value;
  u64 length;
};

typedef struct string string;

/*
 * Only accepts readonly, compile-time C strings.
 * Do NOT pass char
 */
#define StringFromLiteral(source)                                                                                      \
  ((struct string){                                                                                                    \
      .value = (u8 *)source,                                                                                           \
      .length = sizeof(source) - 1,                                                                                    \
  })

static inline struct string
StringFromBuffer(u8 *buffer, u64 length)
{
  debug_assert(buffer != 0);
  struct string string = {
      .value = buffer,
      .length = length,
  };
  return string;
}

static inline struct string
StringFromZeroTerminated(u8 *src, u64 max)
{
  debug_assert(src != 0);
  struct string string = {.value = 0, .length = 0};

  string.value = src;

  while (*src) {
    string.length++;
    if (string.length == max)
      break;
    src++;
  }

  return string;
}

static inline struct string *
MakeString(memory_arena *arena, u64 length)
{
  struct string *result = MemoryArenaPush(arena, sizeof(*result));
  result->length = length;
  result->value = MemoryArenaPush(arena, result->length);
  return result;
}

static inline struct string
StringSlice(struct string *string, u64 startIndex, u64 endIndex)
{
  debug_assert(string != 0);
  debug_assert(startIndex < endIndex);
  debug_assert(endIndex - startIndex <= string->length);
  struct string sliced = {.value = string->value + startIndex, .length = endIndex - startIndex};
  return sliced;
}

static inline b8
IsStringNull(struct string *string)
{
  return string && string->value == 0;
}

static inline b8
IsStringEmpty(struct string *string)
{
  return string && string->value != 0 && string->length == 0;
}

static inline b8
IsStringNullOrEmpty(struct string *string)
{
  return IsStringNull(string) || IsStringEmpty(string);
}

static inline b8
IsStringEqual(struct string *left, struct string *right)
{
  if (!left || !right || (IsStringNull(left) && IsStringEmpty(right)) || (IsStringEmpty(left) && IsStringNull(right)))
    return 0;

  if ((IsStringNull(left) && IsStringNull(right)) || (IsStringEmpty(left) && IsStringEmpty(right)))
    return 1;

  if (left->length != right->length)
    return 0;

  // TODO: speed up this by using SSE or AVX
  for (u64 index = 0; index < left->length; index++) {
    if (left->value[index] != right->value[index])
      return 0;
  }

  return 1;
}

static inline b8
IsStringNotEqual(struct string *left, struct string *right)
{
  return !IsStringEqual(left, right);
}

static u8
ToLowerASCII(u8 character)
{
  if (character >= 'A' && character <= 'Z')
    return character + 0x20;
  return character;
}

static inline b8
IsStringEqualIgnoreCase(struct string *left, struct string *right)
{
  if (!left || !right || (IsStringNull(left) && IsStringEmpty(right)) || (IsStringEmpty(left) && IsStringNull(right)))
    return 0;

  if ((IsStringNull(left) && IsStringNull(right)) || (IsStringEmpty(left) && IsStringEmpty(right)))
    return 1;

  if (left->length != right->length)
    return 0;

  // TODO: speed up this by using SSE or AVX
  for (u64 index = 0; index < left->length; index++) {
    if (ToLowerASCII(left->value[index]) != ToLowerASCII(right->value[index]))
      return 0;
  }

  return 1;
}

static inline b8
IsStringContains(struct string *string, struct string *search)
{
  if (!string || !search || string->length < search->length)
    return 0;

  for (u64 stringIndex = 0; stringIndex < string->length; stringIndex++) {
    if (stringIndex + search->length > string->length)
      return 0;

    struct string substring = StringSlice(string, stringIndex, stringIndex + search->length);
    b8 isFound = 1;
    for (u64 index = 0; index < substring.length; index++) {
      if (substring.value[index] != search->value[index]) {
        isFound = 0;
        break;
      }
    }

    if (!isFound)
      continue;

    return 1;
  }

  return 0;
}

static inline b8
IsStringStartsWith(struct string *string, struct string *search)
{
  if (!string || !search || search->length == 0 || string->length < search->length)
    return 0;

  struct string substring = StringSlice(string, 0, search->length);
  for (u64 index = 0; index < substring.length; index++) {
    if (substring.value[index] != search->value[index])
      return 0;
  }
  return 1;
}

static inline b8
IsStringEndsWith(struct string *string, struct string *search)
{
  if (!string || !search || search->length == 0 || string->length < search->length)
    return 0;

  struct string substring = StringSlice(string, string->length - search->length, string->length);
  for (u64 index = 0; index < substring.length; index++) {
    if (substring.value[index] != search->value[index])
      return 0;
  }
  return 1;
}

static struct string
StringStripWhitespace(struct string *string)
{
  struct string result = {.value = 0, .length = 0};
  if (!string || string->length == 0)
    return result;

  b8 whitespace[U8_MAX] = {
      // horizontal tab, line feed, vertical tab, form feed, carriage return, space
      0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, // 0x00
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x10
      1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x20
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x30
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x40
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x50
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x60
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x70
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x80
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x90
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xa0
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xb0
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xc0
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xd0
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xe0
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0     // 0xf0
  };

  u64 position = 0;
  while (position < string->length) {
    u8 character = string->value[position];
    if (!whitespace[character])
      break;
    position++;
  }
  u64 start = position;

  position = string->length - 1;
  while (position > start) {
    u8 character = string->value[position];
    if (!whitespace[character])
      break;
    position--;
  }
  u64 end = position + 1;

  if (end == start)
    return result;

  result.value = string->value + start;
  result.length = end - start;
  return result;
}

struct duration {
  u64 ns;
};

static inline void
DurationAddRef(struct duration *duration, struct duration elapsed)
{
  duration->ns += elapsed.ns;
}

static inline struct duration
DurationAddList(struct duration *list, u64 count)
{
  struct duration result = {.ns = 0};
  for (u64 index = 0; index < count; index++) {
    struct duration *duration = list + index;
    DurationAddRef(&result, *duration);
  }
  return result;
}

#define DurationAddMultiple(...)                                                                                       \
  DurationAddList((struct duration[]){__VA_ARGS__}, sizeof((struct duration[]){__VA_ARGS__}) / sizeof(struct duration))

static inline void
DurationSubRef(struct duration *duration, struct duration elapsed)
{
  duration->ns -= elapsed.ns;
}

static inline struct duration
DurationSubList(struct duration *list, u64 count)
{
  struct duration result = {.ns = 0};
  for (u64 index = 0; index < count; index++) {
    struct duration *duration = list + index;
    DurationSubRef(&result, *duration);
  }
  return result;
}

#define DurationSubMultiple(...)                                                                                       \
  DurationSubList((struct duration[]){__VA_ARGS__}, sizeof((struct duration[]){__VA_ARGS__}) / sizeof(struct duration))

static inline struct duration
DurationInNanoseconds(u64 nanoseconds)
{
  return (struct duration){
      .ns = nanoseconds,
  };
}

static inline struct duration
DurationInMicroseconds(u64 microseconds)
{
  return (struct duration){
      .ns = microseconds * 1000 /* 1e3 */,
  };
}

static inline struct duration
DurationInMilliseconds(u64 milliseconds)
{
  return (struct duration){
      .ns = milliseconds * 1000000 /* 1e6 */,
  };
}

static inline struct duration
DurationInSeconds(u64 seconds)
{
  return (struct duration){
      .ns = seconds * 1000000000 /* 1e9 */,
  };
}

static inline struct duration
DurationInMinutes(u64 minutes)
{
  return (struct duration){
      .ns = minutes * 1000000000 /* 1e9 */ * 60,
  };
}

static inline struct duration
DurationInHours(u64 hours)
{
  return (struct duration){
      .ns = hours * 1000000000 /* 1e9 */ * 60 * 60,
  };
}

static inline struct duration
DurationInDays(u64 days)
{
  return (struct duration){
      .ns = days * 1000000000 /* 1e9 */ * 60 * 60 * 24,
  };
}

static inline struct duration
DurationInWeeks(u64 weeks)
{
  return (struct duration){
      .ns = weeks * 1000000000 /* 1e9 */ * 60 * 60 * 24 * 7,
  };
}

static inline struct duration
DurationBetweenSeconds(u64 start, u64 end)
{
  debug_assert(end >= start);
  return (struct duration){
      .ns = (end - start) * 1000000000 /* 1e9 */,
  };
}

static inline struct duration
DurationBetweenNanoseconds(u64 start, u64 end)
{
  debug_assert(end >= start);
  return (struct duration){
      .ns = end - start,
  };
}

static inline b8
ParseDuration(struct string *string, struct duration *duration)
{
  if (!string || string->length == 0 || string->length < 3)
    return 0;

  // | Duration | Length      |
  // |----------|-------------|
  // | ns       | nanosecond  |
  // | us       | microsecond |
  // | ms       | millisecond |
  // | sec      | second      |
  // | min      | minute      |
  // | hr       | hour        |
  // | day      | day         |
  // | wk       | week        |
  struct string nanosecondUnitString = StringFromLiteral("ns");
  struct string microsecondUnitString = StringFromLiteral("us");
  struct string millisocondUnitString = StringFromLiteral("ms");
  struct string secondUnitString = StringFromLiteral("sec");
  struct string minuteUnitString = StringFromLiteral("min");
  struct string hourUnitString = StringFromLiteral("hr");
  struct string dayUnitString = StringFromLiteral("day");
  struct string weekUnitString = StringFromLiteral("wk");

  b8 isUnitExistsInString =
      IsStringContains(string, &secondUnitString) || IsStringContains(string, &minuteUnitString) ||
      IsStringContains(string, &hourUnitString) || IsStringContains(string, &nanosecondUnitString) ||
      IsStringContains(string, &microsecondUnitString) || IsStringContains(string, &millisocondUnitString) ||
      IsStringContains(string, &dayUnitString) || IsStringContains(string, &weekUnitString);
  if (!isUnitExistsInString) {
    return 0;
  }

  struct duration parsed = {.ns = 0};
  u64 value = 0;
  for (u64 index = 0; index < string->length; index++) {
    u8 digitCharacter = string->value[index];
    b8 isDigit = digitCharacter >= '0' && digitCharacter <= '9';
    if (!isDigit) {
      // - get unit
      struct string unitString = {.value = string->value + index, .length = string->length - index};
      if (/* unit: nanosecond */ IsStringStartsWith(&unitString, &nanosecondUnitString)) {
        DurationAddRef(&parsed, DurationInNanoseconds(value));
        index += nanosecondUnitString.length - 1;
      } else if (/* unit: microsecond */ IsStringStartsWith(&unitString, &microsecondUnitString)) {
        DurationAddRef(&parsed, DurationInMicroseconds(value));
        index += microsecondUnitString.length - 1;
      } else if (/* unit: millisecond */ IsStringStartsWith(&unitString, &millisocondUnitString)) {
        DurationAddRef(&parsed, DurationInMilliseconds(value));
        index += millisocondUnitString.length - 1;
      } else if (/* unit: second */ IsStringStartsWith(&unitString, &secondUnitString)) {
        DurationAddRef(&parsed, DurationInSeconds(value));
        index += secondUnitString.length - 1;
      } else if (/* unit: minute */ IsStringStartsWith(&unitString, &minuteUnitString)) {
        DurationAddRef(&parsed, DurationInMinutes(value));
        index += minuteUnitString.length;
      } else if (/* unit: hour */ IsStringStartsWith(&unitString, &hourUnitString)) {
        DurationAddRef(&parsed, DurationInHours(value));
        index += hourUnitString.length - 1;
      } else if (/* unit: day */ IsStringStartsWith(&unitString, &dayUnitString)) {
        DurationAddRef(&parsed, DurationInDays(value));
        index += dayUnitString.length - 1;
      } else if (/* unit: week */ IsStringStartsWith(&unitString, &weekUnitString)) {
        DurationAddRef(&parsed, DurationInNanoseconds(value));
        index += weekUnitString.length - 1;
      } else {
        // unsupported unit
        return 0;
      }

      // - reset value
      value = 0;
      continue;
    }

    value *= 10;
    u8 digit = digitCharacter - (u8)'0';
    value += digit;
  }

  *duration = parsed;

  return 1;
}

static inline b8
IsDurationLessThan(struct duration *left, struct duration *right)
{
  return left->ns < right->ns;
}

static inline b8
IsDurationLessOrEqualThan(struct duration *left, struct duration *right)
{
  return left->ns <= right->ns;
}

static inline b8
IsDurationGreaterThan(struct duration *left, struct duration *right)
{
  return left->ns > right->ns;
}

static inline b8
IsDurationGreaterOrEqualThan(struct duration *left, struct duration *right)
{
  return left->ns >= right->ns;
}

static inline b8
IsDurationEqual(struct duration *left, struct duration *right)
{
  return left->ns == right->ns;
}

static inline b8
ParseU64(struct string *string, u64 *value)
{
  // max u64: 18446744073709551615
  if (!string || IsStringNull(string) || IsStringEmpty(string) || string->length > 20)
    return 0;

  u64 parsed = 0;
  for (u64 index = 0; index < string->length; index++) {
    u8 digitCharacter = string->value[index];
    comptime b8 allowed[U8_MAX] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x00
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x10
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x20
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, // 0x30
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x40
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x50
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x60
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x70
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x80
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x90
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xa0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xb0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xc0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xd0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xe0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0     // 0xf0
    };
    b8 isDigit = allowed[digitCharacter];
    if (!isDigit)
      return 0;

    parsed *= 10;
    u8 digit = digitCharacter - (u8)'0';
    parsed += digit;
  }

  *value = parsed;
  return 1;
}

/*
 * Format u64 into string buffer
 * string buffer must at least able to hold 1 bytes, at most 20 bytes.
 */
static inline struct string
FormatU64(struct string *stringBuffer, u64 value)
{
  // max 18446744073709551615
  struct string result = {.value = 0, .length = 0};
  if (!stringBuffer || stringBuffer->length == 0)
    return result;

  u64 countOfDigits = 1;
  while (countOfDigits < ARRAY_COUNT(POWERS_OF_10) && value >= POWERS_OF_10[countOfDigits])
    countOfDigits++;

  if (countOfDigits > stringBuffer->length)
    return result;

  u64 index = 0;
  while (countOfDigits > 0) {
    u64 power = POWERS_OF_10[countOfDigits - 1];
    u64 digit = value / power;

    // turn digit into character
    stringBuffer->value[index] = (u8)digit + '0';

    value -= digit * power;

    index++;
    countOfDigits--;
  }

  result.value = stringBuffer->value;
  result.length = index; // written digits
  return result;
}

/*
 * Format s64 into string buffer
 * string buffer must at least able to hold 1 bytes, at most 20 bytes.
 */
static inline struct string
FormatS64(struct string *stringBuffer, s64 value)
{
  struct string result = {.value = 0, .length = 0};
  if (!stringBuffer || stringBuffer->length == 0)
    return result;

  b8 isNegativeValue = value < 0;
  if (isNegativeValue) {
    value *= -1;
    stringBuffer->value[0] = '-';
    stringBuffer->value += 1;
    stringBuffer->length -= 1;
  }

  result = FormatU64(stringBuffer, (u64)value);
  return result;
}

/* FormatF32 is in "teju.h" */
/*
 * string buffer must at least able to hold 3 bytes.
 * fractionCount [1,8]
 */
static inline struct string
FormatF32Slow(struct string *stringBuffer, f32 value, u32 fractionCount)
{
  debug_assert(fractionCount >= 1 && fractionCount <= 8);

  struct string result = {.value = 0, .length = 0};
  if (!stringBuffer || stringBuffer->length < 3)
    return result;

  // 1 - convert integer part to string
  // assume value: 10.123
  //        integerValue: 10
  struct string stringBufferForInteger = {
      .value = stringBuffer->value,
      .length = stringBuffer->length,
  };

  b8 isNegativeValue = value < 0;
  if (isNegativeValue) {
    value *= -1;
    stringBufferForInteger.value[0] = '-';
    stringBufferForInteger.value += 1;
    stringBufferForInteger.length -= 1;
  }

  u32 integerValue = (u32)value;
  struct string integerString = FormatU64(&stringBufferForInteger, (u64)integerValue);
  if (integerString.length == 0)
    return result;

  // 2 - insert point
  stringBufferForInteger.value[integerString.length] = '.';

  // 3 - convert fraction to string
  struct string stringBufferForFraction = {
      .value = stringBufferForInteger.value + integerString.length + 1,
      .length = stringBufferForInteger.length - (integerString.length + 1),
  };

  // assume fractionCount = 2
  //        0.123 = 10.123 - 10.000
  //        12.30 =  0.123 * (10 ^ fractionCount)
  //        12    = (int)12.30
  u64 fractionMultiplier = 10;
  for (u32 fractionIndex = 1; fractionIndex < fractionCount; fractionIndex++)
    fractionMultiplier *= 10;

  f32 fractionFloat = (value - (f32)integerValue);
  u32 fractionValue = (u32)(fractionFloat * (f32)fractionMultiplier);

  f32 epsilon = 0.001f;
  // if value is rounded up, edge case
  if (fractionValue + 1 != fractionMultiplier &&
      (fractionFloat - ((f32)fractionValue / (f32)fractionMultiplier) > (1.0f / (f32)fractionMultiplier - epsilon)))
    fractionValue++;

  /*
   * 0.05f
   *   └── put zeros before putting fraction value
   */
  for (u64 m = fractionMultiplier / 10; fractionValue < m; m /= 10) {
    *stringBufferForFraction.value = '0';
    stringBufferForFraction.value++;
    stringBufferForFraction.length--;
  }

  struct string fractionString = FormatU64(&stringBufferForFraction, fractionValue);
  if (fractionString.length == 0)
    return result;

  /*
   * 0.50f
   *    └── put zeros after putting fraction value
   */
  while (fractionString.length < fractionCount) {
    fractionString.value[fractionString.length] = '0';
    fractionString.length++;
  }

  result.value = stringBuffer->value;
  result.length = isNegativeValue + integerString.length + 1 + fractionString.length;
  return result;
}

static inline b8
ParseHex(struct string *string, u64 *value)
{
  // max 0xffffffffffffffff => 18446744073709551615
  if (!string || IsStringNull(string) || IsStringEmpty(string) || string->length > 16)
    return 0;

  u64 parsed = 0;
  for (u64 index = 0; index < string->length; index++) {
    comptime s8 ASCIItoHEX[256] = {
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, // 0x00
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, // 0x10
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, // 0x20
        0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, -1, -1, -1, -1, -1, -1, // 0x30
        -1,  0xa, 0xb, 0xc, 0xd, 0xe, 0xf, -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, // 0x40
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, // 0x50
        -1,  0xA, 0xB, 0xC, 0xD, 0xE, 0xF, -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, // 0x60
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, // 0x70
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, // 0x80
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, // 0x90
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, // 0xa0
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, // 0xb0
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, // 0xc0
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, // 0xd0
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, // 0xe0
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1      // 0xf0
    };
    u8 digitCharacter = string->value[index];
    s8 digit = ASCIItoHEX[digitCharacter];

    b8 isHexadecimal = digit >= 0;
    if (!isHexadecimal)
      return 0;

    u64 power = (string->length - 1) - index;
    parsed += (u64)digit * ((u64)1 << (4 * power));
  }

  *value = parsed;
  return 1;
}

/*
 *
 * Converts unsigned 64-bit integer to hex string.
 *
 * @param stringBuffer needs at least 2 bytes, at 16 maximum
 * @return sub string from stringBuffer, returns 0 on string.value on failure
 *
 * @note Adapted from
 * https://github.com/jart/cosmopolitan/blob/master/libc/intrin/formathex64.c
 * @copyright
 * ╒══════════════════════════════════════════════════════════════════════════════╕
 * │ Copyright 2021 Justine Alexandra Roberts Tunney                              │
 * │                                                                              │
 * │ Permission to use, copy, modify, and/or distribute this software for         │
 * │ any purpose with or without fee is hereby granted, provided that the         │
 * │ above copyright notice and this permission notice appear in all copies.      │
 * │                                                                              │
 * │ THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL                │
 * │ WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED                │
 * │ WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE             │
 * │ AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL         │
 * │ DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR        │
 * │ PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER               │
 * │ TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR             │
 * │ PERFORMANCE OF THIS SOFTWARE.                                                │
 * └──────────────────────────────────────────────────────────────────────────────┘
 */
static inline struct string
FormatHex(struct string *stringBuffer, u64 value)
{
  struct string result = {.value = 0, .length = 0};
  if (!stringBuffer || stringBuffer->length < 2)
    return result;

  if (value == 0) {
    // edge case 0x00
    u8 *digit = stringBuffer->value;
    *digit++ = '0';
    *digit++ = '0';
    result.value = stringBuffer->value;
    result.length = 2;
    return result;
  }

  u64 index = 0;

  // 1 - pick good width
  u64 width;
  {
    u8 n = bsrl(value);
    if (n < 16) {
      if (n < 8)
        width = 8;
      else
        width = 16;
    } else {
      if (n < 32)
        width = 32;
      else
        width = 64;
    }
  }

  // 2 - turn value into hex
  do {
    width -= 4;
    stringBuffer->value[index] = (u8)("0123456789abcdef"[(value >> width) & 15]);
    index++;
  } while (width);

  result.value = stringBuffer->value;
  result.length = index;
  return result;
}

static inline struct string
PathGetDirectory(struct string *path)
{
  struct string directory = {.value = 0, .length = 0};

  if (!path || !path->value || path->length == 0)
    return directory;

  u64 lastSlashIndex = path->length - 1;
  while (1) {
#if IS_PLATFORM_WINDOWS
    if (path->value[lastSlashIndex] == '\\')
#else
    if (path->value[lastSlashIndex] == '/')
#endif
      break;

    // if slash not found
    if (lastSlashIndex == 0)
      return directory;

    lastSlashIndex--;
  }

  directory.value = path->value;
  directory.length = lastSlashIndex;
  if (directory.length == 0)
    directory.length++;
  return directory;
}

/*
 * Splits string into multiple strings.
 * When splits array is empty, number of parts string can be split returned in splitCount.
 * @param string string to be split
 * @param splitCount how many different parts are in string. [1,∞]
 * @param splits pointer to array of strings.
 * @return 1 when string can be split into parts, 0 otherwise.
 * @code
 *   u64 splitCount;
 *   string *separator = &StringFromLiteral(" ");
 *   if (!StringSplit(string, separator, &splitCount, 0))
 *     return;
 *   string *splits = MemoryArenaPush(arena, sizeof(*splits) * splitCount);
 *   StringSplit(string, separator, &splitCount, splits);
 * @endcode
 */
static inline b8
StringSplit(struct string *string, struct string *separator, u64 *splitCount, struct string *splits)
{
  debug_assert(splitCount && "only splits can be null");

  if (!string || !splitCount)
    return 0;

  b8 isCalculatingSplitCount = splits == 0;
  if (isCalculatingSplitCount) {
    u64 count = 0;
    for (u64 index = 0; index < string->length;) {
      struct string substring = StringFromBuffer(string->value + index, separator->length);

      if (separator->length > string->length - index) {
        if (count == 0)
          // no separator found
          return 0;
        break;
      }

      if (IsStringEqual(&substring, separator)) {
        count++;
        index += separator->length;
      } else {
        index++;
      }
    }
    *splitCount = count + 1;
  } else {
    u64 startIndex = 0;
    u64 splitIndex = 0;

    for (u64 index = 0; index < string->length;) {
      struct string substring = StringFromBuffer(string->value + index, separator->length);
      if (separator->length > string->length - index)
        break;

      if (IsStringEqual(&substring, separator)) {
        struct string split = StringFromBuffer(string->value + startIndex, index - startIndex);
        if (split.length == 0)
          split.value = 0;
        *(splits + splitIndex) = split;
        splitIndex++;
        index += separator->length;
        startIndex = index;
      } else {
        index++;
      }
    }

    // last one
    struct string lastSplit = StringFromBuffer(string->value + startIndex, string->length - startIndex);
    if (lastSplit.length == 0)
      lastSplit.value = 0;
    *(splits + splitIndex) = lastSplit;
    debug_assert(splitIndex + 1 == *splitCount);
  }

  return 1;
}

static inline b8
StringSplitBySpace(struct string *string, u64 *splitCount, struct string *splits)
{
  return StringSplit(string, &StringFromLiteral(" "), splitCount, splits);
}
