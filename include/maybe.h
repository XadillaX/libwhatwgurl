#ifndef INCLUDE_MAYBE_H_
#define INCLUDE_MAYBE_H_

#include <type_traits>
#include <vector>
#include "utils/assert.h"

namespace whatwgurl {

template <typename T>
class MaybeNull {
 public:
  inline MaybeNull() : _is_null(true) {}
  inline explicit MaybeNull(const T& value) : _value(value), _is_null(false) {}
  inline explicit MaybeNull(const MaybeNull<T>& other)
      : _value(other._value), _is_null(other._is_null) {}
  virtual inline ~MaybeNull() = default;

  virtual inline MaybeNull& operator=(const MaybeNull<T>& other) {
    _value = other._value;
    _is_null = other._is_null;
    return *this;
  }

  virtual inline MaybeNull& operator=(const T& value) {
    _value = value;
    _is_null = false;
    return *this;
  }

  virtual inline MaybeNull& operator=(std::nullptr_t) {
    _is_null = true;
    return *this;
  }

  virtual inline operator bool() const { return !_is_null; }

  virtual inline const T& operator*() const { return _value; }
  virtual inline T& operator*() { return _value; }
  virtual inline const T* operator->() const { return &_value; }
  virtual inline T* operator->() { return &_value; }
  virtual inline const T& value() const { return _value; }
  virtual inline bool is_null() const { return _is_null; }

  virtual inline void SetValue(const T& value) {
    _value = value;
    _is_null = false;
  }

  virtual inline void SetValue(std::nullptr_t) { _is_null = true; }

 protected:
  T _value;
  bool _is_null;
};

template <typename T>
class MaybeList {
 public:
  explicit inline MaybeList(bool is_list = false) : _is_list(is_list) {}
  explicit inline MaybeList(const std::vector<T>& list)
      : _list(list), _is_list(true) {}
  explicit inline MaybeList(const T& value) : _is_list(false) {
    _list.push_back(value);
  }
  inline MaybeList(const MaybeList<T>& other)
      : _list(other._list), _is_list(other._is_list) {}
  virtual inline ~MaybeList() = default;

  virtual inline void Reset(bool is_list = false) {
    _is_list = is_list;
    _list.clear();
    if (!_is_list) {
      _list.push_back(T());
    }
  }

  virtual inline bool is_list() const { return _is_list; }
  virtual inline T& value() {
    CHECK(!_is_list);
    return _list[0];
  }

  virtual inline const T& value() const {
    CHECK(!_is_list);
    return _list[0];
  }

  virtual inline T& operator[](size_t index) {
    CHECK(_is_list);
    return _list[index];
  }

  virtual inline const T& operator[](size_t index) const {
    CHECK(_is_list);
    return _list[index];
  }

  virtual inline MaybeList& operator=(const MaybeList& other) {
    _list = other._list;
    _is_list = other._is_list;
    return *this;
  }

  virtual inline MaybeList& operator=(const std::vector<T>& list) {
    _list = list;
    _is_list = true;
    return *this;
  }

  virtual inline MaybeList& operator=(const T& value) {
    _list.clear();
    _list.push_back(value);
    _is_list = false;
    return *this;
  }

  virtual inline size_t size() const {
    CHECK(_is_list);
    return _list.size();
  }

  virtual inline void PushBack(const T& value) {
    CHECK(_is_list);
    _list.push_back(value);
  }

  virtual inline void PushFront(const T& value) {
    CHECK(_is_list);
    _list.insert(_list.begin(), value);
  }

  virtual inline void PopBack() {
    CHECK(_is_list);
    _list.pop_back();
  }

  virtual inline void PopFront() {
    CHECK(_is_list);
    _list.erase(_list.begin());
  }

  virtual inline void Clear() {
    CHECK(_is_list);
    _list.clear();
  }

  virtual inline T& Front() {
    CHECK(_is_list);
    return _list.front();
  }

  virtual inline const T& Front() const {
    CHECK(_is_list);
    return _list.front();
  }

  virtual inline T& Back() {
    CHECK(_is_list);
    return _list.back();
  }

  virtual inline const T& Back() const {
    CHECK(_is_list);
    return _list.back();
  }

  virtual inline void Insert(size_t index, const T& value) {
    CHECK(_is_list);
    _list.insert(_list.begin() + index, value);
  }

  virtual inline void Erase(size_t index) {
    CHECK(_is_list);
    _list.erase(_list.begin() + index);
  }

 private:
  std::vector<T> _list;
  bool _is_list;
};

}  // namespace whatwgurl

#endif  // INCLUDE_MAYBE_H_
