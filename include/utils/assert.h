#ifndef INCLUDE_UTILS_ASSERT_H_
#define INCLUDE_UTILS_ASSERT_H_

#ifdef __linux__
#include <linux/limits.h>
#else
#include <limits.h>
#endif
#include <unistd.h>

namespace whatwgurl {

struct AssertionInfo {
  const char* file_line;  // filename:line
  const char* message;
  const char* function;
};

[[noreturn]] void Assert(const AssertionInfo& info);

}  // namespace whatwgurl

#define ERROR_AND_ABORT(expr)                                                  \
  do {                                                                         \
    /* Make sure that this struct does not end up in inline code, but      */  \
    /* rather in a read-only data section when modifying this code.        */  \
    static const whatwgurl::AssertionInfo args = {                             \
        __FILE__ ":" STRINGIFY(__LINE__), #expr, PRETTY_FUNCTION_NAME};        \
    whatwgurl::Assert(args);                                                   \
  } while (0)

#ifdef __GNUC__
#define LIKELY(expr) __builtin_expect(!!(expr), 1)
#define UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#define PRETTY_FUNCTION_NAME __PRETTY_FUNCTION__
#else
#define LIKELY(expr) expr
#define UNLIKELY(expr) expr
#define PRETTY_FUNCTION_NAME ""
#endif

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

#define CHECK(expr)                                                            \
  do {                                                                         \
    if (UNLIKELY(!(expr))) {                                                   \
      ERROR_AND_ABORT(expr);                                                   \
    }                                                                          \
  } while (0)

#define CHECK_EQ(a, b) CHECK((a) == (b))
#define CHECK_GE(a, b) CHECK((a) >= (b))
#define CHECK_GT(a, b) CHECK((a) > (b))
#define CHECK_LE(a, b) CHECK((a) <= (b))
#define CHECK_LT(a, b) CHECK((a) < (b))
#define CHECK_NE(a, b) CHECK((a) != (b))
#define CHECK_NULL(val) CHECK((val) == nullptr)
#define CHECK_NOT_NULL(val) CHECK((val) != nullptr)
#define CHECK_IMPLIES(a, b) CHECK(!(a) || (b))

#define UNREACHABLE(...)                                                       \
  ERROR_AND_ABORT("Unreachable code reached" __VA_OPT__(": ") __VA_ARGS__)

#endif  // INCLUDE_UTILS_ASSERT_H_
