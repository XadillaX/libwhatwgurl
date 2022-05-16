#include "parse.h"
#include "code_points.h"
#include "percent_encode.h"
#include "scheme.h"
#include "string_utils.h"
#include "temp_string_buffer.h"
#include "utils/assert.h"
#include "utils/resizable_buffer.h"

namespace whatwgurl {

using std::make_shared;
using std::shared_ptr;
using std::string;

constexpr char kEOF = -1;

bool Parse(const string& input,
           const ParsedURL* base,
           shared_ptr<ParsedURL>* url,
           bool* validation_error,
           ParseState state_override) {
  // If `url` is not given, set `url` to a new `ParsedURL`
  shared_ptr<ParsedURL>& out = *url;

  *validation_error = false;
  string trimed_url = input;

  if (!out.get()) {
    out = make_shared<ParsedURL>();

    // If `input` contains any leading or trailing C0 control or space,
    // validation error.
    //
    // And then Remove any leading and trailing C0 control or space from input.
    if (string_utils::Trim(&trimed_url, IsC0ControlOrSpaceFunction)) {
      *validation_error = true;
    }
  }

  // If input contains any ASCII tab or newline, validation error. Then remove
  // all ASCII tab or newline from input.
  if (string_utils::Remove(&trimed_url, IsASCIITabOrNewlineFunction)) {
    *validation_error = true;
  }

  // Let state be state override if given, or scheme start state otherwise.
  bool state_override_is_given = state_override != kNotGiven;
  ParseState state =
      state_override == kNotGiven ? kSchemeStartState : state_override;

  // TODO(XadillaX): Set encoding to the result of getting an output encoding
  // from encoding.

  // Let atSignSeen, insideBrackets, and passwordTokenSeen be false.
  bool at_sign_seen = false;
  bool inside_brackets = false;
  bool password_token_seen = false;

  // Let buffer be the empty string.
  TempStringBuffer buffer(trimed_url.length());

  // Let pointer be a pointer for input.
  const unsigned char* ptr =
      reinterpret_cast<const unsigned char*>(trimed_url.c_str());
  const unsigned char* end = ptr + trimed_url.length();

  // Keep running the following state machine by switching on state. If after a
  // run pointer points to the EOF code point, go to the next step. Otherwise,
  // increase pointer by 1 and continue with the state machine.
  while (ptr <= end) {
    unsigned char c = (ptr == end ? kEOF : *ptr);

    // printf("pos: %zu, %c, state: %d\n",
    //        reinterpret_cast<const char*>(ptr) - trimed_url.c_str(),
    //        c,
    //        state);

    switch (state) {
      // https://url.spec.whatwg.org/#scheme-start-state
      case kSchemeStartState: {
        // If c is an ASCII alpha, append c, lowercased, to buffer, and set
        // state to scheme state.
        if (IsASCIIAlpha(c)) {
          buffer.Append(string_utils::ToLower(c));
          state = kSchemeState;
          break;

          // Otherwise, if state override is not given, set state to no scheme
          // state and decrease pointer by 1.
        } else if (!state_override_is_given) {
          state = kNoSchemeState;
          --ptr;
          break;

          // Otherwise, validation error, return failure.
        } else {
          *validation_error = true;
          return false;
        }

        break;
      }

      // https://url.spec.whatwg.org/#scheme-state
      case kSchemeState: {
        // If c is an ASCII alphanumeric, U+002B (+), U+002D (-), or U+002E (.),
        // append c, lowercased, to buffer.
        if (IsASCIILowerAlpha(c) || IsASCIIDigit(c) || c == '+' || c == '-' ||
            c == '.') {
          buffer.Append(c);
          break;
        } else if (IsASCIIUpperAlpha(c)) {
          buffer.Append(string_utils::ToLower(c));
          break;
        }

        // Otherwise, if c is U+003A (:), then:
        if (c == ':') {
          // If state override is given, then:
          if (state_override_is_given) {
            bool is_scheme_special = out->IsSpecial();
            bool is_buffer_special = IsSpecialScheme(buffer.string());

            // If url's scheme is a special scheme and buffer is not a special
            // scheme, then return. And if url’s scheme is not a special scheme
            // and buffer is a special scheme, then return.
            if (is_scheme_special ^ is_buffer_special) {
              return true;
            }

            // If url includes credentials or has a non-null port, and buffer is
            // "file", then return.
            if ((out->IncludeCredentials() || !out->port.is_null()) &&
                strcmp(buffer.string(), "file") == 0) {
              return true;
            }

            // If url's scheme is "file" and its host is an empty host, then
            // return.
            if (out->scheme == "file" && !out->host.is_null() &&
                out->host->type == HostType::kEmptyHost) {
              return true;
            }
          }

          // Set url’s scheme to buffer.
          out->scheme = buffer.string();

          // If state override is given, then:
          if (state_override_is_given) {
            // If url’s port is url’s scheme’s default port, then set url’s port
            // to null.
            DefaultPortResult default_port = GetDefaultPort(out->scheme);
            if (!default_port.is_null && !out->port.is_null() &&
                out->port.value() == default_port.port) {
              out->port = nullptr;
            }

            // Return.
            return true;
          }

          // Set buffer to the empty string.
          buffer.SetEmpty();

          bool is_special = out->IsSpecial();

          // If url’s scheme is "file", then:
          if (out->scheme == "file") {
            // If remaining does not start with "//", validation error.
            if (ptr + 2 >= end || ptr[1] != '/' || ptr[2] != '/') {
              *validation_error = true;
            }

            // Set state to file state.
            state = kFileState;
            break;

            // Otherwise, if url is special, base is non-null, and base’s scheme
            // is url’s scheme:
          } else if (is_special && base && base->scheme == out->scheme) {
            // Assert: base is is special (and therefore does not have an opaque
            // path).
            CHECK(base->IsSpecial());

            // Set state to special relative or authority state.
            state = kSpecialRelativeOrAuthorityState;
            break;

            // Otherwise, if url is special, set state to special authority
            // slashes state.
          } else if (is_special) {
            state = kSpecialAuthoritySlashesState;
            break;

            // Otherwise, if remaining starts with an U+002F (/), set state to
            // path or authority state and increase pointer by 1.
          } else if (ptr + 1 < end && ptr[1] == '/') {
            state = kPathOrAuthorityState;
            ++ptr;
            break;

            // Otherwise, set url’s path to the empty string and set state to
            // opaque path state.
          } else {
            out->path.Reset(true);
            state = kOpaquePathState;
            break;
          }

          // Otherwise, if state override is not given, set buffer to the empty
          // string, state to no scheme state, and start over (from the first
          // code point in input).
        } else if (!state_override_is_given) {
          buffer.SetEmpty();
          state = kNoSchemeState;

          // DANGER: This pointer is `start - 1`, and it will be incremented
          // by 1 after the `switch`.
          ptr = reinterpret_cast<const unsigned char*>(trimed_url.c_str()) - 1;

          break;

          // Otherwise, validation error, return failure.
        } else {
          *validation_error = true;
          return false;
        }

        break;
      }

      // https://url.spec.whatwg.org/#no-scheme-state
      case kNoSchemeState: {
        // If base is null, or base has an opaque path and c is not U+0023 (#),
        // validation error, return failure.
        if (base == nullptr || (base->path.IsOpaquePath() && c != '#')) {
          *validation_error = true;
          return false;
        }

        // Otherwise, if base has an opaque path and c is U+0023 (#), set url’s
        // scheme to base’s scheme, url’s path to base’s path, url’s query to
        // base’s query, url’s fragment to the empty string, and set state to
        // fragment state.
        if (base->path.IsOpaquePath() && c == '#') {
          out->scheme = base->scheme;
          out->path = base->path;
          out->query = base->query;
          out->fragment = "";
          state = kFragmentState;
          break;

          // Otherwise, if base’s scheme is not "file", set state to relative
          // state and decrease pointer by 1.
        } else if (base->scheme != "file") {
          state = kRelativeState;
          --ptr;
          break;
        }

        // Otherwise, set state to file state and decrease pointer by 1.
        state = kFileState;
        --ptr;

        break;
      }

      // https://url.spec.whatwg.org/#special-relative-or-authority-state
      case kSpecialRelativeOrAuthorityState: {
        // If c is U+002F (/) and remaining starts with U+002F (/), then set
        // state to special authority ignore slashes state and increase pointer
        // by 1.
        if (c == '/' && ptr + 1 < end && ptr[1] == '/') {
          state = kSpecialAuthorityIgnoreSlashesState;
          ++ptr;
          break;
        }

        // Otherwise, validation error, set state to relative state and
        // decrease pointer by 1.
        state = kRelativeState;
        --ptr;
        *validation_error = true;

        break;
      }

      // https://url.spec.whatwg.org/#path-or-authority-state
      case kPathOrAuthorityState: {
        // If c is U+002F (/), then set state to authority state.
        if (c == '/') {
          state = kAuthorityState;
          break;
        }

        // Otherwise, set state to path state, and decrease pointer by 1.
        state = kPathState;
        --ptr;

        break;
      }

        // https://url.spec.whatwg.org/#relative-state
      case kRelativeState: {
        // Assert: base’s scheme is not "file".
        CHECK_NE(base->scheme, "file");

        // Set url’s scheme to base’s scheme.
        out->scheme = base->scheme;

        // If c is U+002F (/), then set state to relative slash state.
        if (c == '/') {
          state = kRelativeSlashState;
          break;

          // Otherwise, if url is special and c is U+005C (\), validation error,
          // set state to relative slash state.
        } else if (out->IsSpecial() && c == '\\') {
          state = kRelativeSlashState;
          *validation_error = true;
          break;
        }

        // Otherwise:

        // Set url’s username to base’s username, url’s password to base’s
        // password, url’s host to base’s host, url’s port to base’s port, url’s
        // path to a clone of base’s path, and url’s query to base’s query.
        out->username = base->username;
        out->password = base->password;
        out->host = base->host;
        out->port = base->port;
        out->path = base->path;
        out->query = base->query;

        switch (static_cast<char>(c)) {
          // If c is U+003F (?), then set url’s query to the empty string, and
          // state to query state.
          case '?':
            out->query = "";
            state = kQueryState;
            break;

          // Otherwise, if c is U+0023 (#), set url’s fragment to the empty
          // string and state to fragment state.
          case '#':
            out->fragment = "";
            state = kFragmentState;
            break;

          case kEOF:
            break;

            // Otherwise, if c is not the EOF code point:
          default:
            // Set url’s query to null.
            out->query = nullptr;

            // Shorten url’s path.
            out->path.Shorten(*out);

            // Set state to path state and decrease pointer by 1.
            state = kPathState;
            --ptr;

            break;
        }

        break;
      }

      // https://url.spec.whatwg.org/#relative-slash-state
      case kRelativeSlashState: {
        // If url is special and c is U+002F (/) or U+005C (\), then:
        if (out->IsSpecial() && (c == '/' || c == '\\')) {
          // If c is U+005C (\), validation error.
          if (c == '\\') {
            *validation_error = true;
          }

          // Set state to special authority ignore slashes state.
          state = kSpecialAuthorityIgnoreSlashesState;
          break;

          // Otherwise, if c is U+002F (/), then set state to authority state.
        } else if (c == '/') {
          state = kAuthorityState;
          break;
        }

        // Otherwise, set url’s username to base’s username, url’s password to
        // base’s password, url’s host to base’s host, url’s port to base’s
        // port, state to path state, and then, decrease pointer by 1.
        out->username = base->username;
        out->password = base->password;
        out->host = base->host;
        out->port = base->port;
        state = kPathState;
        --ptr;
        break;
      }

      // https://url.spec.whatwg.org/#special-authority-slashes-state
      case kSpecialAuthoritySlashesState: {
        state = kSpecialAuthorityIgnoreSlashesState;

        // If c is U+002F (/) and remaining starts with U+002F (/), then set
        // state to special authority ignore slashes state and increase pointer
        // by 1.
        if (c == '/' && ptr + 1 < end && ptr[1] == '/') {
          ++ptr;

          // Otherwise, validation error, set state to special authority ignore
          // slashes state and decrease pointer by 1.
        } else {
          --ptr;
          *validation_error = true;
        }

        break;
      }

      // https://url.spec.whatwg.org/#special-authority-ignore-slashes-state
      case kSpecialAuthorityIgnoreSlashesState: {
        // If c is neither U+002F (/) nor U+005C (\), then set state to
        // authority state and decrease pointer by 1.
        if (c != '/' && c != '\\') {
          state = kAuthorityState;
          --ptr;
          break;
        }

        // Otherwise, validation error.
        *validation_error = true;

        break;
      }

      // https://url.spec.whatwg.org/#authority-state
      case kAuthorityState: {
        switch (static_cast<char>(c)) {
          // If c is U+0040 (@), then:
          case '@': {
            // Validation error.
            *validation_error = true;

            // If atSignSeen is true, then prepend "%40" to buffer.
            if (at_sign_seen) {
              buffer.Prepend("%40");
            }

            // Set atSignSeen to true.
            at_sign_seen = true;

            // For each codePoint in buffer:
            const unsigned char* code_point_ptr = buffer.unsigned_string();
            const unsigned char* code_point_end =
                code_point_ptr + buffer.length();

            TempStringBuffer code_point_buffer(PER_PERCENT_HEX_LENGTH);
            while (code_point_ptr < code_point_end) {
              unsigned char code_point = *code_point_ptr;

              // If codePoint is U+003A (:) and passwordTokenSeen is false, then
              // set passwordTokenSeen to true and continue.
              if (code_point == ':' && !password_token_seen) {
                password_token_seen = true;
                code_point_ptr++;
                continue;
              }

              // Let encodedCodePoints be the result of running UTF-8
              // percent-encode codePoint using the userinfo percent-encode set.
              percent_encode::EncodeByUserInfoPercentEncodeSet(
                  code_point_ptr, 1, &code_point_buffer, false);

              // If passwordTokenSeen is true, then append encodedCodePoints to
              // url’s password.
              if (password_token_seen) {
                out->password += code_point_buffer.string();

                // Otherwise, append encodedCodePoints to url’s username.
              } else {
                out->username += code_point_buffer.string();
              }

              code_point_ptr++;
            }

            // Set buffer to the empty string.
            buffer.SetEmpty();

            break;
          }

          // Otherwise, if one of the following is true:
          // c is the EOF code point, U+002F (/), U+003F (?), or U+0023 (#)
          case kEOF:
          case '/':
          case '?':
          case '#':
          // url is special and c is U+005C (\)
          case '\\': {
            if (c != '\\' || out->IsSpecial()) {
              // then:

              // If atSignSeen is true and buffer is the empty string,
              // validation error, return failure.
              if (at_sign_seen && !buffer.length()) {
                *validation_error = true;
                return false;
              }

              // Decrease pointer by the number of code points in buffer plus
              // one, set buffer to the empty string, and set state to host
              // state.
              ptr -= buffer.length() + 1;
              buffer.SetEmpty();
              state = kHostState;

              break;
            } else {
              // fallthrough
            }
          }

          // Otherwise, append c to buffer.
          default:
            buffer.Append(c);
            break;
        }

        break;
      }

      // https://url.spec.whatwg.org/#host-state
      case kHostState:
      // https://url.spec.whatwg.org/#hostname-state
      case kHostNameState: {
        bool is_special = out->IsSpecial();

#define HANDLE_HOST_STATE_HOST_NAME_STATE_3_4_5(next_state)                    \
  /* 3. Let host be the result of host parsing buffer with url is not */       \
  /* special. */                                                               \
  Host host;                                                                   \
  bool parsed =                                                                \
      Host::Parse(buffer, &host, validation_error, !out->IsSpecial());         \
                                                                               \
  /* 4. If host is failure, then return failure. */                            \
  if (!parsed) {                                                               \
    return false;                                                              \
  }                                                                            \
                                                                               \
  /* 5. Set url’s host to host, buffer to the empty string, and state to */  \
  /* next_state. */                                                            \
  out->host = host;                                                            \
  buffer.SetEmpty();                                                           \
  state = next_state

        // If state override is given and url’s scheme is "file", then decrease
        // pointer by 1 and set state to file host state.
        if (state_override_is_given && out->scheme == "file") {
          --ptr;
          state = kFileHostState;
          break;

          // Otherwise, if c is U+003A (:) and insideBrackets is false, then:
        } else if (c == ':' && !inside_brackets) {
          // If buffer is the empty string, validation error, return failure.
          if (!buffer.length()) {
            *validation_error = true;
            return false;
          }

          // If state override is given and state override is hostname state,
          // then return.
          if (state_override_is_given && state_override == kHostNameState) {
            return true;
          }

          HANDLE_HOST_STATE_HOST_NAME_STATE_3_4_5(kPortState);

          break;

          // Otherwise, if one of the following is true:
          //
          // 1. c is the EOF code point, U+002F (/), U+003F (?), or U+0023 (#)
          // 2. url is special and c is U+005C (\)
        } else if (static_cast<char>(c) == kEOF || c == '/' || c == '?' ||
                   c == '#' || (c == '\\' && is_special)) {
          // then decrease pointer by 1, and then:
          --ptr;

          // If url is special and buffer is the empty string, validation error,
          // return failure.
          if (is_special && !buffer.length()) {
            *validation_error = true;
            return false;

            // Otherwise, if state override is given, buffer is the empty
            // string, and either url includes credentials or url’s port is
            // non-null, return.
          } else if (state_override_is_given && !buffer.length() &&
                     (out->IncludeCredentials() || !out->port.is_null())) {
            return true;
          }

          HANDLE_HOST_STATE_HOST_NAME_STATE_3_4_5(kPathStartState);

          // If state override is given, then return.
          if (state_override_is_given) {
            return true;
          }

          // Otherwise:
          break;
        } else {
          switch (c) {
            // If c is U+005B ([), then set insideBrackets to true.
            case '[':
              inside_brackets = true;
              break;

            // If c is U+005D (]), then set insideBrackets to false.
            case ']':
              inside_brackets = false;
              break;

            default:
              break;
          }

          // Append c to buffer.
          buffer.Append(c);
          break;
        }

#undef HANDLE_HOST_STATE_HOST_NAME_STATE_3_4_5

        break;
      }

      // https://url.spec.whatwg.org/#port-state
      case kPortState: {
        bool is_special = out->IsSpecial();

        // If c is an ASCII digit, append c to buffer.
        if (IsASCIIDigit(c)) {
          buffer.Append(c);
          break;
        }

        bool hit = false;
        // Otherwise, if one of the following is true:
        switch (static_cast<char>(c)) {
          // c is the EOF code point, U+002F (/), U+003F (?), or U+0023 (#)
          case kEOF:
          case '/':
          case '?':
          case '#':
            hit = true;
            break;

          // url is special and c is U+005C (\)
          // state override is given
          default:
            hit = (is_special && c == '\\') || state_override_is_given;
            break;
        }

        // Otherwise, validation error, return failure.
        if (!hit) {
          *validation_error = true;
          return false;
        }

        // If buffer is not the empty string, then:
        if (buffer.length()) {
          // Let port be the mathematical integer value that is represented by
          // buffer in radix-10 using ASCII digits for digits with values 0
          // through 9.
          uint32_t port = buffer.ToInteger();

          // If port is greater than 2^16 − 1, validation error, return failure.
          if (port > 0xffff) {
            *validation_error = true;
            return false;
          }

          // Set url’s port to null, if port is url’s scheme’s default port;
          // otherwise to port.
          DefaultPortResult default_port = GetDefaultPort(out->scheme);
          if (!default_port.is_null && port == default_port.port) {
            out->port.SetValue(nullptr);
          } else {
            out->port.SetValue(port);
          }

          // Set buffer to the empty string.
          buffer.SetEmpty();
        }

        // If state override is given, then return.
        if (state_override_is_given) {
          return true;
        }

        // Set state to path start state and decrease pointer by 1.
        state = kPathStartState;
        --ptr;

        break;
      }

      // https://url.spec.whatwg.org/#file-state
      case kFileState: {
        // Set url’s scheme to "file".
        out->scheme = "file";

        // Set url’s host to the empty string.
        out->host.SetEmptyHost();

        // If c is U+002F (/) or U+005C (\), then:
        if (c == '/' || c == '\\') {
          // If c is U+005C (\), validation error.
          if (c == '\\') {
            *validation_error = true;
          }

          // Set state to file slash state.
          state = kFileSlashState;
          break;

          // Otherwise, if base is non-null and base’s scheme is "file":
        } else if (base && base->scheme == "file") {
          // Set url’s host to base’s host, url’s path to a clone of base’s
          // path, and url’s query to base’s query.
          out->host = base->host;
          out->path = base->path;
          out->query = base->query;

          switch (static_cast<char>(c)) {
            // If c is U+003F (?), then set url’s query to the empty string and
            // state to query state.
            case '?':
              out->query = "";
              state = kQueryState;
              break;

            // Otherwise, if c is U+0023 (#), set url’s fragment to the empty
            // string and state to fragment state.
            case '#':
              out->fragment = "";
              state = kFragmentState;
              break;

            // Otherwise, if c is not the EOF code point:
            case kEOF:
              break;

            default:
              // Set url’s query to null.
              out->query.SetValue(nullptr);

              // If the code point substring from pointer to the end of input
              // does not start with a Windows drive letter, then shorten url’s
              // path.
              if (!Path::IsStartsWithWindowsDriveLetter(
                      reinterpret_cast<const char*>(ptr))) {
                out->path.Shorten(*out);

                // Otherwise:
                // This is a (platform-independent) Windows drive letter quirk.
              } else {
                // Validation error.
                *validation_error = true;

                // Set url’s path to an empty list.
                out->path.Reset(false);
              }

              // Set state to path state and decrease pointer by 1.
              state = kPathState;
              --ptr;
              break;
          }

          break;

          // Otherwise, set state to path state, and decrease pointer by 1.
        } else {
          state = kPathState;
          --ptr;
        }

        break;
      }

      // https://url.spec.whatwg.org/#file-slash-state
      case kFileSlashState: {
        switch (c) {
          // If c is U+002F (/) or U+005C (\), then:
          case '/':
          case '\\':
            // If c is U+005C (\), validation error.
            if (c == '\\') {
              *validation_error = true;
            }

            // Set state to file host state.
            state = kFileHostState;

            break;

          // Otherwise:
          default:
            // If base is non-null and base’s scheme is "file", then:
            if (base && base->scheme == "file") {
              // Set url’s host to base’s host.
              out->host = base->host;

              // If the code point substring from pointer to the end of input
              // does not start with a Windows drive letter and base’s path[0]
              // is a normalized Windows drive letter, then append base’s
              // path[0] to url’s path.
              //
              // This is a (platform-independent) Windows drive letter quirk.
              if (!Path::IsStartsWithWindowsDriveLetter(
                      reinterpret_cast<const char*>(ptr)) &&
                  !base->path.IsOpaquePath() && base->path.size() &&
                  Path::IsNormalizedWindowsDriverLetter(
                      base->path.Front().c_str())) {
                out->path.PushBack(base->path.Front());
              }
            }

            // Set state to path state, and decrease pointer by 1.
            state = kPathState;
            --ptr;

            break;
        }

        break;
      }

      // https://url.spec.whatwg.org/#file-host-state
      case kFileHostState: {
        // If c is the EOF code point, U+002F (/), U+005C (\), U+003F (?), or
        // U+0023 (#), then decrease pointer by 1 and then:
        switch (static_cast<char>(c)) {
          case kEOF:
          case '/':
          case '\\':
          case '?':
          case '#': {
            --ptr;

            // If state override is not given and buffer is a Windows drive
            // letter, validation error, set state to path state.
            if (!state_override_is_given &&
                Path::IsWindowsDriveLetter(buffer.string())) {
              // This is a (platform-independent) Windows drive letter quirk.
              // buffer is not reset here and instead used in the path state.
              *validation_error = true;
              state = kPathState;
              break;

              // Otherwise, if buffer is the empty string, then:
            } else if (!buffer.length()) {
              // Set url’s host to the empty string.
              out->host.SetEmptyHost();

              // If state override is given, then return.
              if (state_override_is_given) {
                return true;
              }

              // Set state to path start state.
              state = kPathStartState;
              break;

              // Otherwise, run these steps:
            } else {
              // Let host be the result of host parsing buffer with url is not
              // special.
              Host host;
              bool parsed = Host::Parse(
                  buffer, &host, validation_error, !out->IsSpecial());

              // If host is failure, then return failure.
              if (!parsed) {
                return false;
              }

              // If host is "localhost", then set host to the empty string.
              if (!host.is_null()) {
                string host_string;
                switch (host->type) {
                  case kOpaqueHost:
                    host_string = host->opaque_host();
                    break;
                  case kDomain:
                    host_string = host->domain();
                    break;
                  default:
                    break;
                }

                if (host_string == "localhost") {
                  host.SetEmptyHost();
                }
              }

              // Set url’s host to host.
              out->host = host;

              // If state override is given, then return.
              if (state_override_is_given) {
                return true;
              }

              // Set buffer to the empty string and state to path start state.
              buffer.SetEmpty();
              state = kPathStartState;

              break;
            }
          }

          default: {
            // Otherwise, append c to buffer.
            buffer.Append(c);
            break;
          }
        }

        break;
      }

      // https://url.spec.whatwg.org/#path-start-state
      case kPathStartState: {
        bool is_special = out->IsSpecial();

        // If url is special, then:
        if (is_special) {
          // If c is U+005C (\), validation error.
          if (c == '\\') {
            *validation_error = true;
          }

          // Set state to path state.
          state = kPathState;

          // If c is neither U+002F (/) nor U+005C (\), then decrease pointer
          // by 1.
          if (c != '/' && c != '\\') {
            --ptr;
          }

          break;

          // Otherwise, if state override is not given
        } else if (!state_override_is_given) {
          bool breaked = false;
          switch (c) {
            // and c is U+003F (?), set url’s query to the empty string and
            // state to query state.
            case '?':
              out->query = "";
              state = kQueryState;
              breaked = true;
              break;

            // and c is U+0023 (#), set url’s fragment to the empty string and
            // state to fragment state.
            case '#':
              out->fragment = "";
              state = kFragmentState;
              breaked = true;
              break;

            // Otherwise, do not break
            default:
              break;
          }

          if (breaked) {
            break;
          }
        }

        // Otherwise, if c is not the EOF code point:
        if (static_cast<char>(c) != kEOF) {
          // Set state to path state.
          state = kPathState;

          // If c is not U+002F (/), then decrease pointer by 1.
          if (c != '/') {
            --ptr;
          }
          break;

          // Otherwise, if state override is given and url’s host is null,
          // append the empty string to url’s path.
        } else if (state_override_is_given && out->host.is_null()) {
          out->path.PushBack("");
          break;
        }

        break;
      }

      // https://url.spec.whatwg.org/#path-state
      case kPathState: {
        bool is_special = out->IsSpecial();

        // If one of the following is true:
        //
        // + c is the EOF code point or U+002F (/)
        // + url is special and c is U+005C (\)
        // + state override is not given and c is U+003F (?) or U+0023 (#)
        //
        // then:
        if (static_cast<char>(c) == kEOF || c == '/' ||
            (is_special && c == '\\') ||
            (!state_override_is_given && (c == '?' || c == '#'))) {
          // If url is special and c is U+005C (\), validation error.
          if (is_special && c == '\\') {
            *validation_error = true;
          }

          // If buffer is a double-dot path segment, then:
          if (Path::IsDoubleDotPathSegment(buffer.string(), buffer.length())) {
            // Shorten url’s path.
            out->path.Shorten(*out);

            // If neither c is U+002F (/), nor url is special and c is U+005C
            // (\), append the empty string to url’s path.
            //
            // This means that for input /usr/.. the result is / and not a
            // lack of a path.
            if (c != '/' && (!is_special || c != '\\')) {
              out->path.PushBack("");
            }
          } else {
            bool is_single_dot_path_segment =
                Path::IsSingleDotPathSegment(buffer.string(), buffer.length());

            // Otherwise, if buffer is a single-dot path segment and if
            // neither c is U+002F (/), nor url is special and c is U+005C
            // (\), append the empty string to url’s path.
            if (is_single_dot_path_segment && c != '/' &&
                (!is_special || c != '\\')) {
              out->path.PushBack("");

              // Otherwise, if buffer is not a single-dot path segment, then:
            } else if (!is_single_dot_path_segment) {
              // If url’s scheme is "file", url’s path is empty, and buffer is
              // a Windows drive letter, then replace the second code point in
              // buffer with U+003A (:).
              //
              // This is a (platform-independent) Windows drive letter quirk.
              if (out->scheme == "file" && !out->path.size() &&
                  Path::IsWindowsDriveLetter(buffer.string())) {
                buffer.Replace(1, ':');
              }

              // Append buffer to url’s path.
              out->path.PushBack(buffer.string());
            }
          }

          // Set buffer to the empty string.
          buffer.SetEmpty();

          // If c is U+003F (?), then set url’s query to the empty string and
          // state to query state. If c is U+0023 (#), then set url’s fragment
          // to the empty string and state to fragment state.
          switch (c) {
            case '?':
              out->query = "";
              state = kQueryState;
              break;

            case '#':
              out->fragment = "";
              state = kFragmentState;
              break;

            default:
              break;
          }

          break;

          // Otherwise, run these steps:
        } else {
          // TODO(XadillaX): If c is not a URL code point and not U+0025 (%),
          // validation error.

          // If c is U+0025 (%) and remaining does not start with two ASCII
          // hex digits, validation error.
          if (c == '%' && (ptr + 2 >= end || !(IsASCIIHexDigit(ptr[1]) &&
                                               IsASCIIHexDigit(ptr[2])))) {
            *validation_error = true;
          }

          // UTF-8 percent-encode c using the path percent-encode set and
          // append the result to buffer.
          TempStringBuffer temp(1);
          percent_encode::EncodeByPathPercentEncodeSet(ptr, 1, &temp);
          buffer.Append(temp.string(), temp.length());

          break;
        }

        break;
      }

      // https://url.spec.whatwg.org/#cannot-be-a-base-url-path-state
      case kOpaquePathState: {
        switch (c) {
          // If c is U+003F (?), then set url’s query to the empty string and
          // state to query state.
          case '?':
            out->query = "";
            state = kQueryState;
            break;

          // Otherwise, if c is U+0023 (#), then set url’s fragment to the
          // empty string and state to fragment state.
          case '#':
            out->fragment = "";
            state = kFragmentState;
            break;

          // Otherwise:
          default:
            // TODO(XadillaX): If c is not the EOF code point, not a URL code
            // point, and not U+0025 (%), validation error.

            // If c is U+0025 (%) and remaining does not start with two ASCII
            // hex digits, validation error.
            if (c == '%' && (ptr + 2 >= end || !(IsASCIIHexDigit(ptr[1]) &&
                                                 IsASCIIHexDigit(ptr[2])))) {
              *validation_error = true;
            }

            // If c is not the EOF code point, UTF-8 percent-encode c using
            // the C0 control percent-encode set and append the result to
            // url’s path.
            if (static_cast<char>(c) != kEOF) {
              TempStringBuffer temp(1);
              percent_encode::EncodeByC0ControlPercentEncodeSet(ptr, 1, &temp);
              out->path.ASCIIString() += string(temp.string(), temp.length());
            }

            break;
        }

        break;
      }

      // https://url.spec.whatwg.org/#query-state
      case kQueryState: {
        // TODO(XadillaX): If encoding is not UTF-8 and one of the following
        // is true:
        //
        // 1. url is not special
        // 2. url’s scheme is "ws" or "wss"
        //
        // then set encoding to UTF-8.

        bool is_special = out->IsSpecial();

        // If one of the following is true:
        //
        // 1. state override is not given and c is U+0023 (#)
        // 2. c is the EOF code point
        //
        // then:
        if ((!state_override_is_given && c == '#') ||
            static_cast<char>(c) == kEOF) {
          // Let queryPercentEncodeSet be the special-query percent-encode set
          // if url is special; otherwise the query percent-encode set.
          const uint8_t* query_percent_encode_set =
              (is_special) ? percent_encode::kSpecialQueryPercentEncodeSet
                           : percent_encode::kQueryPercentEncodeSet;

          // Percent-encode after encoding, with encoding, buffer, and
          // queryPercentEncodeSet, and append the result to url’s query.
          //
          // This operation cannot be invoked code-point-for-code-point due to
          // the stateful ISO-2022-JP encoder.
          TempStringBuffer temp(buffer.length() * 2);
          percent_encode::Encode(buffer.unsigned_string(),
                                 buffer.length(),
                                 query_percent_encode_set,
                                 &temp);
          if (out->query.is_null()) {
            out->query = string(temp.string(), temp.length());
          } else {
            out->query->append(string(temp.string(), temp.length()));
          }

          // Set buffer to the empty string.
          buffer.SetEmpty();

          // If c is U+0023 (#), then set url’s fragment to the empty string
          // and state to fragment state.
          if (c == '#') {
            out->fragment = "";
            state = kFragmentState;
          }

          break;

          // Otherwise, if c is not the EOF code point:
        } else if (static_cast<char>(c) != kEOF) {
          // TODO(XadillaX): If c is not a URL code point and not U+0025 (%),
          // validation error.

          // If c is U+0025 (%) and remaining does not start with two ASCII
          // hex digits, validation error.
          if (c == '%' && (ptr + 2 >= end || !(IsASCIIHexDigit(ptr[1]) &&
                                               IsASCIIHexDigit(ptr[2])))) {
            *validation_error = true;
          }

          // Append c to buffer.
          buffer.Append(c);
          break;
        }

        break;
      }

      // https://url.spec.whatwg.org/#fragment-state
      case kFragmentState: {
        // If c is not the EOF code point, then:
        if (static_cast<char>(c) != kEOF) {
          // TODO(XadillaX): If c is not a URL code point and not U+0025 (%),
          // validation error.

          // If c is U+0025 (%) and remaining does not start with two ASCII
          // hex digits, validation error.
          if (c == '%' && (ptr + 2 >= end || !(IsASCIIHexDigit(ptr[1]) &&
                                               IsASCIIHexDigit(ptr[2])))) {
            *validation_error = true;
          }

          // UTF-8 percent-encode c using the fragment percent-encode set and
          // append the result to url’s fragment.
          TempStringBuffer temp(3);
          percent_encode::EncodeByFragmentPercentEncodeSet(ptr, 1, &temp);
          if (out->fragment.is_null()) {
            out->fragment = string(temp.string(), temp.length());
          } else {
            out->fragment->append(string(temp.string(), temp.length()));
          }
        }

        break;
      }

      default:
        char buf[64];
        snprintf(buf, sizeof(buf), "unknown state: %d", state);
        fprintf(stderr, "%s\n", buf);
        UNREACHABLE();
    }

    ++ptr;
  }

  return true;
}  // NOLINT(readability/fn_size)

}  // namespace whatwgurl
