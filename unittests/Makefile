# Copyright (C) 2018 G.P. Halkes
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3, as
# published by the Free Software Foundation.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

SOURCES.copy_file_test := \
  copy_file_test.cc \
  src/copy_file.cc \
  /usr/src/googletest/googletest/src/gtest-all.cc

CXXFLAGS./usr/src/googletest/googletest/src/gtest-all := -I/usr/src/googletest/googletest
LDLIBS.copy_file_test := -lgflags

CXXTARGETS := copy_file_test
#================================================#
# NO RULES SHOULD BE DEFINED BEFORE THIS INCLUDE #
#================================================#
include ../../makesys/rules.mk
include ../../t3shared/rules-base.mk
#================================================#

LDLIBS += -lt3widget
LDFLAGS += $(T3LDFLAGS.t3widget)
LDFLAGS += -pthread

CXXFLAGS += -DTILDE_UNITTEST
CXXFLAGS += -DHAS_SENDFILE
CXXFLAGS += -DHAS_COPY_FILE_RANGE
CXXFLAGS += -DHAS_FICLONE
CXXFLAGS += -std=c++11

clang-format:
	clang-format -i *.cc

.clang-tidy-opts: Makefile
	@echo "-xc++ -std=c++11 $(filter -D%, $(CXXFLAGS)) $(filter -I%, $(CXXFLAGS) $(CXXFLAGS.option))" | sed "s/'/\"/g" > .clang-tidy-opts

clang-tidy: .clang-tidy-opts $(patsubst %, clang-tidy/%, $(foreach STEM, $(CXXTARGETS), $(SOURCES.$(STEM)))) \
	$(patsubst %.cc, clang-tidy/%.h, $(foreach STEM, $(CXXTARGETS), $(SOURCES.$(STEM))))

clang-tidy/%:
	@if [[ -f "$*" ]] ; then echo "[CLANG-TIDY] $*" ; clang-tidy $(CT_CHECKS) $* -- `cat .clang-tidy-opts` ; fi

clean::
	rm -f .clang-tidy-opts

.PHONY: clang-format clang-tidy
