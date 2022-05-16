#ifndef BINDING_NODE_URL_SEARCH_PARAMS_H_
#define BINDING_NODE_URL_SEARCH_PARAMS_H_

#include <nan.h>
#include "url_search_params.h"

namespace whatwgurl {

class NodeURL;
class NodeURLSearchParams : public Nan::ObjectWrap {
  friend class NodeURL;

 public:
  static NAN_MODULE_INIT(Init);

 public:
  void SetInternal(std::shared_ptr<URLSearchParams> intrnl);

 public:
  static NAN_METHOD(New);
  static NAN_METHOD(Append);
  static NAN_METHOD(Delete);
  static NAN_METHOD(Get);
  static NAN_METHOD(GetAll);
  static NAN_METHOD(Has);
  static NAN_METHOD(Set);
  static NAN_METHOD(Sort);

  static NAN_METHOD(ToString);
  static NAN_METHOD(GetIterableArray);

 private:
  static Nan::Global<v8::Function> constructor;

  explicit NodeURLSearchParams(v8::Isolate* isolate);
  void OnPassivelyUpdate();
  void GenerateIterableArray(v8::Local<v8::Array>* array);

 private:
  std::shared_ptr<URLSearchParams> _internal;
  v8::Global<v8::Array> _iterable_array;
  v8::Isolate* _isolate;
};

}  // namespace whatwgurl

#endif  // BINDING_NODE_URL_SEARCH_PARAMS_H_
