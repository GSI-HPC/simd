#!/bin/bash
grep -q :memory: /proc/$$/cgroup || exit 0
[[ -z "$1" ]] && exit 0 # single job => unconditionally start
max_jobs=$1
cgroup=$(grep :memory: /proc/$$/cgroup | cut -d: -f3)
sysfs=/sys/fs/cgroup/memory${cgroup%/*}
limit=$(<$sysfs/memory.limit_in_bytes)
usage=$sysfs/memory.usage_in_bytes

while true; do
  running=$(ps h -C cc1plus|wc -l)
  avail=$(((limit - $(<$usage)) >> 30))
  atleast=$(((3 * (max_jobs - running)) >> 1))
  ((avail >= atleast || 4 * running < max_jobs)) && exit 0
  echo "Waiting for memory (at least $atleast GiB). Avail: $avail GiB"
  sleep 20
done
