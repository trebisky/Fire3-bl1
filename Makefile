 # Copyright (C) 2016  Nexell Co., Ltd.
 # Author: Sangjong, Han <hans@nexell.co.kr>
 #
 # This program is free software; you can redistribute it and/or
 # modify it under the terms of the GNU General Public License
 #
 # as published by the Free Software Foundation; either version 2
 # of the License, or (at your option) any later version.
 #
 # This program is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License
 # along with this program.  If not, see <http://www.gnu.org/licenses/>.

include config.mak

LDFLAGS		=	-Bstatic							\
			-T$(LDS_NAME).lds						\
			-Wl,--start-group						\
			-Lsrc/$(DIR_OBJOUTPUT)						\
			-Wl,--end-group							\
			-Wl,--build-id=none						\
			-nostdlib

# for .map file
#		-Wl,-Map=bl1.map,--cref		\

SYS_INCLUDES	=	-I src -I prototype/base -I prototype/module


#OBJS	=	startup_$(OPMODE).o $(OPMODE)_libs.o $(OPMODE)_exception_handler.o secondboot.o subcpu.o sleep.o	\

OBJS	=	$(OPMODE)_libs.o $(OPMODE)_exception_handler.o secondboot.o subcpu.o sleep.o	\
			resetcon.o GPIO.o CRC32.o	SecureManager.o							\
			clockinit.o debug.o lib2ndboot.o buildinfo.o							\
			printf.o psci.o sysbus.o

ifeq ($(MEMTYPE),DDR3)
OBJS	+=	init_DDR3.o
endif

ifeq ($(MEMTYPE),LPDDR3)
OBJS	+=	init_LPDDR3.o
endif

OBJS	+=	CRYPTO.o
#OBJS	+=	nx_tieoff.o

ifeq ($(INITPMIC),YES)
OBJS	+=	i2c_gpio.o pmic.o
endif

ifeq ($(SUPPORT_USB_BOOT),y)
CFLAGS		+= -DSUPPORT_USB_BOOT
OBJS	+=	iUSBBOOT.o
endif

ifeq ($(SUPPORT_SDMMC_BOOT),y)
CFLAGS		+= -DSUPPORT_SDMMC_BOOT
OBJS	+=	iSDHCBOOT.o
endif

ifeq ($(MEMTEST),y)
OBJS	+=	memtester.o
endif

USB_OBJS	= startup_usb.o $(OBJS)
SD_OBJS		= startup_sd.o $(OBJS)

USB_OBJS_LIST	=	$(addprefix $(DIR_OBJOUTPUT)/,$(USB_OBJS))
SD_OBJS_LIST	=	$(addprefix $(DIR_OBJOUTPUT)/,$(SD_OBJS))

###################################################################################################

#all: mkobjdir $(SYS_OBJS_LIST) link bin

all: bl1-usb.bin bl1-sd.bin

install: bl1-usb.bin
	dd if=bl1-usb.bin of=/dev/sdf bs=512 seek=1 conv=fdatasync

sdcard: bl1-usb.bin
	dd if=bl1-sd.bin of=/dev/sdf bs=512 seek=1 conv=fdatasync

usb: bl1-usb.bin
	usb_loader bl1-usb.bin

###################################################################################################

$(DIR_OBJOUTPUT)/startup_sd.o: src/startup_$(OPMODE).S
	@echo [compile....$<]
	$(Q)$(CC) $< -c -o $@ $(ASFLAG) $(CFLAGS) $(SYS_INCLUDES)

$(DIR_OBJOUTPUT)/startup_usb.o: src/startup_$(OPMODE).S
	@echo [compile....$<]
	$(Q)$(CC) $< -c -o $@ $(ASFLAG) $(CFLAGS) -DBOOT_USB $(SYS_INCLUDES)

$(DIR_OBJOUTPUT)/%.o: src/%.c
	@echo [compile....$<]
	$(Q)$(CC) $< -c -o $@ $(CFLAGS) $(SYS_INCLUDES)
	$(Q)##$(CC) -MMD $< -c -o $@ $(CFLAGS) $(SYS_INCLUDES)

$(DIR_OBJOUTPUT)/%.o: src/%.S
	@echo [compile....$<]
	$(Q)$(CC) $< -c -o $@ $(ASFLAG) $(CFLAGS) $(SYS_INCLUDES)
	$(Q)##$(CC) -MMD $< -c -o $@ $(ASFLAG) $(CFLAGS) $(SYS_INCLUDES)

###################################################################################################


#mkobjdir:
##	@if [ ! -L prototype ] ; then			\
##		ln -s ../../../prototype/s5p6818/ prototype ; \
#	fi
#	@if	[ ! -e $(DIR_OBJOUTPUT) ]; then 	\
#		$(MKDIR) $(DIR_OBJOUTPUT);		\
#	fi;
#	@if	[ ! -e $(DIR_TARGETOUTPUT) ]; then 	\
#		$(MKDIR) $(DIR_TARGETOUTPUT);		\
#	fi;

#link:
#	@echo [link.... $(DIR_TARGETOUTPUT)/$(TARGET_NAME).elf]
#
#	$(Q)$(CC) $(SYS_OBJS_LIST) $(LDFLAGS) -o $(DIR_TARGETOUTPUT)/$(TARGET_NAME).elf
#
#bin:
#	@echo [binary.... $(DIR_TARGETOUTPUT)/$(TARGET_NAME).bin]
#	$(Q)$(MAKEBIN) -O binary $(DIR_TARGETOUTPUT)/$(TARGET_NAME).elf $(DIR_TARGETOUTPUT)/$(TARGET_NAME).bin
#	@if	[ -e $(DIR_OBJOUTPUT) ]; then 		\
#		$(RM) $(DIR_OBJOUTPUT)/buildinfo.o;	\
#	fi;

bl1-usb.elf: $(USB_OBJS_LIST)
	@echo [link.... $@]
	@$(CC) $(USB_OBJS_LIST) $(LDFLAGS) -o bl1-usb.elf

bl1-usb.bin:	bl1-usb.elf
	$(MAKEBIN) -O binary bl1-usb.elf bl1-usb.bin

bl1-sd.elf: $(SD_OBJS_LIST)
	@echo [link.... $@]
	@$(CC) $(SD_OBJS_LIST) $(LDFLAGS) -o bl1-sd.elf

bl1-sd.bin:	bl1-sd.elf
	$(MAKEBIN) -O binary bl1-sd.elf bl1-sd.bin

###################################################################################################
clean:
	@if [ -L prototype ] ; then			\
		$(RM) prototype ;			\
	fi
	rm -f obj/*
	rm -f *.elf *.bin

#	@if	[ -e $(DIR_OBJOUTPUT) ]; then 		\
#		$(RMDIR) $(DIR_OBJOUTPUT);		\
#	fi;
#	@if	[ -e $(DIR_TARGETOUTPUT) ]; then 	\
#		$(RMDIR) $(DIR_TARGETOUTPUT);		\
#	fi;

#-include $(SYS_OBJS_LIST:.o=.d)

# THE END
