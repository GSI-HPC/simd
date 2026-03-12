# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright © 2025–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
#                       Matthias Kretz <m.kretz@gsi.de>

benchmarks=$(patsubst %.cpp,%,$(wildcard *.cpp))
archs=native ivybridge westmere x86-64
CXXFLAGS=-g0 -O2 -std=gnu++26 -Wall -Wextra -Wno-psabi -fmax-errors=2 -D VIR_PATCH_MATH -DVIR_EXTENSIONS

fastmath=-ffast-math
default=
improve_cx1=-DVIR_PATCH_IMPROVE_CX=1
improve_cx2=-DVIR_PATCH_IMPROVE_CX=2
improve_cx3=-DVIR_PATCH_IMPROVE_CX=3

variants=default fastmath improve_cx1 improve_cx2 improve_cx3

all: all-targets

bin:
	@mkdir -p bin

data:
	@mkdir -p data

targets=
define maketarget
bin/$1-$2-$3: $1.cpp bench.h ../obj/libsimd.so bin/compile_commands.json | bin
	$$(CXX) $$(CXXFLAGS) $$($2) -march=$3 -lmvec ../obj/libsimd.so $1.cpp -o $$@

data/$1-$2-$3.out: bin/$1-$2-$3 | data
	@./benchmark-mode.sh on
	@$$< | tee $$@
	@./benchmark-mode.sh off

.PHONY: $1-$2-$3
$1-$2-$3: bin/$1-$2-$3
run-$1-$2-$3: data/$1-$2-$3.out

targets+=$1-$2-$3
targets+=run-$1-$2-$3

endef

$(foreach b,$(benchmarks),$(foreach v,$(variants),$(foreach a,$(archs),$(eval $(call maketarget,$b,$v,$a)))))

.PHONY: all-targets
all-targets: $(patsubst %,bin/%,$(targets))

.PHONY: benchmark
benchmark: $(targets)

define ccjson
  { "directory": "$(PWD)",
    "arguments": ["$(CXX)", $(2:%="%",) "-march=$1", "$3"],
    "file": "$3" },
endef

bin/compile_commands.json: Makefile run.sh bin
	@echo "Setting up 'compilation database' for Clang tooling."
	$(file >$@,[)
	$(foreach v,$(variants),$(foreach b,$(benchmarks),$(foreach a,$(archs),$(file >>$@,$(call ccjson,$a,$(CXXFLAGS) $($v),$b.cpp)))))
	@truncate --size=-2 "$@"
	@echo "" >> "$@"
	@echo "]" >> "$@"

help: bin/compile_commands.json
	@echo "$(targets)"|tr ' ' '\n'
	@echo "benchmark"
	@echo "all"

../obj/libsimd.so: ../lib/*.* ../include/bits/*.h
	$(MAKE) -C .. obj/libsimd.so
