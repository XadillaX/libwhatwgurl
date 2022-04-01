#include <whatwgurl.h>
#include <numeric>
#include "whatwgurl-inl.h"

namespace whatwgurl {

UnicodeASCIITransformer ToUnicode = DefaultToUnicode;
UnicodeASCIITransformer ToASCII = DefaultToASCII;

void URL::InitEnvironment(const InitOptions& options) {
  if (options.to_ascii) {
    ToASCII = options.to_ascii;
  }

  if (options.to_unicode) {
    ToUnicode = options.to_unicode;
  }
}

// https://url.spec.whatwg.org/#url-serializing
std::string URL::SerializeURL(const UrlData& url, bool exclude = false) {
  std::string output;
  output.reserve(
      10 +  // We generally insert < 10 separator characters between URL parts
      url.scheme.size() + url.username.size() + url.password.size() +
      url.host.size() + url.query.size() + url.fragment.size() +
      url.href.size() +
      std::accumulate(
          url.path.begin(), url.path.end(), 0, [](size_t sum, const auto& str) {
            return sum + str.size();
          }));

  output += url.scheme;
  if (url.flags & kUrlFlagsHasHost) {
    output += "//";
    if (url.flags & kUrlFlagsHasUsername || url.flags & kUrlFlagsHasPassword) {
      if (url.flags & kUrlFlagsHasUsername) {
        output += url.username;
      }

      if (url.flags & kUrlFlagsHasPassword) {
        output += ":" + url.password;
      }

      output += "@";
    }

    output += url.host;
    if (url.port != -1) {
      output += ":" + std::to_string(url.port);
    }
  }

  if (url.flags & kUrlFlagsCannotBeBase) {
    output += url.path[0];
  } else {
    if (!(url.flags & kUrlFlagsHasHost) && url.path.size() > 1 &&
        url.path[0].empty()) {
      output += "/.";
    }

    for (size_t i = 1; i < url.path.size(); i++) {
      output += "/" + url.path[i];
    }
  }

  if (url.flags & kUrlFlagsHasQuery) {
    output += "?" + url.query;
  }

  if (!exclude && (url.flags & kUrlFlagsHasFragment)) {
    output += "#" + url.fragment;
  }

  output.shrink_to_fit();
  return output;
}

URL URL::FromFilePath(const std::string& file_path) {
  URL url("file://");
  std::string escaped_file_path;
  for (size_t i = 0; i < file_path.length(); ++i) {
    escaped_file_path += file_path[i];
    if (file_path[i] == '%')
      escaped_file_path += "25";
  }

  URL::Parse(escaped_file_path.c_str(), escaped_file_path.length(), kPathStart,
             &url._context, true, nullptr, false);
  return url;
}

std::string URL::ToFilePath() const {
  if (_context.scheme != "file:") {
    return "";
  }

#ifdef _WIN32
  const char* slash = "\\";
  auto is_slash = [] (char ch) {
    return ch == '/' || ch == '\\';
  };
#else
  const char* slash = "/";
  auto is_slash = [] (char ch) {
    return ch == '/';
  };

  if ((_context.flags & kUrlFlagsHasHost) &&
      _context.host.length() > 0) {
    return "";
  }
#endif

  std::string decoded_path;
  for (const std::string& part : _context.path) {
    std::string decoded = PercentDecode(part.c_str(), part.length());
    for (char& ch : decoded) {
      if (is_slash(ch)) {
        return "";
      }
    }
    decoded_path += slash + decoded;
  }

#ifdef _WIN32
  // TODO(TimothyGu): Use "\\?\" long paths on Windows.

  // If hostname is set, then we have a UNC path. Pass the hostname through
  // ToUnicode just in case it is an IDN using punycode encoding. We do not
  // need to worry about percent encoding because the URL parser will have
  // already taken care of that for us. Note that this only causes IDNs with an
  // appropriate `xn--` prefix to be decoded.
  if ((context_.flags & URL_FLAGS_HAS_HOST) &&
      context_.host.length() > 0) {
    std::string unicode_host;
    if (!ToUnicode(context_.host, &unicode_host)) {
      return "";
    }
    return "\\\\" + unicode_host + decoded_path;
  }
  // Otherwise, it's a local path that requires a drive letter.
  if (decoded_path.length() < 3) {
    return "";
  }
  if (decoded_path[2] != ':' ||
      !IsASCIIAlpha(decoded_path[1])) {
    return "";
  }
  // Strip out the leading '\'.
  return decoded_path.substr(1);
#else
  return decoded_path;
#endif
}

}  // namespace whatwgurl
