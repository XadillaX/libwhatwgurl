#ifndef INCLUDE_WHATWGURL_H_
#define INCLUDE_WHATWGURL_H_

#include <string>
#include <vector>
#include "whatwgurl_macros.h"

namespace whatwgurl {

typedef bool (*UnicodeASCIITransformer)(const std::string&, std::string*);

namespace table_data {
extern const char hex[1024];
extern const uint8_t C0_CONTROL_ENCODE_SET[32];
extern const uint8_t FRAGMENT_ENCODE_SET[32];
extern const uint8_t PATH_ENCODE_SET[32];
extern const uint8_t USERINFO_ENCODE_SET[32];
extern const uint8_t QUERY_ENCODE_SET_NONSPECIAL[32];
extern const uint8_t QUERY_ENCODE_SET_SPECIAL[32];
}  // namespace table_data

enum UrlParseState {
  kUnknownState = -1,
#define XX(name) name,
  PARSESTATES(XX)
#undef XX
};

enum UrlFlags {
#define XX(name, val) name = val,
  FLAGS(XX)
#undef XX
};

struct UrlData {
  int32_t flags;
  int port;
  std::string scheme;
  std::string username;
  std::string password;
  std::string host;
  std::string query;
  std::string fragment;
  std::vector<std::string> path;
  std::string href;
  UrlData() : flags(kUrlFlagsNone), port(-1) {}
};

struct InitOptions {
  UnicodeASCIITransformer to_unicode;
  UnicodeASCIITransformer to_ascii;
  InitOptions() : to_unicode(nullptr), to_ascii(nullptr) {}
};

class URL {
 public:
  static void InitEnvironment(const InitOptions& options);

  // Get the file URL from native file system path.
  static URL FromFilePath(const std::string& file_path);

  static void Parse(const char* input,
                    size_t len,
                    UrlParseState state_override,
                    UrlData* url,
                    bool has_url,
                    const struct UrlData* base,
                    bool has_base);

  static std::string SerializeURL(const UrlData& url, bool exclude);

 public:
  inline URL() : URL("") {}

  inline URL(const char* input, const size_t len) {
    Parse(input, len, kUnknownState, &_context, false, nullptr, false);
  }

  inline URL(const char* input, const size_t len, const URL* base) {
    if (base != nullptr)
      Parse(
          input, len, kUnknownState, &_context, false, &(base->_context), true);
    else
      Parse(input, len, kUnknownState, &_context, false, nullptr, false);
  }

  inline URL(const char* input,
             const size_t len,
             const char* base,
             const size_t baselen) {
    if (base != nullptr && baselen > 0) {
      URL base_url(base, baselen);
      Parse(input,
            len,
            kUnknownState,
            &_context,
            false,
            &(base_url._context),
            true);
    } else {
      Parse(input, len, kUnknownState, &_context, false, nullptr, false);
    }
  }

  inline explicit URL(const std::string& input)
      : URL(input.c_str(), input.length()) {}

  inline URL(const std::string& input, const URL* base)
      : URL(input.c_str(), input.length(), base) {}

  inline URL(const std::string& input, const URL& base)
      : URL(input.c_str(), input.length(), &base) {}

  inline URL(const std::string& input, const std::string& base)
      : URL(input.c_str(), input.length(), base.c_str(), base.length()) {}

  URL(const URL&) = default;
  URL& operator=(const URL&) = default;
  URL(URL&&) = default;
  URL& operator=(URL&&) = default;

 public:
  inline int32_t flags() const { return _context.flags; }

  inline int port() const { return _context.port; }

  inline const std::string& protocol() const { return _context.scheme; }

  inline const std::string& username() const { return _context.username; }

  inline const std::string& password() const { return _context.password; }

  inline const std::string& host() const { return _context.host; }

  inline const std::string& query() const { return _context.query; }

  inline const std::string& fragment() const { return _context.fragment; }

  std::string path() const {
    std::string ret;
    for (const std::string& element : _context.path) {
      ret += '/' + element;
    }
    return ret;
  }

  std::string href() const { return SerializeURL(_context, false); }

  // Get the path of the file: URL in a format consumable by native file system
  // APIs. Returns an empty string if something went wrong.
  std::string ToFilePath() const;

 private:
  UrlData _context;
};

}  // namespace whatwgurl

#endif  // INCLUDE_WHATWGURL_H_
