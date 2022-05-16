#ifndef INCLUDE_URL_SEARCH_PARAMS_H_
#define INCLUDE_URL_SEARCH_PARAMS_H_

#include <map>
#include <string>
#include <utility>
#include <vector>
#include "maybe.h"

namespace whatwgurl {

struct URLSearchParamsKVPairWithIndex {
 public:
  inline URLSearchParamsKVPairWithIndex() : index(0), key(), value() {}
  inline URLSearchParamsKVPairWithIndex(std::string key,
                                        std::string value,
                                        uint32_t index)
      : index(index), key(key), value(value) {}
  ~URLSearchParamsKVPairWithIndex() = default;

  uint32_t index;
  std::string key;
  std::string value;
};

class MaybeNullURLSearchParamsKVPair
    : public MaybeNull<URLSearchParamsKVPairWithIndex> {
 public:
  explicit MaybeNullURLSearchParamsKVPair(
      const URLSearchParamsKVPairWithIndex& p)
      : MaybeNull<URLSearchParamsKVPairWithIndex>(p) {}
  MaybeNullURLSearchParamsKVPair()
      : MaybeNull<URLSearchParamsKVPairWithIndex>() {}
};

typedef std::vector<URLSearchParamsKVPairWithIndex> URLSearchParamsList;
typedef void (*OnPassivelyUpdateFunction)(void* context);

class URLCore;
class URLSearchParams {
  friend class URLCore;

 public:
  explicit URLSearchParams(const std::string& init = "",
                           bool force_reserve_first_letter = false);
  explicit URLSearchParams(const URLSearchParamsList& init);
  explicit URLSearchParams(const std::map<std::string, std::string>& init);

  void Append(const std::string& name, const std::string& value);
  void Delete(const std::string& name);
  MaybeNullURLSearchParamsKVPair Get(const std::string& name) const;
  void GetAll(const std::string& name, URLSearchParamsList* result) const;
  bool Has(const std::string& name) const;
  void Set(const std::string& name, const std::string& value);
  void Sort();

  inline const URLSearchParamsList& list() const { return _list; }
  std::string Stringify();

  inline void SetOnPassivelyUpdateFunction(OnPassivelyUpdateFunction on_update,
                                           void* context) {
    _on_passively_update = on_update;
    _on_passively_update_context = context;
  }

 private:
  void Initialize(const std::string& init);
  void Update();

  inline void EmitPassivelyUpdate() {
    if (_on_passively_update) {
      _on_passively_update(_on_passively_update_context);
    }
  }

 private:
  URLSearchParamsList _list;
  URLCore* _url;

  OnPassivelyUpdateFunction _on_passively_update = nullptr;
  void* _on_passively_update_context = nullptr;
};

}  // namespace whatwgurl

#endif  // INCLUDE_URL_SEARCH_PARAMS_H_
