#!/bin/bash
#
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright © 2023–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
#                       Matthias Kretz <m.kretz@gsi.de>

dir="${0%/*}"
[[ "$dir" == '.' ]] && dir="$PWD"

usage() {
  archlist=$($CXX -x c++ -march=xxx - 2>&1 </dev/null|grep 'valid arguments'|sed 's/^.*are: //')
  cat <<EOF
Usage: $0 <name> [<compiler -std -f -O -include -I or -D flags>] [<arch list>]

<name> must be one of:
$(cd "$dir"; echo *.cpp|sed 's/\.cpp\>//g')

<arch list> can be any combination of:
$archlist

The arguments can be given in any order.
EOF
}

name=
set_name() {
  if [[ -n "$name" ]]; then
    echo "ERROR: more than one benchmark name was given."
    echo
    usage
    exit 1
  elif [[ ! -r "$dir/${1}.cpp" ]]; then
    echo "ERROR: benchmark '$1' not found."
    echo
    usage
    exit 1
  else
    name="$1"
  fi
}

std=-std=gnu++26
opt=-O3
flags=("-Wall" "-fmax-errors=2" "-static-libstdc++" "-DVIR_NEXT_PATCH" "-DVIR_EXTENSIONS")
while (($# > 0)); do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    -O*)
      opt="$1"
      ;;
    -std=*)
      std="$1"
      ;;
    -f*)
      flags=("${flags[@]}" "$1")
      ;;
    -include)
      flags=("${flags[@]}" "$1" "$2")
      shift
      ;;
    -[ID])
      flags=("${flags[@]}" "$1$2")
      shift
      ;;
    -[ID]*)
      flags=("${flags[@]}" "$1")
      ;;
    *.cpp)
      set_name "${1%.cpp}"
      ;;
    *)
      if [[ -r "$dir/${1}.cpp" ]]; then
        set_name "$1"
      else
        arch_list="$arch_list $1"
      fi
      ;;
  esac
  shift
done

test -z "$arch_list" && arch_list="native westmere k8"

CCACHE=`which ccache 2>/dev/null` || CCACHE=

mkdir -p "$dir/bin"

cxxflags="-g0 -Wall -Wextra -Wno-psabi $opt $std"
{
  cd "$dir"
  make -s CXXFLAGS="$cxxflags" bin/compile_commands.json
}

if [[ -z "$name" ]]; then
  echo "ERROR: benchmark name is missing."
  echo
  usage
  exit 1
fi

realtime="chrt --fifo 10"
$realtime true 2>/dev/null || realtime=

if [[ -z "$realtime" ]]; then
  echo "Add '$USER  -  rtprio  10' to /etc/security/limits.conf for less noisy benchmark results"
fi

for arch in ${arch_list}; do
  if [[ $arch == "generic" ]]; then
    CXXFLAGS="$cxxflags -lmvec"
  else
    CXXFLAGS="$cxxflags -march=$arch -lmvec"
  fi

  echo $CCACHE $CXX $CXXFLAGS "${flags[@]}" "$dir/${name}.cpp" -o "$dir/bin/$name-$arch"
  $CCACHE $CXX $CXXFLAGS "${flags[@]}" "$dir/${name}.cpp" -o "$dir/bin/$name-$arch"
  if (($? == 0)); then
    echo "-march=$arch $flags:"
    "$dir/benchmark-mode.sh" on
    $realtime "$dir/bin/$name-$arch"
    "$dir/benchmark-mode.sh" off
  fi
done

# vim: tw=0 si
