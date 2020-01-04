#include "tilde/option_access.h"

#include <cstring>

#include <t3widget/widget.h>

namespace {

static struct {
  const char *string;
  t3_attr_t attr;
} attribute_map[] = {{"underline", T3_ATTR_UNDERLINE | T3_ATTR_UNDERLINE_SET},
                     {"bold", T3_ATTR_BOLD | T3_ATTR_BOLD_SET},
                     {"reverse", T3_ATTR_REVERSE | T3_ATTR_REVERSE_SET},
                     {"blink", T3_ATTR_BLINK | T3_ATTR_BLINK_SET},
                     {"dim", T3_ATTR_DIM | T3_ATTR_DIM_SET},

                     {"no underline", T3_ATTR_UNDERLINE_SET},
                     {"no bold", T3_ATTR_BOLD_SET},
                     {"no reverse", T3_ATTR_REVERSE_SET},
                     {"no blink", T3_ATTR_BLINK_SET},
                     {"no dim", T3_ATTR_DIM_SET},

                     {"fg default", T3_ATTR_FG_DEFAULT},
                     {"fg black", T3_ATTR_FG_BLACK},
                     {"fg red", T3_ATTR_FG_RED},
                     {"fg green", T3_ATTR_FG_GREEN},
                     {"fg yellow", T3_ATTR_FG_YELLOW},
                     {"fg blue", T3_ATTR_FG_BLUE},
                     {"fg magenta", T3_ATTR_FG_MAGENTA},
                     {"fg cyan", T3_ATTR_FG_CYAN},
                     {"fg white", T3_ATTR_FG_WHITE},

                     {"bg default", T3_ATTR_BG_DEFAULT},
                     {"bg black", T3_ATTR_BG_BLACK},
                     {"bg red", T3_ATTR_BG_RED},
                     {"bg green", T3_ATTR_BG_GREEN},
                     {"bg yellow", T3_ATTR_BG_YELLOW},
                     {"bg blue", T3_ATTR_BG_BLUE},
                     {"bg magenta", T3_ATTR_BG_MAGENTA},
                     {"bg cyan", T3_ATTR_BG_CYAN},
                     {"bg white", T3_ATTR_BG_WHITE}};

OptionAccess option_access[] = {
    OptionAccess("color", &runtime_options_t::color, &term_options_t::color, true),
    OptionAccess("wrap", &runtime_options_t::wrap, &options_t::wrap, false),
    OptionAccess("tab_spaces", &runtime_options_t::tab_spaces, &options_t::tab_spaces, false),
    OptionAccess("auto_indent", &runtime_options_t::auto_indent, &options_t::auto_indent, true),
    OptionAccess("indent_aware_home", &runtime_options_t::indent_aware_home,
                 &options_t::indent_aware_home, true),
    OptionAccess("show_tabs", &runtime_options_t::show_tabs, &options_t::show_tabs, false),
    OptionAccess("strip_spaces", &runtime_options_t::strip_spaces, &options_t::strip_spaces, false),
    OptionAccess("make_backup", &runtime_options_t::make_backup, &options_t::make_backup, false),
    OptionAccess("hide_menubar", &runtime_options_t::hide_menubar, &options_t::hide_menubar, false),
    OptionAccess("parse_file_positions", nullptr, &options_t::parse_file_positions, true),
    OptionAccess("disable_primary_selection_over_ssh", nullptr,
                 &options_t::disable_primary_selection_over_ssh, false),
    OptionAccess("save_recent_files", &runtime_options_t::save_recent_files,
                 &options_t::save_recent_files, true),
    OptionAccess("restore_cursor_position", &runtime_options_t::restore_cursor_position,
                 &options_t::restore_cursor_position, true),
    OptionAccess("tabsize", &runtime_options_t::tabsize, &options_t::tabsize, 8),
    OptionAccess("max_recent_files", &runtime_options_t::max_recent_files,
                 &options_t::max_recent_files, 16),
    OptionAccess("key_timeout", &runtime_options_t::key_timeout, &term_options_t::key_timeout),

    OptionAccess("brace_highlight", &runtime_options_t::brace_highlight,
                 &term_options_t::brace_highlight, nullopt),
    OptionAccess("non_print", nullptr, &term_options_t::non_print, attribute_t::NON_PRINT),
    OptionAccess("text_selection_cursor", nullptr, &term_options_t::text_selection_cursor,
                 attribute_t::TEXT_SELECTION_CURSOR),
    OptionAccess("text_selection_cursor2", nullptr, &term_options_t::text_selection_cursor2,
                 attribute_t::TEXT_SELECTION_CURSOR2),
    OptionAccess("bad_draw", nullptr, &term_options_t::bad_draw, attribute_t::BAD_DRAW),
    OptionAccess("text_cursor", nullptr, &term_options_t::text_cursor, attribute_t::TEXT_CURSOR),
    OptionAccess("text", nullptr, &term_options_t::text, attribute_t::TEXT),
    OptionAccess("text_selected", nullptr, &term_options_t::text_selected,
                 attribute_t::TEXT_SELECTED),
    OptionAccess("hotkey_highlight", nullptr, &term_options_t::hotkey_highlight,
                 attribute_t::HOTKEY_HIGHLIGHT),

    OptionAccess("dialog", nullptr, &term_options_t::dialog, attribute_t::DIALOG),
    OptionAccess("dialog_selected", nullptr, &term_options_t::dialog_selected,
                 attribute_t::DIALOG_SELECTED),
    OptionAccess("button_selected", nullptr, &term_options_t::button_selected,
                 attribute_t::BUTTON_SELECTED),
    OptionAccess("scrollbar", nullptr, &term_options_t::scrollbar, attribute_t::SCROLLBAR),
    OptionAccess("menubar", nullptr, &term_options_t::menubar, attribute_t::MENUBAR),
    OptionAccess("menubar_selected", nullptr, &term_options_t::menubar_selected,
                 attribute_t::MENUBAR_SELECTED),

    OptionAccess("shadow", nullptr, &term_options_t::shadow, attribute_t::SHADOW),
    OptionAccess("meta_text", nullptr, &term_options_t::meta_text, attribute_t::META_TEXT),
    OptionAccess("background", nullptr, &term_options_t::background, attribute_t::BACKGROUND),
};

}  // namespace

const OptionAccess *get_option_access(const std::string &name) {
  static std::map<std::string, const OptionAccess *> *mapping = [] {
    auto *mapping = new std::map<std::string, const OptionAccess *>;
    for (const OptionAccess &access : option_access) {
      (*mapping)[access.name] = &access;
    }
    return mapping;
  }();
  auto iter = mapping->find(name);
  if (iter == mapping->end()) {
    return nullptr;
  }
  return iter->second;
}

static t3_attr_t attribute_string_to_bin(const char *attr) {
  bool foreground;
  char *endptr;
  int color;

  for (const auto &mapping : attribute_map) {
    if (strcmp(attr, mapping.string) == 0) {
      return mapping.attr;
    }
  }

  if (strncmp(attr, "fg ", 3) == 0) {
    foreground = true;
  } else if (strncmp(attr, "bg ", 3) == 0) {
    foreground = false;
  } else {
    return 0;
  }

  color = static_cast<int>(std::strtol(attr + 3, &endptr, 0));
  if (*endptr != 0) {
    return 0;
  }
  if (color < 0 || color > 255) {
    return 0;
  }
  return foreground ? T3_ATTR_FG(color) : T3_ATTR_BG(color);
}

static optional<t3_attr_t> convert_config_attribute(t3_config_t *attr_config) {
  t3_attr_t accumulated_attr = 0;

  if (attr_config == nullptr) {
    return nullopt;
  }

  for (attr_config = t3_config_get(attr_config, nullptr); attr_config != nullptr;
       attr_config = t3_config_get_next(attr_config)) {
    if (t3_config_get_type(attr_config) == T3_CONFIG_STRING) {
      accumulated_attr = t3_term_combine_attrs(
          attribute_string_to_bin(t3_config_get_string(attr_config)), accumulated_attr);
    }
  }

  return accumulated_attr;
}

void get_term_options(t3_config_t *config, term_options_t *term_options) {
  t3_config_t *attributes = t3_config_get(config, "attributes");
  for (const OptionAccess &access : option_access) {
    t3_config_t *tmp = t3_config_get(
        access.type == OptionAccess::TERM_T3_ATTR_T ? attributes : config, access.name.c_str());
    switch (access.type) {
      case OptionAccess::BOOL:
      case OptionAccess::INT:
      case OptionAccess::SIZE_T:
        break;
      case OptionAccess::TERM_BOOL:
        if (tmp != nullptr) {
          term_options->*access.bool_term_opt = t3_config_get_bool(tmp);
        }
        break;
      case OptionAccess::TERM_OPTIONAL_INT:
        if (tmp != nullptr) {
          term_options->*access.int_term_opt = t3_config_get_int(tmp);
        }
        break;
      case OptionAccess::TERM_T3_ATTR_T:
        term_options->*access.t3_attr_t_term_opt = convert_config_attribute(tmp);
        break;
    }
  }
  t3_config_t *highlight_attributes = t3_config_get(config, "highlight_attributes");
  for (t3_config_t *ptr = t3_config_get(highlight_attributes, nullptr); ptr != nullptr;
       ptr = t3_config_get_next(ptr)) {
    optional<t3_attr_t> attribute = convert_config_attribute(ptr);
    if (attribute.is_valid()) {
      term_options->highlights.insert_mapping(t3_config_get_name(ptr), attribute.value());
    }
  }
}

void get_default_options(t3_config_t *config) {
  for (const OptionAccess &access : option_access) {
    t3_config_t *tmp = t3_config_get(config, access.name.c_str());
    if (tmp != nullptr) {
      switch (access.type) {
        case OptionAccess::BOOL:
          default_option.*access.bool_option = t3_config_get_bool(tmp);
          break;
        case OptionAccess::INT:
          default_option.*access.int_option = t3_config_get_int(tmp);
          break;
        case OptionAccess::SIZE_T:
          default_option.*access.size_t_option = t3_config_get_int64(tmp);
          break;
        case OptionAccess::TERM_BOOL:
        case OptionAccess::TERM_OPTIONAL_INT:
        case OptionAccess::TERM_T3_ATTR_T:
          break;
      }
    }
  }
  get_term_options(config, &default_option.term_options);
}

/* Helper templates to map a type to a specific t3_config_add_XXX function. */
template <typename ValueType>
typename std::enable_if<std::is_same<ValueType, bool>::value>::type set_option_helper(
    t3_config_t *config, const std::string &name, ValueType value) {
  t3_config_add_bool(config, name.c_str(), value);
}

template <typename ValueType>
typename std::enable_if<std::is_same<ValueType, int>::value>::type set_option_helper(
    t3_config_t *config, const std::string &name, ValueType value) {
  t3_config_add_int(config, name.c_str(), value);
}

template <typename ValueType>
typename std::enable_if<std::is_same<ValueType, size_t>::value>::type set_option_helper(
    t3_config_t *config, const std::string &name, ValueType value) {
  t3_config_add_int64(config, name.c_str(), value);
}

template <typename OptionType, typename MemberPtr>
void set_option(t3_config_t *config, const std::string &name, const OptionType &opts,
                MemberPtr member) {
  if ((opts.*member).is_valid()) {
    set_option_helper(config, name, (opts.*member).value());
  } else {
    t3_config_erase(config, name.c_str());
  }
}

static void set_config_attribute(t3_config_t *config, const char *section_name, const char *name,
                                 optional<t3_attr_t> attr) {
  static t3_attr_t attribute_masks[] = {T3_ATTR_FG_MASK,
                                        T3_ATTR_BG_MASK,
                                        T3_ATTR_UNDERLINE | T3_ATTR_UNDERLINE_SET,
                                        T3_ATTR_BOLD | T3_ATTR_BOLD_SET,
                                        T3_ATTR_REVERSE | T3_ATTR_REVERSE_SET,
                                        T3_ATTR_BLINK | T3_ATTR_BLINK_SET,
                                        T3_ATTR_DIM | T3_ATTR_DIM_SET};

  t3_config_t *attributes;
  if ((attributes = t3_config_get(config, section_name)) == nullptr ||
      t3_config_get_type(attributes) != T3_CONFIG_SECTION) {
    if (!attr.is_valid()) {
      return;
    }
    attributes = t3_config_add_section(config, section_name, nullptr);
  }

  if (!attr.is_valid()) {
    t3_config_erase(attributes, name);
    return;
  }

  config = t3_config_add_list(attributes, name, nullptr);

  for (const auto &mask : attribute_masks) {
    t3_attr_t search = attr.value() & mask;
    if (search == 0) {
      continue;
    }

    bool mapping_found = false;
    for (const auto &mapping : attribute_map) {
      if (mapping.attr == search) {
        mapping_found = true;
        t3_config_add_string(config, nullptr, mapping.string);
        break;
      }
    }

    if (!mapping_found && (mask == T3_ATTR_FG_MASK || mask == T3_ATTR_BG_MASK)) {
      char color_name_buffer[32];
      if (mask == T3_ATTR_FG_MASK) {
        sprintf(color_name_buffer, "fg %d", (search >> T3_ATTR_COLOR_SHIFT) - 1);
      } else {
        sprintf(color_name_buffer, "bg %d", (search >> (T3_ATTR_COLOR_SHIFT + 9)) - 1);
      }
      t3_config_add_string(config, nullptr, color_name_buffer);
    }
  }
}

void set_term_options(t3_config_t *config, const term_options_t &term_options) {
  for (const OptionAccess &access : option_access) {
    switch (access.type) {
      case OptionAccess::BOOL:
      case OptionAccess::INT:
      case OptionAccess::SIZE_T:
        break;
      case OptionAccess::TERM_BOOL:
        set_option(config, access.name, term_options, access.bool_term_opt);
        break;
      case OptionAccess::TERM_OPTIONAL_INT:
        set_option(config, access.name, term_options, access.int_term_opt);
        break;
      case OptionAccess::TERM_T3_ATTR_T:
        set_config_attribute(config, "attributes", access.name.c_str(),
                             term_options.*access.t3_attr_t_term_opt);
        break;
    }
  }
  for (const auto &attribute : term_options.highlights) {
    set_config_attribute(config, "highlight_attributes", std::string(attribute.first).c_str(),
                         attribute.second);
  }

  /* Remove empty sections. */
  if (t3_config_get(t3_config_get(config, "highlight_attributes"), nullptr) == nullptr) {
    t3_config_erase(config, "highlight_attributes");
  }
  if (t3_config_get(t3_config_get(config, "attributes"), nullptr) == nullptr) {
    t3_config_erase(config, "attributes");
  }
}

void set_default_options(t3_config_t *config) {
  for (const OptionAccess &access : option_access) {
    switch (access.type) {
      case OptionAccess::BOOL:
        set_option(config, access.name, default_option, access.bool_option);
        break;
      case OptionAccess::INT:
        set_option(config, access.name, default_option, access.int_option);
        break;
      case OptionAccess::SIZE_T:
        set_option(config, access.name, default_option, access.size_t_option);
        break;
      case OptionAccess::TERM_BOOL:
      case OptionAccess::TERM_OPTIONAL_INT:
      case OptionAccess::TERM_T3_ATTR_T:
        break;
    }
  }
  set_term_options(config, default_option.term_options);
  default_option.term_options.key_timeout.reset();
}

void derive_runtime_options() {
  for (const OptionAccess &access : option_access) {
    switch (access.type) {
      case OptionAccess::BOOL:
        if (access.bool_runtime_opt != nullptr) {
          option.*access.bool_runtime_opt =
              (default_option.*access.bool_option).value_or(access.bool_default);
        }
        break;
      case OptionAccess::INT:
        if (access.int_runtime_opt != nullptr) {
          option.*access.int_runtime_opt =
              (default_option.*access.int_option).value_or(access.int_default);
        }
        break;
      case OptionAccess::SIZE_T:
        if (access.size_t_runtime_opt != nullptr) {
          option.*access.size_t_runtime_opt =
              (default_option.*access.size_t_option).value_or(access.size_t_default);
        }
        break;
      case OptionAccess::TERM_BOOL:
        if (access.bool_runtime_opt != nullptr) {
          option.*access.bool_runtime_opt =
              (term_specific_option.*access.bool_term_opt)
                  .value_or((default_option.term_options.*access.bool_term_opt)
                                .value_or(access.bool_default));
        }
        break;
      case OptionAccess::TERM_OPTIONAL_INT:
        if (access.optional_int_runtime_opt) {
          option.*access.optional_int_runtime_opt = term_specific_option.*access.int_term_opt;
          if (!(option.*access.optional_int_runtime_opt).is_valid()) {
            option.*access.optional_int_runtime_opt =
                default_option.term_options.*access.int_term_opt;
          }
        }
        break;
      case OptionAccess::TERM_T3_ATTR_T:
        if (access.t3_attr_t_runtime_opt) {
          option.*access.t3_attr_t_runtime_opt =
              (term_specific_option.*access.t3_attr_t_term_opt)
                  .value_or((default_option.term_options.*access.t3_attr_t_term_opt)
                                .value_or(get_default_attr(BRACE_HIGHLIGHT)));
        }
        break;
    }
  }

  /* FIXME: this should be derivable from a table, rather than having to be listed here. */
  option.highlights.insert_mapping("comment", get_default_attr(COMMENT));
  option.highlights.insert_mapping("comment-keyword", get_default_attr(COMMENT_KEYWORD));
  option.highlights.insert_mapping("keyword", get_default_attr(KEYWORD));
  option.highlights.insert_mapping("number", get_default_attr(NUMBER));
  option.highlights.insert_mapping("string", get_default_attr(STRING));
  option.highlights.insert_mapping("string-escape", get_default_attr(STRING_ESCAPE));
  option.highlights.insert_mapping("misc", get_default_attr(MISC));
  option.highlights.insert_mapping("variable", get_default_attr(VARIABLE));
  option.highlights.insert_mapping("error", get_default_attr(ERROR));
  option.highlights.insert_mapping("addition", get_default_attr(ADDITION));
  option.highlights.insert_mapping("deletion", get_default_attr(DELETION));

  for (const auto &attribute : default_option.term_options.highlights) {
    option.highlights.insert_mapping(attribute.first, attribute.second);
  }
  for (const auto &attribute : term_specific_option.highlights) {
    option.highlights.insert_mapping(attribute.first, attribute.second);
  }

  /* Overrides from command line interface. */
  if (cli_option.color.is_valid()) {
    option.color = cli_option.color.value();
  }
  if (cli_option.ask_input_method) {
    option.key_timeout.reset();
  }
}

void set_attributes() {
  for (const OptionAccess &access : option_access) {
    if (access.type == OptionAccess::TERM_T3_ATTR_T && access.attribute.is_valid()) {
      optional<t3_attr_t> attr = term_specific_option.*access.t3_attr_t_term_opt;
      if (!attr.is_valid()) {
        attr = default_option.term_options.*access.t3_attr_t_term_opt;
      }
      set_attribute(access.attribute.value(),
                    attr.value_or(get_default_attribute(access.attribute.value(), option.color)));
    }
  }
}
