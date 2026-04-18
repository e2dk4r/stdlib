#include "platform.h"
#include "string_builder.h"
#include "text.h"

#define TEST_ERROR_LIST(X)                                                                                             \
  X(LIST_EXIST_MUST_BE_TRUE, "ListSearch(): Item must be found in list")                                               \
  X(LIST_EXIST_MUST_BE_FALSE, "ListSearch(): Non existent item could NOT be found in list")                            \
  X(LIST_APPEND_MUST_ADD, "ListAppend(): Item must be appended to list")                                               \
  X(LIST_APPEND_MUST_NOT_ADD, "ListAppend(): Item must be REJECTED")

enum test_error {
  TEST_ERROR_NONE = 0,
#define X(tag, message) TEST_ERROR_##tag,
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
StringBuilderAppendErrorMessage(struct string_builder *stringBuilder, enum test_error errorCode)
{
  struct error {
    enum test_error code;
    struct string message;
  } errors[] = {
#define XX(tag, msg) {.code = TEST_ERROR_##tag, .message = StringFromLiteral(msg)},
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

static void
MemoryCopyBackwards(void *dest, void *src, u64 length)
{
  u8 *destination = dest;
  u8 *source = src;
  while (length--) {
    destination[length] = source[length];
  }
}

struct list {
  u32 count;
  u32 total;
  u32 *items;
};

static u32
ListSearch(struct list *list, u64 item)
{
  u32 count = list->count;
  u32 invalid = list->total;
  if (count == 0)
    return invalid;

#if 1
  // binary search
  u32 leftIndex = 0;
  u32 rightIndex = count - 1;

  while (leftIndex < rightIndex) {
    u32 middleIndex = leftIndex + (rightIndex - leftIndex) / 2;

    u64 middle = list->items[middleIndex];
    if (item == middle)
      return middleIndex;
    else if (item > middle)
      leftIndex = middleIndex + 1;
    else if (middleIndex != 0)
      rightIndex = middleIndex - 1;
    else
      rightIndex = 0;
  }

  if (item == list->items[leftIndex])
    return leftIndex;

#else
  // linear search
  for (u32 index = 0; index < count; index++) {
    if (list->items[index] == item)
      return index;
  }
#endif

  return invalid;
}

static b8
ListExist(struct list *list, u32 item)
{
  u32 invalid = list->total;
  return ListSearch(list, item) != invalid;
}

static void
ListAppend(struct list *list, u32 item)
{
  u32 count = list->count;
  if (count != 0 && ListExist(list, item))
    return;

  u32 *items = list->items;
  u32 index = count;
  if (count != 0) {
    u32 leftIndex = 0;
    u32 rightIndex = list->count - 1;

    while (leftIndex < rightIndex) {
      u32 middleIndex = leftIndex + (rightIndex - leftIndex) / 2;

      u32 middle = items[middleIndex];
      debug_assert(item != middle && "item is already inserted");
      if (item > middle)
        leftIndex = middleIndex + 1;
      else
        rightIndex = middleIndex - 1;
    }

    index = leftIndex;
    if (item > items[index])
      // if item biggest, append to end of the list
      index++;

    if (count - index != 0)
      // move all items to right
      MemoryCopyBackwards(items + index + 1, items + index, (count - index) * sizeof(*items));
  }

  items[index] = item;
  list->count += 1;
  debug_assert(list->count <= list->total);
}

static void
StringBuilderAppendList(string_builder *sb, struct list *list)
{
  u32 count = list->count;
  for (u32 index = 0; index < count; index++) {
    StringBuilderAppendU32(sb, list->items[index]);
    if (index != count - 1) // is not last item
      StringBuilderAppendStringLiteral(sb, ", ");
  }
}

int
main(void)
{
  enum test_error errorCode = TEST_ERROR_NONE;

  // setup
  enum { KILOBYTES = (1 << 10) };
  u8 stackBuffer[8 * KILOBYTES];
  memory_arena stackMemory = {
      .block = stackBuffer,
      .total = ARRAY_COUNT(stackBuffer),
  };

  string_builder *sb = MakeStringBuilder(&stackMemory, 1024, 32);

  // b8 ListExist(struct list *list, u32 item)
  {
    struct test_case {
      struct list list;
      u32 search;
      b8 expected;
    } testCases[] = {
        {
            // at start
            .list =
                (struct list){
                    .count = 5,
                    .total = 5,
                    .items = (u32[]){1, 2, 3, 4, 5},
                },
            .search = 1,
            .expected = 1,
        },
        {
            // at middle
            .list =
                (struct list){
                    .count = 5,
                    .total = 5,
                    .items = (u32[]){1, 2, 3, 4, 5},
                },
            .search = 3,
            .expected = 1,
        },
        {
            // at end
            .list =
                (struct list){
                    .count = 5,
                    .total = 5,
                    .items = (u32[]){1, 2, 3, 4, 5},
                },
            .search = 5,
            .expected = 1,
        },
        {
            // not exists
            .list =
                (struct list){
                    .count = 5,
                    .total = 5,
                    .items = (u32[]){1, 2, 3, 4, 5},
                },
            .search = 999,
            .expected = 0,
        },
        {
            .list =
                (struct list){
                    .count = 5,
                    .total = 5,
                    .items = (u32[]){1, 2, 3, 4, 5},
                },
            .search = 0,
            .expected = 0,
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct list *list = &testCase->list;
      u32 search = testCase->search;
      b8 expected = testCase->expected;
      b8 got = ListExist(list, search);
      if (got != expected) {
        errorCode = expected ? TEST_ERROR_LIST_EXIST_MUST_BE_TRUE : TEST_ERROR_LIST_EXIST_MUST_BE_FALSE;

        StringBuilderAppendErrorMessage(sb, errorCode);
        StringBuilderAppendStringLiteral(sb, "\n  list: ");
        StringBuilderAppendList(sb, list);
        StringBuilderAppendStringLiteral(sb, "\n  item: ");
        StringBuilderAppendU32(sb, search);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  // void ListAppend(struct list *list, u32 item)
  {
    struct test_case {
      struct list list;
      u32 item;
      struct list expected;
    } testCases[] = {
        {
            // at to front
            .list =
                (struct list){
                    .count = 4,
                    .total = 5,
                    .items = (u32[]){2, 3, 4, 5, 0},
                },
            .item = 1,
            .expected =
                (struct list){
                    .count = 5,
                    .total = 5,
                    .items = (u32[]){1, 2, 3, 4, 5},
                },
        },
        {
            // at to second
            .list =
                (struct list){
                    .count = 4,
                    .total = 5,
                    .items = (u32[]){1, 3, 4, 5, 0},
                },
            .item = 2,
            .expected =
                (struct list){
                    .count = 5,
                    .total = 5,
                    .items = (u32[]){1, 2, 3, 4, 5},
                },
        },
        {
            // at to middle
            .list =
                (struct list){
                    .count = 4,
                    .total = 5,
                    .items = (u32[]){1, 2, 4, 5, 0},
                },
            .item = 3,
            .expected =
                (struct list){
                    .count = 5,
                    .total = 5,
                    .items = (u32[]){1, 2, 3, 4, 5},
                },
        },
        {
            // at to fourth
            .list =
                (struct list){
                    .count = 4,
                    .total = 5,
                    .items = (u32[]){1, 2, 3, 5, 0},
                },
            .item = 4,
            .expected =
                (struct list){
                    .count = 5,
                    .total = 5,
                    .items = (u32[]){1, 2, 3, 4, 5},
                },
        },
        {
            // at to end
            .list =
                (struct list){
                    .count = 4,
                    .total = 5,
                    .items = (u32[]){1, 2, 3, 4, 0},
                },
            .item = 5,
            .expected =
                (struct list){
                    .count = 5,
                    .total = 5,
                    .items = (u32[]){1, 2, 3, 4, 5},
                },
        },
        {
            // must not be added
            .list =
                (struct list){
                    .count = 4,
                    .total = 5,
                    .items = (u32[]){1, 2, 3, 4, 0},
                },
            .item = 4,
            .expected =
                (struct list){
                    .count = 4,
                    .total = 5,
                    .items = (u32[]){1, 2, 3, 4, 0},
                },
        },
    };

    for (u32 testCaseIndex = 0; testCaseIndex < ARRAY_COUNT(testCases); testCaseIndex++) {
      struct test_case *testCase = testCases + testCaseIndex;

      struct list *list = &testCase->list;
      u32 startedWith = list->count;
      u32 item = testCase->item;
      ListAppend(list, item);

      struct list *expected = &testCase->expected;
      b8 ok = 1;

      if (list->count != expected->count) {
        ok = 0;
      } else {
        for (u32 index = 0; index < list->count; index++) {
          if (list->items[index] != expected->items[index]) {
            ok = 0;
            break;
          }
        }
      }

      if (!ok) {
        errorCode =
            expected->count > startedWith ? TEST_ERROR_LIST_APPEND_MUST_ADD : TEST_ERROR_LIST_APPEND_MUST_NOT_ADD;

        StringBuilderAppendErrorMessage(sb, errorCode);
        StringBuilderAppendStringLiteral(sb, "\n  expected: ");
        StringBuilderAppendList(sb, expected);
        StringBuilderAppendStringLiteral(sb, "\n      list: ");
        StringBuilderAppendList(sb, list);
        StringBuilderAppendStringLiteral(sb, "\n      item: ");
        StringBuilderAppendU32(sb, item);
        StringBuilderAppendStringLiteral(sb, "\n");
        struct string errorMessage = StringBuilderFlush(sb);
        PrintString(&errorMessage);
      }
    }
  }

  return (int)errorCode;
}
