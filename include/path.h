#ifndef INCLUDE_PATH_H_
#define INCLUDE_PATH_H_

#include <string>
#include <vector>
#include "code_points.h"
#include "maybe.h"
#include "utils/assert.h"

namespace whatwgurl {

struct ParsedURL;
class Path : public MaybeList<std::string> {
 public:
  static inline bool IsWindowsDriveLetter(const char* letters) {
    // A Windows drive letter is two code points, of which the first is an ASCII
    // alpha and the second is either U+003A (:) or U+007C (|).
    return IsASCIIAlpha(letters[0]) &&
           (letters[1] == ':' || letters[1] == '|') && letters[2] == 0;
  }

  static inline bool IsNormalizedWindowsDriverLetter(const char* letters) {
    // A normalized Windows drive letter is a Windows drive letter of which the
    // second code point is U+003A (:).
    return IsASCIIAlpha(letters[0]) && letters[1] == ':' && letters[2] == 0;
  }

  // A string starts with a Windows drive letter if all of the following are
  // true:
  static inline bool IsStartsWithWindowsDriveLetter(const char* letters) {
    // its length is greater than or equal to 2
    // its first two code points are a Windows drive letter
    // its length is 2 or its third code point is U+002F (/), U+005C (\), U+003F
    // (?), or U+0023 (#).
    return IsASCIIAlpha(letters[0]) &&
           (letters[1] == ':' || letters[1] == '|') &&
           (letters[2] == 0 || letters[2] == '/' || letters[2] == '\\' ||
            letters[2] == '?' || letters[2] == '#');
  }

  static bool IsDoubleDotPathSegment(const char* segment, size_t length);
  static bool IsSingleDotPathSegment(const char* segment, size_t length);

 public:
  explicit Path(bool is_opaque_path = false);
  ~Path() = default;

  inline bool IsOpaquePath() const { return !is_list(); }
  inline const std::string& ASCIIString() const { return value(); }
  inline std::string& ASCIIString() { return value(); }
  inline void Reset(bool is_opaque_path = false) {
    MaybeList<std::string>::Reset(!is_opaque_path);
  }

  inline Path& operator=(const Path& other) {
    MaybeList<std::string>::operator=(other);
    return *this;
  }

  void Shorten(const ParsedURL& parent);
};

}  // namespace whatwgurl

#endif  // INCLUDE_PATH_H_
