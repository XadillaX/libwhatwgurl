#include "url_search_params.h"
#include <algorithm>
#include "code_points.h"
#include "percent_encode.h"
#include "temp_string_buffer.h"
#include "url_core.h"
#include "utils/assert.h"

namespace whatwgurl {

using std::map;
using std::string;

// The application/x-www-form-urlencoded parser takes a byte sequence input, and
// then runs these steps:
static inline void ApplicationXWWWFormUrlencodedParse(
    const string& init, URLSearchParamsList* list) {
  // Let output be an initially empty list of name-value tuples where both name
  // and value hold a string.
  list->clear();

  if (init.empty()) {
    return;
  }

  // Let sequences be the result of splitting input on 0x26 (&).
  const char* ptr = init.c_str();
  const char* end = ptr + init.size();

  TempStringBuffer key(1);
  TempStringBuffer value(1);
  enum State {
    kKey,
    kValue,
  } state = kKey;

#define DECODE_CASE(name)                                                      \
  if (ptr + 2 >= end) {                                                        \
    name.Append(*ptr);                                                         \
    break;                                                                     \
  }                                                                            \
                                                                               \
  const unsigned char c1 = percent_encode::hexval[*(                           \
      reinterpret_cast<const unsigned char*>(ptr + 1))];                       \
  const unsigned char c2 = percent_encode::hexval[*(                           \
      reinterpret_cast<const unsigned char*>(ptr + 2))];                       \
                                                                               \
  if (c1 == 0xff || c2 == 0xff) {                                              \
    name.Append(*ptr);                                                         \
  } else {                                                                     \
    name.Append(c1 << 4 | c2);                                                 \
    ptr += 2;                                                                  \
  }                                                                            \
                                                                               \
  break

  uint32_t idx = 0;
  for (; ptr < end; ptr++) {
    switch (state) {
      case kKey: {
        switch (*ptr) {
          case '=':
            state = kValue;
            break;

          case '&':
            if (!key.length()) {
              continue;
            }

            list->push_back(URLSearchParamsKVPairWithIndex(
                string(key.string(), key.length()), "", idx++));
            key.SetEmpty();
            value.SetEmpty();
            break;

          case '+':
            key.Append(' ');
            break;

          case '%': {
            DECODE_CASE(key);
          }

          default:
            key.Append(*ptr);
            break;
        }

        break;
      }

      case kValue: {
        switch (*ptr) {
          case '&':
            list->push_back(URLSearchParamsKVPairWithIndex(
                string(key.string(), key.length()),
                string(value.string(), value.length()),
                idx++));
            key.SetEmpty();
            value.SetEmpty();
            state = kKey;

            break;

          case '+':
            value.Append(' ');
            break;

          case '%': {
            DECODE_CASE(value);
          }

          default:
            value.Append(*ptr);
            break;
        }

        break;
      }

      default:
        UNREACHABLE();
    }
  }

  if (state == kValue) {
    list->push_back(
        URLSearchParamsKVPairWithIndex(string(key.string(), key.length()),
                                       string(value.string(), value.length()),
                                       idx++));
  } else if (key.length() > 0) {
    list->push_back(URLSearchParamsKVPairWithIndex(
        string(key.string(), key.length()), "", idx++));
  }
}

// The application/x-www-form-urlencoded serializer takes a list of name-value
// tuples tuples, with an optional encoding encoding (default UTF-8), and then
// runs these steps:
static inline string ApplicationXWWWFormUrlencodedSerialize(
    const URLSearchParamsList& list) {
  // Let output be the empty string.
  string output;

  // For each tuple of tuples:
  TempStringBuffer name(1);
  TempStringBuffer value(1);
  for (const auto& item : list) {
    // Let name be the result of running percent-encode after encoding with
    // encoding, tuple’s name, the application/x-www-form-urlencoded
    // percent-encode set, and true.
    percent_encode::EncodeByApplicationXFormUrlEncodedPercentEncodeSet(
        reinterpret_cast<const unsigned char*>(item.key.c_str()),
        item.key.length(),
        &name,
        true);

    // Let value be the result of running percent-encode after encoding with
    // encoding, tuple’s value, the application/x-www-form-urlencoded
    // percent-encode set, and true.
    percent_encode::EncodeByApplicationXFormUrlEncodedPercentEncodeSet(
        reinterpret_cast<const unsigned char*>(item.value.c_str()),
        item.value.length(),
        &value,
        true);

    // If output is not the empty string, then append U+0026 (&) to output.
    if (!output.empty()) {
      output.push_back('&');
    }

    // Append name, followed by U+003D (=), followed by value, to output.
    output.append(name.string(), name.length());
    output.push_back('=');
    output.append(value.string(), value.length());

    name.SetEmpty();
    value.SetEmpty();
  }

  return output;
}

URLSearchParams::URLSearchParams(const string& init,
                                 bool force_reserve_first_letter)
    : _url(nullptr) {
  // If init is a string and starts with U+003F (?), then remove the first code
  // point from init.
  Initialize(
      (!force_reserve_first_letter && init.length() > 0 && init[0] == '?')
          ? init.substr(1)
          : init);
}

URLSearchParams::URLSearchParams(const URLSearchParamsList& init)
    : _url(nullptr) {
  // If init is a sequence, then for each pair in init:

  // Append a new name-value pair whose name is pair’s first item, and value is
  // pair’s second item, to query’s list.
  _list = init;
}

URLSearchParams::URLSearchParams(const map<string, string>& init)
    : _url(nullptr) {
  uint32_t index = 0;
  // Otherwise, if init is a record, then for each name → value of init, append
  // a new name-value pair whose name is name and value is value, to query’s
  // list.
  for (const auto& pair : init) {
    _list.push_back(
        URLSearchParamsKVPairWithIndex(pair.first, pair.second, index++));
  }
}

void URLSearchParams::Append(const string& name, const string& value) {
  // Append a new name-value pair whose name is name and value is value, to
  // list.
  _list.push_back(URLSearchParamsKVPairWithIndex(name, value, _list.size()));

  // Update this.
  Update();
}

void URLSearchParams::Delete(const string& name) {
  uint32_t index = 0;

  // Remove all name-value pairs whose name is name from list.
  for (auto it = _list.begin(); it != _list.end();) {
    if (it->key == name) {
      it = _list.erase(it);
    } else {
      it->index = index++;
      ++it;
    }
  }

  // Update this.
  Update();
}

MaybeNullURLSearchParamsKVPair URLSearchParams::Get(const string& name) const {
  // The get(name) method steps are to return the value of the first name-value
  // pair whose name is name in this’s list, if there is such a pair, and null
  // otherwise.
  for (const auto& p : _list) {
    if (p.key == name) {
      return MaybeNullURLSearchParamsKVPair(p);
    }
  }

  return MaybeNullURLSearchParamsKVPair();
}

void URLSearchParams::GetAll(const string& name,
                             URLSearchParamsList* values) const {
  values->clear();

  // The getAll(name) method steps are to return the values of all name-value
  // pairs whose name is name, in this’s list, in list order, and the empty
  // sequence otherwise.
  for (const auto& p : _list) {
    if (p.key == name) {
      values->push_back(p);
    }
  }
}

bool URLSearchParams::Has(const string& name) const {
  // The has(name) method steps are to return true if there is a name-value pair
  // whose name is name in this’s list, and false otherwise.
  for (const auto& p : _list) {
    if (p.key == name) {
      return true;
    }
  }

  return false;
}

void URLSearchParams::Set(const string& name, const string& value) {
  uint32_t index = 0;
  // The set(name, value) method steps are:

  // If this’s list contains any name-value pairs whose name is name, then set
  // the value of the first such name-value pair to value and remove the others.
  bool found = false;
  for (auto it = _list.begin(); it != _list.end();) {
    if (it->key == name) {
      if (found) {
        it = _list.erase(it);
      } else {
        it->value = value;
        it->index = index++;
        found = true;
        ++it;
      }
    } else {
      it->index = index++;
      ++it;
    }
  }

  // Otherwise, append a new name-value pair whose name is name and value is
  // value, to this’s list.
  if (!found) {
    _list.push_back(URLSearchParamsKVPairWithIndex(name, value, _list.size()));
  }

  // Update this.
  Update();
}

void URLSearchParams::Sort() {
  // The sort() method steps are:

  // Sort all name-value pairs, if any, by their names. Sorting must be done by
  // comparison of code units. The relative order between name-value pairs with
  // equal names must be preserved.
  std::sort(_list.begin(),
            _list.end(),
            [](const URLSearchParamsKVPairWithIndex& a,
               const URLSearchParamsKVPairWithIndex& b) {
              // Refs:
              // https://github.com/rmisev/url_whatwg/blob/9388886d903cb5f676320c90c24e8ac8223b28fd/src/url_search_params.h#L461
              int ret = CompareByCodeUnits(a.key, b.key);
              if (ret < 0) return true;
              if (ret > 0) return false;

              return a.index < b.index;
            });

  uint32_t index = 0;
  for (auto& p : _list) {
    p.index = index++;
  }

  // Update this.
  Update();
}

string URLSearchParams::Stringify() {
  // The stringification behavior steps are to return the serialization of
  // this’s list.
  return ApplicationXWWWFormUrlencodedSerialize(_list);
}

void URLSearchParams::Initialize(const string& init) {
  // Set query’s list to the result of parsing init.
  ApplicationXWWWFormUrlencodedParse(init, &_list);
}

void URLSearchParams::Update() {
  // If query’s URL object is null, then return.
  if (!_url) return;

  // Let serializedQuery be the serialization of query’s list.
  MaybeNull<std::string> serialized_query(
      ApplicationXWWWFormUrlencodedSerialize(_list));

  // If serializedQuery is the empty string, then set serializedQuery to null.
  if (serialized_query->empty()) {
    serialized_query = nullptr;
  }

  // Set query’s URL object’s URL’s query to serializedQuery.
  _url->_parsed_url->query = serialized_query;
  _url->EmitPassivelyUpdate();
}

}  // namespace whatwgurl
