#ifndef CHARACTERINFODIALOG_H
#define CHARACTERINFODIALOG_H

#include <t3widget/widget.h>
using namespace t3widget;

class character_details_dialog_t : public dialog_t {
 private:
  list_pane_t *list;
  std::vector<uint32_t> codepoints;

 public:
  character_details_dialog_t(int height, int width);

  bool set_size(optint height, optint width) override;
  void show() override;

  void set_codepoints(const char *data, size_t size);
};

#endif  // CHARACTERINFODIALOG_H
