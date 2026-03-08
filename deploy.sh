#!/bin/bash
#
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
#                       Matthias Kretz <m.kretz@gsi.de>
#
# Deploy script that copies include/bits/*.h files while removing VIR_EXTENSIONS blocks

set -e

# Source and destination directories
SRC_DIR="include/bits"
DEST_DIR="/home/mkretz/src/gcc-simd/libstdc++-v3/include/bits"

# Create destination directory if it doesn't exist
mkdir -p "$DEST_DIR"

filter() {
  invert=0
  [[ "$2" == "invert" ]] && invert=1
awk '
BEGIN {
  skip = 0
  invert = '$invert'
  in_negated_if = 0
  drop_next_endif = 0
}
function sanity_check(cond) {
  if (!cond) {
    print "Error: Nested '$1' at line " NR ": " $0 > "/dev/stderr"
    exit 1
  }
  if (drop_next_endif != 0) {
    print "Error at line " NR ": " $0 > "/dev/stderr"
    exit 1
  }
}
/^#if '$1'\>/ {
  if (invert == 1) {
    sanity_check(in_negated_if == 0)
    in_negated_if = 1
  } else {
    sanity_check(skip == 0)
    skip = 1
  }
  next
}
/^#if !'$1'$/ {
  if (invert == 0) {
    sanity_check(in_negated_if == 0)
    in_negated_if = 1
  } else {
    sanity_check(skip == 0)
    skip = 1
  }
  next
}
/^#elif '$1'\>/ {
  if (invert == 1) {
    print "#else"
    sanity_check(in_negated_if == 0 && skip == 0)
    in_negated_if = 1
  } else {
    sanity_check(in_negated_if == 0)
    skip = 1
  }
  next
}
/^#if/ && skip > 0 {
  skip += 1
  next
}
/^#if/ && in_negated_if > 0 {
  in_negated_if += 1
}
/^#elif\>/ && skip == 1 {
  skip = 0
  sub(/^#elif/, "#if")
}
/^#elif\>/ && in_negated_if == 1 {
  in_negated_if = 0
  sub(/^#elif/, "#if")
}
/^#else\>/ && (in_negated_if == 1 || skip == 1) {
  if (in_negated_if == 1) {
    in_negated_if = 0
    if (skip != 0) {
      print "Error: Nested '$1' at line " NR ": " $0 > "/dev/stderr"
      exit 1
    }
    skip = 1
  } else {
    drop_next_endif = 1
    skip = 0
  }
  next
}
/^#endif\>/ && skip > 0 {
  skip -= 1
  if (skip == 0) {
    next
  }
}
/^#endif\>/ && in_negated_if > 0 {
  in_negated_if -= 1
  if (in_negated_if == 0) {
    next
  }
}
/^#endif\>/ && drop_next_endif == 1 {
  drop_next_endif = 0
  next
}
skip == 0 {
  print
}
'
}

fix_copyright() {
  cat <<EOF
// Implementation of <simd> -*- C++ -*-

// Copyright The GNU Toolchain Authors.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.
EOF
grep '^$' -A100000
}

# Process each .h file in the source directory
for file in "$SRC_DIR"/*.h; do
  if [ -f "$file" ]; then
    filename=$(basename "$file")
    echo "Processing $filename..."

    # Use awk to handle VIR_EXTENSIONS conditional blocks
    cat "$file" | filter VIR_EXTENSIONS \
      | filter VIR_PATCH_PERMUTE_DYNAMIC \
      | filter VIR_PATCH_MATH \
      | filter VIR_PATCH_MISSED_OPT \
      | filter VIR_PATCH_TEST_STORES \
      | filter VIR_ASSERT_SANITY \
      | fix_copyright > "$DEST_DIR/$filename"
  fi
done

echo "Deployment complete. Processed files copied to $DEST_DIR/"
