#!/bin/bash

fail() {
  echo "$*"
  exit 1
}

[[ "${BASH_SOURCE[0]}" != "${0}" ]] || fail "Please use source (or .) to run this script"

eval $({

fail() {
  echo "$*"
  exit 1
}

dd if=/dev/zero of=/tmp/copy_file_test_ext3 bs=512 count=10240
dd if=/dev/zero of=/tmp/copy_file_test_btrfs bs=512 count=51200
chmod 0666 /tmp/copy_file_test_ext3
chmod 0666 /tmp/copy_file_test_btrfs
mkfs.ext3 -E root_owner /tmp/copy_file_test_ext3
cat > /tmp/proto <<EOF
/dummy
1 2
d--770 $(id -u) $(id -g)
$
EOF
mkfs.xfs -p /tmp/proto -m reflink=1 /tmp/copy_file_test_btrfs

set -o pipefail

EXT3_LOOP=$(udisksctl loop-setup --file /tmp/copy_file_test_ext3 | egrep -o '/dev/loop[0-9]+')
[[ $? -eq 0 ]] || fail "Could not set up loop device"
BTRFS_LOOP=$(udisksctl loop-setup --file /tmp/copy_file_test_btrfs | egrep -o '/dev/loop[0-9]+')
[[ $? -eq 0 ]] || fail "Could not set up loop device"
EXT3_MOUNT=$(udisksctl mount --block-device ${EXT3_LOOP} | egrep -o '/media/[^.]+')
[[ $? -eq 0 ]] || fail "Could not mount loop device"
BTRFS_MOUNT=$(udisksctl mount --block-device ${BTRFS_LOOP} | egrep -o '/media/[^.]+')
[[ $? -eq 0 ]] || fail "Could not mount loop device"

echo "export EXT3_LOOP=$EXT3_LOOP BTRFS_LOOP=$BTRFS_LOOP EXT3_MOUNT=$EXT3_MOUNT BTRFS_MOUNT=$BTRFS_MOUNT"

} | egrep '^export ' )
