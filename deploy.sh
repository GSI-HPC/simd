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
#DEST_DIR="bits"

# Create destination directory if it doesn't exist
mkdir -p "$DEST_DIR"

filter() {
awk '
BEGIN {
  skip = 0
  in_negated_if = 0
}
/^#if '$1'\>/ {
  if (skip != 0) {
    print "Error: Nested '$1' at line " NR ": " $0 > "/dev/stderr"
    exit 1
  }
  skip = 1
  next
}
/^#elif '$1'\>/ {
  if (skip > 1) {
    print "Error: Nested '$1' at line " NR ": " $0 > "/dev/stderr"
    exit 1
  }
  skip = 1
  next
}
/^#if/ && skip > 0 {
  skip += 1
  next
}
/^#elif/ && skip == 1 {
  skip = 0
  sub(/^#elif/, "#if")
}
/^#if !'$1'$/ {
  in_negated_if = 1
  next
}
/^#else$/ && in_negated_if {
  in_negated_if = 0
  if (skip != 0) {
    print "Error: Nested '$1' at line " NR ": " $0 > "/dev/stderr"
    exit 1
  }
  skip = 1
  next
}
/^#endif$/ && skip > 0 {
  if (skip > 0) {
    skip -= 1
    if (skip == 0) {
      next
    }
  } else if (in_negated_if) {
    print "Error: #if !'$1' expects #else" > "/dev/stderr"
    exit 1
  }
}
skip == 0 {
  print
}
'
}

# Process each .h file in the source directory
for file in "$SRC_DIR"/*.h; do
  if [ -f "$file" ]; then
    filename=$(basename "$file")
    echo "Processing $filename..."

    # Use awk to handle VIR_EXTENSIONS conditional blocks
    cat "$file" | filter VIR_EXTENSIONS | filter VIR_NEXT_PATCH > "$DEST_DIR/$filename"
  fi
done

echo "Deployment complete. Processed files copied to $DEST_DIR/"
