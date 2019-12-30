#ifndef OPTION_ACCESS_H
#define OPTION_ACCESS_H

#include "tilde/option.h"

using namespace t3widget;

struct OptionAccess {
  enum Type {
    BOOL,
    INT,
    SIZE_T,
    TERM_BOOL,
    TERM_OPTIONAL_INT,
    TERM_T3_ATTR_T,
  };
  const Type type;
  const std::string name;
  union {
    bool runtime_options_t::*bool_runtime_opt;
    int runtime_options_t::*int_runtime_opt;
    size_t runtime_options_t::*size_t_runtime_opt;
    optional<int> runtime_options_t::*optional_int_runtime_opt;
    t3_attr_t runtime_options_t::*t3_attr_t_runtime_opt;
  };

  union {
    optional<bool> options_t::*bool_option;
    optional<int> options_t::*int_option;
    optional<size_t> options_t::*size_t_option;
  };

  union {
    optional<bool> term_options_t::*bool_term_opt;
    optional<int> term_options_t::*int_term_opt;
    optional<t3_attr_t> term_options_t::*t3_attr_t_term_opt;
  };

  union {
    bool bool_default;
    int int_default;
    size_t size_t_default;
  };

  optional<attribute_t> attribute;

  OptionAccess(const std::string &name_arg, bool runtime_options_t::*bool_runtime_opt_arg,
               optional<bool> options_t::*bool_option_arg, bool dflt)
      : type(BOOL),
        name(name_arg),
        bool_runtime_opt(bool_runtime_opt_arg),
        bool_option(bool_option_arg),
        bool_term_opt(nullptr),
        bool_default(dflt) {}

  OptionAccess(const std::string &name_arg, int runtime_options_t::*int_runtime_opt_arg,
               optional<int> options_t::*int_option_arg, int dflt)
      : type(INT),
        name(name_arg),
        int_runtime_opt(int_runtime_opt_arg),
        int_option(int_option_arg),
        int_term_opt(nullptr),
        int_default(dflt) {}

  OptionAccess(const std::string &name_arg, size_t runtime_options_t::*size_t_runtime_opt_arg,
               optional<size_t> options_t::*size_t_option_arg, size_t dflt)
      : type(SIZE_T),
        name(name_arg),
        size_t_runtime_opt(size_t_runtime_opt_arg),
        size_t_option(size_t_option_arg),
        int_term_opt(nullptr),
        size_t_default(dflt) {}

  OptionAccess(const std::string &name_arg, bool runtime_options_t::*bool_runtime_opt_arg,
               optional<bool> term_options_t::*bool_term_opt_arg, bool dflt)
      : type(TERM_BOOL),
        name(name_arg),
        bool_runtime_opt(bool_runtime_opt_arg),
        bool_option(nullptr),
        bool_term_opt(bool_term_opt_arg),
        bool_default(dflt) {}

  OptionAccess(const std::string &name_arg,
               optional<int> runtime_options_t::*optional_int_runtime_opt_arg,
               optional<int> term_options_t::*int_term_opt_arg)
      : type(TERM_OPTIONAL_INT),
        name(name_arg),
        optional_int_runtime_opt(optional_int_runtime_opt_arg),
        int_option(nullptr),
        int_term_opt(int_term_opt_arg) {}

  OptionAccess(const std::string &name_arg, t3_attr_t runtime_options_t::*t3_attr_t_runtime_opt_arg,
               optional<t3_attr_t> term_options_t::*t3_attr_t_term_opt_arg,
               optional<attribute_t> attribute_arg)
      : type(TERM_T3_ATTR_T),
        name(name_arg),
        t3_attr_t_runtime_opt(t3_attr_t_runtime_opt_arg),
        int_option(nullptr),
        t3_attr_t_term_opt(t3_attr_t_term_opt_arg),
        attribute(attribute_arg) {}
};

extern OptionAccess option_access[];

void get_term_options(t3_config_t *config, term_options_t *term_options);
void get_default_options(t3_config_t *config);
void set_term_options(t3_config_t *config, const term_options_t &term_options);
void set_default_options(t3_config_t *config);

void derive_runtime_options();
void set_attributes();

#endif  // OPTION_ACCESS_H
