#!/bin/bash

no_turbo=/sys/devices/system/cpu/intel_pstate/no_turbo
boost=/sys/devices/system/cpu/cpufreq/boost

write() {
  #echo "write $*"
  val="$1"
  file="$2"
  if [[ -w "$file" ]]; then
    echo "$1" > "$file"
  else
    echo "$1" | sudo tee "$file" >/dev/null
  fi
}

turn_on() {
  echo "enabling benchmark mode (no clock scaling/turbo)"
  for i in /sys/devices/system/cpu/cpufreq/policy[0-9]*/scaling_governor; do
    write performance $i
  done
  if test -f $no_turbo; then
    write 1 $no_turbo
  elif test -f $boost; then
    write 0 $boost
  else
    echo "failed to disable turbo/boost" >&2
  fi
}

turn_off() {
  echo "disabling benchmark mode"
  governor=schedutil
  if test -f $no_turbo; then
    write 0 $no_turbo
    governor=powersave
  elif test -f $boost; then
    echo 1 > $boost
  else
    echo "failed to enable turbo/boost" >&2
  fi
  for i in /sys/devices/system/cpu/cpufreq/policy[0-9]*/scaling_governor; do
    write $governor $i
  done
}

while (($# > 0)); do
  case "$1" in
    -h|--help)
      usage
      exit
      ;;
    --chown)
      test -f $no_turbo && sudo chown $USER $no_turbo
      test -f $boost && sudo chown $USER $boost
      sudo chown $USER \
        /sys/devices/system/cpu/cpufreq/policy[0-9]*/scaling_governor
      ;;
    on|start) turn_on ;;
    off|stop) turn_off ;;
  esac
  shift
done

# vim: tw=0 si sw=2
