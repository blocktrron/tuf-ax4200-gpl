/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Configuration for MediaTek MT7986 SoC
 *
 * Copyright (C) 2020 MediaTek Inc.
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#ifndef __MT7986_H
#define __MT7986_H

#include <linux/sizes.h>

#define CONFIG_SYS_MAXARGS		32
#define CONFIG_SYS_BOOTM_LEN		SZ_128M
#define CONFIG_SYS_CBSIZE		SZ_1K
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE +	\
					sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_NONCACHED_MEMORY	SZ_1M
#define CONFIG_SYS_MMC_ENV_DEV 0

/* Uboot definition */
#define CONFIG_SYS_UBOOT_BASE                   CONFIG_SYS_TEXT_BASE

/* SPL -> Uboot */
#define CONFIG_SYS_UBOOT_START		CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_TEXT_BASE + SZ_2M - \
					 GENERATED_GBL_DATA_SIZE)

/* Flash */
#define CONFIG_SYS_NAND_MAX_CHIPS	1

/* DRAM */
#define CONFIG_SYS_SDRAM_BASE		0x40000000

/* Ethernet */
#define CONFIG_IPADDR			192.168.1.1
#if !defined(CONFIG_ASUS_PRODUCT)
#define CONFIG_SERVERIP			192.168.1.2
#else
#define CONFIG_SERVERIP			192.168.1.70
#endif
#define CONFIG_NETMASK			255.255.255.0

#if defined(CONFIG_ASUS_PRODUCT)
#define CFG_SYS_FLASH_BASE	0xC0000000	/* define fake flash address. */
#define XMK_STR(x)	#x
#define MK_STR(x)	XMK_STR(x)

#ifndef __ASSEMBLY__
extern const char *model;
extern const char *blver;
extern int modifies;
#endif

///////////// Partition ///////////////
// 0x00000000 - 0x00100000  BL2
// 0x00100000 - 0x00180000  uboot-env
// 0x00180000 - 0x00400000  FIP(bl31+uboot)
// 0x00400000 - 0x00600000  Factory
// 0x00600000 - xxxxx       FW
///////////////////////////////////////

/*-----------------------------------------------------------------------
 * Bootloader size and Config size definitions
 */
#define CFG_MAX_BL2_BINARY_SIZE		0x100000
#define CFG_ENV_MAX_SIZE		0x80000
#define CFG_MAX_FIP_BINARY_SIZE		0x280000
#define CFG_BOOTLOADER_SIZE		(CFG_MAX_BL2_BINARY_SIZE + CFG_MAX_FIP_BINARY_SIZE)
#if CONFIG_ENV_OFFSET != 0x100000
#error check ENV offset!
#endif
#if CONFIG_ENV_SIZE > CFG_ENV_MAX_SIZE
#error check ENV size!
#endif

#define CFG_BL2_OFFSET			0x0
#define CFG_FIP_OFFSET			(CFG_BL2_OFFSET + CFG_MAX_BL2_BINARY_SIZE + CFG_ENV_MAX_SIZE)
/*
 * UBI volume size definitions
 * Don't define size for tailed reserved space due to it's size varies.
 */
#define PEB_SIZE			(128 * 1024)
#define LEB_SIZE			(PEB_SIZE - (2 * 2 * 1024))
#define CFG_UBI_NVRAM_NR_LEB		1
#define CFG_UBI_FACTORY_NR_LEB		8

#if 0 // bootcode v1000, OLD 128MB plan
#define CFG_UBI_FIRMWARE_NR_LEB		330	/* 124KB x 330 = 39.96MB */
#define CFG_UBI_FIRMWARE2_NR_LEB	330
#define CFG_UBI_APP_NR_LEB		287	/* 124KB x 287 ~= 34.75 MB. => cannot reach requested size due to UBIFS overhead. */
#else // bootcoee v1001+, new 256MB/128MB plan
#define CFG_UBI_OLD_FIRMWARE_LEB	330	/* for workaround judgement */
#define CFG_UBI_FIRMWARE_NR_LEB		578	/* 124KB x 578 ~= 69.99MB */
#define CFG_UBI_FIRMWARE2_NR_LEB	578	/* 124KB x 550 ~= 69.99MB */
#define CFG_UBI_APP_NR_LEB		799	/* 124KB x 799 ~= 96.75MB => jffs overhead ~= 87.5MB */
#endif

#define CFG_UBI_NVRAM_SIZE		(LEB_SIZE * CFG_UBI_NVRAM_NR_LEB)
#define CFG_UBI_FACTORY_SIZE		(LEB_SIZE * CFG_UBI_FACTORY_NR_LEB)
#define CFG_UBI_FACTORY2_SIZE		(LEB_SIZE * CFG_UBI_FACTORY_NR_LEB)
#define CFG_UBI_FIRMWARE_SIZE		(LEB_SIZE * CFG_UBI_FIRMWARE_NR_LEB)
#define CFG_UBI_FIRMWARE2_SIZE		(LEB_SIZE * CFG_UBI_FIRMWARE2_NR_LEB)
#define CFG_UBI_APP_SIZE		(LEB_SIZE * CFG_UBI_APP_NR_LEB)

#define CFG_NVRAM_SIZE			CFG_UBI_NVRAM_SIZE

#define CFG_FACTORY_SIZE		(CFG_UBI_FACTORY_SIZE + CFG_UBI_FACTORY2_SIZE)

#define CFG_UBI_DEV_OFFSET		(CFG_BOOTLOADER_SIZE + CFG_ENV_MAX_SIZE)

/* Environment address, factory address, and firmware address definitions */
/* Basically, CFG_FACTORY_ADDR and CFG_KERN_ADDR are used to compatible to original code infrastructure.
 * Real nvram area would be moved into the nvram volume of UBI device.
 * Real Factory area would be moved into the Factory volume of UBI device.
 * Real firmware area would be moved into the linux and linux2 volume of UBI device.
 */
#define CFG_NVRAM_ADDR			(CFG_SYS_FLASH_BASE + CFG_UBI_DEV_OFFSET)
#define CFG_FACTORY_ADDR		(CFG_NVRAM_ADDR + CFG_NVRAM_SIZE)
#define CFG_KERN_ADDR			(CFG_FACTORY_ADDR + CFG_FACTORY_SIZE)
#define CFG_KERN2_ADDR			(CFG_KERN_ADDR + CFG_UBI_FIRMWARE_SIZE)

/*-----------------------------------------------------------------------
 * Factory
 */
#define CFG_EEPROM_OFFSET	0x0
#define CFG_EEPROM_SIZE		0xA0000	/* 640KB*/
#define CFG_MAC_OFFSET		0x4	/* MAC address */

#define FTRY_PARM_SHIFT			(CFG_EEPROM_OFFSET + CFG_EEPROM_SIZE + 0x10000) /* EEPROM end + 64KB */
#define OFFSET_PIN_CODE			(FTRY_PARM_SHIFT + 0x180)	/* 8 bytes */
#define OFFSET_COUNTRY_CODE		(FTRY_PARM_SHIFT + 0x188)	/* 2 bytes */
#define OFFSET_BOOT_VER			(FTRY_PARM_SHIFT + 0x18A)	/* 4 bytes */
#define OFFSET_HWID			(FTRY_PARM_SHIFT + 0xFE00)	/* 4 bytes */

/*-----------------------------------------------------------------------*/

#define CONFIG_BOOTFILE		CONFIG_MODEL ".trx"
#define CFG_ETHADDR		00:aa:bb:cc:dd:e0

#define CFG_UBI_SUPPORT
#define CFG_FLASH_TYPE		"nand"
#define CFG_DUAL_TRX
#define CONFIG_EXTRA_ENV_SETTINGS	\
        "ethact=ethernet0@15100000\0"

#if defined(PANTHERB)
#define CFG_BLVER		"1001"
#elif defined(PANTHERA)
#define CFG_BLVER		"1001"
#elif defined(TUFAX4200)
#define CFG_BLVER		"1002"
#elif defined(TUFAX6000)
#define CFG_BLVER		"1002"
#elif defined(RTAX59U)
#define CFG_BLVER		"1003"
#endif
#endif // ASUS_PRODUCT

#endif
