# Copyright (C) 2011,2018 G.P. Halkes
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
	copy_file.cc \
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
	dialogs/characterdetailsdialog.cc \
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
include ../../t3shared/rules-base.mk
#================================================#

LDLIBS += -lt3widget -lt3window -ltranscript -lt3config -lt3highlight
LDFLAGS += $(T3LDFLAGS.t3widget) $(T3LDFLAGS.t3window) $(T3LDFLAGS.transcript) $(T3LDFLAGS.t3config) $(T3LDFLAGS.t3highlight)
LDLIBS += -lunistring
CXXFLAGS.option = -I.objects

CXXFLAGS += -DTILDE_DEBUG
CXXFLAGS += -DHAS_STRDUP
CXXFLAGS += -DHAS_POSIX_FALLOCATE
CXXFLAGS += -DHAS_SENDFILE
CXXFLAGS += -DHAS_COPY_FILE_RANGE
CXXFLAGS += -DHAS_FICLONE
#~ CXXFLAGS += -DUSE_GETTEXT -DLOCALEDIR=\"locales\"
CXXFLAGS += -std=c++11
CXXFLAGS += -DCXX11SWITCH=1
CXXFLAGS += -DDATADIR='"$(CURDIR)"'

clean-logs:
	@if [ -d .recordings ] ; then find .recordings/ -name "record-*" -mtime +7 | xargs -r rm ; fi
	@if [ -d .recordings ] ; then find .recordings/ -name "record-*" -mtime +1 | egrep -v '\.bz2$$' | xargs -r bzip2 -9 || true ; fi

linecount:
	@echo "WARNING: this doesn't count the header files, only .c files!"
	@echo "Linecounts as reported by wc -l"
	@wc -l $(CXXFILES)
	@if [ `which c_count 2>/dev/null` ] ; then echo "Linecounts as reported by the SLOCCount utilities" ; c_count $(CXXFILES) ; fi

xgettext: all
	xgettext -k_ -kN_ --add-comments=TRANSLATORS -o ../po/messages.pot `cat $$(find .deps -type f) | sed -r 's/^.+://;s/\\\\//g'`

.objects/%.bytes: %.schema
	$(GENOBJDIR)
	$(_VERBOSE_GEN) ../../t3config/src/data2bytes -s -- $< > $@

.objects/option.o: .objects/config.bytes

clang-format:
	clang-format -i *.cc *.h */*.cc */*.h

.clang-tidy-opts: Makefile
	@echo "-xc++ -std=c++11 $(filter -D%, $(CXXFLAGS)) $(filter -I%, $(CXXFLAGS) $(CXXFLAGS.option))" | sed "s/'/\"/g" > .clang-tidy-opts

clang-tidy: .clang-tidy-opts $(patsubst %, clang-tidy/%, $(foreach STEM, $(CXXTARGETS), $(SOURCES.$(STEM)))) \
	$(patsubst %.cc, clang-tidy/%.h, $(foreach STEM, $(CXXTARGETS), $(SOURCES.$(STEM))))

clang-tidy/%:
	@if [[ -f "$*" ]] ; then echo "[CLANG-TIDY] $*" ; clang-tidy $(CT_CHECKS) $* -- `cat .clang-tidy-opts` ; fi

clean::
	rm -f .clang-tidy-opts

.PHONY: links xgettext linecount clang-format clang-tidy
