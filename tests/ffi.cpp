#include <cstdint>
#include <cstring>

extern "C" std::int32_t ffi_add(std::int32_t left, std::int32_t right) {
  return left + right;
}

extern "C" std::int64_t ffi_wide(std::int64_t value) {
  return value + 5000000000LL;
}

extern "C" void ffi_bump(std::int32_t* value) {
  *value += 5;
}

extern "C" std::int32_t ffi_MixedCase() {
  return 17;
}

extern "C" std::int32_t ffi_text_length(const char* text) {
  return static_cast<std::int32_t>(std::strlen(text));
}

extern "C" const char* ffi_text() {
  return "foreign";
}

extern "C" void* ffi_object() {
  static std::int32_t value = 81;
  return &value;
}

extern "C" std::int32_t ffi_read(const void* pointer) {
  return pointer ? *static_cast<const std::int32_t*>(pointer) : -1;
}

extern "C" double ffi_real_add(double left, double right) {
  return left + right + 0.25;
}

extern "C" void ffi_clear(void** pointer) {
  *pointer = nullptr;
}

using NumberCallback = std::int32_t (*)(std::int32_t);
static NumberCallback savedCallback = nullptr;

using RealCallback = double (*)(double);
static RealCallback savedRealCallback = nullptr;

extern "C" void ffi_set_callback(NumberCallback callback) {
  savedCallback = callback;
}

extern "C" void ffi_set_real_callback(RealCallback callback) {
  savedRealCallback = callback;
}

extern "C" std::int32_t ffi_run_callback(std::int32_t value) {
  return savedCallback ? savedCallback(value) : -1;
}

extern "C" double ffi_run_real_callback(double value) {
  return savedRealCallback ? savedRealCallback(value) : -1.0;
}

extern "C" void ffi_run_reference(void (*callback)(std::int32_t*), std::int32_t* value) {
  callback(value);
}
