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

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "util.h"
#include "string_view.h"

#if 0
#define ASSERT(x)                                      \
  if (!(x)) {                                          \
    fatal("%d:Assertion (" #x ") failed\n", __LINE__); \
  }
void fatal(const char *fmt, ...) __attribute__((format(printf, 1, 2))) __attribute__((noreturn));

void fatal(const char *fmt, ...) {
  va_list args;

  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  exit(10);
}
#endif

class json_value_t {
 public:
  enum type_t {
    NONE,
    STRING,
    NUMBER,
    NULL_VALUE,
    BOOL,
    OBJECT,
    ARRAY,
  };

  json_value_t(bool value) : type_(BOOL), bool_(value) {}
  json_value_t(type_t type, std::string &&str) : type_(type), str_(new std::string(str)) {
    ASSERT(type_ == STRING || type_ == NUMBER);
  }
  json_value_t(type_t type) : type_(type) {
    ASSERT(type_ == OBJECT || type_ == ARRAY || type_ == NULL_VALUE);
    if (type_ == OBJECT) {
      object_ = new std::map<std::string, json_value_t>;
    } else if (type_ == ARRAY) {
      array_ = new std::vector<json_value_t>;
    }
  }

  json_value_t(json_value_t &&other) {
    type_ = other.type_;
    str_ = other.str_;
    other.type_ = NONE;
  }

  ~json_value_t() {
    if (type_ == STRING || type_ == NUMBER) {
      delete str_;
    } else if (type_ == OBJECT) {
      delete object_;
    } else if (type_ == ARRAY) {
      delete array_;
    }
  }

  type_t type() const { return type_; }

  const std::string &get_string() const {
    ASSERT(type_ == STRING || type_ == NUMBER);
    return *str_;
  }

  std::string &&move_string() const {
    ASSERT(type_ == STRING || type_ == NUMBER);
    return std::move(*str_);
  }

  bool get_bool() const {
    ASSERT(type_ == BOOL);
    return bool_;
  }

  bool is_integer() const {
    ASSERT(type_ == NUMBER);
    return str_->find_first_of("eE.") == std::string::npos;
  }

  int64_t get_integer() const {
    ASSERT(type_ == NUMBER);
    return std::strtoll(str_->c_str(), nullptr, 0);
  }

  double get_float() const {
    ASSERT(type_ == NUMBER);
    return std::strtod(str_->c_str(), nullptr);
  }

  void insert(std::string &&key, json_value_t &&value) {
    ASSERT(type_ == OBJECT);
    object_->emplace(std::move(key), std::move(value));
  }

  void insert(json_value_t &&value) {
    ASSERT(type_ == ARRAY);
    array_->emplace_back(std::move(value));
  }

 private:
  type_t type_;
  union {
    std::string *str_;
    std::map<std::string, json_value_t> *object_;
    std::vector<json_value_t> *array_;
    bool bool_;
  };
};

static bool is_letter(char c) { return c >= 'a' && c <= 'z'; }

static bool is_space(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }

static bool is_token_char(char c) {
  return c == '{' || c == '}' || c == '[' || c == ']' || c == ':' || c == ',';
}

class json_deserializer_t {
 public:
  using done_callback_t = std::function<bool(json_value_t &&value)>;

  json_deserializer_t(done_callback_t done_callback) : done_callback_(done_callback) {}

  bool append(string_view bytes) {
    for (char c : bytes) {
      switch (lexer_state_) {
        case INITIAL:
          if (c == '"') {
            lexer_state_ = STRING;
          } else if (c == '-') {
            current_piece_.push_back(c);
            lexer_state_ = NUMBER_FIRST_DIGIT;
          } else if (c == '0') {
            current_piece_.push_back(c);
            lexer_state_ = NUMBER_NO_MORE_DIGIT;
          } else if (is_digit(c)) {
            current_piece_.push_back(c);
            lexer_state_ = NUMBER_NEXT_DIGIT;
          } else if (is_letter(c)) {
            current_piece_.push_back(c);
            lexer_state_ = CONSTANT;
          } else if (is_space(c)) {
            // Do nothing.
          } else if (is_token_char(c)) {
            if (!handle_token(c)) {
              return false;
            }
          } else {
            return false;
          }
          break;
        case CONSTANT:
          if (is_letter(c)) {
            current_piece_.push_back(c);
          } else if (is_space(c)) {
            if (!handle_constant()) {
              return false;
            }
            lexer_state_ = INITIAL;
          } else if (is_token_char(c)) {
            if (!handle_constant() || !handle_token(c)) {
              return false;
            }
            lexer_state_ = INITIAL;
          } else {
            return false;
          }
          break;
        case NUMBER_FIRST_DIGIT:
          if (c == '0') {
            current_piece_.push_back(c);
            lexer_state_ = NUMBER_NO_MORE_DIGIT;
          } else if (is_digit(c)) {
            current_piece_.push_back(c);
            lexer_state_ = NUMBER_NEXT_DIGIT;
          } else {
            return false;
          }
          break;
        case NUMBER_NEXT_DIGIT:
          if (is_digit(c)) {
            current_piece_.push_back(c);
          } else if (c == '.') {
            current_piece_.push_back(c);
            lexer_state_ = NUMBER_FIRST_DECIMAL;
          } else if (c == 'e' || c == 'E') {
            current_piece_.push_back('e');
            lexer_state_ = NUMBER_EXPONENT_SIGN;
          } else if (!handle_number_other(c)) {
            return false;
          }
          break;
        case NUMBER_NO_MORE_DIGIT:
          if (c == '.') {
            current_piece_.push_back(c);
            lexer_state_ = NUMBER_FIRST_DECIMAL;
          } else if (c == 'e' || c == 'E') {
            current_piece_.push_back('e');
            lexer_state_ = NUMBER_EXPONENT_SIGN;
          } else if (!handle_number_other(c)) {
            return false;
          }
          break;
        case NUMBER_FIRST_DECIMAL:
          if (is_digit(c)) {
            current_piece_.push_back(c);
            lexer_state_ = NUMBER_NEXT_DECIMAL;
          } else {
            return false;
          }
          break;
        case NUMBER_NEXT_DECIMAL:
          if (is_digit(c)) {
            current_piece_.push_back(c);
          } else if (c == 'e' || c == 'E') {
            current_piece_.push_back('e');
            lexer_state_ = NUMBER_EXPONENT_SIGN;
          } else if (!handle_number_other(c)) {
            return false;
          }
          break;
        case NUMBER_EXPONENT_SIGN:
          if (c == '-' || c == '+') {
            current_piece_.push_back(c);
            lexer_state_ = NUMBER_EXPONENT_FIRST_DIGIT;
          } else if (is_digit(c)) {
            current_piece_.push_back(c);
            lexer_state_ = NUMBER_EXPONENT_NEXT_DIGIT;
          } else {
            return false;
          }
          break;
        case NUMBER_EXPONENT_FIRST_DIGIT:
          if (is_digit(c)) {
            current_piece_.push_back(c);
            lexer_state_ = NUMBER_EXPONENT_NEXT_DIGIT;
          } else {
            return false;
          }
          break;
        case NUMBER_EXPONENT_NEXT_DIGIT:
          if (is_digit(c)) {
            current_piece_.push_back(c);
          } else if (!handle_number_other(c)) {
            return false;
          }
          break;
        case STRING:
          if (c == '"') {
            if (!handle_token(TOKEN_STRING)) {
              return false;
            }
            lexer_state_ = INITIAL;
          } else if (c == '\\') {
            current_piece_.push_back(c);
            lexer_state_ = STRING_ESCAPE;
          } else {
            current_piece_.push_back(c);
          }
          break;
        case STRING_ESCAPE:
          current_piece_.push_back(c);
          lexer_state_ = STRING;
          break;
      }
    }
    return true;
  }

  bool eof() {
    bool result;
    switch (lexer_state_) {
      case INITIAL:
        result = true;
        break;
      case CONSTANT:
        result = handle_constant();
        break;
      case NUMBER_FIRST_DIGIT:
      case NUMBER_FIRST_DECIMAL:
      case NUMBER_EXPONENT_SIGN:
      case NUMBER_EXPONENT_FIRST_DIGIT:
        return false;
      case NUMBER_NEXT_DIGIT:
      case NUMBER_NO_MORE_DIGIT:
      case NUMBER_NEXT_DECIMAL:
      case NUMBER_EXPONENT_NEXT_DIGIT:
        result = handle_number_other(' ');
        break;
      case STRING:
      case STRING_ESCAPE:
        return false;
    }
    return result && parser_state_ == PARSER_INITIAL;
  }

 private:
  bool handle_constant() {
    if (current_piece_ == "null") {
      return handle_token(TOKEN_NULL);
    } else if (current_piece_ == "true") {
      return handle_token(TOKEN_TRUE);
    } else if (current_piece_ == "false") {
      return handle_token(TOKEN_FALSE);
    } else {
      return false;
    }
  }

  bool handle_number_other(char c) {
    if (is_space(c)) {
      if (!handle_token(TOKEN_NUMBER)) {
        return false;
      }
      lexer_state_ = INITIAL;
    } else if (is_token_char(c)) {
      if (!handle_token(TOKEN_NUMBER) || !handle_token(c)) {
        return false;
      }
    } else {
      return false;
    }
    lexer_state_ = INITIAL;
    return true;
  }

  bool handle_token(int token) {
    switch (parser_state_) {
      case PARSER_INITIAL:
        if (token == '{') {
          parser_state_ = PARSER_OBJECT_FIRST_KEY;
          compound_object_stack_.emplace_back(json_value_t::OBJECT);
        } else if (token == '[') {
          parser_state_ = PARSER_ARRAY_FIRST_ELEMENT;
          compound_object_stack_.emplace_back(json_value_t::ARRAY);
        } else if (token >= TOKEN_NULL) {
          if (!prepare_value()) {
            return false;
          }
          if (!done_callback_(token_to_value(token))) {
            return false;
          }
        } else {
          return false;
        }
        break;
      case PARSER_ARRAY_FIRST_ELEMENT:
        if (token == ']') {
          if (!finish_compound()) {
            return false;
          }
          break;
        }
      /* FALLTHROUGH */
      case PARSER_ARRAY_NEXT_ELEMENT:
        if (token == '{') {
          parser_state_ = PARSER_OBJECT_FIRST_KEY;
          compound_object_stack_.emplace_back(json_value_t::OBJECT);
        } else if (token == '[') {
          parser_state_ = PARSER_ARRAY_FIRST_ELEMENT;
          compound_object_stack_.emplace_back(json_value_t::ARRAY);
        } else if (token >= TOKEN_NULL) {
          if (!prepare_value()) {
            return false;
          }
          compound_object_stack_.back().insert(token_to_value(token));
          parser_state_ = PARSER_ARRAY_COMMA;
        } else {
          return false;
        }
        break;
      case PARSER_ARRAY_COMMA:
        if (token == ',') {
          parser_state_ = PARSER_ARRAY_NEXT_ELEMENT;
        } else if (token == ']') {
          if (!finish_compound()) {
            return false;
          }
        } else {
          return false;
        }
        break;
      case PARSER_OBJECT_FIRST_KEY:
        if (token == '}') {
          if (!finish_compound()) {
            return false;
          }
          break;
        }
      /* FALLTHROUGH */
      case PARSER_OBJECT_NEXT_KEY:
        if (token == TOKEN_STRING) {
          std::string key;
          bool is_valid;
          std::tie(key, is_valid) = unescape_string();
          if (!is_valid) {
            return false;
          }
          key_stack_.push_back(std::move(key));
          parser_state_ = PARSER_OBJECT_COLON;
        } else {
          return false;
        }
        break;
      case PARSER_OBJECT_COLON:
        if (token == ':') {
          parser_state_ = PARSER_OBJECT_VALUE;
        } else {
          return false;
        }
        break;
      case PARSER_OBJECT_VALUE:
        if (token == '[') {
          compound_object_stack_.emplace_back(json_value_t::ARRAY);
          parser_state_ = PARSER_ARRAY_FIRST_ELEMENT;
        } else if (token == '{') {
          compound_object_stack_.emplace_back(json_value_t::OBJECT);
          parser_state_ = PARSER_ARRAY_FIRST_ELEMENT;
        } else if (token >= TOKEN_NULL) {
          if (!prepare_value()) {
            return false;
          }
          compound_object_stack_.back().insert(std::move(key_stack_.back()), token_to_value(token));
          key_stack_.pop_back();
          parser_state_ = PARSER_OBJECT_COMMA;
        } else {
          return false;
        }
        break;
      case PARSER_OBJECT_COMMA:
        if (token == '}') {
          if (!finish_compound()) {
            return false;
          }
        } else if (token == ',') {
          parser_state_ = PARSER_OBJECT_NEXT_KEY;
        } else {
          return false;
        }
        break;
    }
    current_piece_.clear();
    return true;
  }

  bool prepare_value() {
    std::string unescaped;
    bool is_valid;
    std::tie(unescaped, is_valid) = unescape_string();
    current_piece_ = std::move(unescaped);
    return is_valid;
  }

  json_value_t token_to_value(int token) {
    if (token == TOKEN_NULL) {
      return json_value_t(json_value_t::NULL_VALUE);
    } else if (token == TOKEN_TRUE || token == TOKEN_FALSE) {
      return json_value_t(token == TOKEN_TRUE);
    } else if (token == TOKEN_NUMBER) {
      return json_value_t(json_value_t::NUMBER, std::move(current_piece_));
    } else if (token == TOKEN_STRING) {
      return json_value_t(json_value_t::STRING, std::move(current_piece_));
    }
    ASSERT(false);
  }

  std::pair<std::string, bool> unescape_string() {
    std::string result;
    result.resize(current_piece_.size());
    for (std::string::iterator iter = current_piece_.begin(); iter != current_piece_.end();
         ++iter) {
      if (*iter == '\\') {
        ++iter;
        switch (*iter) {
          case '"':
          case '\\':
          case '/':
            result.push_back(*iter);
            break;
          case 'b':
            result.push_back(8);
            break;
          case 'f':
            result.push_back(12);
            break;
          case 'n':
            result.push_back(10);
            break;
          case 'r':
            result.push_back(13);
            break;
          case 't':
            result.push_back(9);
            break;
          case 'u': {
            int i;
            int32_t value = 0;
            for (i = 0, ++iter; i < 4 && iter != current_piece_.end(); ++i, ++iter) {
              if (i != 0) {
                value <<= 4;
              }
              char c = *iter;
              if (is_digit(c)) {
                value += c - '0';
              } else if (c >= 'a' && c <= 'f') {
                value += c - 'a' + 10;
              } else if (c >= 'A' && c <= 'F') {
                value += c - 'A' + 10;
              } else {
                return {result, false};
              }
            }
            --iter;
            if (i != 4) {
              return {result, false};
            }
#warning FIXME: Append UTF-8 value to string
          } break;
          default:
            return {result, false};
        }
      } else if (static_cast<uint8_t>(*iter) < 20) {
        return {result, false};
#warning FIXME: validate UTF-8
      } else {
        result.push_back(*iter);
      }
    }
    return {result, true};
  }

  bool finish_compound() {
    json_value_t value = std::move(compound_object_stack_.back());
    compound_object_stack_.pop_back();
    if (compound_object_stack_.empty()) {
      if (!done_callback_(std::move(value))) {
        return false;
      }
      parser_state_ = PARSER_INITIAL;
    } else if (compound_object_stack_.back().type() == json_value_t::OBJECT) {
      compound_object_stack_.back().insert(std::move(key_stack_.back()), std::move(value));
      key_stack_.pop_back();
      parser_state_ = PARSER_OBJECT_COMMA;
    } else if (compound_object_stack_.back().type() == json_value_t::ARRAY) {
      compound_object_stack_.back().insert(std::move(value));
      parser_state_ = PARSER_ARRAY_COMMA;
    }
    return true;
  }

  enum lexer_state_t {
    INITIAL,
    CONSTANT,
    NUMBER_FIRST_DIGIT,
    NUMBER_NEXT_DIGIT,
    NUMBER_NO_MORE_DIGIT,
    NUMBER_FIRST_DECIMAL,
    NUMBER_NEXT_DECIMAL,
    NUMBER_EXPONENT_SIGN,
    NUMBER_EXPONENT_FIRST_DIGIT,
    NUMBER_EXPONENT_NEXT_DIGIT,
    STRING,
    STRING_ESCAPE,
  };

  enum {
    TOKEN_NULL = 256,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_STRING,
    TOKEN_NUMBER,
  };

  enum parser_state_t {
    PARSER_INITIAL,
    PARSER_ARRAY_FIRST_ELEMENT,
    PARSER_ARRAY_NEXT_ELEMENT,
    PARSER_ARRAY_COMMA,
    PARSER_OBJECT_FIRST_KEY,
    PARSER_OBJECT_NEXT_KEY,
    PARSER_OBJECT_COLON,
    PARSER_OBJECT_VALUE,
    PARSER_OBJECT_COMMA,
  };

  lexer_state_t lexer_state_{INITIAL};
  std::string current_piece_;

  parser_state_t parser_state_{PARSER_INITIAL};
  std::vector<json_value_t> compound_object_stack_;
  std::vector<std::string> key_stack_;
  std::function<bool(json_value_t &&)> done_callback_;
};

#if 0
int main(int argc, char *argv[]) {
  ASSERT(argc == 2);

  FILE *file = fopen(argv[1], "rb");
  if (file == nullptr) {
    fatal("Could not open file %s: %m", argv[1]);
  }

  bool toplevel_seen = false;
  auto callback = [&](json_value_t &&value) {

    if (toplevel_seen) {
      return false;
    }
    toplevel_seen = true;
    return true;
  };

  json_deserializer_t deserialize(callback);

  char buffer[1024];
  while (!feof(file)) {
    size_t result = fread(buffer, 1, sizeof(buffer), file);
    if (!deserialize.append(string_view(buffer, result))) {
      return 1;
    }
  }
  return deserialize.eof() && toplevel_seen ? 0 : 1;
}
#endif
