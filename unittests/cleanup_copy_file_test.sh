#!/bin/bash

udisksctl unmount --block-device ${EXT3_LOOP}
udisksctl unmount --block-device ${BTRFS_LOOP}
udisksctl loop-delete --block-device ${EXT3_LOOP}
udisksctl loop-delete --block-device ${BTRFS_LOOP}
