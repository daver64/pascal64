#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <utility>
extern "C" void p64_write_int(std::int64_t n){std::printf("%lld",static_cast<long long>(n));}
extern "C" void p64_write_real(double value){
  if (value == 0.0) value = 0.0;
  std::printf("%.15g", value);
}
extern "C" void p64_write_str(const char* s){std::fputs(s,stdout);}
extern "C" void p64_newline(){std::putchar('\n');}
extern "C" std::int64_t p64_array_index(std::int64_t index, std::int64_t lower, std::int64_t upper, int line) {
  if (index < lower || index > upper) {
    std::fprintf(stderr, "line %d: array index %lld outside %lld..%lld\n", line,
                 static_cast<long long>(index), static_cast<long long>(lower), static_cast<long long>(upper));
    std::exit(EXIT_FAILURE);
  }
  return index - lower;
}

namespace {
struct ManagedString {
  std::size_t references = 1;
  std::size_t length = 0;
  char* bytes = nullptr;
};

ManagedString* stringAlloc(const char* text, std::size_t length) {
  auto* value = new ManagedString;
  value->length = length;
  value->bytes = static_cast<char*>(std::malloc(length + 1));
  if (!value->bytes) { delete value; std::fputs("out of memory allocating string\n", stderr); std::exit(EXIT_FAILURE); }
  if (length) std::memcpy(value->bytes, text, length);
  value->bytes[length] = '\0';
  return value;
}
}

extern "C" ManagedString* p64_string_new(const char* value) { return stringAlloc(value ? value : "", value ? std::strlen(value) : 0); }
extern "C" const char* p64_string_cstr(const ManagedString* value) {
  static const char empty[] = "";
  return (value && value->bytes) ? value->bytes : empty;
}
extern "C" void p64_string_retain(ManagedString* value) { if (value) ++value->references; }
extern "C" void p64_string_release(ManagedString* value) {
  if (!value) return;
  if (--value->references != 0) return;
  std::free(value->bytes);
  delete value;
}
extern "C" ManagedString* p64_string_concat(const ManagedString* left, const ManagedString* right) {
  std::size_t leftLen = left ? left->length : 0;
  std::size_t rightLen = right ? right->length : 0;
  std::size_t length = leftLen + rightLen;
  auto* value = new ManagedString;
  value->length = length;
  value->bytes = static_cast<char*>(std::malloc(length + 1));
  if (!value->bytes) { delete value; std::fputs("out of memory allocating string\n", stderr); std::exit(EXIT_FAILURE); }
  if (leftLen) std::memcpy(value->bytes, left->bytes, leftLen);
  if (rightLen) std::memcpy(value->bytes + leftLen, right->bytes, rightLen);
  value->bytes[length] = '\0';
  return value;
}
extern "C" int p64_string_compare(const ManagedString* left, const ManagedString* right) {
  const char* leftBytes = left && left->bytes ? left->bytes : "";
  const char* rightBytes = right && right->bytes ? right->bytes : "";
  std::size_t leftLen = left ? left->length : 0;
  std::size_t rightLen = right ? right->length : 0;
  std::size_t common = leftLen < rightLen ? leftLen : rightLen;
  int cmp = common ? std::memcmp(leftBytes, rightBytes, common) : 0;
  if (cmp) return cmp;
  return leftLen < rightLen ? -1 : leftLen > rightLen ? 1 : 0;
}
extern "C" void p64_string_write(const ManagedString* value) {
  if (!value || !value->bytes || !value->length) return;
  std::fwrite(value->bytes, 1, value->length, stdout);
}

extern "C" ManagedString* p64_read_line() {
  char* line = nullptr;
  std::size_t capacity = 0;
  auto read = ::getline(&line, &capacity, stdin);
  if (read < 0) {
    if (line) std::free(line);
    return stringAlloc("", 0);
  }

  std::size_t length = static_cast<std::size_t>(read);
  while (length && (line[length - 1] == '\n' || line[length - 1] == '\r')) --length;
  auto* value = stringAlloc(line, length);
  std::free(line);
  return value;
}

namespace {
[[noreturn]] void parseFail(int line, const char* kind, const char* text) {
  std::fprintf(stderr, "line %d: invalid %s '%s'\n", line, kind, text ? text : "");
  std::exit(EXIT_FAILURE);
}

template <typename Fn>
void skipSpace(const char*& ptr, Fn predicate) {
  while (*ptr && predicate(static_cast<unsigned char>(*ptr))) ++ptr;
}
}

extern "C" std::int32_t p64_parse_int32(const ManagedString* value, int line) {
  const char* text = p64_string_cstr(value);
  const char* cursor = text;
  skipSpace(cursor, [](unsigned char ch) { return std::isspace(ch) != 0; });
  errno = 0;
  char* end = nullptr;
  long long parsed = std::strtoll(cursor, &end, 10);
  if (errno != 0 || end == cursor) parseFail(line, "integer", text);
  skipSpace(cursor = end, [](unsigned char ch) { return std::isspace(ch) != 0; });
  if (*cursor) parseFail(line, "integer", text);
  if (parsed < INT32_MIN || parsed > INT32_MAX) parseFail(line, "integer", text);
  return static_cast<std::int32_t>(parsed);
}

extern "C" std::int64_t p64_parse_int64(const ManagedString* value, int line) {
  const char* text = p64_string_cstr(value);
  const char* cursor = text;
  skipSpace(cursor, [](unsigned char ch) { return std::isspace(ch) != 0; });
  errno = 0;
  char* end = nullptr;
  long long parsed = std::strtoll(cursor, &end, 10);
  if (errno != 0 || end == cursor) parseFail(line, "int64", text);
  skipSpace(cursor = end, [](unsigned char ch) { return std::isspace(ch) != 0; });
  if (*cursor) parseFail(line, "int64", text);
  return static_cast<std::int64_t>(parsed);
}

extern "C" double p64_parse_real(const ManagedString* value, int line) {
  const char* text = p64_string_cstr(value);
  const char* cursor = text;
  skipSpace(cursor, [](unsigned char ch) { return std::isspace(ch) != 0; });
  errno = 0;
  char* end = nullptr;
  double parsed = std::strtod(cursor, &end);
  if (errno != 0 || end == cursor) parseFail(line, "real", text);
  skipSpace(cursor = end, [](unsigned char ch) { return std::isspace(ch) != 0; });
  if (*cursor) parseFail(line, "real", text);
  return parsed;
}
extern "C" void* p64_class_new(std::uint64_t size) {
  void* object = std::calloc(1, size ? static_cast<std::size_t>(size) : 1);
  if (!object) { std::fputs("out of memory allocating class instance\n", stderr); std::exit(EXIT_FAILURE); }
  return object;
}
extern "C" void* p64_class_checked(void* object, int line) {
  if (!object) { std::fprintf(stderr, "line %d: nil class instance\n", line); std::exit(EXIT_FAILURE); }
  return object;
}
extern "C" void p64_class_free(void* object) { std::free(object); }

extern "C" [[noreturn]] void p64_raise() {
  throw 1;
}

namespace {
int p64_argc_value = 0;
char** p64_argv_value = nullptr;
}

extern "C" void p64rt_set_args(int argc, char** argv) {
  p64_argc_value = argc;
  p64_argv_value = argv;
}

extern "C" int p64rt_get_argc() {
  return p64_argc_value;
}

extern "C" const char* p64rt_get_argv(std::int64_t index) {
  if (index < 0 || index >= static_cast<std::int64_t>(p64_argc_value) || !p64_argv_value) return "";
  auto* value = p64_argv_value[static_cast<int>(index)];
  return value ? value : "";
}
