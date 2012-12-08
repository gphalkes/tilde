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

#~ COMPILER := gcc

SOURCES..objects/edit := \
	fileautocompleter.cc \
	filebuffer.cc \
	fileeditwindow.cc \
	fileline.cc \
	filestate.cc \
	filewrapper.cc \
	log.cc \
	main.cc \
	openfiles.cc \
	option.cc \
	util.cc \
	dialogs/attributesdialog.cc \
	dialogs/encodingdialog.cc \
	dialogs/highlightdialog.cc \
	dialogs/openrecentdialog.cc \
	dialogs/selectbufferdialog.cc \
	dialogs/optionsdialog.cc

LDLIBS..objects/edit += -lt3widget -lt3window -ltranscript -lt3config -lt3highlight
LDFLAGS..objects/edit += -L../include/t3widget/.libs -L../include/t3window/.libs -L../include/transcript/.libs -L../include/t3config/.libs -L../include/t3highlight/.libs
LDFLAGS..objects/edit += -Wl,-rpath=$(CURDIR)/../include/t3widget/.libs:$(CURDIR)/../include/t3window/.libs:$(CURDIR)/../include/transcript/.libs:$(CURDIR)/../include/t3config/.libs:$(CURDIR)/../include/t3highlight/.libs:$(CURDIR)/../../key/src/.libs
LDLIBS..objects/edit += `pkg-config --libs sigc++-2.0` -lunistring
LDLIBS..objects/edit += -lacl
LDLIBS..objects/edit += -lattr
CXXFLAGS.option = -I.objects

CXXTARGETS := .objects/edit
EXTRATARGETS := clean-logs
#================================================#
# NO RULES SHOULD BE DEFINED BEFORE THIS INCLUDE #
#================================================#
include ../../makesys/rules.mk
#================================================#
CXXFLAGS += -I../include
CXXFLAGS += -DTILDE_DEBUG
CXXFLAGS += -DHAS_STRDUP
CXXFLAGS += -DHAS_LIBACL
CXXFLAGS += -DHAS_LIBATTR
#~ CXXFLAGS += -DUSE_GETTEXT -DLOCALEDIR=\"locales\"
CXXFLAGS += `pkg-config --cflags sigc++-2.0`

clean-logs:
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

.objects/%.bytes: %.schema
	$(GENOBJDIR)
	$(_VERBOSE_GEN) ../include/t3config/data2bytes -s -- $< > $@

.objects/option.o: .objects/config.bytes

.PHONY: links xgettext linecount

