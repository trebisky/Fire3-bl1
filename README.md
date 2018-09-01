This is a second stage boot loader for the NanoPi Fire3

This thing gets loaded to on-chip static ram at 0xffff0000 by the on chip
boot loader.  The on-chip loader can load it over USB, but I typically put
it onto an SD card via:

dd if=out/bl1.bin of=/dev/sdf bs=512 seek=1 conv=fdatasync

It can be built as two variants.  One is a USB variant that is what I
use for bare-metal development.  The other is an SD card variant, that
typically boots U-boot, reading from sector 129 on the SD card.

The USB variant is very handy.  With an SD card in the Fire3 and a proper
usb loader on the host side, you hit reset, type the command line to
load over USB and you are running your code.

The binary image fed to this over USB must have a proper NSIH style
header that indicates the load address and start address.
I typically use 0x40000000 for both, which is the start of DRAM.

