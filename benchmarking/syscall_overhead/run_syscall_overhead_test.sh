#!/bin/bash

set -e

LOG_WITH_PRELOAD="syscall_with_preload.log"
LOG_WITHOUT_PRELOAD="syscall_without_preload.log"
CMD="./syscall_overhead"

sudo LD_PRELOAD=../../build/injector/libinjector.so strace -o "$LOG_WITH_PRELOAD" $CMD
sudo strace -o "$LOG_WITHOUT_PRELOAD" $CMD
