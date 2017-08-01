# bl1-nanopi-m3

## Boot loader for NanoPi M3 board from FriendlyARM

Kernel loading on this board is performed in a few stages, namely:

 * S5P6818 boot ROM loads image, named first stage loader, to static RAM. The RAM is named "static" in opposite to "dynamic" RAM used as the main memory of the device. Dynamic RAM needs some initialization before use. Static RAM does not need any initialization. It is available immediately and it is the only memory available at startup. This board has only 64 kilobytes of static RAM. The static memory starts at address 0xffff0000 and ends at 0xffffffff. Boot ROM loads the first stage loader at address 0xffff0000.
 * First stage loader initializes main memory and then loads u-boot to main memory. The u-boot is loaded at address chosen by programmer. The 32-bit u-boot provided by FriendlyARM should be loaded at address 0x42c00000. On the other hand, u-boot for Samsung ARTIK (from which the [u-boot-nanopi-m3](https://github.com/rafaello7/u-boot-nanopi-m3) is forked) should be loaded at address 0x43c00000.
 * The u-boot loads Linux kernel and initrd image (initial ramdisk) to memory and boots the kernel. On arm64 platform also a _device tree_ image must be loaded. The _device tree_ image contains information about the SOC layout: what hardware it contains and how the hardware components are connected together. More information about device tree may be found at [elinux.org](http://elinux.org/Device_Tree_What_It_Is).

The images are normally loaded from SD card. But NanoPi may be also connected to PC USB port and all the images may be loaded using USB connection. The device must be plugged through micro-usb port, such used for powering the device. Images may be loaded using [nanopi-load](https://github.com/rafaello7/nanopi-load) tool.

The _nanopi-load_ tool needs _libusb-1.0_ library. To compile the tool on _Debian_ or _Ubuntu_, _libusb-1.0-0-dev_ and _pkg-config_ packages should be installed.

## Compilation

To build the bl1 loader and u-boot, gcc 4.x cross-compiler is needed. I'm using [gcc-linaro 4.9.4](http://releases.linaro.org/components/toolchain/gcc-linaro/4.9-2017.01/).

For Linux compilation I'm using gcc-linaro 6.x, but maybe the 4.x compiler will also work. _Ubuntu_ has _gcc-aarch64-linux-gnu_ package which contains some recent cross-compiler from Linaro, currently 6.3.0.

### Bl1 compilation

To build _bl1_ boot loader, the following command below should be invoked in _bl1_ source directory:

        make CROSS_TOOL_TOP=/path/to/your/gcc-linaro/bin/

The command builds _out/bl1-drone.bin_ binary. This is the _bl1_ binary for NanoPi.

### U-boot compilation

Like Linux on arm64, the _u-boot_ uses _device tree_, which must be built using _dtc_ script located in Linux kernel source tree. Path to the script must be on $PATH or the path must be provided in _make_ command line.

The u-boot performs booting according to their configuration specified in environment variables. Environment variables may control several aspects of u-boot behavior. For booting process two environment variables are important: _bootcmd_ and _bootargs_. The _bootcmd_ variable instructs u-boot how to boot Linux. The _bootargs_ variable contains Linux command line parameters.

The environment variables may be provided to u-boot in two ways. Namely, it is possible to specify some defaults in u-boot configuration header file, _include/configs/s5p6818\_nanopim3.h_. The environment may be also embedded in SD card. Embedding environment in SD card needs a special tool. I think it is easier to modify configuration header to set proper environment. Environment must be modified, because with current defaults NanoPi u-boot will not boot but will wait for environment on USB port.

So, before compilation please open _include/configs/s5p6818\_nanopim3.h_ file and modify two #define's: CONFIG\_BOOTCOMMAND and CONFIG\_BOOTARGS. The CONFIG\_BOOTARGS line may look like this:

	#define CONFIG_BOOTARGS \
		"initrd=0x49000000,0x400000 root=/dev/mmcblk1p1 console=tty1"

If you want to communicate with Linux also over serial console, the line should look like below:

	#define CONFIG_BOOTARGS \
		"console=ttySAC0,115200n8 initrd=0x49000000,0x400000 root=/dev/mmcblk1p1 console=tty1"

The CONFIG\_BOOTCOMMAND may look like below:

	#define CONFIG_BOOTCOMMAND	\
		"ext4load mmc 0:1 0x48000000 boot/Image; " \
		"mw 0x49000000 0 0x400000; " \
		"ext4load mmc 0:1 0x49000000 boot/initrd.img; " \
		"ext4load mmc 0:1 0x4a000000 boot/s5p6818-nanopi-m3.dtb; " \
		"booti 0x48000000 - 0x4a000000"

When finished, the u-boot may be complied. Go to _u-boot_ main directory and invoke:

        make s5p6818_nanopim3_defconfig
        make ARCH=arm CROSS_COMPILE=/path/to/your/gcc-linaro/bin/aarch64-linux-gnu- DTC=/path/to/linux-nanopi-m3/scripts/dtc/dtc

If the _dtc_ script is on $PATH, the _DTC=..._ part may be omitted.

### Linux compilation

To build kernel, ensure that aarch64-linux-gnu-gcc is on $PATH, then invoke:

        cd linux-nanopi-m3
        make nanopim3_defconfig
        make

If the PC has a multi-core processor, _make_ command may be invoked with _-j 4_ or even with _-j 8_ option to speed up compilation.

## SD card preparation

### SD card formatting 

The _fdisk_ command or similar tool may be used to prepare SD card with _ext4_ partition for Linux system. It is important that first partition should start at sector at least 2048. Sectors 2 .. 2047 are used for emedding _bl1_ and _u-boot_. Using _fdisk_ command the card may be prepared by invoke "o" command, then "n", then accept defaults. Finally "w" command should be invokded to store changes on SD card.

### Embedding bl1

_Bl1_ loader must be embedded in SD card starting at sector 2. On Linux system _dd_ command may be used. Assume the SD card appears as _/dev/sdX_. The _bl1-drone.bin_ binary may be embedded as follows:

        dd if=bl1-drone.bin of=/dev/sdX seek=1

### Embedding u-boot

The _u-boot_ must have NSIH header added before embed. This may be done using _nanopi-load_ tool, as follows:

        nanopi-load -o u-boot-nsih.bin u-boot.bin 0x43bffe00

The _u-boot-nsih.bin_ should be embedded in the SD card at 32kB offset, i.e. from sector 64. With _dd_ command, it may be embedded as follows:

        dd if=u-boot-nsih.bin of=/dev/sdX seek=64

### Debian bootstrap

Next step is ext4 filesystem creation on the newly created partition. Invoke:

        mkfs -t ext4 /dev/sdX1

Be careful to choose the proper drive.

Debian system may be bootstraped using _debootstrap_ command. The command is available on Debian and Ubuntu in _debootstrap_ package. Assume the newly created _ext4_ partition is mounted in _/mnt_ directory. Invoke:

        debootstrap --arch=arm64 --foreign --include=net-tools,initramfs-tools stretch /mnt

This command bootstraps Debian _stretch_, but any other Debian release may be chosen. It can be also some _Ubuntu_ release, e.g. _zesty_ from [ports.ubuntu.com](http://ports.ubuntu.com/ubuntu-ports/). In this case an additional argument shall be added to command line, the host - [http://ports.ubuntu.com/ubuntu-ports/]()

When bootstrap finishes, the compiled kernel image and device tree blob should be copied to _/mnt/boot_ directory. More precisely, these files should be copied to _/boot_:

        arch/arm64/boot/Image
        arch/arm64/boot/dts/nexell/s5p6818-nanopi-m3.dtb

These files will be loaded by u-boot. Also _/etc/fstab_ file (_/mnt/etc/fstab_) should be updated. It is leaved empty by debootstrap. The file should contain at least root partition:

        # <file system> <dir>   <type>  <options>   <dump>  <pass>
        /dev/mmcblk1p1  /       ext4    rw,noatime  0       0

Optionally also host name in _/etc/hostname_ may be set now. But this can be also done later, at any time.

The u-boot loads also _initrd_ image. There is a problem with the initrd image creation, because it may be created only from the running target Debian system. It is easy when Debian is bootstraped on another Linux running on arm64 machine. In such case _--foreign_ flag in _debootrstap_ may be omitted and initrd may be generated after chroot to /mnt by invoke _update-initramfs_ command on the chroot-ed environment. But if another arm64 machine is not available, the first run of new Debian must be made using some hand-made initrd.

To prepare such hand-made initrd, statically linked _busybox_ binary for arm64 is needed. To obtain such _busybox_ binary, open [busybox package download](https://packages.debian.org/stretch/arm64/busybox-static/download) page on Debian packages site, choose mirror nearest to you and download the busybox-static "_deb_" package. The package contents may be extracted using _dpkg-deb_ command with _-x_ option, e.g.:

        dpkg-deb -x busybox-static_1.22.0-19+b3_arm64.deb

The busybox binary should be located in _bin_ subdirectory.

Next, root filesystem for the hand-made initrd must be prepared. For this purpose create a new directory and create the following subdirectories in it: _bin_, _dev_, _mnt_, _proc_. Copy the extracted busybox binary to bin directory. Go to the _bin_ directory and invoke:

        ln -s busybox sh
        ln -s busybox mount
        ln -s busybox chroot
        ln -s busybox umount
        ln -s busybox reboot

Go back to parent directory and create init script - a script file named _init_. The script will be invoked by Linux kernel as the first program after boot. The script may be quite simple, for example:

        #!/bin/sh
        mount -t proc none /proc
        mount -t devtmpfs none /dev
        /bin/sh
        echo "Rebooting..."
        reboot -f

Don't forget to set the script execution permissions. Now the initrd image may be created by invoke from the prepared filesystem main directory:

        find . -print | cpio -R 0.0 -o -H newc | gzip >../initrd.img

To verify the created initrd image is correct, invoke the following command:

        gzip -d -c initrd.img | cpio -tv

Output similar to the following should appear:

        drwxrwxr-x   6 root     root            0 Jul  6 18:06 .
        drwxrwxr-x   2 root     root            0 Jul  6 18:06 mnt
        -rwxrwxr-x   1 root     root          283 Jul  6 18:06 init
        drwxrwxr-x   2 root     root            0 Jul  6 18:06 proc
        drwxrwxr-x   2 root     root            0 Jul  6 18:06 dev
        drwxrwxr-x   2 root     root            0 Jul  6 18:08 bin
        lrwxrwxrwx   1 root     root            7 Jul  6 18:08 bin/umount -> busybox
        -rwxr-xr-x   1 root     root      1553912 Jul  6 18:07 bin/busybox
        lrwxrwxrwx   1 root     root            7 Jul  6 18:08 bin/reboot -> busybox
        lrwxrwxrwx   1 root     root            7 Jul  6 18:08 bin/sh -> busybox
        lrwxrwxrwx   1 root     root            7 Jul  6 18:08 bin/chroot -> busybox
        lrwxrwxrwx   1 root     root            7 Jul  6 18:08 bin/mount -> busybox
        3039 blocks

The created _initrd.img_ file should be copied to _/boot_ directory on SD card (_/mnt/boot_). Now the SD card may be unmounted and put into SD slot on NanoPi board.


## First boot

If everything goes well, shell prompt should appear after boot. Mount the ext4 partition, chroot into the munted partition and finish bootstrap:

        mount -t ext4 /dev/mmcblk1p1 /mnt
        chroot /mnt
        /debootstrap/debootstrap --second-stage

The command execution will take a few minutes. When the command finishes, the right initrd image may be created:

        update-initramfs -c -k 4.11.6+

Errors from depmod may be ignored. When command finishes, go to /boot, remove or rename the _initrd.img_ file and create symbolic link to the generated initrd instead:

        mv initrd.img initrd.img.bak
        ln -s initrd.img-4.11.6+ initrd.img

Set also password for root, otherwise you will not be able to access the new system:

        passwd

Exit from the chroot shell, umount the mounted partition and reboot:

        umount /mnt
        reboot -f

## Next steps

### Ethernet configuration

To set up ethernet, go to /etc/network/interfaces.d/ directory and create file _eth0_ with the following contents:

        auto eth0
        iface eth0 inet dhcp

To make ethernet available immediately, invoke:

        ifup eth0

Otherwise ethernet will be available only after reboot.

### Adding usual user

Starting desktop environment as root is highly discouraged. To create an usual user, _adduser_ command should be invoked with single parameter - the user name.

### Desktop environment installation

I like _xfce_. It is lightweight and powerful. But any desktop environment may be chosen. The command below installs _xfce_ desktop environment:

        apt install xfce4 xfce4-goodies

This command installs a bunch of packages, including X server and X display manager. Other desktop environmens have similar metapackages.


### Time zone, locale, etc.

Debian installer configures a lot of things, including time zone, locale etc. Most of these things may be configured using _dpkg-reconfigure_ command with appropriate package name. The time zone may be set by invoke:

        dpkg-reconfigure tzdata

To configure locale, _locales_ package should be installed.


### Wireless network setup

There are two drivers working with the wifi chip. First one is native _brcmfmac_ driver. Second one is _bcmdhd_ driver ported from 3.x kernel. The second driver was ported by me because of some problems encountered by me with the native driver, but now the _brcmfmac_ driver seems to work well. Only one driver should be loaded, second one should be disabled (blacklist-ed) or not built at all.

Both drivers need firmware. Linux image provided by FriendlyArm has the firmware in _/lib/firmware/ap6212_ directory. The best way is to copy the whole directory to the new board, exactly at the same location. After copy and load bcmdhd module the _wlan0_ device should appear. On the other hand, brcmfmac driver expects firmware in _/lib/firmware/brcm_ directory. For this driver symbolic links may be created, like below:

		lrwxrwxrwx 1 root root 27 Jul  7 16:15 brcmfmac43430-sdio.bin -> ../ap6212/fw_bcm43438a0.bin
		lrwxrwxrwx 1 root root 26 Jul  7 16:18 brcmfmac43430-sdio.txt -> ../ap6212/nvram_ap6212.txt


The _wlan0_ device should become visible after _brcmfmac_ module load.


### VPU - Video Processing Unit


The board has also video processing unit (coda960) which allows to play video files with little CPU consumption. To utilize the VPU some userland tools are needed. Samsung Artik provides on github some GStreamer plugins which allow to play the video. They need some Nexell libraries whose are also available on github in the Samsung ARTIK repositories.


Enjoy ;)

- - -

## Alternative: booting from USB


This method is useful for development. Embedding of a new binary on SD card after every smallest change is cumbersome.

As mentioned, for this method NanoPi needs to be connected to PC's through micro-usb port - such used for powering the device. All images may be uploaded using [nanopi-load](https://github.com/rafaello7/nanopi-load) tool.

### Uploading bl1

The boot ROM first attempts to load bl1 image from SD card. When the image cannot be loaded, boot ROM attempts to load it from USB. To upload the image, invoke the following command:

        nanopi-load -f bl1-drone.bin

The _-f_ option fixes image length in _NSIH_ header of the image.

### Uploading u-boot


If the u-boot has to be uploaded on USB instead of loading from SD card, the _bl1_ image should be adjusted accordingly and recompiled. Namely: go to _bl1-nanopi-m3_ directory, open file _src/startup_aarch64.S_ and go to line containing comment: <code>_// 0x054 ..._</code> The line contains informaton, from which source the u-boot image should be loaded. To load _u-boot_ image from SD card, set value in this line to _0x03000000_. To load it from USB, set the value _0x00000000_

Using the modified bl1 image u-boot may be uploaded as follows:

        nanopi-load u-boot.bin 0x43bffe00

This command uploads u-boot.bin image to address 0x43c00000. After load, the _bl1_ loader runs _u-boot_ by jump to address 0x43c00000.

The numeric value provided to _nanopi-load_ command is less by 512 (0x200) than actual load address. This is because additional 512-byte header is uploaded before image, named "NSIH" header. This header provides information about image load address, image size and jump address. _Bl1_ loader interprets load address as location of first byte of the _NSIH_ header. Hence the 512 value must be subtracted.

Actually the _nanopi-load_ command may be invoked with two numeric arguments: the first one is image load address, second one - image start address. If the second argument is omitted, it defaults to the load address + 0x200.

### Uploading kernel and initrd

The u-boot for NanoPi has _udown_ command, which can be used to upload any file to the device memory. The _udown_ command has one argument - memory location. When the command is invoked, it "hangs", waiting for data from usb. The data may be upoaded using _nanopi-load_ tool. For example, to load kernel at address 0x48000000, do the following. On u-boot prompt, invoke:

        udown 0x48000000

Next, invoke on PC:

        nanopi-load /path/to/Image 0

The "0" value is the image load address, but this value is ignored by u-boot. Image is loaded at address provided as the udown command argument.


Enjoy!

