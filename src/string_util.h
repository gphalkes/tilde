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

#ifndef STRINGS_H_
#define STRINGS_H_

#include <cstddef>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

namespace strings {

template <class T>
std::vector<T> Split(const std::string &str, char sep, bool keep_empty) {
  std::vector<T> result;

  size_t last_start = 0;
  size_t last_found = str.find(sep, last_start);
  while (last_found != std::string::npos) {
    if (keep_empty || last_found - last_start > 0) {
      result.emplace_back(str.substr(last_start, last_found - last_start));
    }
    last_start = last_found + 1;
    last_found = str.find(sep, last_start);
  }
  if (keep_empty || str.size() - last_start > 0) {
    result.emplace_back(str.substr(last_start, str.size() - last_start));
  }
  return result;
}

namespace internal {
inline void AppendOne(std::string *base, const std::string &str) { base->append(str); }
inline void AppendOne(std::string *base, const char *str) { base->append(str); }
inline void AppendOne(std::string *base, char c) { base->append(1, c); }

namespace internal {
/* Define is_negative using templates, to stop the compiler complaining about
   comparing an unsigned number using < 0. */
template <typename T>
typename std::enable_if<std::is_signed<T>::value, bool>::type is_negative(T i) {
  return i < 0;
}
template <typename T>
typename std::enable_if<std::is_unsigned<T>::value, bool>::type is_negative(T i) {
  (void)i;
  return false;
}

template <typename T>
typename std::enable_if<std::is_signed<T>::value, T>::type make_abs(T i) {
  return std::abs(i);
}
template <typename T>
typename std::enable_if<std::is_unsigned<T>::value, T>::type make_abs(T i) {
  return i;
}
}  // namespace internal

template <typename T, class = typename std::enable_if<std::is_integral<T>::value>::type>
void AppendOne(std::string *base, T i) {
  bool negative = internal::is_negative(i);
  char buffer[sizeof(i) * 3 + 3];
  char *next_char = buffer + sizeof(buffer) - 1;
  *next_char-- = 0;
  if (i == 0) {
    *next_char-- = '0';
  }
  while (i != 0) {
    *next_char-- = '0' + internal::make_abs(i % 10);
    i /= 10;
  }
  if (negative) {
    *next_char-- = '-';
  }
  AppendOne(base, next_char + 1);
}
}  // namespace internal

inline size_t Len(const std::string &str) { return str.size(); }
inline size_t Len(const char *str) { return strlen(str); }
inline size_t Len(char c) {
  (void)c;
  return 1;
}

template <typename T, class = typename std::enable_if<std::is_integral<T>::value>::type>
inline size_t Len(T i) {
  (void)i;
  return sizeof(i) * 3 + 2;
}

template <typename T1>
void Append(std::string *base, const T1 &arg1) {
  internal::AppendOne(base, arg1);
}

template <typename T1, typename T2>
void Append(std::string *base, const T1 &arg1, const T2 &arg2) {
  base->reserve(base->size() + Len(arg1) + Len(arg2));
  internal::AppendOne(base, arg1);
  internal::AppendOne(base, arg2);
}

template <typename T1, typename T2, typename T3>
void Append(std::string *base, const T1 &arg1, const T2 &arg2, const T3 &arg3) {
  base->reserve(base->size() + Len(arg1) + Len(arg2) + Len(arg3));
  internal::AppendOne(base, arg1);
  internal::AppendOne(base, arg2);
  internal::AppendOne(base, arg3);
}

template <typename T1, typename T2, typename T3, typename T4>
void Append(std::string *base, const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4) {
  base->reserve(base->size() + Len(arg1) + Len(arg2) + Len(arg3) + Len(arg4));
  internal::AppendOne(base, arg1);
  internal::AppendOne(base, arg2);
  internal::AppendOne(base, arg3);
  internal::AppendOne(base, arg4);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
void Append(std::string *base, const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4,
            const T5 &arg5) {
  base->reserve(base->size() + Len(arg1) + Len(arg2) + Len(arg3) + Len(arg4) + Len(arg5));
  internal::AppendOne(base, arg1);
  internal::AppendOne(base, arg2);
  internal::AppendOne(base, arg3);
  internal::AppendOne(base, arg4);
  internal::AppendOne(base, arg5);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
void Append(std::string *base, const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4,
            const T5 &arg5, const T6 &arg6) {
  base->reserve(base->size() + Len(arg1) + Len(arg2) + Len(arg3) + Len(arg4) + Len(arg5) +
                Len(arg6));
  internal::AppendOne(base, arg1);
  internal::AppendOne(base, arg2);
  internal::AppendOne(base, arg3);
  internal::AppendOne(base, arg4);
  internal::AppendOne(base, arg5);
  internal::AppendOne(base, arg6);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
void Append(std::string *base, const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4,
            const T5 &arg5, const T6 &arg6, const T7 &arg7) {
  base->reserve(base->size() + Len(arg1) + Len(arg2) + Len(arg3) + Len(arg4) + Len(arg5) +
                Len(arg6) + Len(arg7));
  internal::AppendOne(base, arg1);
  internal::AppendOne(base, arg2);
  internal::AppendOne(base, arg3);
  internal::AppendOne(base, arg4);
  internal::AppendOne(base, arg5);
  internal::AppendOne(base, arg6);
  internal::AppendOne(base, arg7);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8>
void Append(std::string *base, const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4,
            const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8) {
  base->reserve(base->size() + Len(arg1) + Len(arg2) + Len(arg3) + Len(arg4) + Len(arg5) +
                Len(arg6) + Len(arg7) + Len(arg8));
  internal::AppendOne(base, arg1);
  internal::AppendOne(base, arg2);
  internal::AppendOne(base, arg3);
  internal::AppendOne(base, arg4);
  internal::AppendOne(base, arg5);
  internal::AppendOne(base, arg6);
  internal::AppendOne(base, arg7);
  internal::AppendOne(base, arg8);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9>
void Append(std::string *base, const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4,
            const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9) {
  base->reserve(base->size() + Len(arg1) + Len(arg2) + Len(arg3) + Len(arg4) + Len(arg5) +
                Len(arg6) + Len(arg7) + Len(arg8) + Len(arg9));
  internal::AppendOne(base, arg1);
  internal::AppendOne(base, arg2);
  internal::AppendOne(base, arg3);
  internal::AppendOne(base, arg4);
  internal::AppendOne(base, arg5);
  internal::AppendOne(base, arg6);
  internal::AppendOne(base, arg7);
  internal::AppendOne(base, arg8);
  internal::AppendOne(base, arg9);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10>
void Append(std::string *base, const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4,
            const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9,
            const T10 &arg10) {
  base->reserve(base->size() + Len(arg1) + Len(arg2) + Len(arg3) + Len(arg4) + Len(arg5) +
                Len(arg6) + Len(arg7) + Len(arg8) + Len(arg9) + Len(arg10));
  internal::AppendOne(base, arg1);
  internal::AppendOne(base, arg2);
  internal::AppendOne(base, arg3);
  internal::AppendOne(base, arg4);
  internal::AppendOne(base, arg5);
  internal::AppendOne(base, arg6);
  internal::AppendOne(base, arg7);
  internal::AppendOne(base, arg8);
  internal::AppendOne(base, arg9);
  internal::AppendOne(base, arg10);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11>
void Append(std::string *base, const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4,
            const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9,
            const T10 &arg10, const T11 &arg11) {
  base->reserve(base->size() + Len(arg1) + Len(arg2) + Len(arg3) + Len(arg4) + Len(arg5) +
                Len(arg6) + Len(arg7) + Len(arg8) + Len(arg9) + Len(arg10) + Len(arg11));
  internal::AppendOne(base, arg1);
  internal::AppendOne(base, arg2);
  internal::AppendOne(base, arg3);
  internal::AppendOne(base, arg4);
  internal::AppendOne(base, arg5);
  internal::AppendOne(base, arg6);
  internal::AppendOne(base, arg7);
  internal::AppendOne(base, arg8);
  internal::AppendOne(base, arg9);
  internal::AppendOne(base, arg10);
  internal::AppendOne(base, arg11);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11, typename T12>
void Append(std::string *base, const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4,
            const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9,
            const T10 &arg10, const T11 &arg11, const T12 &arg12) {
  base->reserve(base->size() + Len(arg1) + Len(arg2) + Len(arg3) + Len(arg4) + Len(arg5) +
                Len(arg6) + Len(arg7) + Len(arg8) + Len(arg9) + Len(arg10) + Len(arg11) +
                Len(arg12));
  internal::AppendOne(base, arg1);
  internal::AppendOne(base, arg2);
  internal::AppendOne(base, arg3);
  internal::AppendOne(base, arg4);
  internal::AppendOne(base, arg5);
  internal::AppendOne(base, arg6);
  internal::AppendOne(base, arg7);
  internal::AppendOne(base, arg8);
  internal::AppendOne(base, arg9);
  internal::AppendOne(base, arg10);
  internal::AppendOne(base, arg11);
  internal::AppendOne(base, arg12);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11, typename T12, typename T13>
void Append(std::string *base, const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4,
            const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9,
            const T10 &arg10, const T11 &arg11, const T12 &arg12, const T13 &arg13) {
  base->reserve(base->size() + Len(arg1) + Len(arg2) + Len(arg3) + Len(arg4) + Len(arg5) +
                Len(arg6) + Len(arg7) + Len(arg8) + Len(arg9) + Len(arg10) + Len(arg11) +
                Len(arg12) + Len(arg13));
  internal::AppendOne(base, arg1);
  internal::AppendOne(base, arg2);
  internal::AppendOne(base, arg3);
  internal::AppendOne(base, arg4);
  internal::AppendOne(base, arg5);
  internal::AppendOne(base, arg6);
  internal::AppendOne(base, arg7);
  internal::AppendOne(base, arg8);
  internal::AppendOne(base, arg9);
  internal::AppendOne(base, arg10);
  internal::AppendOne(base, arg11);
  internal::AppendOne(base, arg12);
  internal::AppendOne(base, arg13);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11, typename T12, typename T13,
          typename T14>
void Append(std::string *base, const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4,
            const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9,
            const T10 &arg10, const T11 &arg11, const T12 &arg12, const T13 &arg13,
            const T14 &arg14) {
  base->reserve(base->size() + Len(arg1) + Len(arg2) + Len(arg3) + Len(arg4) + Len(arg5) +
                Len(arg6) + Len(arg7) + Len(arg8) + Len(arg9) + Len(arg10) + Len(arg11) +
                Len(arg12) + Len(arg13) + Len(arg14));
  internal::AppendOne(base, arg1);
  internal::AppendOne(base, arg2);
  internal::AppendOne(base, arg3);
  internal::AppendOne(base, arg4);
  internal::AppendOne(base, arg5);
  internal::AppendOne(base, arg6);
  internal::AppendOne(base, arg7);
  internal::AppendOne(base, arg8);
  internal::AppendOne(base, arg9);
  internal::AppendOne(base, arg10);
  internal::AppendOne(base, arg11);
  internal::AppendOne(base, arg12);
  internal::AppendOne(base, arg13);
  internal::AppendOne(base, arg14);
}

template <typename T1>
std::string Cat(const T1 &arg1) {
  std::string result;
  Append(&result, arg1);
  return result;
}

template <typename T1, typename T2>
std::string Cat(const T1 &arg1, const T2 &arg2) {
  std::string result;
  Append(&result, arg1, arg2);
  return result;
}

template <typename T1, typename T2, typename T3>
std::string Cat(const T1 &arg1, const T2 &arg2, const T3 &arg3) {
  std::string result;
  Append(&result, arg1, arg2, arg3);
  return result;
}

template <typename T1, typename T2, typename T3, typename T4>
std::string Cat(const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4) {
  std::string result;
  Append(&result, arg1, arg2, arg3, arg4);
  return result;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
std::string Cat(const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4, const T5 &arg5) {
  std::string result;
  Append(&result, arg1, arg2, arg3, arg4, arg5);
  return result;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
std::string Cat(const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4, const T5 &arg5,
                const T6 &arg6) {
  std::string result;
  Append(&result, arg1, arg2, arg3, arg4, arg5, arg6);
  return result;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
std::string Cat(const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4, const T5 &arg5,
                const T6 &arg6, const T7 &arg7) {
  std::string result;
  Append(&result, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
  return result;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8>
std::string Cat(const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4, const T5 &arg5,
                const T6 &arg6, const T7 &arg7, const T8 &arg8) {
  std::string result;
  Append(&result, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
  return result;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9>
std::string Cat(const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4, const T5 &arg5,
                const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9) {
  std::string result;
  Append(&result, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
  return result;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10>
std::string Cat(const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4, const T5 &arg5,
                const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10) {
  std::string result;
  Append(&result, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
  return result;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11>
std::string Cat(const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4, const T5 &arg5,
                const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10,
                const T11 &arg11) {
  std::string result;
  Append(&result, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
  return result;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11, typename T12>
std::string Cat(const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4, const T5 &arg5,
                const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10,
                const T11 &arg11, const T12 &arg12) {
  std::string result;
  Append(&result, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12);
  return result;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11, typename T12, typename T13>
std::string Cat(const T1 &arg1, const T2 &arg2, const T3 &arg3, const T4 &arg4, const T5 &arg5,
                const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10,
                const T11 &arg11, const T12 &arg12, const T13 &arg13) {
  std::string result;
  Append(&result, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13);
  return result;
}

template <typename T>
std::string Join(const T &list, const std::string &separator) {
  std::string result;
  if (list.empty()) return result;
  auto list_iter = list.begin();
  Append(&result, *list_iter);
  ++list_iter;
  for (; list_iter != list.end(); ++list_iter) {
    Append(&result, separator, *list_iter);
  }
  return result;
}

}  // namespace strings

#endif
