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

# Process each .h file in the source directory
for file in "$SRC_DIR"/*.h; do
  if [ -f "$file" ]; then
    filename=$(basename "$file")
    echo "Processing $filename..."

    # Use awk to handle VIR_EXTENSIONS conditional blocks
    awk '
    /^#if VIR_EXTENSIONS$/ {
      skip = 1
      next
    }
    /^#if !VIR_EXTENSIONS$/ {
      in_negated_if = 1
      next
    }
    /^#else$/ && in_negated_if {
      in_negated_if = 0
      skip = 1
      next
    }
    /^#endif$/ && skip {
      skip = 0
      next
    }
    !skip {
        print
    }
    ' "$file" > "$DEST_DIR/$filename"
  fi
done

echo "Deployment complete. Processed files copied to $DEST_DIR/"
