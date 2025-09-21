#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <unistd.h>

internalfn void
PrintString(struct string *string)
{
  (void)write(STDOUT_FILENO, string->value, string->length);
}

internalfn void
StringBuilderAppendPlatformError(struct string_builder *sb)
{
  struct string error = StringFromZeroTerminated((u8 *)strerror(errno), 4096);
  StringBuilderAppendString(sb, &error);

  StringBuilderAppendStringLiteral(sb, " (Errno ");
  StringBuilderAppendS32(sb, errno);
  StringBuilderAppendStringLiteral(sb, ")");
}

internalfn enum platform_error
PlatformGetRandom(struct string *buffer)
{
  s64 result = getrandom(buffer->value, buffer->length, 0);
  if (result < 0)
    return IO_ERROR_PLATFORM;

  if ((u64)result != buffer->length)
    return IO_ERROR_BUFFER_PARTIALLY_FILLED;

  return IO_ERROR_NONE;
}

internalfn b8
PlatformIsFileExists(struct string *path, enum platform_error *error)
{
  debug_assert(path->value[path->length] == 0 && "must be zero-terminated string");

  struct stat sb;
  if (lstat((char *)path->value, &sb)) {
    *error = IO_ERROR_PLATFORM;
    return 0;
  }

  // if it is not regular file. man inode.7
  return S_ISREG(sb.st_mode);
}

internalfn enum platform_error
PlatformReadFile(struct string *buffer, struct string *path, struct string *content)
{
  debug_assert(path->value[path->length] == 0 && "must be zero-terminated string");
  enum platform_error error = IO_ERROR_NONE;

  int fd = open((char *)path->value, O_RDONLY);
  if (fd < 0) {
    return IO_ERROR_FILE_NOT_FOUND;
  }

  u64 fileSize = 0;
  {
    struct stat sb;
    if (fstat(fd, &sb) < 0)
      return IO_ERROR_PLATFORM;

    if (S_ISREG(sb.st_mode))
      return IO_ERROR_FILE_NOT_FOUND;

    fileSize = (u64)sb.st_size;
  }

  struct string_cursor bufferCursor = StringCursorFromString(buffer);
  while (bufferCursor.position != fileSize) {
    struct string remainingBuffer = StringCursorExtractRemaining(&bufferCursor);
    if (IsStringCursorAtEnd(&bufferCursor)) {
      error = IO_ERROR_BUFFER_OUT_OF_MEMORY;
      goto failure;
    }

    s64 readBytes = read(fd, remainingBuffer.value, remainingBuffer.length);
    if (readBytes == -1) {
      error = IO_ERROR_PLATFORM;
      goto failure;
    } else if (readBytes == 0) {
      // end of file
      break;
    }

    bufferCursor.position += (u64)readBytes;
  }

  close(fd);

  *content = StringCursorExtractConsumed(&bufferCursor);
  return error;

failure:
  close(fd);
  return error;
}
