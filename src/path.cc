#include "path.h"
#include "parsed_url.h"
#include "utils/assert.h"

namespace whatwgurl {

bool Path::IsDoubleDotPathSegment(const char* segment, size_t length) {
  // A double-dot path segment must be ".." or an ASCII case-insensitive match
  // for ".%2e", "%2e.", or "%2e%2e".
  return (length == 2 && segment[0] == '.' && segment[1] == '.') ||
         (length == 4 &&
          ((segment[0] == '.' && segment[1] == '%' && segment[2] == '2' &&
            (segment[3] == 'e' || segment[3] == 'E')) ||
           (segment[0] == '%' && segment[1] == '2' &&
            (segment[2] == 'e' || segment[2] == 'E') && segment[3] == '.'))) ||
         (length == 6 && segment[0] == '%' && segment[1] == '2' &&
          (segment[2] == 'e' || segment[2] == 'E') && segment[3] == '%' &&
          (segment[4] == '2' && (segment[5] == 'e' || segment[5] == 'E')));
}

bool Path::IsSingleDotPathSegment(const char* segment, size_t length) {
  // A single-dot path segment must be "." or an ASCII case-insensitive match
  // for "%2e".
  return (length == 1 && segment[0] == '.') ||
         (length == 3 && segment[0] == '%' && segment[1] == '2' &&
          (segment[2] == 'e' || segment[2] == 'E'));
}

Path::Path(bool is_opaque_path) : MaybeList(!is_opaque_path) {}

// https://url.spec.whatwg.org/#shorten-a-urls-path
// To shorten a url’s path:
void Path::Shorten(const ParsedURL& parent) {
  // Assert: url does not have an opaque path.
  CHECK(!IsOpaquePath());

  // If url’s scheme is "file", path’s size is 1, and path[0] is a normalized
  // Windows drive letter, then return.
  size_t size = this->size();
  if (parent.scheme == "file" && size == 1 &&
      IsNormalizedWindowsDriverLetter((*this)[0].c_str())) {
    return;
  }

  // Remove path’s last item, if any.
  if (size > 0) {
    this->PopBack();
  }
}

}  // namespace whatwgurl
