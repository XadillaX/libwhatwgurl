#ifndef INCLUDE_URL_CORE_H_
#define INCLUDE_URL_CORE_H_

#include <memory>
#include <string>
#include "idna.h"
#include "origin.h"
#include "parsed_url.h"
#include "url_search_params.h"
#include "utils/assert.h"

namespace whatwgurl {

struct InitParams {
  IDNAToASCIIFunction idna_to_ascii;
};

void InitEnvironment(const InitParams& params);
void CleanEnvironment();

class URLCore {
  friend class URLSearchParams;

 public:
  explicit URLCore(const std::string& url);
  URLCore(const std::string& url, const std::string& base);
  URLCore(const std::string& url, const URLCore& base);
  URLCore(const std::string& url, const ParsedURL& base);

  ~URLCore();

  inline bool validation_error() const { return _validation_error; }
  inline bool failed() const { return _failed || !_parsed_url.get(); }

  std::string href() const { return Serialize(); }
  bool set_href(const std::string& url);

  std::string origin() const;

  // The protocol getter steps are to return this’s URL’s scheme, followed by
  // U+003A (:).
  inline std::string protocol() const {
    CHECK(!_failed);
    return _parsed_url->scheme + ':';
  }
  bool set_protocol(const std::string& protocol);

  // The username getter steps are to return this’s URL’s username.
  inline std::string username() const {
    CHECK(!_failed);
    return _parsed_url->username;
  }
  bool set_username(const std::string& username);

  // The password getter steps are to return this’s URL’s password.
  inline std::string password() const {
    CHECK(!_failed);
    return _parsed_url->password;
  }
  bool set_password(const std::string& password);

  std::string host() const;
  bool set_host(const std::string& host);

  std::string hostname() const;
  bool set_hostname(const std::string& hostname);

  std::string port() const;
  bool set_port(const std::string& port);

  // The pathname getter steps are to return the result of URL path serializing
  // this’s URL.
  inline std::string pathname() const { return SerializePath(); }
  bool set_pathname(const std::string& pathname);

  std::string search() const;
  bool set_search(const std::string& search);

  std::shared_ptr<URLSearchParams> search_params();

  std::string hash() const;
  bool set_hash(const std::string& hash);

  inline void SetOnPassivelyUpdateFunction(OnPassivelyUpdateFunction on_update,
                                           void* context) {
    _on_passively_update = on_update;
    _on_passively_update_context = context;
  }

 private:
  void GetOriginObject(TupleOrigin* origin) const;

  std::string Serialize(bool exclude_fragment = false) const;
  std::string SerializePath() const;

  inline void EmitPassivelyUpdate() {
    if (_on_passively_update) {
      _on_passively_update(_on_passively_update_context);
    }
  }

 private:
  std::shared_ptr<ParsedURL> _parsed_url;
  std::shared_ptr<URLSearchParams> _cached_search_params;
  bool _validation_error = false;
  bool _failed = false;

  OnPassivelyUpdateFunction _on_passively_update = nullptr;
  void* _on_passively_update_context = nullptr;
};

}  // namespace whatwgurl

#endif  // INCLUDE_URL_CORE_H_
