default: info

include Makefile.common

info: $(check_targets)
	@echo "This library is header-only and doesn't need to be built."
	@echo "However, you might want to run tests. For a complete list"
	@echo "call 'make help'."

fortests := for t in $(tests); do
fortestarchs := for a in $(testarchs); do
fortesttypes := for type in $(testtypes); do
fortestwidths := for w in $(testwidths); do

.PHONY: debug
debug:
	@echo "compiler: $(compiler)"
	@echo "$(testarchs)"
	@echo "$(tests)"
	@echo "$(testwidths)"
	@for i in $(testtypes); do echo "- $$i"; done
	@echo "width=$(call getwidth,shift_left.core2/signed-char.34)"
	@echo "type=$(call gettype,shift_left.core2/signed-char.34)"
	@echo "arch=$(call getarch,shift_left.core2/signed-char.34)"
	@echo "test=$(call gettest,shift_left.core2/signed-char.34)"
	@echo "type=$(call gettype,shift_left.core2/std--float32_t.34)"
	@$(MAKE) -f Makefile.more $@

more_checks := check check10 check1 check-failed check-passed check-untested

.PHONY: $(more_checks)
$(more_checks): $(check_targets)
	@$(MAKE) -f Makefile.more --no-print-directory $@

# run-missing sets exe_template to "" so that binaries are not rebuild but still a dependency of the check/% targets
.PHONY: check-run-missing
check-run-missing: $(check_targets)
	@$(MAKE) -f Makefile.more --no-print-directory exe_template= $@

helptargets := $(more_checks) check-run-missing $(codegen_targets) check-constexpr

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
	      echo "check/$(t).$(arch)/$(type).$$$$w";done)))) \
	)

$(check_targets): $(wildcard tests/*.cpp) Makefile
	$(file >$@)
	$(foreach t,$(tests),$(foreach w,$(testwidths),$(foreach y,$(testtypes),$(foreach a,$(testarchs),\
		$(file >>$@,check/$t.$a/$y.$w)))))

.NOTINTERMEDIATE: $(subst check,obj,$(patsubst %,%.exe,$(shell cat $(check_targets))))

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
	rm -rf obj
	rm -rf check

