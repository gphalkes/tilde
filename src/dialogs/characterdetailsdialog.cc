#include "characterdetailsdialog.h"

#include <t3window/utf8.h>
#include <uniname.h>

static std::string get_codepoint_name(uint32_t codepoint) {
  static const char digit_to_char[] = "0123456789ABCDEF";
  std::string full_name = "U+";
  if (codepoint > 0xffff) {
    if (codepoint > 0xfffff) {
      full_name += digit_to_char[(codepoint >> 20) & 0xf];
    }
    full_name += digit_to_char[(codepoint >> 16) & 0xf];
  }
  for (int i = 3; i >= 0; --i) {
    full_name += digit_to_char[(codepoint >> (i * 4)) & 0xf];
  }

  char name_buffer[UNINAME_MAX];
  const char *name = unicode_character_name(codepoint, name_buffer);
  if (name) {
    full_name += " ";
    full_name += name;
  } else if (codepoint < 0x20) {
    static const char *control_char_names[]{"NULL",
                                            "START OF HEADING",
                                            "START OF TEXT",
                                            "END OF TEXT",
                                            "END OF TRANSMISSION",
                                            "ENQUIRY",
                                            "ACKNOWLEDGE",
                                            "BELL",
                                            "BACKSPACE",
                                            "CHARACTER TABULATION",
                                            "LINE FEED",
                                            "LINE TABULATION",
                                            "FORM FEED",
                                            "CARRIAGE RETURN",
                                            "SHIFT OUT",
                                            "SHIFT IN",
                                            "DATA LINK ESCAPE",
                                            "DEVICE CONTROL ONE",
                                            "DEVICE CONTROL TWO",
                                            "DEVICE CONTROL THREE",
                                            "DEVICE CONTROL FOUR",
                                            "NEGATIVE ACKNOWLEDGE",
                                            "SYNCHRONOUS IDLE",
                                            "END OF TRANSMISSION BLOCK",
                                            "CANCEL",
                                            "END OF MEDIUM",
                                            "SUBSTITUTE",
                                            "ESCAPE",
                                            "INFORMATION SEPARATOR FOUR",
                                            "INFORMATION SEPARATOR THREE",
                                            "INFORMATION SEPARATOR TWO",
                                            "INFORMATION SEPARATOR ONE"};
    full_name += " ";
    full_name += control_char_names[codepoint];
  } else if ((codepoint >= 0xE000 && codepoint <= 0xF8FF) ||
             (codepoint >= 0xF0000 && codepoint <= 0xFFFFD) ||
             (codepoint >= 0x100000 && codepoint <= 0x10FFFD)) {
    full_name += " Private use area";
  }
  return full_name;
}

character_details_dialog_t::character_details_dialog_t(int height, int width)
    : dialog_t(height, width, "Character Details") {
  list = emplace_back<list_pane_t>(false);
  list->set_size(height - 2, width - 2);
  list->set_position(1, 1);
  list->connect_activate(bind_front(&character_details_dialog_t::close, this));
}

bool character_details_dialog_t::set_size(optint height, optint width) {
  bool result = true;

  if (!height.is_valid()) {
    height = window.get_height();
  }
  if (!width.is_valid()) {
    width = window.get_width();
  }

  result &= dialog_t::set_size(height, width);

  result &= list->set_size(height.value() - 3, width.value() - 2);
  return result;
}

void character_details_dialog_t::show() {
  list->set_current(0);
  dialog_t::show();
}

void character_details_dialog_t::set_codepoints(const char *data, size_t size) {
  codepoints.clear();
  while (!list->empty()) {
    list->erase(list->begin());
  }
  size_t bytes_read = size;
  while (bytes_read > 0) {
    uint32_t codepoint = t3_utf8_get(data, &bytes_read);
    codepoints.push_back(codepoint);
    list->push_back(make_unique<label_t>(get_codepoint_name(codepoint).c_str()));
    size -= bytes_read;
    data += bytes_read;
    bytes_read = size;
  }
}
