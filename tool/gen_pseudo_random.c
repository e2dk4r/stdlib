#include "platform.h"
#include "string_builder.h"
#include "string_cursor.h"

// TODO: refactor parsing options to be more easy

struct options {
  u32 randomNumberCount;
  string templatePath;
};

internalfn void
OptionsInit(struct options *options)
{
  options->randomNumberCount = 4096;
  options->templatePath = StringFromLiteral("");
}

struct context {
  struct options options;
  string_builder *sb;
};

int
main(int argc, char *argv[])
{
  // setup
  struct context context = {0};
  struct options *options = &context.options;
  OptionsInit(options);

  enum {
    KILOBYTES = (1 << 10),
    MEGABYTES = (1 << 20),
  };
  u8 stackBuffer[1 * MEGABYTES];
  memory_arena stackMemory = {
      .block = stackBuffer,
      .total = ARRAY_COUNT(stackBuffer),
  };

  string_builder *sb = MakeStringBuilder(&stackMemory, 128 * KILOBYTES, 32);
  context.sb = sb;

  // parse options
  u32 argumentCount = (u32)argc;
  for (u32 argumentIndex = 1; argumentIndex < argumentCount; argumentIndex++) {
    string option = StringFromZeroTerminated((u8 *)argv[argumentIndex], 1024);

    if (IsStringEqual(&option, &StringFromLiteral("-t")) || IsStringEqual(&option, &StringFromLiteral("--template"))) {
      if (!IsStringNullOrEmpty(&options->templatePath)) {
        PrintString(&StringFromLiteral("Only one template file is allowed\n"));
        return -1;
      }

      argumentIndex++;
      if (argumentIndex == argumentCount) {
        PrintString(&StringFromLiteral("Template is required to take a file\n"));
        return -1;
      }

      string value = StringFromZeroTerminated((u8 *)argv[argumentIndex], 1024);
      enum platform_error error = IO_ERROR_NONE;
      if (!PlatformIsFileExists(&value, &error)) {
        StringBuilderAppendStringLiteral(sb, "Template at '");
        StringBuilderAppendString(sb, &value);
        StringBuilderAppendStringLiteral(sb, "' is not found\n");

        if (error == IO_ERROR_PLATFORM) {
          StringBuilderAppendStringLiteral(sb, "  ");
          StringBuilderAppendPlatformError(sb);
          StringBuilderAppendStringLiteral(sb, "\n");
        }

        string message = StringBuilderFlush(sb);
        PrintString(&message);
        return -1;
      }

      options->templatePath = value;
    } else if (IsStringEqual(&option, &StringFromLiteral("-c")) ||
               IsStringEqual(&option, &StringFromLiteral("--count"))) {
      argumentIndex++;
      if (argumentIndex == argumentCount) {
        PrintString(&StringFromLiteral("Count is required to take positive value\n"));
        return -1;
      }
      string value = StringFromZeroTerminated((u8 *)argv[argumentIndex], 1024);

      b8 isHex = 0;
      if (IsStringStartsWith(&value, &StringFromLiteral("0x"))) {
        value = StringSlice(&value, 2, value.length);
        isHex = 1;
      }

      u64 randomNumberCount;
      u32 randomNumberMin = 1;
      u32 randomNumberMax = 200000;

      b8 parseOK = ParseU64(&value, &randomNumberCount);
      if (isHex)
        parseOK = ParseHex(&value, &randomNumberCount);

      if (!parseOK || randomNumberCount < randomNumberMin || randomNumberCount > randomNumberMax) {
        StringBuilderAppendStringLiteral(sb, "Expected count value between [");
        StringBuilderAppendU32(sb, randomNumberMin);
        StringBuilderAppendStringLiteral(sb, ", ");
        StringBuilderAppendU32(sb, randomNumberMax);
        StringBuilderAppendStringLiteral(sb, "]\n");
        string message = StringBuilderFlush(sb);
        PrintString(&message);
        return -1;
      }

      options->randomNumberCount = (u32)randomNumberCount;
    } else if (IsStringEqual(&option, &StringFromLiteral("-h")) ||
               IsStringEqual(&option, &StringFromLiteral("--help"))) {
      StringBuilderAppendStringLiteral(sb, "NAME");
      StringBuilderAppendStringLiteral(sb, "\n  gen_pseudo_random - Generate pseudo random numbers with template");
      StringBuilderAppendStringLiteral(sb, "\n\nSYNOPSIS:");
      StringBuilderAppendStringLiteral(sb, "\n  gen_pseudo_random --template path [OPTIONS]");
      StringBuilderAppendStringLiteral(sb, "\n\nTEMPLATE:");
      StringBuilderAppendStringLiteral(
          sb, "\n  In template file you can specify below variables with prefix and postfix $$");
      StringBuilderAppendStringLiteral(sb, "\n  (two dollar signs).");
      StringBuilderAppendStringLiteral(sb, "\n  ");
      StringBuilderAppendStringLiteral(sb, "\n  RANDOM_NUMBER_TABLE");
      StringBuilderAppendStringLiteral(sb, "\n    Comma seperated list of u32 in hex format. Range is [0, 4294967295]");
      StringBuilderAppendStringLiteral(sb, "\n  RANDOM_NUMBER_COUNT");
      StringBuilderAppendStringLiteral(sb, "\n    Count of random numbers");
      StringBuilderAppendStringLiteral(sb, "\n  RANDOM_NUMBER_MIN");
      StringBuilderAppendStringLiteral(sb, "\n    Minimum (smallest) random number in table");
      StringBuilderAppendStringLiteral(sb, "\n  RANDOM_NUMBER_MAX");
      StringBuilderAppendStringLiteral(sb, "\n    Maximum (biggest) random number in table");
      StringBuilderAppendStringLiteral(sb, "\n\nOPTIONS:");
      StringBuilderAppendStringLiteral(sb, "\n  -t, --template path");
      StringBuilderAppendStringLiteral(sb, "\n    Location of template file");
      StringBuilderAppendStringLiteral(sb, "\n    This option is required");
      StringBuilderAppendStringLiteral(sb, "\n  -c, --count count");
      StringBuilderAppendStringLiteral(sb, "\n    How many random numbers must be generated");
      StringBuilderAppendStringLiteral(sb, "\n    You also can enter in hex format starting with '0x'");
      StringBuilderAppendStringLiteral(sb, "\n    Range is [1, 200000]");
      StringBuilderAppendStringLiteral(sb, "\n  -h, --help");
      StringBuilderAppendStringLiteral(sb, "\n    Show this help message");
      StringBuilderAppendStringLiteral(sb, "\n");
      string message = StringBuilderFlush(sb);
      PrintString(&message);
      return 0;
    } else {
      StringBuilderAppendStringLiteral(sb, "Option '");
      StringBuilderAppendString(sb, &option);
      StringBuilderAppendStringLiteral(sb, "' is not understand");
      StringBuilderAppendStringLiteral(sb, "\nSee --help for more information");
      StringBuilderAppendStringLiteral(sb, "\n");
      string message = StringBuilderFlush(sb);
      PrintString(&message);
      return -1;
    }
  }

  if (IsStringNullOrEmpty(&options->templatePath)) {
    StringBuilderAppendStringLiteral(sb, "--template option is required");
    StringBuilderAppendStringLiteral(sb, "\nSee --help for more information");
    StringBuilderAppendStringLiteral(sb, "\n");
    string message = StringBuilderFlush(sb);
    PrintString(&message);
    return -1;
  }

  // generate
  u32 bytesPerRandomNumber = sizeof(u32);
  u32 *randomNumbers = MemoryArenaPush(&stackMemory, options->randomNumberCount * bytesPerRandomNumber);
  u32 randomNumberMinIndex = 0;
  u32 randomNumberMaxIndex = 0;
  {
    memory_temp tempMemory = MemoryTempBegin(&stackMemory);
    string *randomBuffer = MakeString(tempMemory.arena, options->randomNumberCount * bytesPerRandomNumber);
    string_cursor randomCursor = StringCursorFromString(randomBuffer);
    randomCursor.position = StringCursorRemainingLength(&randomCursor);

    u32 min = 0;
    u32 max = 0;
    for (u32 randomNumberIndex = 0; randomNumberIndex < options->randomNumberCount; randomNumberIndex++) {
      if (IsStringCursorAtEnd(&randomCursor)) {
        enum platform_error error = PlatformGetRandom(randomBuffer);
        if (error != IO_ERROR_NONE) {
          StringBuilderAppendStringLiteral(sb, "Error: GetRandom() ");
          if (error == IO_ERROR_BUFFER_PARTIALLY_FILLED)
            StringBuilderAppendStringLiteral(sb, "insufficent entropy");
          else if (error == IO_ERROR_PLATFORM)
            StringBuilderAppendPlatformError(sb);
          StringBuilderAppendStringLiteral(sb, "\n");
          struct string message = StringBuilderFlush(sb);
          PrintString(&message);
          return -1;
        }

        randomCursor = StringCursorFromString(randomBuffer);
      }

      string slice = StringCursorConsumeSubstring(&randomCursor, bytesPerRandomNumber);

      u32 randomNumber = 0;
      for (u32 sliceIndex = 0; sliceIndex < slice.length; sliceIndex++) {
        randomNumber *= 10;
        randomNumber += slice.value[sliceIndex];
      }

      if (randomNumberIndex == 0) {
        min = randomNumber;
        max = randomNumber;
      } else {
        if (randomNumber < min) {
          min = randomNumber;
          randomNumberMinIndex = randomNumberIndex;
        }

        if (randomNumber > max) {
          max = randomNumber;
          randomNumberMaxIndex = randomNumberIndex;
        }
      }

      *(randomNumbers + randomNumberIndex) = randomNumber;
    }

    MemoryTempEnd(&tempMemory);
  }

  // Read template
  string *templateBuffer = MakeString(&stackMemory, 256 * KILOBYTES);
  string template;
  {
    enum platform_error error = PlatformReadFile(templateBuffer, &options->templatePath, &template);
    if (error != IO_ERROR_NONE) {
      StringBuilderAppendStringLiteral(sb, "Error: file not read\n");

      StringBuilderAppendStringLiteral(sb, "  path: '");
      StringBuilderAppendString(sb, &options->templatePath);
      StringBuilderAppendStringLiteral(sb, "'\n");

      StringBuilderAppendStringLiteral(sb, "  ");
      if (error == IO_ERROR_FILE_NOT_FOUND)
        StringBuilderAppendStringLiteral(sb, "File not found");
      else if (error == IO_ERROR_BUFFER_OUT_OF_MEMORY)
        StringBuilderAppendStringLiteral(sb, "File is too big");
      else if (error == IO_ERROR_PLATFORM)
        StringBuilderAppendPlatformError(sb);

      StringBuilderAppendStringLiteral(sb, "\n");
      string message = StringBuilderFlush(sb);
      PrintString(&message);
      return -1;
    }

    if (IsStringNullOrEmpty(&template)) {
      // file is not valid
      StringBuilderAppendStringLiteral(sb, "Error: file is not valid");
      string message = StringBuilderFlush(sb);
      PrintString(&message);
      return 1;
    }
  }
  // TODO: Write output to a file

  // Replace variables with values
  string_cursor templateCursor = StringCursorFromString(&template);
  while (1) {
    string *variableMagicStart = &StringFromLiteral("$$");
    string *variableMagicEnd = variableMagicStart;

    string beforeVariable = StringCursorConsumeUntilOrRest(&templateCursor, variableMagicStart);
    if (beforeVariable.length != 0)
      PrintString(&beforeVariable);
    if (IsStringCursorAtEnd(&templateCursor))
      break;
    templateCursor.position += variableMagicStart->length;

    u64 variableStartPosition = templateCursor.position;
    string variable = StringCursorConsumeUntil(&templateCursor, variableMagicEnd);
    if (IsStringCursorAtEnd(&templateCursor))
      break;

    templateCursor.position += variableMagicEnd->length;

    if (IsStringEqual(&variable, &StringFromLiteral("RANDOM_NUMBER_TABLE"))) {
      for (u32 randomNumberIndex = 0; randomNumberIndex < options->randomNumberCount; randomNumberIndex++) {
        u32 randomNumber = *(randomNumbers + randomNumberIndex);

        StringBuilderAppendStringLiteral(sb, "0x");
        string randomNumberInHex = FormatHex(sb->stringBuffer, randomNumber);
        if (randomNumberInHex.length < 8) {
          string sliced = StringSlice(&StringFromLiteral("00000000"), 0, 8 - randomNumberInHex.length);
          StringBuilderAppendString(sb, &sliced);
        }
        StringBuilderAppendString(sb, &randomNumberInHex);

        if (randomNumberIndex + 1 != options->randomNumberCount)
          StringBuilderAppendStringLiteral(sb, ", ");
      }

      string message = StringBuilderFlush(sb);
      PrintString(&message);
    } else if (IsStringEqual(&variable, &StringFromLiteral("RANDOM_NUMBER_COUNT"))) {
      StringBuilderAppendU64(sb, options->randomNumberCount);
      string message = StringBuilderFlush(sb);
      PrintString(&message);
    } else if (IsStringEqual(&variable, &StringFromLiteral("RANDOM_NUMBER_MIN"))) {
      u32 randomNumberMin = *(randomNumbers + randomNumberMinIndex);
      StringBuilderAppendU64(sb, randomNumberMin);
      string message = StringBuilderFlush(sb);
      PrintString(&message);
    } else if (IsStringEqual(&variable, &StringFromLiteral("RANDOM_NUMBER_MAX"))) {
      u32 randomNumberMax = *(randomNumbers + randomNumberMaxIndex);
      StringBuilderAppendU64(sb, randomNumberMax);
      string message = StringBuilderFlush(sb);
      PrintString(&message);
    } else {
      StringBuilderAppendStringLiteral(sb, "Variable '");
      StringBuilderAppendString(sb, &variable);
      StringBuilderAppendStringLiteral(sb, "' at: ");
      StringBuilderAppendU64(sb, variableStartPosition);
      StringBuilderAppendStringLiteral(sb, " is NOT identified\n");
      string message = StringBuilderFlush(sb);
      PrintString(&message);
      return 1;
    }
  }
}
