#!/bin/sh
#
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
#                       Matthias Kretz <m.kretz@gsi.de>

srcfile="$1"
asmfile="$2"

parse=false
n=0
cat "$srcfile" | while read line; do
  if ! $parse; then
    [ "$line" = "/* codegen" ] && parse=true
  else
    case "$line" in
      '')
        n=$((n+1))
        ;;

      *'*/')
        parse=false
        ;;

      '^'*)
        regex="$line"
        n=1
        ;;

      *)
        asm="$(grep -A$n "$regex" "$asmfile" | tail -n1)"
        if [ "?${line#?}" = "$line" ]; then
          optional=true
          line="${line#?}"
        else
          optional=false
        fi
        if ! echo "$asm" | grep -E --color=auto "$line"; then
          if $optional; then
            echo "Skip optional $line"
            continue
          fi
          #asm="$(grep -A$((n+1)) "$regex" "$asmfile" | tail -n1)"
          #if echo "$asm" | grep -E --color=auto "$line"; then
            #echo "Warning: skipped $(grep -A$n "$regex" "$asmfile" | tail -n1)"
            #n=$((n+1))
          #else
            echo "Failure on:"
            grep -A$((n+1)) "$regex" "$asmfile"
            echo "Expected '$line'."
            exit 2
          #fi
        fi
        n=$((n+1))
        ;;
    esac
  fi
done
[ $? = 1 ]
