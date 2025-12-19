#!/bin/bash
grep -q :memory: /proc/$$/cgroup || exit 0
cgroup=$(grep :memory: /proc/$$/cgroup | cut -d: -f3)
sysfs=/sys/fs/cgroup/memory${cgroup%/*}
#ls $sysfs
limit=$(<$sysfs/memory.limit_in_bytes)
usage=$sysfs/memory.usage_in_bytes
while ((limit - $(<$usage) < 6*1024*1024*1024)); do
  sleep 20
done
