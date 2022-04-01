#include <whatwgurl.h>
#include <whatwgurl_host.h>
#include "whatwgurl-inl.h"

namespace whatwgurl {

using table_data::C0_CONTROL_ENCODE_SET;
using table_data::FRAGMENT_ENCODE_SET;
using table_data::PATH_ENCODE_SET;
using table_data::QUERY_ENCODE_SET_NONSPECIAL;
using table_data::QUERY_ENCODE_SET_SPECIAL;
using table_data::USERINFO_ENCODE_SET;

inline bool ParseHost(const std::string& input,
                      std::string* output,
                      bool is_special,
                      bool unicode = false) {
  if (input.empty()) {
    output->clear();
    return true;
  }

  URLHost host;
  host.ParseHost(input.c_str(), input.length(), is_special, unicode);
  if (host.ParsingFailed()) return false;
  *output = host.ToStringMove();
  return true;
}

void URL::Parse(const char* input,
                size_t len,
                UrlParseState state_override,
                UrlData* url,
                bool has_url,
                const struct UrlData* base,
                bool has_base) {
  const char* p = input;
  const char* end = input + len;

  if (!has_url) {
    for (const char* ptr = p; ptr < end; ptr++) {
      if (IsC0ControlOrSpace(*ptr))
        p++;
      else
        break;
    }
    for (const char* ptr = end - 1; ptr >= p; ptr--) {
      if (IsC0ControlOrSpace(*ptr))
        end--;
      else
        break;
    }
    input = p;
    len = end - p;
  }

  // The spec says we should strip out any ASCII tabs or newlines.
  // In those cases, we create another std::string instance with the filtered
  // contents, but in the general case we avoid the overhead.
  std::string whitespace_stripped;
  for (const char* ptr = p; ptr < end; ptr++) {
    if (!IsASCIITabOrNewline(*ptr)) continue;
    // Hit tab or newline. Allocate storage, copy what we have until now,
    // and then iterate and filter all similar characters out.
    whitespace_stripped.reserve(len - 1);
    whitespace_stripped.assign(p, ptr - p);
    // 'ptr + 1' skips the current char, which we know to be tab or newline.
    for (ptr = ptr + 1; ptr < end; ptr++) {
      if (!IsASCIITabOrNewline(*ptr)) whitespace_stripped += *ptr;
    }

    // Update variables like they should have looked like if the string
    // had been stripped of whitespace to begin with.
    input = whitespace_stripped.c_str();
    len = whitespace_stripped.size();
    p = input;
    end = input + len;
    break;
  }

  bool atflag = false;                    // Set when @ has been seen.
  bool square_bracket_flag = false;       // Set inside of [...]
  bool password_token_seen_flag = false;  // Set after a : after an username.

  std::string buffer;

  // Set the initial parse state.
  const bool has_state_override = state_override != kUnknownState;
  UrlParseState state = has_state_override ? state_override : kSchemeStart;

  if (state < kSchemeStart || state > kFragment) {
    url->flags |= kUrlFlagsInvalidParseState;
    return;
  }

  while (p <= end) {
    const char ch = p < end ? p[0] : kEOL;
    bool special = (url->flags & kUrlFlagsSpecial);
    bool cannot_be_base;
    bool special_back_slash = (special && ch == '\\');

    switch (state) {
      case kSchemeStart:
        if (IsASCIIAlpha(ch)) {
          buffer += ASCIILowercase(ch);
          state = kScheme;
        } else if (!has_state_override) {
          state = kNoScheme;
          continue;
        } else {
          url->flags |= kUrlFlagsFailed;
          return;
        }

        break;

      case kScheme:
        if (IsASCIIAlphanumeric(ch) || ch == '+' || ch == '-' || ch == '.') {
          buffer += ASCIILowercase(ch);
        } else if (ch == ':' || (has_state_override && ch == kEOL)) {
          if (has_state_override && buffer.size() == 0) {
            url->flags |= kUrlFlagsTerminated;
            return;
          }
          buffer += ':';

          bool new_is_special = IsSpecial(buffer);

          if (has_state_override) {
            if ((special != new_is_special) ||
                ((buffer == "file:") &&
                 ((url->flags & kUrlFlagsHasUsername) ||
                  (url->flags & kUrlFlagsHasPassword) || (url->port != -1))) ||
                (url->scheme == "file:" && url->host.empty())) {
              url->flags |= kUrlFlagsTerminated;
              return;
            }
          }

          url->scheme = std::move(buffer);
          url->port = NormalizePort(url->scheme, url->port);
          if (new_is_special) {
            url->flags |= kUrlFlagsSpecial;
            special = true;
          } else {
            url->flags &= ~kUrlFlagsSpecial;
            special = false;
          }

          special_back_slash = (special && ch == '\\');
          buffer.clear();

          if (has_state_override) return;
          if (url->scheme == "file:") {
            state = kFile;
          } else if (special && has_base && url->scheme == base->scheme) {
            state = kSpecialRelativeOrAuthority;
          } else if (special) {
            state = kSpecialAuthoritySlashes;
          } else if (p + 1 < end && p[1] == '/') {
            state = kPathOrAuthority;
            p++;
          } else {
            url->flags |= kUrlFlagsCannotBeBase;
            url->flags |= kUrlFlagsHasPath;
            url->path.emplace_back("");
            state = kCannotBeBase;
          }
        } else if (!has_state_override) {
          buffer.clear();
          state = kNoScheme;
          p = input;
          continue;
        } else {
          url->flags |= kUrlFlagsFailed;
          return;
        }

        break;

      case kNoScheme:
        cannot_be_base = has_base && (base->flags & kUrlFlagsCannotBeBase);
        if (!has_base || (cannot_be_base && ch != '#')) {
          url->flags |= kUrlFlagsFailed;
          return;
        } else if (cannot_be_base && ch == '#') {
          url->scheme = base->scheme;
          if (IsSpecial(url->scheme)) {
            url->flags |= kUrlFlagsSpecial;
            special = true;
          } else {
            url->flags &= ~kUrlFlagsSpecial;
            special = false;
          }

          special_back_slash = (special && ch == '\\');
          if (base->flags & kUrlFlagsHasPath) {
            url->flags |= kUrlFlagsHasPath;
            url->path = base->path;
          }

          if (base->flags & kUrlFlagsHasQuery) {
            url->flags |= kUrlFlagsHasQuery;
            url->query = base->query;
          }

          if (base->flags & kUrlFlagsHasFragment) {
            url->flags |= kUrlFlagsHasFragment;
            url->fragment = base->fragment;
          }

          url->flags |= kUrlFlagsCannotBeBase;
          state = kFragment;
        } else if (has_base && base->scheme != "file:") {
          state = kRelative;
          continue;
        } else {
          url->scheme = "file:";
          url->flags |= kUrlFlagsSpecial;
          special = true;
          state = kFile;
          special_back_slash = (special && ch == '\\');
          continue;
        }

        break;

      case kSpecialRelativeOrAuthority:
        if (ch == '/' && p + 1 < end && p[1] == '/') {
          state = kSpecialAuthorityIgnoreSlashes;
          p++;
        } else {
          state = kRelative;
          continue;
        }

        break;

      case kPathOrAuthority:
        if (ch == '/') {
          state = kAuthority;
        } else {
          state = kPath;
          continue;
        }

        break;

      case kRelative:
        url->scheme = base->scheme;
        if (IsSpecial(url->scheme)) {
          url->flags |= kUrlFlagsSpecial;
          special = true;
        } else {
          url->flags &= ~kUrlFlagsSpecial;
          special = false;
        }

        special_back_slash = (special && ch == '\\');
        switch (ch) {
          case kEOL:
            if (base->flags & kUrlFlagsHasUsername) {
              url->flags |= kUrlFlagsHasUsername;
              url->username = base->username;
            }

            if (base->flags & kUrlFlagsHasPassword) {
              url->flags |= kUrlFlagsHasPassword;
              url->password = base->password;
            }

            if (base->flags & kUrlFlagsHasHost) {
              url->flags |= kUrlFlagsHasHost;
              url->host = base->host;
            }

            if (base->flags & kUrlFlagsHasQuery) {
              url->flags |= kUrlFlagsHasQuery;
              url->query = base->query;
            }

            if (base->flags & kUrlFlagsHasPath) {
              url->flags |= kUrlFlagsHasPath;
              url->path = base->path;
            }

            url->port = base->port;
            break;

          case '/':
            state = kRelativeSlash;
            break;

          case '?':
            if (base->flags & kUrlFlagsHasUsername) {
              url->flags |= kUrlFlagsHasUsername;
              url->username = base->username;
            }

            if (base->flags & kUrlFlagsHasPassword) {
              url->flags |= kUrlFlagsHasPassword;
              url->password = base->password;
            }

            if (base->flags & kUrlFlagsHasHost) {
              url->flags |= kUrlFlagsHasHost;
              url->host = base->host;
            }

            if (base->flags & kUrlFlagsHasPath) {
              url->flags |= kUrlFlagsHasPath;
              url->path = base->path;
            }

            url->port = base->port;
            state = kQuery;
            break;

          case '#':
            if (base->flags & kUrlFlagsHasUsername) {
              url->flags |= kUrlFlagsHasUsername;
              url->username = base->username;
            }

            if (base->flags & kUrlFlagsHasPassword) {
              url->flags |= kUrlFlagsHasPassword;
              url->password = base->password;
            }

            if (base->flags & kUrlFlagsHasHost) {
              url->flags |= kUrlFlagsHasHost;
              url->host = base->host;
            }

            if (base->flags & kUrlFlagsHasQuery) {
              url->flags |= kUrlFlagsHasQuery;
              url->query = base->query;
            }

            if (base->flags & kUrlFlagsHasPath) {
              url->flags |= kUrlFlagsHasPath;
              url->path = base->path;
            }

            url->port = base->port;
            state = kFragment;

            break;

          default:
            if (special_back_slash) {
              state = kRelativeSlash;
            } else {
              if (base->flags & kUrlFlagsHasUsername) {
                url->flags |= kUrlFlagsHasUsername;
                url->username = base->username;
              }

              if (base->flags & kUrlFlagsHasPassword) {
                url->flags |= kUrlFlagsHasPassword;
                url->password = base->password;
              }

              if (base->flags & kUrlFlagsHasHost) {
                url->flags |= kUrlFlagsHasHost;
                url->host = base->host;
              }

              if (base->flags & kUrlFlagsHasPath) {
                url->flags |= kUrlFlagsHasPath;
                url->path = base->path;
                ShortenUrlPath(url);
              }

              url->port = base->port;
              state = kPath;
              continue;
            }
        }

        break;

      case kRelativeSlash:
        if (IsSpecial(url->scheme) && (ch == '/' || ch == '\\')) {
          state = kSpecialAuthorityIgnoreSlashes;
        } else if (ch == '/') {
          state = kAuthority;
        } else {
          if (base->flags & kUrlFlagsHasUsername) {
            url->flags |= kUrlFlagsHasUsername;
            url->username = base->username;
          }

          if (base->flags & kUrlFlagsHasPassword) {
            url->flags |= kUrlFlagsHasPassword;
            url->password = base->password;
          }

          if (base->flags & kUrlFlagsHasHost) {
            url->flags |= kUrlFlagsHasHost;
            url->host = base->host;
          }

          url->port = base->port;
          state = kPath;
          continue;
        }

        break;

      case kSpecialAuthoritySlashes:
        state = kSpecialAuthorityIgnoreSlashes;
        if (ch == '/' && p + 1 < end && p[1] == '/') {
          p++;
        } else {
          continue;
        }

        break;

      case kSpecialAuthorityIgnoreSlashes:
        if (ch != '/' && ch != '\\') {
          state = kAuthority;
          continue;
        }

        break;

      case kAuthority:
        if (ch == '@') {
          if (atflag) {
            buffer.reserve(buffer.size() + 3);
            buffer.insert(0, "%40");
          }

          atflag = true;
          size_t blen = buffer.size();
          if (blen > 0 && buffer[0] != ':') {
            url->flags |= kUrlFlagsHasUsername;
          }

          for (size_t n = 0; n < blen; n++) {
            const char bch = buffer[n];
            if (bch == ':') {
              url->flags |= kUrlFlagsHasPassword;
              if (!password_token_seen_flag) {
                password_token_seen_flag = true;
                continue;
              }
            }

            if (password_token_seen_flag) {
              AppendOrEscape(&url->password, bch, USERINFO_ENCODE_SET);
            } else {
              AppendOrEscape(&url->username, bch, USERINFO_ENCODE_SET);
            }
          }

          buffer.clear();
        } else if (ch == kEOL || ch == '/' || ch == '?' || ch == '#' ||
                   special_back_slash) {
          if (atflag && buffer.size() == 0) {
            url->flags |= kUrlFlagsFailed;
            return;
          }

          p -= buffer.size() + 1;
          buffer.clear();
          state = kHost;
        } else {
          buffer += ch;
        }

        break;

      case kHost:
      case kHostname:
        if (has_state_override && url->scheme == "file:") {
          state = kFileHost;
          continue;
        } else if (ch == ':' && !square_bracket_flag) {
          if (buffer.size() == 0) {
            url->flags |= kUrlFlagsFailed;
            return;
          }

          if (state_override == kHostname) {
            return;
          }

          url->flags |= kUrlFlagsHasHost;
          if (!ParseHost(buffer, &url->host, special)) {
            url->flags |= kUrlFlagsFailed;
            return;
          }

          buffer.clear();
          state = kPort;
        } else if (ch == kEOL || ch == '/' || ch == '?' || ch == '#' ||
                   special_back_slash) {
          p--;
          if (special && buffer.size() == 0) {
            url->flags |= kUrlFlagsFailed;
            return;
          }

          if (has_state_override && buffer.size() == 0 &&
              ((url->username.size() > 0 || url->password.size() > 0) ||
               url->port != -1)) {
            url->flags |= kUrlFlagsTerminated;
            return;
          }

          url->flags |= kUrlFlagsHasHost;
          if (!ParseHost(buffer, &url->host, special)) {
            url->flags |= kUrlFlagsFailed;
            return;
          }

          buffer.clear();
          state = kPathStart;
          if (has_state_override) {
            return;
          }
        } else {
          if (ch == '[') square_bracket_flag = true;
          if (ch == ']') square_bracket_flag = false;
          buffer += ch;
        }

        break;

      case kPort:
        if (IsASCIIDigit(ch)) {
          buffer += ch;
        } else if (has_state_override || ch == kEOL || ch == '/' || ch == '?' ||
                   ch == '#' || special_back_slash) {
          if (buffer.size() > 0) {
            unsigned port = 0;

            // the condition port <= 0xffff prevents integer overflow
            for (size_t i = 0; port <= 0xffff && i < buffer.size(); i++)
              port = port * 10 + buffer[i] - '0';

            if (port > 0xffff) {
              // TODO(TimothyGu): This hack is currently needed for the host
              // setter since it needs access to hostname if it is valid, and
              // if the FAILED flag is set the entire response to JS layer
              // will be empty.
              if (state_override == kHost)
                url->port = -1;
              else
                url->flags |= kUrlFlagsFailed;
              return;
            }

            // the port is valid
            url->port = NormalizePort(url->scheme, static_cast<int>(port));
            if (url->port == -1) url->flags |= kUrlFlagsIsDefaultSchemePort;
            buffer.clear();
          } else if (has_state_override) {
            // TODO(TimothyGu): Similar case as above.
            if (state_override == kHost)
              url->port = -1;
            else
              url->flags |= kUrlFlagsTerminated;

            return;
          }

          state = kPathStart;
          continue;
        } else {
          url->flags |= kUrlFlagsFailed;
          return;
        }

        break;

      case kFile:
        url->scheme = "file:";
        url->host.clear();
        url->flags |= kUrlFlagsHasHost;
        if (ch == '/' || ch == '\\') {
          state = kFileSlash;
        } else if (has_base && base->scheme == "file:") {
          switch (ch) {
            case kEOL:
              if (base->flags & kUrlFlagsHasHost) {
                url->host = base->host;
              }

              if (base->flags & kUrlFlagsHasPath) {
                url->flags |= kUrlFlagsHasPath;
                url->path = base->path;
              }

              if (base->flags & kUrlFlagsHasQuery) {
                url->flags |= kUrlFlagsHasQuery;
                url->query = base->query;
              }

              break;

            case '?':
              if (base->flags & kUrlFlagsHasHost) {
                url->host = base->host;
              }

              if (base->flags & kUrlFlagsHasPath) {
                url->flags |= kUrlFlagsHasPath;
                url->path = base->path;
              }

              url->flags |= kUrlFlagsHasQuery;
              url->query.clear();
              state = kQuery;

              break;

            case '#':
              if (base->flags & kUrlFlagsHasHost) {
                url->host = base->host;
              }

              if (base->flags & kUrlFlagsHasPath) {
                url->flags |= kUrlFlagsHasPath;
                url->path = base->path;
              }

              if (base->flags & kUrlFlagsHasQuery) {
                url->flags |= kUrlFlagsHasQuery;
                url->query = base->query;
              }

              url->flags |= kUrlFlagsHasFragment;
              url->fragment.clear();
              state = kFragment;

              break;

            default:
              url->query.clear();
              if (base->flags & kUrlFlagsHasHost) {
                url->host = base->host;
              }

              if (base->flags & kUrlFlagsHasPath) {
                url->flags |= kUrlFlagsHasPath;
                url->path = base->path;
              }

              if (!StartsWithWindowsDriveLetter(p, end)) {
                ShortenUrlPath(url);
              } else {
                url->path.clear();
              }

              state = kPath;
              continue;
          }
        } else {
          state = kPath;
          continue;
        }

        break;
      case kFileSlash:
        if (ch == '/' || ch == '\\') {
          state = kFileHost;
        } else {
          if (has_base && base->scheme == "file:") {
            url->flags |= kUrlFlagsHasHost;
            url->host = base->host;
            if (!StartsWithWindowsDriveLetter(p, end) &&
                IsNormalizedWindowsDriveLetter(base->path[0])) {
              url->flags |= kUrlFlagsHasPath;
              url->path.push_back(base->path[0]);
            }
          }

          state = kPath;
          continue;
        }

        break;

      case kFileHost:
        if (ch == kEOL || ch == '/' || ch == '\\' || ch == '?' || ch == '#') {
          if (!has_state_override && buffer.size() == 2 &&
              IsWindowsDriveLetter(buffer)) {
            state = kPath;
          } else if (buffer.size() == 0) {
            url->flags |= kUrlFlagsHasHost;
            url->host.clear();
            if (has_state_override) return;
            state = kPathStart;
          } else {
            std::string host;
            if (!ParseHost(buffer, &host, special)) {
              url->flags |= kUrlFlagsFailed;
              return;
            }

            if (host == "localhost") host.clear();
            url->flags |= kUrlFlagsHasHost;
            url->host = host;
            if (has_state_override) return;
            buffer.clear();
            state = kPathStart;
          }

          continue;
        } else {
          buffer += ch;
        }

        break;

      case kPathStart:
        if (IsSpecial(url->scheme)) {
          state = kPath;
          if (ch != '/' && ch != '\\') {
            continue;
          }
        } else if (!has_state_override && ch == '?') {
          url->flags |= kUrlFlagsHasQuery;
          url->query.clear();
          state = kQuery;
        } else if (!has_state_override && ch == '#') {
          url->flags |= kUrlFlagsHasFragment;
          url->fragment.clear();
          state = kFragment;
        } else if (ch != kEOL) {
          state = kPath;
          if (ch != '/') {
            continue;
          }
        } else if (has_state_override && !(url->flags & kUrlFlagsHasHost)) {
          url->flags |= kUrlFlagsHasPath;
          url->path.emplace_back("");
        }

        break;

      case kPath:
        if (ch == kEOL || ch == '/' || special_back_slash ||
            (!has_state_override && (ch == '?' || ch == '#'))) {
          if (IsDoubleDotSegment(buffer)) {
            ShortenUrlPath(url);
            if (ch != '/' && !special_back_slash) {
              url->flags |= kUrlFlagsHasPath;
              url->path.emplace_back("");
            }
          } else if (IsSingleDotSegment(buffer) && ch != '/' &&
                     !special_back_slash) {
            url->flags |= kUrlFlagsHasPath;
            url->path.emplace_back("");
          } else if (!IsSingleDotSegment(buffer)) {
            if (url->scheme == "file:" && url->path.empty() &&
                buffer.size() == 2 && IsWindowsDriveLetter(buffer)) {
              buffer[1] = ':';
            }
            url->flags |= kUrlFlagsHasPath;
            url->path.emplace_back(std::move(buffer));
          }

          buffer.clear();
          if (ch == '?') {
            url->flags |= kUrlFlagsHasQuery;
            url->query.clear();
            state = kQuery;
          } else if (ch == '#') {
            url->flags |= kUrlFlagsHasFragment;
            url->fragment.clear();
            state = kFragment;
          }
        } else {
          AppendOrEscape(&buffer, ch, PATH_ENCODE_SET);
        }

        break;

      case kCannotBeBase:
        switch (ch) {
          case '?':
            state = kQuery;
            break;

          case '#':
            state = kFragment;
            break;

          default:
            if (url->path.empty())
              url->path.emplace_back("");
            else if (ch != kEOL)
              AppendOrEscape(&url->path[0], ch, C0_CONTROL_ENCODE_SET);
        }

        break;

      case kQuery:
        if (ch == kEOL || (!has_state_override && ch == '#')) {
          url->flags |= kUrlFlagsHasQuery;
          url->query = std::move(buffer);
          buffer.clear();
          if (ch == '#') state = kFragment;
        } else {
          AppendOrEscape(
              &buffer,
              ch,
              special ? QUERY_ENCODE_SET_SPECIAL : QUERY_ENCODE_SET_NONSPECIAL);
        }

        break;

      case kFragment:
        switch (ch) {
          case kEOL:
            url->flags |= kUrlFlagsHasFragment;
            url->fragment = std::move(buffer);
            break;

          default:
            AppendOrEscape(&buffer, ch, FRAGMENT_ENCODE_SET);
        }

        break;

      default:
        url->flags |= kUrlFlagsInvalidParseState;
        return;
    }

    p++;
  }
}  // NOLINT(readability/fn_size)

}  // namespace whatwgurl
