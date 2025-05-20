# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
#                       Matthias Kretz <m.kretz@gsi.de>

default: info

include Makefile.common

prefix = /usr
includedir = $(prefix)/include

install:
	@echo "Installing to $(prefix)/include"
	install -d $(includedir)/bits
	install -m 644 -t $(includedir) simd
	install -m 644 -t $(includedir)/bits bits/*.h

sysincludedir = $(dir $(shell echo "#include <vector>"|"$(CXX)" -std=c++17 -x c++ -E -o- -|grep '^# 1 "/.*/vector"'|cut -d'"' -f2))

install-system:
	@echo "Installing to $(sysincludedir)"
	@result=0; for i in bits/*.h; do \
		test -f $(sysincludedir)$$i || continue; \
		echo "Error: would overwrite $$i"; \
		result=1; \
	done; exit $$result
	install -m 644 -t $(sysincludedir) simd
	install -m 644 -t $(sysincludedir)/bits bits/*.h

uninstall-system:
	@echo "Uninstalling from $(sysincludedir)"
	@rm $(sysincludedir)simd
	@for i in bits/*.h; do rm $(sysincludedir)$$i; done

info: $(check_targets)
	@echo "This library is header-only and doesn't need to be built."
	@echo
	@echo "Installation"
	@echo "============"
	@echo "> make prefix=~/.local install"
	@echo
	@echo "Alternatively, install into the compiler's standard library directory:"
	@echo "> make install-system"
	@echo
	@echo "To uninstall from the system:"
	@echo "> make uninstall-system"
	@echo
	@echo "Testing"
	@echo "======="
	@echo "Compile and run tests via 'make check'."
	@echo "For all possible check targets call 'make help'."

fortests := for t in $(tests); do
fortestarchs := for a in $(testarchs); do
fortesttypes := for type in $(testtypes); do
fortestwidths := for w in $(testwidths); do

clang_flags = $(CXXFLAGS)
ifneq ($(compiler),clang)
clang_flags := $(filter-out $(CXXFLAGS_$(compiler)),$(clang_flags)) $(CXXFLAGS_clang)
endif
clang_flags := $(filter-out -Werror -ferror-limit=%,$(clang_flags))
define ccjson
  { "directory": "$(PWD)",
    "arguments": ["$(CXX)", $(clang_flags:%="%",) "-march=$1", "-include", "obj/$1.hpp", "-S", "$2"],
    "file": "$2" }
endef

obj/compile_commands.json: obj Makefile Makefile.common
	$(file >$@,[)
	$(file >>$@,$(call ccjson,$(firstword $(testarchs)),constexpr_tests.c++)$(foreach arch,$(wordlist 2,$(words $(testarchs)),$(testarchs)),,$(call ccjson,$(arch),constexpr_tests.c++)))
	$(file >>$@,])

.PHONY: metrics
metrics:
	@sloccount *.h tests/*.h Makefile*

.PHONY: tidy
tidy: obj/compile_commands.json
	@clang-tidy -p obj constexpr_tests.c++

.PHONY: debug
debug:
	@echo "obj dir rule: $(make_obj_dir_rule)"
	@echo "compiler: $(compiler)"
	@echo "CXXFLAGS: $(CXXFLAGS)"
	@echo "$(testarchs)"
	@echo "$(tests)"
	@echo "$(testwidths)"
	@for i in $(testtypes); do echo "- $$i"; done
	@echo "width=$(call getwidth,shift_left.core2/signed-char.34)"
	@echo "type=$(call gettype,shift_left.core2/signed-char.34)"
	@echo "arch=$(call getarch,shift_left.core2/signed-char.34)"
	@echo "test=$(call gettest,shift_left.core2/signed-char.34)"
	@echo "type=$(call gettype,shift_left.core2/std--float32_t.34)"
	@echo "prefix: $(prefix)"
	@$(MAKE) -f Makefile.more $@

more_checks := check check10 check1 check-failed check-passed check-untested check-fast-math rerun

.PHONY: $(more_checks)
$(more_checks): $(check_targets)
	@$(MAKE) -f Makefile.more --no-print-directory $@

helptargets := $(more_checks) $(codegen_targets) check-constexpr

define simple_check_template
check-$(1): $(2)

helptargets+=check-$(1)

endef

define check_template
check-$(1):
	@$$(MAKE) --no-print-directory $$(shell $(2) | shuf)

helptargets+=check-$(1)

endef

$(foreach t,$(tests),\
	$(eval $(call check_template,$(t),$(fortestarchs) $(fortestwidths) $(fortesttypes) \
	  echo "check/$(t).$$$$a/$$$$type.$$$$w";done;done;done)) \
	)

$(foreach type,$(testtypes),\
	$(eval $(call check_template,$(type),$(fortests) $(fortestarchs) $(fortestwidths) \
	  echo "check/$$$$t.$$$$a/$(type).$$$$w";done;done;done)) \
	)

$(foreach arch,$(testarchs),\
	$(eval $(call simple_check_template,constexpr-$(arch),obj/constexpr.$(arch).s))\
	$(eval $(call check_template,$(arch),$(fortesttypes) $(fortests) \
	  echo "check-$$$$t.$(arch).$$$$type";done;done;\
	  echo "check-constexpr-$(arch)")) \
	)

$(foreach type,$(testtypes),\
	$(foreach w,$(testwidths),\
	  $(eval $(call check_template,$(type).$(w),$(fortests) $(fortestarchs) \
	    echo "check/$$$$t.$$$$a/$(type).$(w)";done;done))) \
	)

$(foreach t,$(tests),\
	$(foreach type,$(testtypes),\
	  $(eval $(call check_template,$(t).$(type),$(fortestarchs) $(fortestwidths) \
	    echo "check/$(t).$$$$a/$(type).$$$$w";done;done))) \
	$(foreach type,$(testtypes),\
	  $(foreach w,$(testwidths),\
	    $(eval $(call check_template,$(t).$(type).$(w),$(fortestarchs) \
	      echo "check/$(t).$$$$a/$(type).$(w)";done)))) \
	$(foreach arch,$(testarchs),\
	  $(foreach type,$(testtypes),\
	    $(eval $(call check_template,$(t).$(arch).$(type),$(fortestwidths) \
	      echo "check/$(t).$(arch)/$(type).$$$$w";done))) \
	  $(foreach w,1 4 8 16,\
	    $(eval $(call check_template,$t.$(arch).$w,$(fortesttypes) \
	      echo "check/$t.$(arch)/$$$$type.$w";done)))) \
	)

$(check_targets): obj/compile_commands.json $(wildcard tests/*.cpp) Makefile Makefile.common
	$(file >$@)
	$(foreach t,$(tests),$(foreach w,$(testwidths),$(foreach y,$(testtypes),$(foreach a,$(testarchs),\
		$(file >>$@,check/$t.$a/$y.$w)))))

REPORTFLAGS=-fmem-report -ftime-report -Q

obj/%.report:
	@echo "Build time reports for $*"
	@$(CXX) $(CXXFLAGS) $(REPORTFLAGS) -march=$(call getarch,$*) \
		-D UNITTEST_TYPE="$(call gettype,$*)" \
		-D UNITTEST_WIDTH=$(call getwidth,$*) \
		-c -o obj/$*.o tests/$(call gettest,$*).cpp
	@echo "Build time reports for $* done"

helptxt := obj/help.txt

$(helptxt): Makefile $(check_targets)
	$(file >$@)
	$(foreach t,$(helptargets),$(file >>$@,... $(t)))
	@sed 's/^/... /' $(check_targets) >>$@

.PHONY: help
help: $(helptxt)
	@echo "Define `DIRECT` to anything non-empty to compile and link in one step"
	@cat $(helptxt)

.PHONY: clean
clean:
	test -L obj && { rm -rf $(realpath obj); rm obj; }
	rm -rf obj
	rm -rf check

