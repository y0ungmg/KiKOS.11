#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
make -j"$(nproc)" all
echo
echo "KiKOS.11 image ready: build/kikos.img"
echo "  run it      : ./run.sh"
echo "  write to USB: sudo dd if=build/kikos.img of=/dev/sdX bs=4M status=progress"
echo
exec qemu-system-i386 -m 256 -vga std -drive file=build/kikos.img,format=raw,if=ide "$@"
