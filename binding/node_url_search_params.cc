#include "node_url_search_params.h"
#include "utils/assert.h"

namespace whatwgurl {

using std::make_shared;
using std::shared_ptr;
using std::string;
using v8::Array;
using v8::Context;
using v8::EscapableHandleScope;
using v8::Function;
using v8::FunctionTemplate;
using v8::HandleScope;
using v8::Isolate;
using v8::Local;
using v8::MaybeLocal;
using v8::Number;
using v8::Object;
using v8::ObjectTemplate;
using v8::String;
using v8::Value;

Nan::Global<Function> NodeURLSearchParams::constructor;
const char kURLSearchParamsName[] = "NativeURLSearchParams";

NAN_MODULE_INIT(NodeURLSearchParams::Init) {
  Local<String> name = Nan::New<String>(kURLSearchParamsName).ToLocalChecked();
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  tpl->SetClassName(name);

  Nan::SetPrototypeMethod(tpl, "append", Append);
  Nan::SetPrototypeMethod(tpl, "delete", Delete);
  Nan::SetPrototypeMethod(tpl, "get", Get);
  Nan::SetPrototypeMethod(tpl, "getAll", GetAll);
  Nan::SetPrototypeMethod(tpl, "has", Has);
  Nan::SetPrototypeMethod(tpl, "set", Set);
  Nan::SetPrototypeMethod(tpl, "sort", Sort);
  Nan::SetPrototypeMethod(tpl, "toString", ToString);
  Nan::SetPrototypeMethod(tpl, "getIterableArray", GetIterableArray);

  Local<Function> ctor = Nan::GetFunction(tpl).ToLocalChecked();
  constructor.Reset(ctor);
  Nan::Set(target, name, ctor);
}

NAN_METHOD(NodeURLSearchParams::New) {
  if (info.Length() == 0) {
    (new NodeURLSearchParams(info.GetIsolate()))->Wrap(info.This());
    info.GetReturnValue().Set(info.This());

    // `_internal` will be set by `NodeURL`.
    return;
  }

  enum ParamType {
    kString,
    kSequence,

    kNeither,
  } param_type = info[0]->IsString()
                     ? kString
                     : (info[0]->IsArray() ? kSequence : kNeither);
  CHECK_NE(kNeither, param_type);

  shared_ptr<URLSearchParams> intrnl;
  switch (param_type) {
    case kString: {
      Local<String> str = info[0].As<String>();
      Nan::Utf8String init(str);
      intrnl = make_shared<URLSearchParams>(string(*init, init.length()));
      break;
    }

    case kSequence: {
      Local<Array> arr = info[0].As<Array>();
      URLSearchParamsList list;
      for (uint32_t i = 0; i < arr->Length(); ++i) {
        MaybeLocal<Value> maybe_value = Nan::Get(arr, i);
        CHECK(!maybe_value.IsEmpty());
        Local<Value> value = maybe_value.ToLocalChecked();
        CHECK(value->IsArray());
        Local<Array> p = value.As<Array>();
        CHECK_EQ(2, p->Length());
        MaybeLocal<Value> maybe_key = Nan::Get(p, 0);
        CHECK(!maybe_key.IsEmpty());
        MaybeLocal<Value> maybe_content = Nan::Get(p, 1);
        CHECK(!maybe_content.IsEmpty());
        Local<Value> key = maybe_key.ToLocalChecked();
        Local<Value> content = maybe_content.ToLocalChecked();
        CHECK(key->IsString());
        CHECK(content->IsString());
        Nan::Utf8String key_utf8(key);
        Nan::Utf8String content_utf8(content);
        list.push_back(URLSearchParamsKVPairWithIndex(
            string(*key_utf8, key_utf8.length()),
            string(*content_utf8, content_utf8.length()),
            i));
      }

      intrnl = make_shared<URLSearchParams>(list);
      break;
    }

    default: {
      UNREACHABLE();
    }
  }

  CHECK(intrnl.get());
  NodeURLSearchParams* ret = new NodeURLSearchParams(info.GetIsolate());
  ret->SetInternal(intrnl);

  ret->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(NodeURLSearchParams::Append) {
  CHECK_EQ(info.Length(), 2);
  CHECK(info[0]->IsString());
  CHECK(info[1]->IsString());

  Local<String> key = info[0].As<String>();
  Local<String> value = info[1].As<String>();

  Nan::Utf8String key_utf8(key);
  Nan::Utf8String value_utf8(value);

  NodeURLSearchParams* self =
      Nan::ObjectWrap::Unwrap<NodeURLSearchParams>(info.Holder());
  self->_internal->Append(string(*key_utf8, key_utf8.length()),
                          string(*value_utf8, value_utf8.length()));
  self->OnPassivelyUpdate();
}

NAN_METHOD(NodeURLSearchParams::Delete) {
  CHECK_EQ(info.Length(), 1);
  CHECK(info[0]->IsString());

  Local<String> key = info[0].As<String>();
  Nan::Utf8String key_utf8(key);

  NodeURLSearchParams* self =
      Nan::ObjectWrap::Unwrap<NodeURLSearchParams>(info.Holder());
  self->_internal->Delete(string(*key_utf8, key_utf8.length()));
  self->OnPassivelyUpdate();
}

NAN_METHOD(NodeURLSearchParams::Get) {
  CHECK_EQ(info.Length(), 1);
  CHECK(info[0]->IsString());

  Local<String> key = info[0].As<String>();
  Nan::Utf8String key_utf8(key);

  NodeURLSearchParams* self =
      Nan::ObjectWrap::Unwrap<NodeURLSearchParams>(info.Holder());

  MaybeNullURLSearchParamsKVPair p =
      self->_internal->Get(string(*key_utf8, key_utf8.length()));
  if (p.is_null()) {
    info.GetReturnValue().SetNull();
    return;
  }

  MaybeLocal<String> maybe_value =
      Nan::New<String>((*p).value.c_str(), (*p).value.length());
  CHECK(!maybe_value.IsEmpty());

  info.GetReturnValue().Set(maybe_value.ToLocalChecked());
}

NAN_METHOD(NodeURLSearchParams::GetAll) {
  CHECK_EQ(info.Length(), 1);
  CHECK(info[0]->IsString());

  Local<String> key = info[0].As<String>();
  Nan::Utf8String key_utf8(key);

  NodeURLSearchParams* self =
      Nan::ObjectWrap::Unwrap<NodeURLSearchParams>(info.Holder());

  URLSearchParamsList list;
  self->_internal->GetAll(string(*key_utf8, key_utf8.length()), &list);

  Local<Array> ret = Nan::New<Array>(list.size());
  for (size_t i = 0; i < list.size(); ++i) {
    MaybeLocal<String> second =
        Nan::New<String>(list[i].value.c_str(), list[i].value.length());
    CHECK(!second.IsEmpty());
    Nan::Set(ret, i, second.ToLocalChecked());
  }

  info.GetReturnValue().Set(ret);
}

NAN_METHOD(NodeURLSearchParams::Has) {
  CHECK_EQ(info.Length(), 1);
  CHECK(info[0]->IsString());

  Local<String> key = info[0].As<String>();
  Nan::Utf8String key_utf8(key);

  NodeURLSearchParams* self =
      Nan::ObjectWrap::Unwrap<NodeURLSearchParams>(info.Holder());
  bool ret = self->_internal->Has(string(*key_utf8, key_utf8.length()));
  info.GetReturnValue().Set(ret);
}

NAN_METHOD(NodeURLSearchParams::Set) {
  CHECK_EQ(info.Length(), 2);
  CHECK(info[0]->IsString());

  Local<String> key = info[0].As<String>();
  Local<String> value = info[1].As<String>();

  Nan::Utf8String key_utf8(key);
  Nan::Utf8String value_utf8(value);

  NodeURLSearchParams* self =
      Nan::ObjectWrap::Unwrap<NodeURLSearchParams>(info.Holder());
  self->_internal->Set(string(*key_utf8, key_utf8.length()),
                       string(*value_utf8, value_utf8.length()));
  self->OnPassivelyUpdate();
}

NAN_METHOD(NodeURLSearchParams::Sort) {
  NodeURLSearchParams* self =
      Nan::ObjectWrap::Unwrap<NodeURLSearchParams>(info.Holder());
  self->_internal->Sort();
  self->OnPassivelyUpdate();
}

NAN_METHOD(NodeURLSearchParams::ToString) {
  NodeURLSearchParams* self =
      Nan::ObjectWrap::Unwrap<NodeURLSearchParams>(info.Holder());
  string ret = self->_internal->Stringify();

  MaybeLocal<String> maybe_value = Nan::New<String>(ret.c_str(), ret.length());
  CHECK(!maybe_value.IsEmpty());

  info.GetReturnValue().Set(maybe_value.ToLocalChecked());
}

NAN_METHOD(NodeURLSearchParams::GetIterableArray) {
  NodeURLSearchParams* self =
      Nan::ObjectWrap::Unwrap<NodeURLSearchParams>(info.Holder());

  Local<Array> ret;
  bool cached = false;
  if (!self->_iterable_array.IsEmpty()) {
    ret = Local<Array>::New(self->_isolate, self->_iterable_array);
    cached = true;
  }

  self->GenerateIterableArray(&ret);
  if (!cached) {
    self->_iterable_array.Reset(self->_isolate, ret);
    self->_iterable_array.SetWeak();
  }

  info.GetReturnValue().Set(ret);
}

NodeURLSearchParams::NodeURLSearchParams(Isolate* isolate)
    : _isolate(isolate) {}

void NodeURLSearchParams::OnPassivelyUpdate() {
  if (!_iterable_array.IsEmpty()) {
    Local<Array> ret = Local<Array>::New(_isolate, _iterable_array);
    GenerateIterableArray(&ret);
  }
}

void NodeURLSearchParams::GenerateIterableArray(Local<Array>* array) {
  const URLSearchParamsList& list = _internal->list();
  if (array->IsEmpty()) {
    *array = Nan::New<Array>(list.size());
  } else {
    Nan::Set(*array,
             Nan::New<String>("length").ToLocalChecked(),
             Nan::New<Number>(list.size()));
  }

  Local<Array>& ret = *array;
  for (size_t i = 0; i < list.size(); ++i) {
    MaybeLocal<String> first =
        Nan::New<String>(list[i].key.c_str(), list[i].key.length());
    CHECK(!first.IsEmpty());

    MaybeLocal<String> second =
        Nan::New<String>(list[i].value.c_str(), list[i].value.length());
    CHECK(!second.IsEmpty());

    Local<Object> p = Nan::New<Array>(2);
    Nan::Set(p, 0, first.ToLocalChecked());
    Nan::Set(p, 1, second.ToLocalChecked());
    Nan::Set(ret, i, p);
  }
}

void NodeURLSearchParams::SetInternal(shared_ptr<URLSearchParams> intrnl) {
  CHECK(!_internal.get());
  _internal = intrnl;
  _internal->SetOnPassivelyUpdateFunction(
      [](void* context) {
        NodeURLSearchParams* self = static_cast<NodeURLSearchParams*>(context);
        self->OnPassivelyUpdate();
      },
      this);
}

}  // namespace whatwgurl
