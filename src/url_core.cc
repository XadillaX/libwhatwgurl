#include "url_core.h"
#include "parse.h"
#include "percent_encode.h"
#include "scheme.h"
#include "url_core-inl.h"

namespace whatwgurl {

void InitEnvironment(const InitParams& params) {
  InitSchemePorts();
  kIDNAToASCII = params.idna_to_ascii;
}

void CleanEnvironment() {
  kSchemePorts.clear();
  kIDNAToASCII = nullptr;
}

using std::make_shared;
using std::shared_ptr;
using std::string;

URLCore::URLCore(const string& url) {
  _failed = !Parse(url, nullptr, &_parsed_url, &_validation_error);
}

URLCore::URLCore(const string& url, const string& base) {
  shared_ptr<ParsedURL> base_url;
  _failed = !Parse(base, nullptr, &base_url, &_validation_error);
  if (_failed) return;

  _failed = !Parse(url, base_url.get(), &_parsed_url, &_validation_error);
}

URLCore::URLCore(const string& url, const URLCore& base) {
  if (base.failed()) {
    _failed = true;
    return;
  }

  _failed =
      !Parse(url, base._parsed_url.get(), &_parsed_url, &_validation_error);
}

URLCore::URLCore(const string& url, const ParsedURL& base) {
  _failed = !Parse(url, &base, &_parsed_url, &_validation_error);
}

URLCore::~URLCore() {
  _parsed_url.reset();

  if (_cached_search_params.get()) {
    _cached_search_params->_url = nullptr;
    _cached_search_params.reset();
  }
}

bool URLCore::set_href(const string& url) {
  CHECK(!_failed);

  // Let parsedURL be the result of running the basic URL parser on the given
  // value.
  shared_ptr<ParsedURL> parsed_url;
  bool validation_error;
  bool failed = !Parse(url, nullptr, &parsed_url, &validation_error);

  // If parsedURL is failure, then throw a TypeError.
  if (failed) {
    return false;
  }

  // Set this’s URL to parsedURL.
  _parsed_url = parsed_url;

  // Empty this’s query object’s list.
  if (_cached_search_params.get()) {
    _cached_search_params->_list.clear();

    // Let query be this’s URL’s query.

    // If query is non-null, then set this’s query object’s list to the result
    // of parsing query.
    if (!_parsed_url->query.is_null()) {
      _cached_search_params->Initialize(_parsed_url->query.value());
    }

    _cached_search_params->EmitPassivelyUpdate();
  }

  return true;
}

string URLCore::origin() const {
  TupleOrigin origin;
  GetOriginObject(&origin);
  return origin.Serialize();
}

bool URLCore::set_protocol(const string& protocol) {
  CHECK(!_failed);

  // The protocol setter steps are to basic URL parse the given value, followed
  // by U+003A (:), with this’s URL as url and scheme start state as state
  // override.
  return Parse(protocol + ":",
               nullptr,
               &_parsed_url,
               &_validation_error,
               kSchemeStartState);
}

#define V(name)                                                                \
  /* The ?? setter steps are: */                                               \
  CHECK(!_failed);                                                             \
                                                                               \
  /* If this’s URL cannot have a username/password/port, then return. */     \
  if (_parsed_url->CannotHaveUsernamePasswordPort()) {                         \
    return false;                                                              \
  }                                                                            \
                                                                               \
  /* Set the ?? given this’s URL and the given value. */                     \
  SET_THE_USERNAME_OR_PASSWORD(name);                                          \
  return true

bool URLCore::set_username(const string& username) {
  V(username);
}

bool URLCore::set_password(const string& password) {
  V(password);
}

#undef V

string URLCore::host() const {
  CHECK(!_failed);

  // If url’s host is null, then return the empty string.
  if (_parsed_url->host.is_null()) return "";

  // If url’s port is null, return url’s host, serialized.
  if (_parsed_url->port.is_null()) {
    return _parsed_url->host->Serialize();
  }

  // Return url’s host, serialized, followed by U+003A (:) and url’s port,
  // serialized.
  return _parsed_url->host->Serialize() + ":" + _parsed_url->port.Serialize();
}

bool URLCore::set_host(const string& host) {
  // The host setter steps are:
  CHECK(!_failed);

  // If this’s URL has an opaque path, then return.
  if (_parsed_url->HasOpaquePath()) return false;

  // Basic URL parse the given value with this’s URL as url and host state as
  // state override.
  return Parse(host, nullptr, &_parsed_url, &_validation_error, kHostState);
}

string URLCore::hostname() const {
  CHECK(!_failed);

  // If url’s host is null, then return the empty string.
  if (_parsed_url->host.is_null()) return "";

  // Return this’s URL’s host, serialized.
  return _parsed_url->host->Serialize();
}

bool URLCore::set_hostname(const string& hostname) {
  // The hostname setter steps are:
  CHECK(!_failed);

  // If this’s URL has an opaque path, then return.
  if (_parsed_url->HasOpaquePath()) return false;

  // Basic URL parse the given value with this’s URL as url and hostname state
  // as state override.
  return Parse(
      hostname, nullptr, &_parsed_url, &_validation_error, kHostNameState);
}

string URLCore::port() const {
  CHECK(!_failed);

  // If this’s URL’s port is null, then return the empty string.
  if (_parsed_url->port.is_null()) return "";

  // Return this’s URL’s port, serialized.
  return _parsed_url->port.Serialize();
}

bool URLCore::set_port(const string& port) {
  // The port setter steps are:
  CHECK(!_failed);

  // If this’s URL cannot have a username/password/port, then return.
  if (_parsed_url->CannotHaveUsernamePasswordPort()) return false;

  // If the given value is the empty string, then set this’s URL’s port to null.
  if (port.empty()) {
    _parsed_url->port = nullptr;
    return true;
  }

  // Otherwise, basic URL parse the given value with this’s URL as url and port
  // state as state override.
  return Parse(port, nullptr, &_parsed_url, &_validation_error, kPortState);
}

bool URLCore::set_pathname(const string& pathname) {
  // The pathname setter steps are:
  CHECK(!_failed);

  // If this’s URL has an opaque path, then return.
  if (_parsed_url->HasOpaquePath()) return false;

  // Empty this’s URL’s path.
  _parsed_url->path.Clear();

  // Basic URL parse the given value with this’s URL as url and path start state
  // as state override.
  return Parse(
      pathname, nullptr, &_parsed_url, &_validation_error, kPathStartState);
}

string URLCore::search() const {
  CHECK(!_failed);

  MaybeNull<std::string>& query = _parsed_url->query;

  // If this’s URL’s query is either null or the empty string, then return the
  // empty string.
  if (query.is_null() || query->empty()) {
    return "";
  }

  // Return U+003F (?), followed by this’s URL’s query.
  return "?" + *query;
}

bool URLCore::set_search(const string& search) {
  // The search setter steps are:
  CHECK(!_failed);

  // Let url be this’s URL.
  ParsedURL* url = _parsed_url.get();

  // If the given value is the empty string, set url’s query to null, empty
  // this’s query object’s list, and then return.
  if (search.empty()) {
    url->query = nullptr;

    if (_cached_search_params.get()) {
      _cached_search_params->_list.clear();
      _cached_search_params->EmitPassivelyUpdate();
    }

    return true;
  }

  // Let input be the given value with a single leading U+003F (?) removed, if
  // any.
  string input = search;
  if (input.length() > 0 && input[0] == '?') {
    input = input.substr(1);
  }

  // Set url’s query to the empty string.
  url->query = "";

  // Basic URL parse input with url as url and query state as state override.
  if (!Parse(input, nullptr, &_parsed_url, &_validation_error, kQueryState)) {
    return false;
  }

  // Set this’s query object’s list to the result of parsing input.
  if (_cached_search_params.get()) {
    _cached_search_params->_list.clear();

    if (!_parsed_url->query.is_null()) {
      _cached_search_params->Initialize(_parsed_url->query.value());
    }

    _cached_search_params->EmitPassivelyUpdate();
  }

  return true;
}

shared_ptr<URLSearchParams> URLCore::search_params() {
  CHECK(!_failed);

  // The searchParams getter steps are to return this’s query object.
  if (_cached_search_params.get()) return _cached_search_params;

  // Set this’s query object to a new URLSearchParams object. Initialize this’s
  // query object with query. Set this’s query object’s URL object to this.
  _cached_search_params =
      make_shared<URLSearchParams>(*_parsed_url->query, true);
  _cached_search_params->_url = this;

  return _cached_search_params;
}

string URLCore::hash() const {
  CHECK(!_failed);

  MaybeNull<std::string>& fragment = _parsed_url->fragment;

  // If this’s URL’s fragment is either null or the empty string, then return
  // the empty string.
  if (fragment.is_null() || fragment->empty()) {
    return "";
  }

  // Return U+0023 (#), followed by this’s URL’s fragment.
  return "#" + *fragment;
}

bool URLCore::set_hash(const string& hash) {
  // The hash setter steps are:
  CHECK(!_failed);

  // If the given value is the empty string, then set this’s URL’s fragment to
  // null and return.
  if (hash.empty()) {
    _parsed_url->fragment = nullptr;
    return true;
  }

  // Let input be the given value with a single leading U+0023 (#) removed, if
  // any.
  string input = hash;
  if (input.length() > 0 && input[0] == '#') {
    input = input.substr(1);
  }

  // Set this’s URL’s fragment to the empty string.
  _parsed_url->fragment = "";

  // Basic URL parse input with this’s URL as url and fragment state as state
  // override.
  return Parse(
      input, nullptr, &_parsed_url, &_validation_error, kFragmentState);
}

// The origin of a URL url is the origin returned by running these steps,
// switching on url’s scheme:
void URLCore::GetOriginObject(TupleOrigin* origin) const {
  CHECK(!_failed);

  // "blob"
  if (_parsed_url->scheme == "blob") {
    URLCore parsed(_parsed_url->path.ASCIIString());
    if (parsed.failed()) {
      *origin = nullptr;
      return;
    }

    parsed.GetOriginObject(origin);
    return;
  }

  // "file"
  if (_parsed_url->scheme == "file") {
    // Unfortunate as it is, this is left as an exercise to the reader. When in
    // doubt, return a new opaque origin.
    *origin = nullptr;
    return;
  }

  // "ftp", "http", "https", "ws", "wss"
  if (_parsed_url->IsSpecial()) {
    // Return the tuple origin (url’s scheme, url’s host, url’s port, null).
    CHECK(!_parsed_url->host.is_null());
    TupleOriginItem item;
    item.scheme = _parsed_url->scheme;
    item.host = _parsed_url->host.value();
    item.port = _parsed_url->port;
    if (item.host.type == kDomain) {
      item.domain = item.host.domain();
    } else {
      item.domain = nullptr;
    }

    origin->SetValue(item);
    return;
  }

  // Otherwise: Return a new opaque origin.
  *origin = nullptr;
  return;
}

// The URL serializer takes a URL url, with an optional boolean exclude fragment
// (default false), and then runs these steps. They return an ASCII string.
string URLCore::Serialize(bool exclude_fragment) const {
  CHECK(!_failed);

  // Let output be url’s scheme and U+003A (:) concatenated.
  string output = _parsed_url->scheme + ":";
  printf("Serialize: %s\n", output.c_str());

  // If url’s host is non-null:
  Host& host = _parsed_url->host;
  if (!host.is_null()) {
    // Append "//" to output.
    output += "//";

    // If url includes credentials, then:
    if (_parsed_url->IncludeCredentials()) {
      // Append url’s username to output.
      output += _parsed_url->username;

      // If url’s password is not the empty string, then append U+003A (:),
      // followed by url’s password, to output.
      if (!_parsed_url->password.empty()) {
        output += ":" + _parsed_url->password;
      }

      // Append U+0040 (@) to output.
      output += "@";
    }

    // Append url’s host, serialized, to output.
    output += host->Serialize();

    // If url’s port is non-null, append U+003A (:) followed by url’s port,
    // serialized, to output.
    if (!_parsed_url->port.is_null()) {
      output += ":" + _parsed_url->port.Serialize();
    }
  }

  // If url’s host is null, url does not have an opaque path, url’s path’s size
  // is greater than 1, and url’s path[0] is the empty string, then append
  // U+002F (/) followed by U+002E (.) to output.
  if (host.is_null() && !_parsed_url->HasOpaquePath() &&
      _parsed_url->path.size() > 1 && _parsed_url->path[0] == "") {
    // This prevents web+demo:/.//not-a-host/ or web+demo:/path/..//not-a-host/,
    // when parsed and then serialized, from ending up as web+demo://not-a-host/
    // (they end up as web+demo:/.//not-a-host/).
    output += "/.";
  }

  // Append the result of URL path serializing url to output.
  output += SerializePath();

  // If url’s query is non-null, append U+003F (?), followed by url’s query, to
  // output.
  if (!_parsed_url->query.is_null()) {
    output += "?" + *_parsed_url->query;
  }

  // If exclude fragment is false and url’s fragment is non-null, then append
  // U+0023 (#), followed by url’s fragment, to output.
  if (!exclude_fragment && !_parsed_url->fragment.is_null()) {
    output += "#" + *_parsed_url->fragment;
  }

  // Return output.
  return output;
}

string URLCore::SerializePath() const {
  CHECK(!_failed);

  // If url has an opaque path, then return url’s path.
  if (_parsed_url->HasOpaquePath()) {
    return _parsed_url->path.ASCIIString();
  }

  // Let output be the empty string.
  string output = "";

  // For each segment of url’s path: append U+002F (/) followed by segment to
  // output.
  size_t size = _parsed_url->path.size();
  for (size_t i = 0; i < size; ++i) {
    output += "/";
    output += _parsed_url->path[i];
  }

  // Return output.
  return output;
}

}  // namespace whatwgurl
