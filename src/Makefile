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

#COMPILER := gcc
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

CXXTARGETS := .objects/edit
EXTRATARGETS := clean-logs
#================================================#
# NO RULES SHOULD BE DEFINED BEFORE THIS INCLUDE #
#================================================#
include ../../makesys/rules.mk
#================================================#

LDLIBS += -lt3widget -lt3window -ltranscript -lt3config -lt3highlight
LDFLAGS += $(call L, ../include/t3widget/.libs ../include/t3window/.libs ../include/transcript/.libs ../include/t3config/.libs ../include/t3highlight/.libs)
LDLIBS += -lunistring
ifndef CPP11
LDLIBS += `pkg-config --libs sigc++-2.0`
endif
LDLIBS += -lacl
LDLIBS += -lattr
CXXFLAGS.option = -I.objects

CXXFLAGS += -I../include
CXXFLAGS += -DTILDE_DEBUG
CXXFLAGS += -DHAS_STRDUP
CXXFLAGS += -DHAS_LIBACL
CXXFLAGS += -DHAS_LIBATTR
#~ CXXFLAGS += -DUSE_GETTEXT -DLOCALEDIR=\"locales\"
CXXFLAGS += -std=c++11
CXXFLAGS += -DCXX11SWITCH=1
CXXFLAGS += -DDATADIR='"$(CURDIR)"'

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

.objects/%.bytes: %.schema
	$(GENOBJDIR)
	$(_VERBOSE_GEN) ../include/t3config/data2bytes -s -- $< > $@

.objects/option.o: .objects/config.bytes .objects/base_config.bytes

clang-format:
	clang-format -i *.cc *.h */*.cc */*.h

.clang-tidy-opts: Makefile
	$(call _GENDIR,.clang-tidy,$<)
	@echo "-xc++ -std=c++11 $(filter -D%, $(CXXFLAGS)) $(filter -I%, $(CXXFLAGS))" > .clang-tidy-opts

clang-tidy: .clang-tidy-opts $(patsubst %, clang-tidy/%, $(foreach STEM, $(CXXTARGETS), $(SOURCES.$(STEM)))) \
	$(patsubst %.cc, clang-tidy/%.h, $(foreach STEM, $(CXXTARGETS), $(SOURCES.$(STEM))))

clang-tidy/%:
	@if [[ -f "$*" ]] ; then echo "[CLANG-TIDY] $*" ; clang-tidy $* -- `cat .clang-tidy-opts` ; fi

clean::
	rm -f .clang-tidy-opts

.PHONY: links xgettext linecount clang-format clang-tidy
