/* Copyright (C) 2018 G.P. Halkes
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UTIL_basic_string_view_H
#define UTIL_basic_string_view_H

#include <algorithm>
#include <cstring>
#include <limits>
#include <stddef.h>
#include <stdexcept>
#include <string>

// Simple basic_string_view implementation, which should be compatible with the future standard
// version for most purposes. By the time the standard version is available, it should be easy
// enough to replace basic_string_view with std::basic_string_view.
template <class CharT>
class basic_string_view {
 public:
  constexpr basic_string_view() : data_(""), size_(0) {}
  constexpr basic_string_view(const CharT *data) : data_(data), size_(strlen(data)) {}
  constexpr basic_string_view(const CharT *data, size_t size) : data_(data), size_(size) {}
  basic_string_view(const std::basic_string<CharT> &str) : data_(str.data()), size_(str.size()) {}
  constexpr basic_string_view(const basic_string_view &view) = default;

  basic_string_view &operator=(const basic_string_view &view) = default;

  constexpr const CharT *begin() const { return data_; }
  constexpr const CharT *cbegin() const { return begin(); }
  constexpr const CharT *end() const { return data_ + size_; }
  constexpr const CharT *cend() const { return end(); }

  constexpr const CharT &operator[](size_t pos) const { return data_[pos]; }
  const CharT &at(size_t pos) const {
    if (pos >= size_) throw std::out_of_range("Index out of range");
    return data_[pos];
  }
  constexpr const CharT &front() const { return data_[0]; }
  constexpr const CharT &back() const { return data_[size_ - 1]; }

  constexpr const CharT *data() const { return data_; }
  constexpr size_t size() const { return size_; }
  constexpr size_t length() const { return size_; }
  constexpr size_t max_size() const { return npos; }
  constexpr bool empty() const { return size_ == 0; }

  void clear() {
    data_ = "";
    size_ = 0;
  }
  void remove_prefix(size_t n) {
    data_ += n;
    size_ -= n;
  }
  void remove_suffix(size_t n) { size_ -= n; }
  void swap(basic_string_view &v) {
    std::swap(data_, v.data_);
    std::swap(size_, v.size_);
  }

  std::basic_string<CharT> to_string() const { return std::basic_string<CharT>(data_, size_); }
  explicit operator std::basic_string<CharT>() const {
    return std::basic_string<CharT>(data_, size_);
  }

  size_t copy(CharT *dest, size_t count, size_t pos = 0) const {
    if (pos >= size_) throw std::out_of_range("Index out of range");
    size_t to_copy = std::min(count, size_ - pos);
    memcpy(dest, data_ + pos, to_copy);
    return to_copy;
  }

  basic_string_view substr(size_t pos = 0, size_t count = npos) const {
    if (pos >= size_) throw std::out_of_range("Index out of range");
    return basic_string_view(data_ + pos, std::min(count, size_ - pos));
  }

  int compare(basic_string_view v) const {
    int cmp = memcmp(data_, v.data_, std::min(size_, v.size_));
    if (cmp == 0) {
      cmp = size_ < v.size_ ? -1 : (size_ > v.size_ ? 1 : 0);
    }
    return cmp;
  }
  constexpr int compare(size_t pos1, size_t count1, basic_string_view v) const {
    return substr(pos1, count1).compare(v);
  }
  constexpr int compare(size_t pos1, size_t count1, basic_string_view v, size_t pos2,
                        size_t count2) const {
    return substr(pos1, count1).compare(v.substr(pos2, count2));
  }
  constexpr int compare(const CharT *s) const { return compare(basic_string_view(s)); }
  constexpr int compare(size_t pos1, size_t count1, const CharT *s) const {
    return substr(pos1, count1).compare(basic_string_view(s));
  }
  constexpr int compare(size_t pos1, size_t count1, const CharT *s, size_t count2) const {
    return substr(pos1, count1).compare(basic_string_view(s, count2));
  }

  size_t find(basic_string_view v, size_t pos = 0) const {
    if (npos - pos < v.size_ || pos > size_ || v.size_ > size_) return npos;
    for (size_t x = pos; x + v.size_ <= size_; x++) {
      if (memcmp(data_ + x, v.data_, v.size_) == 0) return x;
    }
    return npos;
  }
  constexpr size_t find(CharT c, size_t pos = 0) const {
    return find(basic_string_view(&c, 1), pos);
  }
  constexpr size_t find(const CharT *s, size_t pos, size_t count) const {
    return find(basic_string_view(s, count), pos);
  }
  constexpr size_t find(const CharT *s, size_t pos = 0) const {
    return find(basic_string_view(s), pos);
  }

  size_t rfind(basic_string_view v, size_t pos = 0) const {
    if (npos - pos < v.size_ || pos > size_ || v.size_ > size_) return npos;
    size_t x;
    for (x = size_ - v.size_; x > pos; x--) {
      if (memcmp(data_ + x, v.data_, v.size_) == 0) return x;
    }
    // Handle the case where x == pos. This can not be done in the loop because x is
    // unsigned and pos may be 0.
    return memcmp(data_ + x, v.data_, v.size_) == 0 ? x : npos;
  }
  constexpr size_t rfind(CharT c, size_t pos = 0) const {
    return rfind(basic_string_view(&c, 1), pos);
  }
  constexpr size_t rfind(const CharT *s, size_t pos, size_t count) const {
    return rfind(basic_string_view(s, count), pos);
  }
  constexpr size_t rfind(const CharT *s, size_t pos = 0) const {
    return rfind(basic_string_view(s), pos);
  }

  size_t find_first_of(basic_string_view v, size_t pos = 0) const {
    for (size_t x = pos; x < size_; x++) {
      if (v.find_first_of(data_[x]) != npos) return x;
    }
    return npos;
  }
  size_t find_first_of(CharT c, size_t pos = 0) const {
    for (size_t x = pos; x < size_; x++) {
      if (data_[x] == c) return x;
    }
    return npos;
  }
  constexpr size_t find_first_of(const CharT *s, size_t pos, size_t count) const {
    return find_first_of(basic_string_view(s, count), pos);
  }
  constexpr size_t find_first_of(const CharT *s, size_t pos = 0) const {
    return find_first_of(basic_string_view(s), pos);
  }

  size_t find_first_not_of(basic_string_view v, size_t pos = 0) const {
    for (size_t x = pos; x < size_; x++) {
      if (v.find_first_of(data_[x]) == npos) return x;
    }
    return npos;
  }
  size_t find_first_not_of(CharT c, size_t pos = 0) const {
    for (size_t x = pos; x < size_; x++) {
      if (data_[x] != c) return x;
    }
    return npos;
  }
  constexpr size_t find_first_not_of(const CharT *s, size_t pos, size_t count) const {
    return find_first_not_of(basic_string_view(s, count), pos);
  }
  constexpr size_t find_first_not_of(const CharT *s, size_t pos = 0) const {
    return find_first_not_of(basic_string_view(s), pos);
  }

  size_t find_last_of(basic_string_view v, size_t pos = 0) const {
    size_t x;
    if (size_ == 0) return npos;
    for (x = size_ - 1; x > pos; x--) {
      if (v.find_first_of(data_[x]) != npos) return x;
    }
    return x == pos ? (v.find_first_of(data_[x]) != npos ? x : npos) : npos;
  }
  size_t find_last_of(CharT c, size_t pos = 0) const {
    size_t x;
    if (size_ == 0) return npos;
    for (x = size_ - 1; x < size_; x--) {
      if (data_[x] == c) return x;
    }
    return x == pos && data_[x] == c ? x : npos;
  }
  constexpr size_t find_last_of(const CharT *s, size_t pos, size_t count) const {
    return find_last_of(basic_string_view(s, count), pos);
  }
  constexpr size_t find_last_of(const CharT *s, size_t pos = 0) const {
    return find_last_of(basic_string_view(s), pos);
  }

  size_t find_last_not_of(basic_string_view v, size_t pos = 0) const {
    size_t x;
    if (size_ == 0) return npos;
    for (x = size_ - 1; x > pos; x--) {
      if (v.find_first_of(data_[x]) == npos) return x;
    }
    return x == pos ? (v.find_first_of(data_[x]) == npos ? x : npos) : npos;
  }
  size_t find_last_not_of(CharT c, size_t pos = 0) const {
    size_t x;
    if (size_ == 0) return npos;
    for (x = size_ - 1; x < size_; x--) {
      if (data_[x] != c) return x;
    }
    return x == pos && data_[x] != c ? x : npos;
  }
  constexpr size_t find_last_not_of(const CharT *s, size_t pos, size_t count) const {
    return find_last_not_of(basic_string_view(s, count), pos);
  }
  constexpr size_t find_last_not_of(const CharT *s, size_t pos = 0) const {
    return find_last_not_of(basic_string_view(s), pos);
  }

  static constexpr size_t npos = std::numeric_limits<size_t>::max();

 private:
  const CharT *data_;
  size_t size_;
};

template <class CharT>
constexpr size_t basic_string_view<CharT>::npos;

template <class CharT>
bool operator==(basic_string_view<CharT> lhs, basic_string_view<CharT> rhs) {
  return lhs.compare(rhs) == 0;
}

template <class CharT>
bool operator!=(basic_string_view<CharT> lhs, basic_string_view<CharT> rhs) {
  return lhs.compare(rhs) != 0;
}

template <class CharT>
bool operator<(basic_string_view<CharT> lhs, basic_string_view<CharT> rhs) {
  return lhs.compare(rhs) < 0;
}

template <class CharT>
bool operator<=(basic_string_view<CharT> lhs, basic_string_view<CharT> rhs) {
  return lhs.compare(rhs) <= 0;
}

template <class CharT>
bool operator>(basic_string_view<CharT> lhs, basic_string_view<CharT> rhs) {
  return lhs.compare(rhs) > 0;
}

template <class CharT>
bool operator>=(basic_string_view<CharT> lhs, basic_string_view<CharT> rhs) {
  return lhs.compare(rhs) >= 0;
}

template <class CharT>
bool operator==(basic_string_view<CharT> lhs, const CharT *rhs) {
  return lhs.compare(rhs) == 0;
}

template <class CharT>
bool operator!=(basic_string_view<CharT> lhs, const CharT *rhs) {
  return lhs.compare(rhs) != 0;
}

template <class CharT>
bool operator<(basic_string_view<CharT> lhs, const CharT *rhs) {
  return lhs.compare(rhs) < 0;
}

template <class CharT>
bool operator<=(basic_string_view<CharT> lhs, const CharT *rhs) {
  return lhs.compare(rhs) <= 0;
}

template <class CharT>
bool operator>(basic_string_view<CharT> lhs, const CharT *rhs) {
  return lhs.compare(rhs) > 0;
}

template <class CharT>
bool operator>=(basic_string_view<CharT> lhs, const CharT *rhs) {
  return lhs.compare(rhs) >= 0;
}

template <class CharT>
bool operator==(basic_string_view<CharT> lhs, const std::basic_string<CharT> &rhs) {
  return lhs.compare(rhs) == 0;
}

template <class CharT>
bool operator!=(basic_string_view<CharT> lhs, const std::basic_string<CharT> &rhs) {
  return lhs.compare(rhs) != 0;
}

template <class CharT>
bool operator<(basic_string_view<CharT> lhs, const std::basic_string<CharT> &rhs) {
  return lhs.compare(rhs) < 0;
}

template <class CharT>
bool operator<=(basic_string_view<CharT> lhs, const std::basic_string<CharT> &rhs) {
  return lhs.compare(rhs) <= 0;
}

template <class CharT>
bool operator>(basic_string_view<CharT> lhs, const std::basic_string<CharT> &rhs) {
  return lhs.compare(rhs) > 0;
}

template <class CharT>
bool operator>=(basic_string_view<CharT> lhs, const std::basic_string<CharT> &rhs) {
  return lhs.compare(rhs) >= 0;
}

using string_view = basic_string_view<char>;

#endif
