#!/bin/bash

udisksctl unmount --block-device ${NONREFLINK_LOOP}
udisksctl unmount --block-device ${REFLINK_LOOP}
udisksctl loop-delete --block-device ${NONREFLINK_LOOP}
udisksctl loop-delete --block-device ${REFLINK_LOOP}
