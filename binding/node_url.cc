#include "node_url.h"

namespace whatwgurl {

using std::make_unique;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using v8::Context;
using v8::EscapableHandleScope;
using v8::Function;
using v8::FunctionTemplate;
using v8::Global;
using v8::HandleScope;
using v8::Isolate;
using v8::Local;
using v8::MaybeLocal;
using v8::Object;
using v8::ObjectTemplate;
using v8::String;
using v8::Value;

const char kURLName[] = "NativeURL";

MaybeLocal<Object> URLInternal::ToStructedV8Object() const {
  Nan::EscapableHandleScope scope;
  Local<Object> obj = Nan::New<Object>();

  string value = href();
  Nan::Set(obj,
           Nan::New<String>("href").ToLocalChecked(),
           Nan::New<String>(value.c_str(), value.length()).ToLocalChecked());
  value = origin();
  Nan::Set(obj,
           Nan::New<String>("origin").ToLocalChecked(),
           Nan::New<String>(value.c_str(), value.length()).ToLocalChecked());
  value = protocol();
  Nan::Set(obj,
           Nan::New<String>("protocol").ToLocalChecked(),
           Nan::New<String>(value.c_str(), value.length()).ToLocalChecked());
  value = username();
  Nan::Set(obj,
           Nan::New<String>("username").ToLocalChecked(),
           Nan::New<String>(value.c_str(), value.length()).ToLocalChecked());
  value = password();
  Nan::Set(obj,
           Nan::New<String>("password").ToLocalChecked(),
           Nan::New<String>(value.c_str(), value.length()).ToLocalChecked());
  value = host();
  Nan::Set(obj,
           Nan::New<String>("host").ToLocalChecked(),
           Nan::New<String>(value.c_str(), value.length()).ToLocalChecked());
  value = hostname();
  Nan::Set(obj,
           Nan::New<String>("hostname").ToLocalChecked(),
           Nan::New<String>(value.c_str(), value.length()).ToLocalChecked());
  value = port();
  Nan::Set(obj,
           Nan::New<String>("port").ToLocalChecked(),
           Nan::New<String>(value.c_str(), value.length()).ToLocalChecked());
  value = pathname();
  Nan::Set(obj,
           Nan::New<String>("pathname").ToLocalChecked(),
           Nan::New<String>(value.c_str(), value.length()).ToLocalChecked());
  value = search();
  Nan::Set(obj,
           Nan::New<String>("search").ToLocalChecked(),
           Nan::New<String>(value.c_str(), value.length()).ToLocalChecked());
  value = hash();
  Nan::Set(obj,
           Nan::New<String>("hash").ToLocalChecked(),
           Nan::New<String>(value.c_str(), value.length()).ToLocalChecked());

  return scope.Escape(obj);
}

Nan::Global<Function> NodeURL::constructor;

NAN_MODULE_INIT(NodeURL::Init) {
  Local<String> name = Nan::New<String>(kURLName).ToLocalChecked();
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);

  Nan::SetPrototypeMethod(tpl, "toStructed", ToStructed);
  Nan::SetPrototypeMethod(tpl, "getURLSearchParams", GetURLSearchParams);
  Nan::SetPrototypeMethod(
      tpl, "setOnPassivelyUpdateFunction", SetOnPassivelyUpdateFunction);

  Nan::SetPrototypeMethod(tpl, "setHref", SetHref);
#define V(name, camel_name)                                                    \
  Nan::SetPrototypeMethod(tpl, "set" #camel_name, Set##camel_name);

  URL_SETTERS(V)

#undef V

  tpl->SetClassName(name);
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Local<Function> ctor = Nan::GetFunction(tpl).ToLocalChecked();
  constructor.Reset(ctor);

  Nan::Set(target, name, ctor);
}

NAN_METHOD(NodeURL::New) {
  NodeURL* obj = nullptr;

  switch (info.Length()) {
    case 1: {
      CHECK(info[0]->IsString());
      Nan::Utf8String url(info[0].As<String>());
      unique_ptr<URLInternal> url_internal =
          make_unique<URLInternal>(string(*url, url.length()));
      if (url_internal->failed()) {
        Nan::ThrowError("Invalid URL");
        return;
      }

      obj = new NodeURL(info.GetIsolate(), std::move(url_internal));
      break;
    }

    case 2: {
      CHECK(info[0]->IsString());
      unique_ptr<URLInternal> url_internal;
      Nan::Utf8String url(info[0].As<String>());

      if (info[1]->IsString()) {
        Nan::Utf8String base(info[1].As<String>());

        URLInternal base_internal(string(*base, base.length()));
        if (base_internal.failed()) {
          Nan::ThrowError("Invalid base URL");
          return;
        }

        url_internal =
            make_unique<URLInternal>(string(*url, url.length()), base_internal);
      } else if (info[1]->IsObject()) {
        NodeURL* base = Nan::ObjectWrap::Unwrap<NodeURL>(info[1].As<Object>());
        if (base == nullptr || !base->_internal.get() ||
            base->_internal->failed()) {
          Nan::ThrowError("Invalid base URL");
          return;
        }

        url_internal = make_unique<URLInternal>(*url, *base->_internal);
      } else {
        Nan::ThrowError("Invalid base URL");
        return;
      }

      if (url_internal->failed()) {
        Nan::ThrowError("Invalid URL");
        return;
      }

      obj = new NodeURL(info.GetIsolate(), std::move(url_internal));
      break;
    }

    default:
      Nan::ThrowError("Invalid number of arguments");
      return;
  }

  obj->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(NodeURL::ToStructed) {
  NodeURL* url = Nan::ObjectWrap::Unwrap<NodeURL>(info.Holder());

  CHECK(url->_internal.get());
  MaybeLocal<Object> obj = url->_internal->ToStructedV8Object();
  CHECK(!obj.IsEmpty());

  info.GetReturnValue().Set(obj.ToLocalChecked());
}

NAN_METHOD(NodeURL::GetURLSearchParams) {
  NodeURL* url = Nan::ObjectWrap::Unwrap<NodeURL>(info.Holder());

  CHECK(url->_internal.get());
  shared_ptr<URLSearchParams> usp = url->_internal->search_params();

  Local<Function> ctor = Nan::New<Function>(NodeURLSearchParams::constructor);
  MaybeLocal<Object> v8_usp = Nan::NewInstance(ctor, 0, nullptr);
  CHECK(!v8_usp.IsEmpty());

  Local<Object> usp_obj = v8_usp.ToLocalChecked();
  NodeURLSearchParams* usp_js =
      Nan::ObjectWrap::Unwrap<NodeURLSearchParams>(usp_obj);
  CHECK(usp_js);

  usp_js->SetInternal(usp);
  info.GetReturnValue().Set(usp_obj);
}

NAN_METHOD(NodeURL::SetOnPassivelyUpdateFunction) {
  NodeURL* url = Nan::ObjectWrap::Unwrap<NodeURL>(info.Holder());
  url->_on_passively_update.Reset(info[0].As<Function>());
}

NAN_METHOD(NodeURL::SetHref) {
  CHECK_EQ(info.Length(), 1);
  CHECK(info[0]->IsString());

  NodeURL* url = Nan::ObjectWrap::Unwrap<NodeURL>(info.Holder());
  CHECK(url);

  Nan::Utf8String value(info[0].As<String>());
  if (!url->_internal->set_href(string(*value, value.length()))) {
    Nan::ThrowTypeError("Invalid URL");
    return;
  }
  url->OnPassivelyUpdate();
}

#define V(name, camel_name)                                                    \
  NAN_METHOD(NodeURL::Set##camel_name) {                                       \
    CHECK_EQ(info.Length(), 1);                                                \
    CHECK(info[0]->IsString());                                                \
                                                                               \
    NodeURL* url = Nan::ObjectWrap::Unwrap<NodeURL>(info.Holder());            \
    CHECK(url);                                                                \
                                                                               \
    Nan::Utf8String value(info[0].As<String>());                               \
    url->_internal->set_##name(string(*value, value.length()));                \
    url->OnPassivelyUpdate();                                                  \
  }

URL_SETTERS(V)

#undef V

NodeURL::NodeURL(Isolate* isolate, unique_ptr<URLInternal> url)
    : _internal(std::move(url)), _isolate(isolate) {
  _internal->SetOnPassivelyUpdateFunction(
      [](void* context) {
        NodeURL* url = static_cast<NodeURL*>(context);
        url->OnPassivelyUpdate();
      },
      this);
}

void NodeURL::OnPassivelyUpdate() {
  Local<Context> context = Nan::GetCurrentContext();
  Local<Function> on_passively_update =
      Nan::New<Function>(_on_passively_update);
  MaybeLocal<Object> structed = _internal->ToStructedV8Object();
  CHECK(!structed.IsEmpty());

  Nan::TryCatch try_cache;
  Local<Value> argv[] = {structed.ToLocalChecked()};
  MaybeLocal<Value> ret =
      on_passively_update->Call(context, v8::Undefined(_isolate), 1, argv);

  if (try_cache.HasCaught()) {
    try_cache.ReThrow();
    return;
  }

  CHECK(!ret.IsEmpty());
}

Nan::Global<Function> to_ascii;

// Workaround due to I can't use Node.js' internal ICU utils.
int32_t ToASCII(std::string* buf,
                const char* input,
                size_t length,
                IDNAMode mode) {
  Local<String> input_str = Nan::New<String>(input, length).ToLocalChecked();
  Local<Function> to_ascii_func = Nan::New<Function>(to_ascii);
  Local<Value> argv[] = {input_str};

  Nan::TryCatch try_cache;
  MaybeLocal<Value> ret =
      to_ascii_func->Call(Nan::GetCurrentContext(),
                          v8::Undefined(to_ascii_func->GetIsolate()),
                          1,
                          argv);

  if (try_cache.HasCaught()) {
    return -1;
  }

  CHECK(!ret.IsEmpty());
  CHECK(ret.ToLocalChecked()->IsString());

  Local<String> output_str = ret.ToLocalChecked().As<String>();
  Nan::Utf8String output_utf8(output_str);
  if (output_utf8.length() > 0) {
    *buf = string(*output_utf8, output_utf8.length());
    return output_utf8.length();
  }

  return -1;
}

NAN_METHOD(InitEnvironment) {
  Local<Object> options = info[0].As<Object>();
  Local<Function> local_to_ascii =
      Nan::Get(options, Nan::New("toASCII").ToLocalChecked())
          .ToLocalChecked()
          .As<Function>();

  to_ascii.Reset(local_to_ascii);

  InitParams params;
  params.idna_to_ascii = ToASCII;

  InitEnvironment(params);
}

NAN_MODULE_INIT(Init) {
  NodeURL::Init(target);
  NodeURLSearchParams::Init(target);
  Nan::SetMethod(target, "init", InitEnvironment);
}

NODE_MODULE(url, Init);

}  // namespace whatwgurl
