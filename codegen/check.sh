# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
#                       Matthias Kretz <m.kretz@gsi.de>

#!/bin/sh

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
        if ! echo "$asm" | grep --color=auto "$line"; then
          asm="$(grep -A$((n+1)) "$regex" "$asmfile" | tail -n1)"
          if echo "$asm" | grep --color=auto "$line"; then
            n=$((n+1))
          else
            echo "Failure on:"
            grep -A$n "$regex" "$asmfile"
            echo "Expected '$line'."
            exit 2
          fi
        fi
        n=$((n+1))
        ;;
    esac
  fi
done
[ $? = 1 ]
