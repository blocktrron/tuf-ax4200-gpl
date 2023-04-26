// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2011 The Chromium OS Authors.
 * (C) Copyright 2002-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 */

#include <common.h>
#include <api.h>
#include <bootstage.h>
#include <cpu_func.h>
#include <exports.h>
#include <flash.h>
#include <hang.h>
#include <image.h>
#include <irq_func.h>
#include <log.h>
#include <net.h>
#include <asm/cache.h>
#include <asm/global_data.h>
#include <u-boot/crc.h>
#include <binman.h>
#include <command.h>
#include <console.h>
#include <dm.h>
#include <env.h>
#include <env_internal.h>
#include <fdtdec.h>
#include <ide.h>
#include <init.h>
#include <initcall.h>
#include <kgdb.h>
#include <irq_func.h>
#include <malloc.h>
#include <mapmem.h>
#include <miiphy.h>
#include <mmc.h>
#include <mux.h>
#include <nand.h>
#include <of_live.h>
#include <onenand_uboot.h>
#include <pvblock.h>
#include <scsi.h>
#include <serial.h>
#include <status_led.h>
#include <stdio_dev.h>
#include <timer.h>
#include <trace.h>
#include <watchdog.h>
#include <xen.h>
#include <asm/sections.h>
#include <dm/root.h>
#include <dm/ofnode.h>
#include <linux/compiler.h>
#include <linux/err.h>
#include <efi_loader.h>
#include <wdt.h>
#include <asm-generic/gpio.h>
#include <efi_loader.h>

#if defined(CONFIG_ASUS_PRODUCT)
#include <cli.h>
#include <version.h>
#include <flash_wrapper.h>
#include <gpio.h>
#include <replace.h>
#include <flash_wrapper.h>
#include <cmd_tftpServer.h>
#include <version_string.h>
#if defined(CFG_UBI_SUPPORT)
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <ubi_uboot.h>
#endif
#define BOOT_IMAGE_NAME "Boot Loader code"
#define SYS_IMAGE_NAME  "System code"
#if defined(CONFIG_PWM_MTK_MM)
void bbtype(int);
#endif
#endif

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_ASUS_PRODUCT)
extern int do_bootm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
extern int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
extern int do_tftpb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
extern int do_tftpd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
extern int do_source (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

#define BOOTFILENAME	"u-boot_" CFG_FLASH_TYPE ".img"

#define SEL_LOAD_LINUX_SDRAM			'1'
#define SEL_LOAD_LINUX_WRITE_FLASH		'2'
#define SEL_BOOT_FLASH				'3'
#define SEL_ENTER_CLI				'4'
#define SEL_LOAD_BOOT_WRITE_FLASH_BY_SERIAL	'7'
#define SEL_LOAD_BOOT_WRITE_FLASH		'9'

int modifies = 0;
static int main_loop_shortcut = 0;

#define ARGV_LEN	128
static char file_name_space[ARGV_LEN];
#endif	/* CONFIG_ASUS_PRODUCT */

ulong monitor_flash_len;

__weak int board_flash_wp_on(void)
{
	/*
	 * Most flashes can't be detected when write protection is enabled,
	 * so provide a way to let U-Boot gracefully ignore write protected
	 * devices.
	 */
	return 0;
}

__weak int cpu_secondary_init_r(void)
{
	return 0;
}

static int initr_trace(void)
{
#ifdef CONFIG_TRACE
	trace_init(gd->trace_buff, CONFIG_TRACE_BUFFER_SIZE);
#endif

	return 0;
}

static int initr_reloc(void)
{
	/* tell others: relocation done */
	gd->flags |= GD_FLG_RELOC | GD_FLG_FULL_MALLOC_INIT;

	return 0;
}

#if defined(CONFIG_ARM) || defined(CONFIG_RISCV)
/*
 * Some of these functions are needed purely because the functions they
 * call return void. If we change them to return 0, these stubs can go away.
 */
static int initr_caches(void)
{
	/* Enable caches */
	enable_caches();
	return 0;
}
#endif

__weak int fixup_cpu(void)
{
	return 0;
}

static int initr_reloc_global_data(void)
{
#ifdef __ARM__
	monitor_flash_len = _end - __image_copy_start;
#elif defined(CONFIG_NDS32) || defined(CONFIG_RISCV)
	monitor_flash_len = (ulong)&_end - (ulong)&_start;
#elif !defined(CONFIG_SANDBOX) && !defined(CONFIG_NIOS2)
	monitor_flash_len = (ulong)&__init_end - gd->relocaddr;
#endif
#if defined(CONFIG_MPC85xx) || defined(CONFIG_MPC86xx)
	/*
	 * The gd->cpu pointer is set to an address in flash before relocation.
	 * We need to update it to point to the same CPU entry in RAM.
	 * TODO: why not just add gd->reloc_ofs?
	 */
	gd->arch.cpu += gd->relocaddr - CONFIG_SYS_MONITOR_BASE;

	/*
	 * If we didn't know the cpu mask & # cores, we can save them of
	 * now rather than 'computing' them constantly
	 */
	fixup_cpu();
#endif
#ifdef CONFIG_SYS_RELOC_GD_ENV_ADDR
	/*
	 * Relocate the early env_addr pointer unless we know it is not inside
	 * the binary. Some systems need this and for the rest, it doesn't hurt.
	 */
	gd->env_addr += gd->reloc_off;
#endif
#ifdef CONFIG_OF_EMBED
	/*
	 * The fdt_blob needs to be moved to new relocation address
	 * incase of FDT blob is embedded with in image
	 */
	gd->fdt_blob += gd->reloc_off;
#endif
#ifdef CONFIG_EFI_LOADER
	/*
	 * On the ARM architecture gd is mapped to a fixed register (r9 or x18).
	 * As this register may be overwritten by an EFI payload we save it here
	 * and restore it on every callback entered.
	 */
	efi_save_gd();

	efi_runtime_relocate(gd->relocaddr, NULL);
#endif

	return 0;
}

__weak int arch_initr_trap(void)
{
	return 0;
}

#if defined(CONFIG_SYS_INIT_RAM_LOCK) && defined(CONFIG_E500)
static int initr_unlock_ram_in_cache(void)
{
	unlock_ram_in_cache();	/* it's time to unlock D-cache in e500 */
	return 0;
}
#endif

static int initr_barrier(void)
{
#ifdef CONFIG_PPC
	/* TODO: Can we not use dmb() macros for this? */
	asm("sync ; isync");
#endif
	return 0;
}

static int initr_malloc(void)
{
	ulong malloc_start;

#if CONFIG_VAL(SYS_MALLOC_F_LEN)
	debug("Pre-reloc malloc() used %#lx bytes (%ld KB)\n", gd->malloc_ptr,
	      gd->malloc_ptr / 1024);
#endif
	/* The malloc area is immediately below the monitor copy in DRAM */
	/*
	 * This value MUST match the value of gd->start_addr_sp in board_f.c:
	 * reserve_noncached().
	 */
	malloc_start = gd->relocaddr - TOTAL_MALLOC_LEN;
	mem_malloc_init((ulong)map_sysmem(malloc_start, TOTAL_MALLOC_LEN),
			TOTAL_MALLOC_LEN);
	return 0;
}

static int initr_of_live(void)
{
	if (CONFIG_IS_ENABLED(OF_LIVE)) {
		int ret;

		bootstage_start(BOOTSTAGE_ID_ACCUM_OF_LIVE, "of_live");
		ret = of_live_build(gd->fdt_blob,
				    (struct device_node **)gd_of_root_ptr());
		bootstage_accum(BOOTSTAGE_ID_ACCUM_OF_LIVE);
		if (ret)
			return ret;
	}

	return 0;
}

#ifdef CONFIG_DM
static int initr_dm(void)
{
	int ret;

	/* Save the pre-reloc driver model and start a new one */
	gd->dm_root_f = gd->dm_root;
	gd->dm_root = NULL;
#ifdef CONFIG_TIMER
	gd->timer = NULL;
#endif
	bootstage_start(BOOTSTAGE_ID_ACCUM_DM_R, "dm_r");
	ret = dm_init_and_scan(false);
	bootstage_accum(BOOTSTAGE_ID_ACCUM_DM_R);
	if (ret)
		return ret;

	return 0;
}
#endif

static int initr_dm_devices(void)
{
	int ret;

	if (IS_ENABLED(CONFIG_TIMER_EARLY)) {
		ret = dm_timer_init();
		if (ret)
			return ret;
	}

	if (IS_ENABLED(CONFIG_MULTIPLEXER)) {
		/*
		 * Initialize the multiplexer controls to their default state.
		 * This must be done early as other drivers may unknowingly
		 * rely on it.
		 */
		ret = dm_mux_init();
		if (ret)
			return ret;
	}

	return 0;
}

static int initr_bootstage(void)
{
	bootstage_mark_name(BOOTSTAGE_ID_START_UBOOT_R, "board_init_r");

	return 0;
}

__weak int power_init_board(void)
{
	return 0;
}

static int initr_announce(void)
{
	debug("Now running in RAM - U-Boot at: %08lx\n", gd->relocaddr);
	return 0;
}

#ifdef CONFIG_NEEDS_MANUAL_RELOC
static int initr_manual_reloc_cmdtable(void)
{
	fixup_cmdtable(ll_entry_start(struct cmd_tbl, cmd),
		       ll_entry_count(struct cmd_tbl, cmd));
	return 0;
}
#endif

static int initr_binman(void)
{
	int ret;

	if (!CONFIG_IS_ENABLED(BINMAN_FDT))
		return 0;

	ret = binman_init();
	if (ret)
		printf("binman_init failed:%d\n", ret);

	return ret;
}

#if defined(CONFIG_MTD_NOR_FLASH)
__weak int is_flash_available(void)
{
	return 1;
}

static int initr_flash(void)
{
	ulong flash_size = 0;
	struct bd_info *bd = gd->bd;

	if (!is_flash_available())
		return 0;

	puts("Flash: ");

	if (board_flash_wp_on())
		printf("Uninitialized - Write Protect On\n");
	else
		flash_size = flash_init();

	print_size(flash_size, "");
#ifdef CONFIG_SYS_FLASH_CHECKSUM
	/*
	 * Compute and print flash CRC if flashchecksum is set to 'y'
	 *
	 * NOTE: Maybe we should add some WATCHDOG_RESET()? XXX
	 */
	if (env_get_yesno("flashchecksum") == 1) {
		const uchar *flash_base = (const uchar *)CONFIG_SYS_FLASH_BASE;

		printf("  CRC: %08X", crc32(0,
					    flash_base,
					    flash_size));
	}
#endif /* CONFIG_SYS_FLASH_CHECKSUM */
	putc('\n');

	/* update start of FLASH memory    */
#ifdef CONFIG_SYS_FLASH_BASE
	bd->bi_flashstart = CONFIG_SYS_FLASH_BASE;
#endif
	/* size of FLASH memory (final value) */
	bd->bi_flashsize = flash_size;

#if defined(CONFIG_SYS_UPDATE_FLASH_SIZE)
	/* Make a update of the Memctrl. */
	update_flash_size(flash_size);
#endif

#if defined(CONFIG_OXC) || defined(CONFIG_RMU)
	/* flash mapped at end of memory map */
	bd->bi_flashoffset = CONFIG_SYS_TEXT_BASE + flash_size;
#elif CONFIG_SYS_MONITOR_BASE == CONFIG_SYS_FLASH_BASE
	bd->bi_flashoffset = monitor_flash_len;	/* reserved area for monitor */
#endif
	return 0;
}
#endif

#ifdef CONFIG_CMD_NAND
/* go init the NAND */
static int initr_nand(void)
{
	puts("NAND:  ");
	nand_init();
	printf("%lu MiB\n", nand_size() / 1024);
	return 0;
}
#endif

#ifdef CONFIG_NMBM_MTD

__weak int board_nmbm_init(void)
{
	return 0;
}

/* go init the NMBM */
static int initr_nmbm(void)
{
	return board_nmbm_init();
}
#endif

#if defined(CONFIG_CMD_ONENAND)
/* go init the NAND */
static int initr_onenand(void)
{
	puts("NAND:  ");
	onenand_init();
	return 0;
}
#endif

#ifdef CONFIG_MMC
static int initr_mmc(void)
{
	puts("MMC:   ");
	mmc_initialize(gd->bd);
	return 0;
}
#endif

#ifdef CONFIG_PVBLOCK
static int initr_pvblock(void)
{
	puts("PVBLOCK: ");
	pvblock_init();
	return 0;
}
#endif

/*
 * Tell if it's OK to load the environment early in boot.
 *
 * If CONFIG_OF_CONTROL is defined, we'll check with the FDT to see
 * if this is OK (defaulting to saying it's OK).
 *
 * NOTE: Loading the environment early can be a bad idea if security is
 *       important, since no verification is done on the environment.
 *
 * Return: 0 if environment should not be loaded, !=0 if it is ok to load
 */
static int should_load_env(void)
{
	if (IS_ENABLED(CONFIG_OF_CONTROL))
		return ofnode_conf_read_int("load-environment", 1);

	if (IS_ENABLED(CONFIG_DELAY_ENVIRONMENT))
		return 0;

	return 1;
}

static int initr_env(void)
{
	/* initialize environment */
	if (should_load_env())
		env_relocate();
	else
		env_set_default(NULL, 0);

	env_import_fdt();

	if (IS_ENABLED(CONFIG_OF_CONTROL))
		env_set_hex("fdtcontroladdr",
			    (unsigned long)map_to_sysmem(gd->fdt_blob));

	/* Initialize from environment */
	image_load_addr = env_get_ulong("loadaddr", 16, image_load_addr);

	return 0;
}

#ifdef CONFIG_SYS_BOOTPARAMS_LEN
static int initr_malloc_bootparams(void)
{
	gd->bd->bi_boot_params = (ulong)malloc(CONFIG_SYS_BOOTPARAMS_LEN);
	if (!gd->bd->bi_boot_params) {
		puts("WARNING: Cannot allocate space for boot parameters\n");
		return -ENOMEM;
	}
	return 0;
}
#endif

#ifdef CONFIG_CMD_NET
static int initr_ethaddr(void)
{
	struct bd_info *bd = gd->bd;

	/* kept around for legacy kernels only ... ignore the next section */
	eth_env_get_enetaddr("ethaddr", bd->bi_enetaddr);

	return 0;
}
#endif /* CONFIG_CMD_NET */

#if defined(CONFIG_LED_STATUS)
static int initr_status_led(void)
{
#if defined(CONFIG_LED_STATUS_BOOT)
	status_led_set(CONFIG_LED_STATUS_BOOT, CONFIG_LED_STATUS_BLINKING);
#else
	status_led_init();
#endif
	return 0;
}
#endif

#if defined(CONFIG_SCSI) && !defined(CONFIG_DM_SCSI)
static int initr_scsi(void)
{
	puts("SCSI:  ");
	scsi_init();
	puts("\n");

	return 0;
}
#endif

#ifdef CONFIG_CMD_NET
static int initr_net(void)
{
	puts("Net:   ");
	eth_initialize();
#if defined(CONFIG_RESET_PHY_R)
	debug("Reset Ethernet PHY\n");
	reset_phy();
#endif
	return 0;
}
#endif

#ifdef CONFIG_POST
static int initr_post(void)
{
	post_run(NULL, POST_RAM | post_bootmode_get(0));
	return 0;
}
#endif

#if defined(CONFIG_IDE) && !defined(CONFIG_BLK)
static int initr_ide(void)
{
	puts("IDE:   ");
#if defined(CONFIG_START_IDE)
	if (board_start_ide())
		ide_init();
#else
	ide_init();
#endif
	return 0;
}
#endif

#if defined(CONFIG_PRAM)
/*
 * Export available size of memory for Linux, taking into account the
 * protected RAM at top of memory
 */
int initr_mem(void)
{
	ulong pram = 0;
	char memsz[32];

	pram = env_get_ulong("pram", 10, CONFIG_PRAM);
	sprintf(memsz, "%ldk", (long int)((gd->ram_size / 1024) - pram));
	env_set("mem", memsz);

	return 0;
}
#endif

#if defined(CONFIG_ASUS_PRODUCT)
static int led_gpio_init(void)
{
	asus_gpio_init();
	return 0;
}

void set_ver(void)
{
	int rc;

	rc = replace(OFFSET_BOOT_VER, (unsigned char*)blver, 4);
	if (rc)
		printf("\n### [set boot ver] flash write fail\n");
}

static void __call_replace(unsigned long addr, unsigned char *ptr, int len, char *msg)
{
	int rc;
	char *status = "ok";

	if (!ptr || len <= 0)
		return;

	if (!msg)
		msg = "";

	rc = replace(addr, ptr, len);
	if (rc)
		status = "fail";

	printf("\n### [%s] flash writs %s\n", msg, status);
}

void init_mac(void)
{
	int i;
	unsigned char mac[6];
	char *tmp, *end, *default_ethaddr_str = MK_STR(CFG_ETHADDR);

	printf("\ninit mac\n");
	for (i = 0, tmp = default_ethaddr_str; i < 6; ++i) {
		mac[i] = tmp? simple_strtoul(tmp, &end, 16):0;
		if (tmp)
			tmp = (*end)? end+1:end;
	}
	mac[5] &= 0xFC;	/* align with 4 */
	__call_replace(CFG_EEPROM_OFFSET + CFG_MAC_OFFSET, mac, sizeof(mac), "init mac");

	__call_replace(OFFSET_COUNTRY_CODE, (unsigned char*) "DB", 2, "init countrycode");
	__call_replace(OFFSET_PIN_CODE, (unsigned char*) "12345670", 8, "init pincode");
}

/* Restore to default. */
int reset_to_default(void)
{
	ulong addr, size;

#if defined(CFG_UBI_SUPPORT)
	unsigned char *p;

	addr = CFG_NVRAM_ADDR;
	size = CFG_NVRAM_SIZE;
	p = malloc(CFG_NVRAM_SIZE);
	if (!p)
		p = (unsigned char*) CONFIG_SYS_LOAD_ADDR;

	memset(p, 0xFF, CFG_NVRAM_SIZE);
	ra_flash_erase_write(p, addr, size, 0);

	if (p != (unsigned char*) CONFIG_SYS_LOAD_ADDR)
		free(p);
#endif
	/* erase U-Boot environment whether it shared same block with nvram or not. */
	addr = CONFIG_ENV_OFFSET + CFG_SYS_FLASH_BASE; // ASUS way
	size = CFG_ENV_MAX_SIZE;
	printf("Erase 0x%08lx size 0x%lx\n", addr, size);
	//ranand_set_sbb_max_addr(addr + size); -> TBD
	ra_flash_erase(addr, size);
	//ranand_set_sbb_max_addr(0);

	return 0;
}

static void input_value(char *str)
{
	if (str)
		strcpy(console_buffer, str);
	else
		console_buffer[0] = '\0';

	while(1) {
		if (__cli_readline ("==:", 1) > 0) {
			strcpy (str, console_buffer);
			break;
		}
		else
			break;
	}
}

int tftp_config(int type, char *argv[])
{
	char *s;
	char default_file[ARGV_LEN], file[ARGV_LEN], devip[ARGV_LEN], srvip[ARGV_LEN], default_ip[ARGV_LEN];
	static char buf_addr[] = "0x80060000XXX";

	printf(" Please Input new ones /or Ctrl-C to discard\n");

	memset(default_file, 0, ARGV_LEN);
	memset(file, 0, ARGV_LEN);
	memset(devip, 0, ARGV_LEN);
	memset(srvip, 0, ARGV_LEN);
	memset(default_ip, 0, ARGV_LEN);

	printf("\tInput device IP ");
	s = env_get("ipaddr");
	memcpy(devip, s, strlen(s));
	memcpy(default_ip, s, strlen(s));

	printf("(%s) ", devip);
	input_value(devip);
	env_set("ipaddr", devip);
	if (strcmp(default_ip, devip) != 0)
		modifies++;

	printf("\tInput server IP ");
	s = env_get("serverip");
	memcpy(srvip, s, strlen(s));
	memset(default_ip, 0, ARGV_LEN);
	memcpy(default_ip, s, strlen(s));

	printf("(%s) ", srvip);
	input_value(srvip);
	env_set("serverip", srvip);
	if (strcmp(default_ip, srvip) != 0)
		modifies++;

	sprintf(buf_addr, "0x%x", CONFIG_SYS_LOAD_ADDR);
	argv[1] = buf_addr;

	switch (type) {
	case SEL_LOAD_BOOT_WRITE_FLASH:	/* fall through */
	case SEL_LOAD_BOOT_WRITE_FLASH_BY_SERIAL:
		printf("\tInput Uboot filename ");
		strncpy(argv[2], BOOTFILENAME, ARGV_LEN);
		break;
	case SEL_LOAD_LINUX_WRITE_FLASH:/* fall through */
	case SEL_LOAD_LINUX_SDRAM:
		printf("\tInput Linux Kernel filename ");
		strncpy(argv[2], "uImage", ARGV_LEN);
		break;
	default:
		printf("%s: Unknown type %d\n", __func__, type);
	}

	s = env_get("bootfile");
	if (s != NULL) {
		memcpy(file, s, strlen(s));
		memcpy(default_file, s, strlen(s));
	}
	printf("(%s) ", file);
	input_value(file);
	if (file == NULL)
		return 1;
	copy_filename(argv[2], file, sizeof(file));
	env_set("bootfile", file);
	if (strcmp(default_file, file) != 0)
		modifies++;

	return 0;
}

#if defined(CONFIG_CMD_NAND_EXT)
int do_nand_io(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[]);
int do_nand_erase(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[]);
#elif defined(CONFIG_CMD_NAND)
#warning !!!Not yet verified this Implement!!!
int do_nand(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[]);
#endif

int program_bootloader_fit_image(ulong addr)
{
	const void *fit = (const void *)addr;
	int images_noffset;
	int noffset;
	int ndepth;
	const char *name;
	char *desc;
	size_t size;
	const void *data;
	ulong flash_offset, flash_max;
	//ulong load, entry;
	char offset_buf[20], length_buf[20], addr_buf[20];
#if defined(CONFIG_CMD_NAND_EXT)
	char *nand_erase[] = { "erase", offset_buf, length_buf };
	char *nand_write[] = { "write", addr_buf, offset_buf, length_buf };
#elif defined(CONFIG_CMD_NAND)
	char *nand_erase[] = { "nand", "erase", offset_buf, length_buf };
	char *nand_write[] = { "nand", "write", addr_buf, offset_buf, length_buf };
#endif

#if defined(CFG_UBI_SUPPORT)
	const struct ubi_device *ubi = get_ubi_device();
	char *ubi_detach[] = { "ubi", "detach"};
#endif

	if (!addr)
		return -1;

#if defined(CFG_UBI_SUPPORT)
	/* detach UBI_DEV */
	if (ubi)
		do_ubi(NULL, 0, ARRAY_SIZE(ubi_detach), ubi_detach);
#endif

	/* Find images parent node offset */
	images_noffset = fdt_path_offset(fit, FIT_IMAGES_PATH);
	if (images_noffset < 0) {
		return -2;
	}

	/* Process its subnodes */
	for (ndepth = 0, noffset = fdt_next_node(fit, images_noffset, &ndepth);
	     (noffset >= 0) && (ndepth > 0);
	     noffset = fdt_next_node(fit, noffset, &ndepth)) {
		if (ndepth == 1) {
				name = fit_get_name(fit, noffset, NULL);
				if (fit_get_desc(fit, noffset, &desc))
					desc = NULL;
				if (fit_image_get_data_and_size(fit, noffset, &data, &size))
					data = NULL;
				if (data == NULL || desc == NULL)
					return -3;
				if (!strcmp(name, "kernel-1") && !strcmp(desc, "BL2")) {
					flash_offset = CFG_BL2_OFFSET;
					flash_max = CFG_MAX_BL2_BINARY_SIZE;
				}
				else if (!strcmp(name, "kernel-2") && !strcmp(desc, "FIP")) {
					flash_offset = CFG_FIP_OFFSET;
					flash_max = CFG_MAX_FIP_BINARY_SIZE;
				}
				else
					return -4;
				if (size > flash_max)
					return -5;

				//printf("flash %s, addr:%p, len:0x%zx(%zd)\n", desc, data, size, size);
				snprintf(offset_buf, sizeof(offset_buf), "0x%lx", flash_offset);
				//snprintf(length_buf, sizeof(length_buf), "0x%lx", flash_max);
				snprintf(length_buf, sizeof(length_buf), "0x%lx", size);
#if defined(CONFIG_CMD_NAND_EXT)
				do_nand_erase(NULL, 0, ARRAY_SIZE(nand_erase), nand_erase);
#elif defined(CONFIG_CMD_NAND)
				do_nand(NULL, 0, ARRAY_SIZE(nand_erase), nand_erase);
#endif
				snprintf(addr_buf, sizeof(addr_buf), "0x%llx", map_to_sysmem(data));
				snprintf(length_buf, sizeof(length_buf), "0x%lx", size);
#if defined(CONFIG_CMD_NAND_EXT)
				do_nand_io(NULL, 0, ARRAY_SIZE(nand_write), nand_write);
#elif defined(CONFIG_CMD_NAND)
				do_nand(NULL, 0, ARRAY_SIZE(nand_write), nand_write);
#endif
				/*
				if (ra_flash_erase_write((uchar *)map_to_sysmem(data), flash_offset, size, 0))
					return -6;
				*/
		}
	}

	return 0;
}

/* System Load %s to SDRAM via TFTP */
static void handle_boottype_1(void)
{
	int argc= 3;
	char *argv[4];
	cmd_tbl_t c, *cmdtp = &c;

	argv[2] = &file_name_space[0];
	memset(file_name_space, 0, sizeof(file_name_space));

	tftp_config(SEL_LOAD_LINUX_SDRAM, argv);
	argc= 3;
	env_set("autostart", "yes");
	do_tftpb(cmdtp, 0, argc, argv);
}

/* System Load %s then write to Flash via TFTP */
static void handle_boottype_2(void)
{
	int argc= 3, confirm = 0;
	char *argv[4];
	cmd_tbl_t c, *cmdtp = &c;
	char addr_str[11+20];
	unsigned long load_address;

	argv[2] = &file_name_space[0];
	memset(file_name_space, 0, sizeof(file_name_space));

	printf(" Warning!! Erase Linux in Flash then burn new one. Are you sure?(Y/N)\n");
	confirm = getchar();
	if (confirm != 'y' && confirm != 'Y') {
		printf(" Operation terminated\n");
		return;
	}
	tftp_config(SEL_LOAD_LINUX_WRITE_FLASH, argv);
	argc= 3;
	env_set("autostart", "no");
	do_tftpb(cmdtp, 0, argc, argv);

	load_address = simple_strtoul(argv[1], NULL, 16);
	{
		ra_flash_erase_write((uchar*)load_address+sizeof(image_header_t), CFG_KERN_ADDR, net_boot_file_size-sizeof(image_header_t), 0);
#if defined(CFG_DUAL_TRX)
		ra_flash_erase_write((uchar*)load_address+sizeof(image_header_t), CFG_KERN2_ADDR, net_boot_file_size-sizeof(image_header_t), 0);
#endif
	}
	getHWID();
#if defined(CONFIG_PWM_MTK_MM)
	bbtype(1);
#endif

	argc= 2;
	sprintf(addr_str, "0x%lX#config-1", load_address+sizeof(image_header_t));
	argv[1] = &addr_str[0];
	do_bootm(cmdtp, 0, argc, argv);
}

/* System Boot Linux via Flash */
static void handle_boottype_3(void)
{
	if (!chkVer())
	       set_ver();

	if ((chkMAC()) < 0)
	       init_mac();

	getHWID();
//	eth_initialize();
	do_tftpd(NULL, 0, 0, NULL);
#if defined(CONFIG_ASUS_PRODUCT)
	leds_off();
	power_led_on();
#endif
}

/* System Enter Boot Command Line Interface */
static void handle_boottype_4(void)
{
	printf ("\n%s\n", version_string);
	/* main_loop() can return to retry autoboot, if so just run it again. */
	for (;;) {
		main_loop ();
	}
}

/////////////////////// copy from board/mediatek/common/load_data.c
#include <xyzModem.h>
#define BUF_SIZE	1024
static int getcymodem(void)
{
	if (tstc())
		return (getchar());
	return -1;
}

static int load_xymodem(int mode, ulong addr, size_t *data_size)
{
	connection_info_t info;
	char *buf = (char *)addr;
	size_t size = 0;
	int ret, err;
	char xyc;

	xyc = (mode == xyzModem_xmodem ? 'X' : 'Y');

	printf("*** Starting %cmodem transmitting ***", xyc);
	printf("\n");

	info.mode = mode;
	ret = xyzModem_stream_open(&info, &err);
	if (ret) {
		printf("\n");
		printf("*** %cmodem error: %s ***", xyc, xyzModem_error(err));
		printf("*** Operation Aborted! ***");
		return CMD_RET_FAILURE;
	}

	while ((ret = xyzModem_stream_read(buf + size, BUF_SIZE, &err)) > 0)
		size += ret;

	xyzModem_stream_close(&err);
	xyzModem_stream_terminate(false, &getcymodem);

	if (data_size)
		*data_size = size;

	return CMD_RET_SUCCESS;
}

static int load_kermit(ulong addr, size_t *data_size)
{
	char *argv[] = { "loadb", NULL, NULL };
	char saddr[16];
	int repeatable;
	size_t size = 0;
	int ret;

	printf("*** Starting Kermit transmitting ***");
	printf("\n");

	sprintf(saddr, "0x%lx", addr);
	argv[1] = saddr;

	ret = cmd_process(0, 2, argv, &repeatable, NULL);
	if (ret)
		return ret;

	size = env_get_hex("filesize", 0);
	if (!size)
		return CMD_RET_FAILURE;

	if (data_size)
		*data_size = size;

	return CMD_RET_SUCCESS;
}
////////////////////////////////////////

/* System Load %s then write to Flash via Serial */
static void handle_boottype_7(void)
{
	int modem_type;
	size_t data_len, ret;

	printf(" Choose protocol X/Y/K (xymodem,kermit, default X)\n");
	modem_type = getchar();

	switch (modem_type) {
		case 'X' :
		case 'x' :
		case '\r' :
		case '\n' :
			ret =load_xymodem(xyzModem_xmodem, CONFIG_SYS_LOAD_ADDR, &data_len);
			break;
		case 'Y' :
		case 'y' :
			ret =load_xymodem(xyzModem_ymodem, CONFIG_SYS_LOAD_ADDR, &data_len);
			break;
		case 'K' :
		case 'k' :
			ret =load_kermit(CONFIG_SYS_LOAD_ADDR, &data_len);
			break;
		default :
			printf(" Invalid type!\n");
			return;
	}

	if (ret == CMD_RET_SUCCESS) {
		if (data_len > 0 && data_len <= CONFIG_MAX_TRX_IMAGE_SIZE)
			program_bootloader(CONFIG_SYS_LOAD_ADDR, data_len);
		else
			printf(" Incorrect length!\n");

		//reset
		do_reset(NULL, 0, 1, NULL);
	}
}

/* System Load %s then write to Flash via TFTP. (.bin) */
static void handle_boottype_9(void)
{
	int argc= 3, confirm = 0;
	char *argv[4];
	cmd_tbl_t c, *cmdtp = &c;

	argv[2] = &file_name_space[0];
	memset(file_name_space, 0, sizeof(file_name_space));

	printf(" Warning!! Erase %s in Flash then burn new one. Are you sure?(Y/N)\n", BOOT_IMAGE_NAME);
	confirm = getchar();
	if (confirm != 'y' && confirm != 'Y') {
		printf(" Operation terminated\n");
		return;
	}
	env_set("bootfile", BOOTFILENAME);
	tftp_config(SEL_LOAD_BOOT_WRITE_FLASH, argv);
	argc= 3;
	env_set("autostart", "no");
	do_tftpb(cmdtp, 0, argc, argv);
	if (net_boot_file_size > 0 && net_boot_file_size <= CONFIG_MAX_TRX_IMAGE_SIZE)
		program_bootloader(CONFIG_SYS_LOAD_ADDR, net_boot_file_size);
	else
		printf(" Incorrect length!\n");

	//reset
	do_reset(NULL, 0, 1, NULL);
}

static struct boot_menu_s {
	char type;
	void (*func)(void);
	char *msg;
	const char *param1;
} boot_menu[] = {
	{ SEL_LOAD_LINUX_SDRAM,			handle_boottype_1, "Load %s to SDRAM via TFTP.", SYS_IMAGE_NAME },
	{ SEL_LOAD_LINUX_WRITE_FLASH,		handle_boottype_2, "Load %s then write to Flash via TFTP.", SYS_IMAGE_NAME },
	{ SEL_BOOT_FLASH,			handle_boottype_3, "Boot %s via Flash (default).", SYS_IMAGE_NAME },
	{ SEL_ENTER_CLI,			handle_boottype_4, "Entr boot command line interface.", NULL },
	{ SEL_LOAD_BOOT_WRITE_FLASH_BY_SERIAL,	handle_boottype_7, "Load %s then write to Flash via Serial.", BOOT_IMAGE_NAME },
	{ SEL_LOAD_BOOT_WRITE_FLASH,		handle_boottype_9, "Load %s then write to Flash via TFTP.", BOOT_IMAGE_NAME },

	{ 0, NULL, NULL, NULL },
};

static int OperationSelect(void)
{
	char valid_boot_type[16];
	char msg[256];
	struct boot_menu_s *p = &boot_menu[0];
	char *s = env_get ("bootdelay"), *q = &valid_boot_type[0];
	int my_tmp, BootType = '3', timer1 = s ? (int)simple_strtol(s, NULL, 10) : CONFIG_BOOTDELAY;

	/* clear uart buf */
	while ((my_tmp = tstc()) != 0)
		BootType = getchar();

	memset(valid_boot_type, 0, sizeof(valid_boot_type));
	printf("\nPlease choose the operation: \n");
	while (p->func) {
		*q++ = p->type;
		sprintf(msg, "   %c: %s\n", p->type, p->msg);
		if (p->param1)
			printf(msg, p->param1);
		else
			printf(msg);

		p++;
	}
	*q = '\0';

	if (timer1 > 5)
		timer1 = 5;

	timer1 *= 100;
	if (!timer1)
		timer1 = 20;
	while (timer1 > 0) {
		--timer1;
		/* delay 10ms */
		if ((my_tmp = tstc()) != 0) {	/* we got a key press	*/
			timer1 = 0;	/* no more delay	*/
			BootType = getchar();
			if (!strchr(valid_boot_type, BootType))
				BootType = '3';
			printf("\n\rYou choosed %c\n\n", BootType);
			break;
		}
		if (DETECT() || DETECT_WPS()) {
			BootType = '3';
			break;
		}
		udelay (10000);
		if ((timer1 / 100 * 100) == timer1)
			printf ("\b\b\b%2d ", timer1 / 100);
	}
	putc ('\n');

	return BootType;
}
#endif /* CONFIG_ASUS_PRODUCT */

static int dm_announce(void)
{
	int device_count;
	int uclass_count;

	if (IS_ENABLED(CONFIG_DM)) {
		dm_get_stats(&device_count, &uclass_count);
		printf("Core:  %d devices, %d uclasses", device_count,
		       uclass_count);
		if (CONFIG_IS_ENABLED(OF_REAL))
			printf(", devicetree: %s", fdtdec_get_srcname());
		printf("\n");
		if (IS_ENABLED(CONFIG_OF_HAS_PRIOR_STAGE) &&
		    (gd->fdt_src == FDTSRC_SEPARATE ||
		     gd->fdt_src == FDTSRC_EMBED)) {
			printf("Warning: Unexpected devicetree source (not from a prior stage)");
			printf("Warning: U-Boot may not function properly\n");
		}
	}

	return 0;
}

static int run_main_loop(void)
{
#if defined(CONFIG_ASUS_PRODUCT)
	cmd_tbl_t *cmdtp = NULL;
	char *argv[4], msg[256];
	int argc = 3, BootType = '3';
	struct boot_menu_s *p = &boot_menu[0];

	argv[2] = &file_name_space[0];
	file_name_space[0] = '\0';
	if (main_loop_shortcut)
		goto menu_select;
#endif

#ifdef CONFIG_SANDBOX
	sandbox_main_loop_init();
#endif
#if defined(CONFIG_ASUS_PRODUCT)
	/* Boot Loader Menu */

	//LANWANPartition();	/* FIXME */
	disable_all_leds();	/* Inhibit ALL LED, except PWR LED. */
#if !defined(TUFAX4200) && !defined(TUFAX6000) && !defined(PANTHERA)
	leds_off();
	power_led_on();
#endif

	ra_flash_init_layout();
	asus_set_ethmac();
#if defined(TUFAX4200) || defined(TUFAX6000) || defined(PANTHERA)
	/* Turn off LEDs later due to MT7531 LEDs keep ON during initialization. */
	leds_off();
	power_led_on();
#endif

menu_select:
	main_loop_shortcut = 0;
	BootType = OperationSelect();
	for (p = &boot_menu[0]; p->func; ++p ) {
		if (p->type != BootType) {
			continue;
		}

		sprintf(msg, "   %c: %s\n", p->type, p->msg);
		if (p->param1)
			printf(msg, p->param1);
		else
			printf(msg);

		p->func();
		break;
	}

	if (!p->func) {
		printf("   \nSystem Boot Linux via Flash.\n");
		do_bootm(cmdtp, 0, 1, argv);
	}

	for (;;) {
		do_reset(cmdtp, 0, argc, argv);
	}
#else	/* !CONFIG_ASUS_PRODUCT */
	/* main_loop() can return to retry autoboot, if so just run it again */
	for (;;)
		main_loop();
#endif	/* CONFIG_ASUS_PRODUCT */
	return 0;
}

#if defined(CONFIG_ASUS_PRODUCT)
void jump_to_menu_select(void)
{
	main_loop_shortcut = 1;
	run_main_loop();
}
#endif
/*
 * We hope to remove most of the driver-related init and do it if/when
 * the driver is later used.
 *
 * TODO: perhaps reset the watchdog in the initcall function after each call?
 */
static init_fnc_t init_sequence_r[] = {
	initr_trace,
	initr_reloc,
	/* TODO: could x86/PPC have this also perhaps? */
#if defined(CONFIG_ARM) || defined(CONFIG_RISCV)
	initr_caches,
	/* Note: For Freescale LS2 SoCs, new MMU table is created in DDR.
	 *	 A temporary mapping of IFC high region is since removed,
	 *	 so environmental variables in NOR flash is not available
	 *	 until board_init() is called below to remap IFC to high
	 *	 region.
	 */
#endif
	initr_reloc_global_data,
#if defined(CONFIG_SYS_INIT_RAM_LOCK) && defined(CONFIG_E500)
	initr_unlock_ram_in_cache,
#endif
	initr_barrier,
	initr_malloc,
	log_init,
	initr_bootstage,	/* Needs malloc() but has its own timer */
#if defined(CONFIG_CONSOLE_RECORD)
	console_record_init,
#endif
#ifdef CONFIG_SYS_NONCACHED_MEMORY
	noncached_init,
#endif
	initr_of_live,
#ifdef CONFIG_DM
	initr_dm,
#endif
#ifdef CONFIG_ADDR_MAP
	init_addr_map,
#endif
#if defined(CONFIG_ARM) || defined(CONFIG_NDS32) || defined(CONFIG_RISCV) || \
	defined(CONFIG_SANDBOX)
	board_init,	/* Setup chipselects */
#endif
	/*
	 * TODO: printing of the clock inforamtion of the board is now
	 * implemented as part of bdinfo command. Currently only support for
	 * davinci SOC's is added. Remove this check once all the board
	 * implement this.
	 */
#ifdef CONFIG_CLOCKS
	set_cpu_clk_info, /* Setup clock information */
#endif
#ifdef CONFIG_EFI_LOADER
	efi_memory_init,
#endif
	initr_binman,
#ifdef CONFIG_FSP_VERSION2
	arch_fsp_init_r,
#endif
	initr_dm_devices,
	stdio_init_tables,
	serial_initialize,
	initr_announce,
	dm_announce,
#if CONFIG_IS_ENABLED(WDT)
	initr_watchdog,
#endif
	INIT_FUNC_WATCHDOG_RESET
#if defined(CONFIG_NEEDS_MANUAL_RELOC) && defined(CONFIG_BLOCK_CACHE)
	blkcache_init,
#endif
#ifdef CONFIG_NEEDS_MANUAL_RELOC
	initr_manual_reloc_cmdtable,
#endif
	arch_initr_trap,
#if defined(CONFIG_BOARD_EARLY_INIT_R)
	board_early_init_r,
#endif
	INIT_FUNC_WATCHDOG_RESET
#ifdef CONFIG_POST
	post_output_backlog,
#endif
	INIT_FUNC_WATCHDOG_RESET
#if defined(CONFIG_PCI_INIT_R) && defined(CONFIG_SYS_EARLY_PCI_INIT)
	/*
	 * Do early PCI configuration _before_ the flash gets initialised,
	 * because PCU resources are crucial for flash access on some boards.
	 */
	pci_init,
#endif
#ifdef CONFIG_ARCH_EARLY_INIT_R
	arch_early_init_r,
#endif
	power_init_board,
#ifdef CONFIG_MTD_NOR_FLASH
	initr_flash,
#endif
	INIT_FUNC_WATCHDOG_RESET
#if defined(CONFIG_PPC) || defined(CONFIG_M68K) || defined(CONFIG_X86)
	/* initialize higher level parts of CPU like time base and timers */
	cpu_init_r,
#endif
#ifdef CONFIG_CMD_NAND
	initr_nand,
#endif
#ifdef CONFIG_CMD_ONENAND
	initr_onenand,
#endif
#ifdef CONFIG_NMBM_MTD
	initr_nmbm,
#endif
#ifdef CONFIG_MMC
	initr_mmc,
#endif
#ifdef CONFIG_XEN
	xen_init,
#endif
#ifdef CONFIG_PVBLOCK
	initr_pvblock,
#endif
	initr_env,
#ifdef CONFIG_SYS_BOOTPARAMS_LEN
	initr_malloc_bootparams,
#endif
	INIT_FUNC_WATCHDOG_RESET
	cpu_secondary_init_r,
#if defined(CONFIG_ID_EEPROM)
	mac_read_from_eeprom,
#endif
	INIT_FUNC_WATCHDOG_RESET
#if defined(CONFIG_PCI_INIT_R) && !defined(CONFIG_SYS_EARLY_PCI_INIT)
	/*
	 * Do pci configuration
	 */
	pci_init,
#endif
	stdio_add_devices,
	jumptable_init,
#ifdef CONFIG_API
	api_init,
#endif
	console_init_r,		/* fully init console as a device */
#ifdef CONFIG_DISPLAY_BOARDINFO_LATE
	console_announce_r,
	show_board_info,
#endif
#ifdef CONFIG_ARCH_MISC_INIT
	arch_misc_init,		/* miscellaneous arch-dependent init */
#endif
#ifdef CONFIG_MISC_INIT_R
	misc_init_r,		/* miscellaneous platform-dependent init */
#endif
#if defined(CONFIG_ASUS_PRODUCT) && defined(CFG_UBI_SUPPORT)
	ranand_check_and_fix_bootloader,
#endif
	INIT_FUNC_WATCHDOG_RESET
#ifdef CONFIG_CMD_KGDB
	kgdb_init,
#endif
	interrupt_init,
#if defined(CONFIG_MICROBLAZE) || defined(CONFIG_M68K)
	timer_init,		/* initialize timer */
#endif
#if defined(CONFIG_LED_STATUS)
	initr_status_led,
#endif
	/* PPC has a udelay(20) here dating from 2002. Why? */
#ifdef CONFIG_CMD_NET
	initr_ethaddr,
#endif
#if defined(CONFIG_GPIO_HOG)
	gpio_hog_probe_all,
#endif
#ifdef CONFIG_BOARD_LATE_INIT
	board_late_init,
#endif
#if defined(CONFIG_SCSI) && !defined(CONFIG_DM_SCSI)
	INIT_FUNC_WATCHDOG_RESET
	initr_scsi,
#endif
#ifdef CONFIG_BITBANGMII
	bb_miiphy_init,
#endif
#ifdef CONFIG_PCI_ENDPOINT
	pci_ep_init,
#endif
#if defined(CONFIG_ASUS_PRODUCT)
	led_gpio_init,
#endif
#ifdef CONFIG_CMD_NET
	INIT_FUNC_WATCHDOG_RESET
	initr_net,
#endif
#ifdef CONFIG_POST
	initr_post,
#endif
#if defined(CONFIG_IDE) && !defined(CONFIG_BLK)
	initr_ide,
#endif
#ifdef CONFIG_LAST_STAGE_INIT
	INIT_FUNC_WATCHDOG_RESET
	/*
	 * Some parts can be only initialized if all others (like
	 * Interrupts) are up and running (i.e. the PC-style ISA
	 * keyboard).
	 */
	last_stage_init,
#endif
#if defined(CONFIG_PRAM)
	initr_mem,
#endif
#ifdef CONFIG_EFI_SETUP_EARLY
	(init_fnc_t)efi_init_obj_list,
#endif
	run_main_loop,
};

void board_init_r(gd_t *new_gd, ulong dest_addr)
{
	/*
	 * Set up the new global data pointer. So far only x86 does this
	 * here.
	 * TODO(sjg@chromium.org): Consider doing this for all archs, or
	 * dropping the new_gd parameter.
	 */
	if (CONFIG_IS_ENABLED(X86_64) && !IS_ENABLED(CONFIG_EFI_APP))
		arch_setup_gd(new_gd);

#ifdef CONFIG_NEEDS_MANUAL_RELOC
	int i;
#endif

#if !defined(CONFIG_X86) && !defined(CONFIG_ARM) && !defined(CONFIG_ARM64)
	gd = new_gd;
#endif
	gd->flags &= ~GD_FLG_LOG_READY;

#ifdef CONFIG_NEEDS_MANUAL_RELOC
	for (i = 0; i < ARRAY_SIZE(init_sequence_r); i++)
		init_sequence_r[i] += gd->reloc_off;
#endif

	if (initcall_run_list(init_sequence_r))
		hang();

	/* NOTREACHED - run_main_loop() does not return */
	hang();
}
