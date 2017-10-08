## Boot loader for NanoPi M3

First stage boot loader, loaded by s5p6818 boot ROM. Loads u-boot.

To update bl1 on NanoPi it is recommended to use _nano-blembed_ utility from
[nanopi-boot-tools](https://github.com/rafaello7/nanopi-boot-tools).


### Compilation

A gcc cross-compiler is needed. I'm using gcc-linaro 6.x available on Ubuntu,
in _gcc-aarch64-linux-gnu_ package. To build, run _make_.  The _make_ command builds
_out/bl1-nanopi.bin_ binary. This is the _bl1_ binary for NanoPi.

### Boot process on NanoPi

Kernel loading on this board is performed in a few stages, namely:

 * S5P6818 boot ROM loads image, a first stage loader, to static RAM. The RAM is named "static" in opposite to "dynamic" RAM used as the main memory of the device. Dynamic RAM needs some initialization before use. Static RAM does not need any initialization. It is available immediately and it is the only memory available at startup. This board has only 64 kilobytes of static RAM. The static memory starts at address 0xffff0000 and ends at 0xffffffff. Boot ROM loads the first stage loader at address 0xffff0000.
 * First stage loader initializes main memory and then loads u-boot to main memory. The u-boot is loaded at address chosen by programmer. The 32-bit u-boot provided by FriendlyARM should be loaded at address 0x42c00000. On the other hand, u-boot for Samsung ARTIK (from which the [u-boot-nanopi-m3](https://github.com/rafaello7/u-boot-nanopi-m3) is forked) should be loaded at address 0x43c00000.
 * The u-boot loads Linux kernel and initrd image (initial ramdisk) to memory and boots the kernel. On arm64 platform also a _device tree_ image must be loaded. The _device tree_ image contains information about the SOC layout: what hardware it contains and how the hardware components are connected together. More information about device tree may be found at [elinux.org](http://elinux.org/Device_Tree_What_It_Is).

The images are normally loaded from SD card. But NanoPi may be also connected to PC USB port and all the images may be loaded using USB connection. The device must be plugged through micro-usb port, such used for powering the device. Images may be loaded using [nanopi-load](https://github.com/rafaello7/nanopi-load) tool.

The _nanopi-load_ tool needs _libusb-1.0_ library. To compile the tool on _Debian_ or _Ubuntu_, _libusb-1.0-0-dev_ and _pkg-config_ packages should be installed.


