/* Copyright (C) 2011 G.P. Halkes
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
#include <cstring>
#include <deque>
#include <algorithm>
#include <t3widget/widget.h>
#include <transcript/transcript.h>
#include "encodingdialog.h"
#include "log.h"

using namespace std;

struct charset_desc_t {
	const char *name; // Display name for this character set
	const char *tag; // Known working name for this character set
};

typedef deque<charset_desc_t> charset_descs_t;

static charset_desc_t friendly_charsets[] = {
	{ "Arabic (ISO-8859-6)", "ISO-8859-6" },
	{ "Arabic (Windows-1256)", "WINDOWS-1256" },
	{ "Baltic (Windows-1257)", "WINDOWS-1257" },
	{ "Baltic (ISO-8859-13)", "ISO-8859-13" },
	{ "Baltic (ISO-8859-4)", "ISO-8859-4" },
	{ "Central European (IBM-852)", "IBM-852" },
	{ "Central European (ISO-8859-2)", "ISO-8859-2" },
	{ "Central European (ISO-8859-3)", "ISO-8859-3" },
	{ "Central European (Windows-1250)", "WINDOWS-1250" },
	{ "Chinese Simplified (GB18030)", "GB18030" },
	{ "Chinese Simplified (GB2312)", "GB2312" },
	{ "Chinese Simplified (GBK)", "GBK" },
	{ "Chinese Traditional (Big5)", "BIG5" },
	{ "Chinese Traditional (Big5-HKSCS)", "BIG5-HKSCS" },
	{ "Chinese Traditional (EUC-TW)", "EUC-TW" },
	{ "Cyrillic (IBM-866)", "IBM-866" },
	{ "Cyrillic (ISO-8859-5)", "ISO-8859-5" },
	{ "Cyrillic (KOI8-R)", "KOI8-R" },
	{ "Cyrillic (KOI8-U)", "KOI8-U" },
	{ "Cyrillic (Windows-1251)", "WINDOWS-1251" },
	{ "Greek (ISO-8859-2)", "ISO-8859-2" },
	{ "Greek (ISO-8859-7)", "ISO-8859-7" },
	{ "Greek (Windows-1253)", "WINDOWS-1253" },
	{ "Hebrew (ISO-8859-8)", "ISO-8859-8" },
	{ "Hebrew (Windows-1255)", "WINDOWS-1255" },
	{ "Japanese (EUC-JP)", "EUC-JP" },
	{ "Japanese (ISO-2022-JP)", "ISO-2022-JP" },
	{ "Japanese (Shift_JIS)", "SHIFT_JIS" },
	{ "Korean (EUC-KR)", "EUC-KR" },
	{ "Korean (ISO-2022-KR)", "ISO-2022-KR" },
	{ "Northern Saami (Winsami2)", "WINSAMI2" },
	{ "South-Eastern European (ISO-8859-16)", "ISO-8859-16" },
	{ "Tamil (TSCII)", "TSCII" },
	{ "Thai (Windows-874)", "WINDOWS-874" },
	{ "Thai (ISO-8859-11)", "ISO-8859-11" },
	{ "Thai (TIS-620)", "TIS-620" },
	{ "Thai (Windows-874)", "WINDOWS-874" },
	{ "Turkish (Windows-1254)", "WINDOWS-1254" },
	{ "Turkish (ISO-8859-9)", "ISO-8859-9" },
	{ "Unicode (UTF-8 with BOM)", "X-UTF-8-BOM" },
	{ "Unicode (UTF-16)", "UTF-16" },
	{ "Unicode (UTF-7)", "UTF-7" },
	{ "Vietnamese (Windows-1258)", "WINDOWS-1258" },
	{ "Western European (IBM-850)", "IBM-850" },
	{ "Western European (ISO-8859-1)", "ISO-8859-1" },
	{ "Western European (ISO-8859-14)", "ISO-8859-14" },
	{ "Western European (ISO-8859-15)", "ISO-8859-15" },
	{ "Western European (Windows-1252)", "WINDOWS-1252" },
	{ NULL, NULL }
};

charset_descs_t available_charsets;

static bool compare_charset_names(charset_desc_t a, charset_desc_t b) {
	return strcmp(a.name, b.name) < 0;
}

void init_charsets(void) {
	// As we use UTF-8 internally, we don't need to convert, and so this is always available
	charset_desc_t utf8 = { "Unicode (UTF-8)", "UTF-8" };
	charset_desc_t other = { "Other (use text field below)", "<OTHER>" };

	transcript_init();
	available_charsets.push_back(utf8);
	for (charset_desc_t *ptr = &friendly_charsets[0]; ptr->name != NULL; ptr++) {
		if (!transcript_probe_converter(ptr->tag)) {
			lprintf("Unavailable: %s\n", ptr->name);
			continue;
		}
		lprintf("Adding character set %s with tag %s\n", ptr->name, ptr->tag);
		available_charsets.push_back(*ptr);
	}
	sort(available_charsets.begin(), available_charsets.end(), compare_charset_names);
	available_charsets.push_back(other);
}



encoding_dialog_t::encoding_dialog_t(int height, int width) :
	dialog_t(height, width, "Encoding"), selected(-1), saved_tag(NULL)
{
	button_t *ok_button, *cancel_button;

	list = new list_pane_t(true);
	list->set_size(height - 4, width - 2);
	list->set_position(1, 1);
	list->connect_activate(sigc::mem_fun(this, &encoding_dialog_t::ok_activated));
	list->connect_selection_changed(sigc::mem_fun(this, &encoding_dialog_t::selection_changed));

	for (charset_descs_t::const_iterator iter = available_charsets.begin(); iter != available_charsets.end(); iter++) {
		label_t *label = new label_t(iter->name);
		list->push_back(label);
	}

	horizontal_separator = new separator_t();
	horizontal_separator->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_BOTTOMLEFT));
	horizontal_separator->set_position(-2, 1);
	horizontal_separator->set_size(None, width - 2);


	manual_entry = new text_field_t();
	manual_entry->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_BOTTOMLEFT));
	manual_entry->set_position(-1, 2);
	manual_entry->set_size(1, 25);
	manual_entry->connect_activate(sigc::mem_fun(this, &encoding_dialog_t::ok_activated));
	manual_entry->hide();

	cancel_button = new button_t("_Cancel", false);
	cancel_button->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
	cancel_button->set_position(-1, -2);
	cancel_button->connect_activate(sigc::mem_fun(this, &encoding_dialog_t::close));
	ok_button = new button_t("_OK", true);
	ok_button->set_anchor(cancel_button, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	ok_button->set_position(0, -2);
	ok_button->connect_activate(sigc::mem_fun(this, &encoding_dialog_t::ok_activated));

	push_back(list);
	push_back(horizontal_separator);
	push_back(manual_entry);
	push_back(ok_button);
	push_back(cancel_button);
}

bool encoding_dialog_t::set_size(optint height, optint width) {
	bool result = true;

	if (!height.is_valid())
		height = t3_win_get_height(window);
	if (!width.is_valid())
		width = t3_win_get_width(window);

	result &= dialog_t::set_size(height, width);
	result &= list->set_size(height - 4, width - 2);
	return result;
}

void encoding_dialog_t::ok_activated(void) {
	string encoding;
	size_t idx = list->get_current();

	if (idx + 1 == list->size()) {
		// User specified character set
		encoding = *manual_entry->get_text();

		lprintf("Testing encoding name: %s\n", encoding.c_str());
		if (!transcript_probe_converter(encoding.c_str())) {
			string message = "Requested character set is not available";
			message_dialog->set_message(&message);
			message_dialog->center_over(this);
			message_dialog->show();
			return;
		}
	} else {
		encoding = available_charsets[idx].tag;
	}
	activate(&encoding);
	hide();
}

void encoding_dialog_t::selection_changed(void) {
	if (list->get_current() + 1 == list->size())
		manual_entry->show();
	else
		manual_entry->hide();
}

void encoding_dialog_t::set_encoding(const char *encoding) {
	charset_descs_t::const_iterator iter;
	int i;

	if (encoding == NULL)
		encoding = "UTF-8";

	for (iter = available_charsets.begin(), i = 0; iter != available_charsets.end(); iter++, i++) {
		if (transcript_equal(encoding, iter->tag)) {
			manual_entry->set_text("");
			manual_entry->hide();
			list->set_current(i);
			return;
		}
	}

	manual_entry->set_text(encoding);
	manual_entry->show();
	list->set_current(list->size() - 1);
}
