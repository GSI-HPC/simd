#!/bin/zsh
[[ -z "$1" ]] && exit 0 # single job => unconditionally start
max_jobs=$1

if grep -q :memory: /proc/$$/cgroup; then
  cgroup=$(grep :memory: /proc/$$/cgroup | cut -d: -f3)
  sysfs=/sys/fs/cgroup/memory${cgroup%/*}
  limit=$(<$sysfs/memory.limit_in_bytes)
  buffer=$((limit >> 33))
  usage=$sysfs/memory.usage_in_bytes
  set_avail() {
    avail=$(((limit - $(<$usage)) >> 30))
  }
else
  grep MemTotal /proc/meminfo|read -A foo
  buffer=$((${foo[2]} >> 23))
  set_avail() {
    grep MemAvail /proc/meminfo|read -A foo
    grep SwapFree /proc/meminfo|read -A swap
    avail=$(((${foo[2]} - ${swap[2]}) >> 20))
  }
fi

((buffer < 4)) && buffer=4

while true; do
  running=$(ps h -C cc1plus|wc -l)
  set_avail
  atleast=$(((3 * (max_jobs - running)) >> 1))
  ((buffer > atleast)) && atleast=$buffer
  if ((avail >= atleast || 4 * running < max_jobs)); then
    #echo "Good to go. Avail: $avail GiB (at least $atleast GiB, buffer: $buffer)"
    exit 0
  else
    echo "Waiting for memory (at least $atleast GiB). Avail: $avail GiB"
    sleep $((10 + ($max_jobs >> 4)))
  fi
done
