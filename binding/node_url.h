#ifndef BINDING_NODE_URL_H_
#define BINDING_NODE_URL_H_

#include <nan.h>
#include <memory>
#include "node_url_search_params.h"
#include "url_core.h"

namespace whatwgurl {

class URLInternal : public URLCore {
 public:
  inline explicit URLInternal(const std::string& url) : URLCore(url) {}
  inline URLInternal(const std::string& url, const URLInternal& base)
      : URLCore(url, base) {}

  v8::MaybeLocal<v8::Object> ToStructedV8Object() const;
};

#define URL_SETTERS(V)                                                         \
  V(protocol, Protocol)                                                        \
  V(username, Username)                                                        \
  V(password, Password)                                                        \
  V(host, Host)                                                                \
  V(hostname, Hostname)                                                        \
  V(port, Port)                                                                \
  V(pathname, Pathname)                                                        \
  V(search, Search)                                                            \
  V(hash, Hash)

class NodeURL : public Nan::ObjectWrap {
 public:
  static NAN_MODULE_INIT(Init);

 public:
  static NAN_METHOD(New);
  static NAN_METHOD(ToStructed);
  static NAN_METHOD(GetURLSearchParams);
  static NAN_METHOD(SetOnPassivelyUpdateFunction);

  static NAN_METHOD(SetHref);
#define V(_, camel_name) static NAN_METHOD(Set##camel_name);
  URL_SETTERS(V)
#undef V

 private:
  NodeURL(v8::Isolate* isolate, std::unique_ptr<URLInternal> url);
  void OnPassivelyUpdate();

 private:
  static Nan::Global<v8::Function> constructor;

  std::unique_ptr<URLInternal> _internal;
  Nan::Global<v8::Function> _on_passively_update;
  v8::Isolate* _isolate;
};

}  // namespace whatwgurl

#endif  // BINDING_NODE_URL_H_
