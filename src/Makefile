# Copyright (C) 2011 G.P. Halkes
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3, as
# published by the Free Software Foundation.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

SOURCES..objects/edit := \
	filebuffer.cc \
	fileeditwindow.cc \
	filestate.cc \
	filewrapper.cc \
	log.cc \
	main.cc \
	openfiles.cc \
	option.cc \
	util.cc \
	dialogs/encodingdialog.cc \
	dialogs/openrecentdialog.cc \
	dialogs/selectbufferdialog.cc \
	dialogs/optionsdialog.cc

LDLIBS..objects/edit += -lt3widget -lt3window -ltranscript -lt3unicode -lt3config
LDFLAGS..objects/edit += -Lt3widget/.libs -Lt3window/.libs -Ltranscript/.libs -Lt3unicode/.libs -Lt3config/.libs
LDFLAGS..objects/edit += `pkg-config --libs sigc++-2.0`

CXXTARGETS := .objects/edit
#================================================#
# NO RULES SHOULD BE DEFINED BEFORE THIS INCLUDE #
#================================================#
include ../../makesys/rules.mk
#================================================#
CXXFLAGS += -I.
#~ CXXFLAGS += -DUSE_GETTEXT -DLOCALEDIR=\"locales\"
#~ CXXFLAGS += -DUSE_UNICODE
#~ CXXFLAGS += -DUSE_NL_LANGINFO
CXXFLAGS += `pkg-config --cflags sigc++-2.0`

all: | links
	@find .recordings/ -name "record-*" -mtime +7 | xargs -r rm
	@find .recordings/ -name "record-*" -mtime +1 | egrep -v '\.bz2$$' | xargs -r bzip2 -9 || true

linecount:
	@echo "WARNING: this doesn't count the header files, only .c files!"
	@echo "Linecounts as reported by wc -l"
	@wc -l $(CXXFILES)
	@if [ `which c_count 2>/dev/null` ] ; then echo "Linecounts as reported by the SLOCCount utilities" ; c_count $(CXXFILES) ; fi

xgettext: all
	xgettext -k_ -kN_ --add-comments=TRANSLATORS -o ../po/messages.pot `cat $$(find .deps -type f) | sed -r 's/^.+://;s/\\\\//g'`

links:
	@[ -L record ] || ln -s ../../../record/src record
	@[ -L t3unicode ] || ln -s ../../unicode/src t3unicode
	@[ -L t3window ] || ln -s ../../window/src t3window
	@[ -L t3widget ] || ln -s ../../widget/src t3widget
	@[ -L t3config ] || ln -s ../../config/src t3config
	@[ -L transcript ] || ln -s ../../../transcript/src transcript

.PHONY: links xgettext linecount
