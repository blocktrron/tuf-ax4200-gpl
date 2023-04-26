#*
# ****************************************************************
# *   Copyright 2022, ASUSTek COMPUTER Inc.  All Rights Reserved *
# ****************************************************************
#
# THIS SOFTWARE IS OFFERED "AS IS", AND ASUSTek GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  ASUSTek
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE
#

# O:	/PATH/TO/OUTPUT_DIRECTORY
# FW_FN:Firmware filename

ifeq ($(SRCBASE),)
	export SRCBASE := $(shell (cd ../../ && pwd))
endif

-include ../../.config
-include ../../../src/router/common.mak

ifeq ($(and $(O),$(FW_FN)),)
$(error O or FW_FN is not defined)
endif

ifeq ($(BUILD_NAME),CHEETAH)
UBOOT_VER = 1000
UBOOT_IMG = u-boot_nand_20220422_v$(UBOOT_VER)_cheetah_ddr3.img
HW_VER = 1.0
FLASH_SIZE = 0x8000000
else ifeq ($(BUILD_NAME),PANTHERA)
UBOOT_VER = 1001
UBOOT_IMG = u-boot_nand_20220817_v$(UBOOT_VER)_panthera_ddr4.img
HW_VER = 1.0
FLASH_SIZE = 0x8000000
else ifeq ($(BUILD_NAME),PANTHERB)
UBOOT_VER = 1001
UBOOT_IMG = u-boot_nand_20220817_v$(UBOOT_VER)_pantherb_ddr3.img
HW_VER = 1.0
FLASH_SIZE = 0x8000000
else ifeq ($(BUILD_NAME),TUF-AX4200)
UBOOT_VER = 1002
UBOOT_IMG = u-boot_nand_20221114_v$(UBOOT_VER)_tuf-ax4200_ddr3.img
HW_ID = A B
HW_VER = 1.0
FLASH_SIZE = 0x10000000
else ifeq ($(BUILD_NAME),TUF-AX6000)
UBOOT_VER = 1002
UBOOT_IMG = u-boot_nand_20221114_v$(UBOOT_VER)_tuf-ax6000_ddr4.img
#HW_ID = A B
HW_VER = 1.0
FLASH_SIZE = 0x10000000
else ifeq ($(BUILD_NAME),RT-AX59U)
UBOOT_VER = 1003
UBOOT_IMG = u-boot_nand_20221111_v$(UBOOT_VER)_rt-ax59u_ddr4.img
HW_ID = A
HW_VER = 1.0
FLASH_SIZE = 0x8000000
else
$(error Define your model!!)
endif

FW_FILE = $(O)/$(FW_FN)
BOOT_FILE = $(BUILD_NAME)_$(UBOOT_VER).U-BOOT.bin
BIN_FILE = $(addsuffix .FLASH,$(basename $(FW_FN)))
UBI_FILE = $(addsuffix .UBI_DEV.bin,$(basename $(FW_FN)))

$(info FW $(FW_FILE))
$(info BOOT $(BOOT_FILE))
$(info BIN $(BIN_FILE).bin)
$(info UBI $(UBI_FILE))

# Tools
MD		= ../modifyData
MK_FAC		= ../mk_factory_image 
DUMPIMAGE	= ../dumpimage

# Flash partition layout
# offset	length		name
# ==========================================================
# 0x00000000	0x003E0000	Bootloader (FIT Image)
# 0x003E0000	0x00020000	U-Boot environment. (hidden)
# 0x00400000	0x07C00000	UBI_DEV

BOOTLOADER	= $(BUILD_NAME).U-BOOT
BOOTLOADER_BASE	= 0x00000000
UBI_DEV		= UBI_DEV
UBI_DEV_BASE	= 0x00400000
UBI_START	= $(shell echo "$$(($(UBI_DEV_BASE))) /(128 * 1024)"|bc) # Start of UBI_DEV (block in flash)
FLASH_SIZE_DEC	= $(shell echo "ibase=16;$(shell echo $(FLASH_SIZE)|sed -e s,0x,,)"|bc)	# Flash size in decimal.
TRX		= $(FW_FILE)
BL2		= bl2.img
FIP		= fip.bin

$(info BOOTLOADER $(BOOTLOADER))

LEB_SIZE		= 124
NVRAM_SIZE_IN_LEBS	= 1
FACTORY_SIZE_IN_LEBS	= 8
LINUX_SIZE_IN_LEBS	= 578
LINUX2_SIZE_IN_LEBS	= 578
JFFS_SIZE_IN_LEBS	= 1

NVRAM_SIZE_KB	= $(shell expr $(NVRAM_SIZE_IN_LEBS) \* $(LEB_SIZE))
FACTORY_SIZE_KB	= $(shell expr $(FACTORY_SIZE_IN_LEBS) \* $(LEB_SIZE))
LINUX_SIZE_KB	= $(shell expr $(LINUX_SIZE_IN_LEBS) \* $(LEB_SIZE))
LINUX2_SIZE_KB	= $(shell expr $(LINUX2_SIZE_IN_LEBS) \* $(LEB_SIZE))
JFFS_SIZE_KB	= $(shell expr $(JFFS_SIZE_IN_LEBS) \* $(LEB_SIZE))

ONE_EEPROM_SET_SIZE_IN_LEBS	= $(FACTORY_SIZE_IN_LEBS)
ONE_EEPROM_SET_SIZE		= $(shell expr $(ONE_EEPROM_SET_SIZE_IN_LEBS) \* $(LEB_SIZE) \* 1024)
EEPROM_SET_PER_FACTORY		= $(shell expr $(FACTORY_SIZE_IN_LEBS) \/ $(ONE_EEPROM_SET_SIZE_IN_LEBS))

# Dummy Factory
FACTORY			= $(BUILD_NAME).FACTORY
FACTORY_POSTFIX		= $(addsuffix .postfix,$(FACTORY))
FACTORY_CSUM		= $(addsuffix .checksum,$(FACTORY))

# Print difference between file1 and file2
# $(1): file1
# $(2): file2
define compare_binary_file
	echo "$(1) v.s. $(2)"; \
	cmp -l $(1) $(2) | awk '{printf "%08X %02X %02X\n", $$1 - 1, strtonum(0$$2), strtonum(0$$3)}';
endef

all: tools clean image $(BUILD_NAME)_PartitionTable.mbn $(BUILD_NAME)_PartitionTable.def

tools:
	$(MAKE) -C ..

# Data I/O NAND Flash programmer, Minato NAND Flash programmer (e.g. M416)
$(BUILD_NAME)_PartitionTable.mbn: UDLEN=$(shell echo "(`stat -c %s $(UBI_DEV)` + 131072 - 1)/131072"|bc)
$(BUILD_NAME)_PartitionTable.mbn: LAST_BLK=$(shell echo "($(FLASH_SIZE_DEC) + 131072 - 1)/131072 - 1"|bc)
$(BUILD_NAME)_PartitionTable.mbn: BL2_LEN=$(shell echo "(`stat -c %s $(BL2)` + 131072 - 1)/131072"|bc)
$(BUILD_NAME)_PartitionTable.mbn: FIP_LEN=$(shell echo "(`stat -c %s $(FIP)` + 131072 - 1)/131072"|bc)
$(BUILD_NAME)_PartitionTable.mbn: image
	@dd if=/dev/zero bs=256 count=1 | sed -e "s,\x0,\xff,g" > $@
	@## BL2, offset 0, max length 0x100000
	@$(MD) $@  0 hex 00:00:00:00
	@$(MD) $@  4 hex 07:00:00:00
	@$(MD) $@  8 hex $(shell printf "%02X" `echo "$(BL2_LEN) % 256"|bc`):$(shell printf "%02X" `echo "$(BL2_LEN) / 256"|bc`):00:00
	@$(MD) $@ 12 hex 00:00:00:00
	@## empty env, skip 0x100000 - 0x180000 (08-0b), max length 0x80000
	@## FIP, offset 0x180000, max lenght 0x280000
	@$(MD) $@ 16 hex 0c:00:00:00
	@$(MD) $@ 20 hex 1f:00:00:00
	@$(MD) $@ 24 hex $(shell printf "%02X" `echo "$(FIP_LEN) % 256"|bc`):$(shell printf "%02X" `echo "$(FIP_LEN) / 256"|bc`):00:00
	@$(MD) $@ 28 hex 00:00:00:00
	@# Start block:		(32), first block of UBI_DEV partition,
	@# End block:		(128MB: 0x3FF, 256MB: 0x7FF), last block of UBI_DEV partition,
	@# Length:		UBI_DEV partition length (unit: block),
	@#	attribute:	skip-bad-block (1), up to 20 (0x14 << 16) bad-block accepted
	@$(MD) $@ 32 hex $(shell printf "%02X" `echo "$(UBI_START) % 256"|bc`):$(shell printf "%02X" `echo "$(UBI_START) / 256"|bc`):00:00
	@$(MD) $@ 36 hex $(shell printf "%02X" `echo "$(LAST_BLK) % 256"|bc`):$(shell printf "%02X" `echo "$(LAST_BLK) / 256"|bc`):00:00
	@$(MD) $@ 40 hex $(shell printf "%02X" `echo "$(UDLEN) % 256"|bc`):$(shell printf "%02X" `echo "$(UDLEN) / 256"|bc`):00:00
	@$(MD) $@ 44 hex 01:00:14:00
	@cp -f $@ $(O)/$@

# Leap Electronic NAND Flash programmer
$(BUILD_NAME)_PartitionTable.def: UDLEN=$(shell echo "(`stat -c %s $(UBI_DEV)` + 131072 - 1)/131072"|bc)
$(BUILD_NAME)_PartitionTable.def: LAST_BLK=$(shell echo "($(FLASH_SIZE_DEC) + 131072 - 1)/131072 - 1"|bc)
$(BUILD_NAME)_PartitionTable.def: BL2_LEN=$(shell echo "(`stat -c %s $(BL2)` + 131072 - 1)/131072"|bc)
$(BUILD_NAME)_PartitionTable.def: FIP_LEN=$(shell echo "(`stat -c %s $(FIP)` + 131072 - 1)/131072"|bc)
$(BUILD_NAME)_PartitionTable.def: image
	@dd if=/dev/zero bs=272 count=1 | sed -e "s,\x0,\xff,g" > $@
	@$(MD) $@  0 "GROUP DEFINE2"
	@$(MD) $@ 13 hex 00:00:00
	@## BL2, offset 0, max length 0x100000
	@$(MD) $@ 16 hex 01:00:00:00
	@$(MD) $@ 20 hex 00:00:00:00
	@$(MD) $@ 24 hex 07:00:00:00
	@$(MD) $@ 28 hex $(shell printf "%02X" `echo "$(BL2_LEN) % 256"|bc`):$(shell printf "%02X" `echo "$(BL2_LEN) / 256"|bc`):00:00
	@## empty env, skip 0x100000 - 0x180000 (08-0b), max length 0x80000
	@## FIP, offset 0x180000, max lenght 0x280000
	@$(MD) $@ 32 hex 01:00:00:00
	@$(MD) $@ 36 hex 0c:00:00:00
	@$(MD) $@ 40 hex 1f:00:00:00
	@$(MD) $@ 44 hex $(shell printf "%02X" `echo "$(FIP_LEN) % 256"|bc`):$(shell printf "%02X" `echo "$(FIP_LEN) / 256"|bc`):00:00
	@# Attribute:		01:00:00:00 (fix pattern, undocument)
	@# Start block:		(32), fist block of UBI_DEV partition,
	@# End block:		(128MB: 0x3FF, 256MB: 0x7FF), last block of UBI_DEV partition,
	@# Data length:		UBI_DEV partition length (unit: block)
	@$(MD) $@ 48 hex 01:00:00:00
	@$(MD) $@ 52 hex $(shell printf "%02X" `echo "$(UBI_START) % 256"|bc`):$(shell printf "%02X" `echo "$(UBI_START) / 256"|bc`):00:00
	@$(MD) $@ 56 hex $(shell printf "%02X" `echo "$(LAST_BLK) % 256"|bc`):$(shell printf "%02X" `echo "$(LAST_BLK) / 256"|bc`):00:00
	@$(MD) $@ 60 hex $(shell printf "%02X" `echo "$(UDLEN) % 256"|bc`):$(shell printf "%02X" `echo "$(UDLEN) / 256"|bc`):00:00
	@cp -f $@ $(O)/$@

define sed_FLASH
	sed	-e 's,@BOOTLOADER@,$(BOOTLOADER),' \
		-e 's,@BOOTLOADER_BASE@,$(BOOTLOADER_BASE),' \
		-e 's,@UBI_DEV@,$(1),' \
		-e 's,@UBI_DEV_BASE@,$(UBI_DEV_BASE),' \
		FLASH.lds.tmpl > FLASH.lds$(2);
endef

define ld_FLASH
	$(LD) -o $@$(2).o \
		-b binary $(BOOTLOADER) \
		-b binary $(1) \
		-T FLASH.lds$(2) -Map $(BUILD_NAME).map \
		-b elf64-littleaarch64;
endef

image: $(BOOTLOADER) $(UBI_DEV)
	@echo; echo "Prepare image $(BIN_FILE).bin"
ifeq ($(HW_ID),)
	@$(call sed_FLASH,$(UBI_DEV),)
	@$(call ld_FLASH,$(UBI_DEV),)
	@$(OBJCOPY) --gap-fill=0xff --pad-to $(FLASH_SIZE) -O binary $@.o $(BIN_FILE).bin
	@cp -f $(BIN_FILE).bin $(O)/$(BIN_FILE).bin
else
	@$(foreach id,$(HW_ID),$(call sed_FLASH,$(UBI_DEV)_HWID-$(id),_HWID-$(id)))
	@$(foreach id,$(HW_ID),$(call ld_FLASH,$(UBI_DEV)_HWID-$(id),_HWID-$(id)))
	@$(foreach id,$(HW_ID),$(OBJCOPY) --gap-fill=0xff --pad-to $(FLASH_SIZE) -O binary $@_HWID-$(id).o $(BIN_FILE)_HWID-$(id).bin;)
	@$(foreach id,$(HW_ID),cp -f $(BIN_FILE)_HWID-$(id).bin $(O)/$(BIN_FILE)_HWID-$(id).bin;)
endif

$(BOOTLOADER):
	@echo; echo "Prepare $@"
	@$(DUMPIMAGE) -T flat_dt -p 0 -o $(BL2) $(UBOOT_IMG)
	@$(DUMPIMAGE) -T flat_dt -p 1 -o $(FIP) $(UBOOT_IMG)
	@$(LD) -o $@.o \
		-b binary $(BL2) \
		-b binary $(FIP) \
		-T BOOT.lds -Map BOOT.map \
		-b elf64-littleaarch64
	@$(OBJCOPY) --gap-fill=0xff --pad-to $(UBI_DEV_BASE) -O binary $@.o $@

ifeq ($(FLASH_SIZE),0x10000000)
define sed_INI
	sed	-e 's,@NVRAM_SIZE@,$(NVRAM_SIZE_KB),' \
		-e 's,@FACTORY@,$(3),' \
		-e 's,@FACTORY_SIZE@,$(FACTORY_SIZE_KB),' \
		-e 's,@TRX@,$(TRX),' \
		-e 's,@LINUX_SIZE@,$(LINUX_SIZE_KB),' \
		-e 's,@LINUX2_SIZE@,$(LINUX2_SIZE_KB),' \
		-e 's,@JFFS_SIZE@,$(JFFS_SIZE_KB),' \
		$(1).ini.tmpl > $(2).ini;
endef
else
define sed_INI
	sed	-e 's,@NVRAM_SIZE@,$(NVRAM_SIZE_KB),' \
		-e 's,@FACTORY@,$(3),' \
		-e 's,@FACTORY_SIZE@,$(FACTORY_SIZE_KB),' \
		-e 's,@TRX@,$(TRX),' \
		-e 's,@LINUX_SIZE@,$(LINUX_SIZE_KB),' \
		-e 's,@JFFS_SIZE@,$(JFFS_SIZE_KB),' \
		$(1)_128M.ini.tmpl > $(2).ini;
endef
endif

$(UBI_DEV): $(FACTORY_CSUM)
	@echo; echo "Prepare $@"
	@$(call sed_INI,$@,$@,$<)
	@ubinize -o $@ --peb-size=128KiB --min-io-size=2048 \
		--sub-page-size=2048 --vid-hdr-offset=2048 -v $@.ini
ifneq ($(HW_ID),)
	@$(foreach id,$(HW_ID),$(call sed_INI,$@,$@_HWID-$(id),$<_HWID-$(id)))
	@$(foreach id,$(HW_ID),ubinize -o $@_HWID-$(id) --peb-size=128KiB --min-io-size=2048 \
		--sub-page-size=2048 --vid-hdr-offset=2048 -v $@_HWID-$(id).ini;)
endif

$(FACTORY_CSUM): $(FACTORY_POSTFIX)
	@echo; echo "Prepare $@"
	$(MK_FAC) -s 2048 -p 2048 -c $(EEPROM_SET_PER_FACTORY) -d $< -o $@
	@echo "Checksum result:"
	@$(call compare_binary_file,$@,$<)
ifneq ($(HW_ID),)
	@$(foreach id,$(HW_ID),$(MK_FAC) -s 2048 -p 2048 -c $(EEPROM_SET_PER_FACTORY) -d $<_HWID-$(id) -o $@_HWID-$(id);)
	@$(foreach id,$(HW_ID),$(call compare_binary_file,$@_HWID-$(id),$<_HWID-$(id)))
endif


# HwId offset 0xA0000+0x10000+0xFE00=0xBFE00
define set_HWID
	cp -f $(1) $(1)_HWID-$(2); \
	$(MD) "$(1)_HWID-$(2)" 0xBFE00 "$(2)";
endef

$(FACTORY_POSTFIX): $(FACTORY)
	@echo; echo "Prepare $@"
	@cp -f $< $@
	@# Replace data at offset 0xA0000+0x10000+0x188=0xB0188 to "DB100X".
	@# It should be at offset 0xB0188 of source file. The 100X is equal to UBOOT_VER.
	# Set BootVersion
	$(MD) $@ 0xB018A "$(UBOOT_VER)"
	# Set 2G MAC address
	$(MD) $@ 0x4 hex 00:AA:BB:CC:DD:E0
	@# Set PINCode as '12345670'
	@#$(MD) $@ 0xB0180 "12345670"
	# HwVersion offset 0xA0000+0x10000+0xFE04=0xBFE04
	$(MD) $@ 0xBFE04 "$(HW_VER)"
ifneq ($(HW_ID),)
	@$(foreach id,$(HW_ID),$(call set_HWID,$@,$(id)))
	@echo "Post fix result:"
	@$(foreach id,$(HW_ID),$(call compare_binary_file,$<,$@_HWID-$(id)))
endif
	@echo "Post fix result:"
	@$(call compare_binary_file,$<,$@)

$(FACTORY):
	@echo; echo "Prepare dummy $@"
	@dd if=/dev/zero bs=$(ONE_EEPROM_SET_SIZE) count=1 | sed -e "s,\x0,\xff,g" > $@

clean:
	@-$(RM) -f $(BL2) $(FIP) *.map FLASH.lds *.o *.ini *.postfix $(BIN_FILE).bin $(BIN_FILE)_*.bin $(UBI_FILE) $(UBI_FILE)_* $(BOOT_FILE) $(RAW_BIN_FILE) $(RAW_UBI_FILE) *.ecc_log *.mbn *.mbne *.def *.defe ak_sbl ak_linux

.PHONY: all tools clean image $(BUILD_NAME)_PartitionTable.mbn $(BOOTLOADER) $(UBI_DEV) $(FACTORY) $(FACTORY_POSTFIX) $(FACTORY_CSUM)
