BUILD_NAME ?= $(shell echo $(MAKECMDGOALS) | tr a-z A-Z)

# related path to platform specific software package
export PLATFORM_ROUTER := mt798x
export LINUXDIR := $(SRCBASE)/linux/linux-5.4.x

export CONFIG_SUPPORT_OPENWRT=y

#
# Kernel load address and entry address
ifeq ($(MUSL64),y)
export LOADADDR := 48080000
else
$(warning only support MUSL64)
endif
export ENTRYADDR := $(LOADADDR)

ifeq ($(or $(MT7986A),$(MT7986B)),y)
include $(SRCBASE)/plat-opt-7986.mak
export SDKCONFIG=$(PLATFORM_ROUTER)/sdk.config-7986
else ifeq ($(MT7981),y)
include $(SRCBASE)/plat-opt-7981.mak
export SDKCONFIG=$(PLATFORM_ROUTER)/sdk.config-7981
endif

export BUILD := $(shell (gcc -dumpmachine))
export KERNEL_BINARY=$(LINUXDIR)/vmlinux
export PLATFORM := aarch64-musl
export TOOLS := /opt/openwrt-gcc840_musl.aarch64
export CROSS_COMPILE := $(TOOLS)/bin/aarch64-openwrt-linux-musl-
export READELF := $(TOOLS)/bin/aarch64-openwrt-linux-musl-readelf

export CROSS_COMPILER := $(CROSS_COMPILE)
export CONFIGURE := ./configure --host=aarch64-linux-gnu --build=$(BUILD)
export HOSTCONFIG := linux-aarch64
# ARCH is used for linux kernel and some other else
export ARCH := arm64
export HOST := aarch64-linux

ifeq ($(EXTRACFLAGS),)
#export EXTRACFLAGS := -DBCMWPA2 -fno-delete-null-pointer-checks -march=armv8-a --target=aarch64-arm-none-eabi -mcpu=cortex-a53
endif

export KERNELCC := $(CROSS_COMPILE)gcc
export KERNELLD := $(CROSS_COMPILE)ld
export KARCH := $(firstword $(subst -, ,$(shell $(KERNELCC) -dumpmachine)))

EXTRA_CFLAGS := -DLINUX26 -DCONFIG_RALINK -DMUSL_LIBC -DKERNEL5_MUSL64 -pipe -DDEBUG_NOISY -DDEBUG_RCTEST

# OpenWRT's toolchain needs STAGING_DIR environment variable that points to top directory of toolchain.
export STAGING_DIR=$(TOOLS)

EXTRA_CFLAGS += -D_GNU_SOURCE -D_BSD_SOURCE

export CONFIG_LINUX26=y
export CONFIG_RALINK=y
export TOOLCHAIN_TARGET_GCCVER := 8.4.0

EXTRA_CFLAGS += -DLINUX30
export CONFIG_LINUX30=y

export PRE_CONFIG_BASE_CHANGE=y
export WTOOL_SUFFIX=_mt798x

define PreConfigChange
	@( \
		if [ "$(1)" = "CHEETAH" ]; then \
			cp -f $(LINUXDIR)/config_base_7981 $(LINUXDIR)/config_base; \
		else \
			cp -f $(LINUXDIR)/config_base_7986 $(LINUXDIR)/config_base; \
		fi; \
	)
endef

define SDKConfig
	@( cp -f $(2) $(1) )
	@( \
		if [ "$(MT7986_AX4200)" = "y" ]; then \
			sed -i "/CONFIG_MTK_WIFI_SKU_TYPE=/d" $(1); \
			echo "CONFIG_MTK_WIFI_SKU_TYPE=\"AX4200\"" >> $(1); \
		fi; \
		if [ "$(MTK_ADIE)" = "mt7976" ]; then \
			sed -i "/CONFIG_MTK_WIFI_ADIE_TYPE=/d" $(1); \
			echo "CONFIG_MTK_WIFI_ADIE_TYPE=\"mt7976\"" >> $(1); \
		elif [ "$(MTK_ADIE)" = "mt7975" ]; then \
			sed -i "/CONFIG_MTK_WIFI_ADIE_TYPE=/d" $(1); \
			echo "CONFIG_MTK_WIFI_ADIE_TYPE=\"mt7975\"" >> $(1); \
		fi; \
	)
endef

define platformRouterOptions
	@( \
	sed -i "/RTCONFIG_RALINK\>/d" $(1); \
	echo "RTCONFIG_RALINK=y" >>$(1); \
	sed -i "/RTCONFIG_FITFDT/d" $(1); \
	echo "RTCONFIG_FITFDT=y" >>$(1); \
	if [ "$(MT798X)" = "y" ]; then \
		sed -i "/RTCONFIG_MT798X\>/d" $(1); \
		echo "RTCONFIG_MT798X=y" >>$(1); \
		sed -i "/RTCONFIG_QAM256_2G\>/d" $(1); \
		echo "RTCONFIG_QAM256_2G=y" >>$(1); \
		sed -i "/RTCONFIG_QAM1024_5G\>/d" $(1); \
		echo "RTCONFIG_QAM1024_5G=y" >>$(1); \
	fi; \
	sed -i "/RTCONFIG_VHT160/d" $(1); \
	if [ "$(BW160M)" = "y" ]; then \
		echo "RTCONFIG_VHT160=y" >>$(1); \
		echo "RTCONFIG_BW160M=y" >>$(1); \
	else \
		echo "# RTCONFIG_VHT160 is not set" >>$(1); \
		echo "# RTCONFIG_BW160M is not set" >>$(1); \
	fi; \
	)
	$(call platformL2Router, $(1))
	$(call SDKConfig, $(PLATFORM_ROUTER)/sdk.config, $(SDKCONFIG))
endef

define platformBusyboxOptions
	@( \
	if [ "$(RALINK)" = "y" ]; then \
		sed -i "/CONFIG_FEATURE_TOP_SMP_CPU/d" $(1); \
		echo "CONFIG_FEATURE_TOP_SMP_CPU=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_TOP_DECIMALS/d" $(1); \
		echo "CONFIG_FEATURE_TOP_DECIMALS=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_TOP_SMP_PROCESS/d" $(1); \
		echo "CONFIG_FEATURE_TOP_SMP_PROCESS=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_TOPMEM/d" $(1); \
		echo "CONFIG_FEATURE_TOPMEM=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_SHOW_THREADS/d" $(1); \
		echo "CONFIG_FEATURE_SHOW_THREADS=y" >>$(1); \
		sed -i "/CONFIG_DEVMEM/d" $(1); \
		echo "CONFIG_DEVMEM=y" >>$(1); \
		sed -i "/CONFIG_TFTP\>/d" $(1); \
		echo "CONFIG_TFTP=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_TFTP_GET/d" $(1); \
		echo "CONFIG_FEATURE_TFTP_GET=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_TFTP_PUT/d" $(1); \
		echo "CONFIG_FEATURE_TFTP_PUT=y" >>$(1); \
		sed -i "/CONFIG_BB_SYSCTL\>/d" $(1); \
		echo "CONFIG_BB_SYSCTL=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_NETSTAT_PRG/d" $(1); \
		echo "CONFIG_FEATURE_NETSTAT_PRG=y" >>$(1); \
	fi; \
	)
	$(call platformL2Busybox, $(1))
endef

define platformKernelConfig
	@( \
	if [ "$(RALINK)" = "y" ]; then \
		sed -i "/CONFIG_$(subst RT4G,4G,$(MODEL))/d" $(1); \
		echo "CONFIG_$(subst RT4G,4G,$(MODEL))=y" >>$(1); \
		sed -i "/CONFIG_BRIDGE_NETFILTER/d" $(1); \
		echo "CONFIG_BRIDGE_NETFILTER=y" >>$(1); \
		sed -i "/CONFIG_NETFILTER_XT_TARGET_TPROXY/d" $(1); \
		echo "CONFIG_NETFILTER_XT_TARGET_TPROXY=m" >>$(1); \
		sed -i "/CONFIG_NF_CONNTRACK_CHAIN_EVENTS/d" $(1); \
		echo "CONFIG_NF_CONNTRACK_CHAIN_EVENTS=y" >>$(1); \
		sed -i "/CONFIG_NETFILTER_XT_MATCH_PHYSDEV/d" $(1); \
		echo "CONFIG_NETFILTER_XT_MATCH_PHYSDEV=y" >>$(1); \
		if [ "$(CONFIG_LINUX30)" = "y" ]; then \
			sed -i "/CONFIG_NF_CONNTRACK_EVENTS/d" $(1); \
			echo "CONFIG_NF_CONNTRACK_EVENTS=y" >>$(1); \
		fi; \
		if [ "$(BT_CONN)" != "" ] ; then \
			sed -i "/CONFIG_BT/d" $(1); \
			echo "CONFIG_BT=y" >>$(1); \
		fi; \
	fi; \
	if [ "$(JFFS2)" = "y" ]; then \
		if [ "$(CONFIG_LINUX30)" = "y" ]; then \
			sed -i "/CONFIG_JFFS2_FS_WBUF_VERIFY/d" $(1); \
			echo "# CONFIG_JFFS2_FS_WBUF_VERIFY is not set" >>$(1); \
			sed -i "/CONFIG_JFFS2_CMODE_FAVOURLZO/d" $(1); \
			echo "# CONFIG_JFFS2_CMODE_FAVOURLZO is not set" >>$(1); \
		fi; \
	else \
		sed -i "/CONFIG_JFFS2_FS/d" $(1); \
		echo "# CONFIG_JFFS2_FS is not set" >>$(1); \
	fi; \
	if [ "$(FTRACE)" = "y" ]; then \
		sed -i "/CONFIG_FTRACE\>/d" $(1); \
		echo "CONFIG_FTRACE=y" >>$(1); \
		sed -i "/CONFIG_FUNCTION_TRACER\>/d" $(1); \
		echo "CONFIG_FUNCTION_TRACER=y" >>$(1); \
		sed -i "/CONFIG_FUNCTION_GRAPH_TRACER\>/d" $(1); \
		echo "CONFIG_FUNCTION_GRAPH_TRACER=y" >>$(1); \
		sed -i "/CONFIG_SCHED_TRACER\>/d" $(1); \
		echo "CONFIG_SCHED_TRACER=y" >>$(1); \
		sed -i "/CONFIG_FTRACE_SYSCALLS\>/d" $(1); \
		echo "CONFIG_FTRACE_SYSCALLS=y" >>$(1); \
		sed -i "/CONFIG_BRANCH_PROFILE_NONE\>/d" $(1); \
		echo "CONFIG_BRANCH_PROFILE_NONE=y" >>$(1); \
		sed -i "/CONFIG_DYNAMIC_FTRACE\>/d" $(1); \
		echo "CONFIG_DYNAMIC_FTRACE=y" >>$(1); \
		sed -i "/CONFIG_FUNCTION_PROFILER\>/d" $(1); \
		echo "CONFIG_FUNCTION_PROFILER=y" >>$(1); \
		sed -i "/CONFIG_TRACING_EVENTS_GPIO\>/d" $(1); \
		echo "CONFIG_TRACING_EVENTS_GPIO=y" >>$(1); \
	fi; \
	if [ "$(UBI)" = "y" ]; then \
		sed -i "/CONFIG_MTD_UBI\>/d" $(1); \
		echo "CONFIG_MTD_UBI=y" >>$(1); \
		sed -i "/CONFIG_MTD_UBI_WL_THRESHOLD/d" $(1); \
		echo "CONFIG_MTD_UBI_WL_THRESHOLD=4096" >>$(1); \
		sed -i "/CONFIG_MTD_UBI_BEB_RESERVE/d" $(1); \
		echo "CONFIG_MTD_UBI_BEB_RESERVE=1" >>$(1); \
		sed -i "/CONFIG_MTD_UBI_GLUEBI/d" $(1); \
		echo "CONFIG_MTD_UBI_GLUEBI=y" >>$(1); \
		sed -i "/CONFIG_FACTORY_CHECKSUM/d" $(1); \
		echo "CONFIG_FACTORY_CHECKSUM=y" >>$(1); \
		if [ "$(UBI_DEBUG)" = "y" ]; then \
			sed -i "/CONFIG_MTD_UBI_DEBUG/d" $(1); \
			echo "CONFIG_MTD_UBI_DEBUG=y" >>$(1); \
			sed -i "/CONFIG_GCOV_KERNEL/d" $(1); \
			echo "# CONFIG_GCOV_KERNEL is not set" >>$(1); \
			sed -i "/CONFIG_L2TP_DEBUGFS/d" $(1); \
			echo "# CONFIG_L2TP_DEBUGFS is not set" >>$(1); \
			sed -i "/CONFIG_MTD_UBI_DEBUG_MSG/d" $(1); \
			echo "CONFIG_MTD_UBI_DEBUG_MSG=y" >>$(1); \
			sed -i "/CONFIG_MTD_UBI_DEBUG_PARANOID/d" $(1); \
			echo "# CONFIG_MTD_UBI_DEBUG_PARANOID is not set" >>$(1); \
			sed -i "/CONFIG_MTD_UBI_DEBUG_DISABLE_BGT/d" $(1); \
			echo "# CONFIG_MTD_UBI_DEBUG_DISABLE_BGT is not set" >>$(1); \
			sed -i "/CONFIG_MTD_UBI_DEBUG_EMULATE_BITFLIPS/d" $(1); \
			echo "CONFIG_MTD_UBI_DEBUG_EMULATE_BITFLIPS=y" >>$(1); \
			sed -i "/CONFIG_MTD_UBI_DEBUG_EMULATE_WRITE_FAILURES/d" $(1); \
			echo "CONFIG_MTD_UBI_DEBUG_EMULATE_WRITE_FAILURES=y" >>$(1); \
			sed -i "/CONFIG_MTD_UBI_DEBUG_EMULATE_ERASE_FAILURES/d" $(1); \
			echo "CONFIG_MTD_UBI_DEBUG_EMULATE_ERASE_FAILURES=y" >>$(1); \
			sed -i "/CONFIG_MTD_UBI_DEBUG_MSG_BLD/d" $(1); \
			echo "CONFIG_MTD_UBI_DEBUG_MSG_BLD=y" >>$(1); \
			sed -i "/CONFIG_MTD_UBI_DEBUG_MSG_EBA/d" $(1); \
			echo "CONFIG_MTD_UBI_DEBUG_MSG_EBA=y" >>$(1); \
			sed -i "/CONFIG_MTD_UBI_DEBUG_MSG_WL/d" $(1); \
			echo "CONFIG_MTD_UBI_DEBUG_MSG_WL=y" >>$(1); \
			sed -i "/CONFIG_MTD_UBI_DEBUG_MSG_IO/d" $(1); \
			echo "CONFIG_MTD_UBI_DEBUG_MSG_IO=y" >>$(1); \
			sed -i "/CONFIG_JBD_DEBUG/d" $(1); \
			echo "# CONFIG_JBD_DEBUG is not set" >>$(1); \
			sed -i "/CONFIG_LKDTM/d" $(1); \
			echo "# CONFIG_LKDTM is not set" >>$(1); \
			sed -i "/CONFIG_DYNAMIC_DEBUG/d" $(1); \
			echo "CONFIG_DYNAMIC_DEBUG=y" >>$(1); \
			sed -i "/CONFIG_SPINLOCK_TEST/d" $(1); \
			echo "# CONFIG_SPINLOCK_TEST is not set" >>$(1); \
		else \
			sed -i "/CONFIG_MTD_UBI_DEBUG/d" $(1); \
			echo "# CONFIG_MTD_UBI_DEBUG is not set" >>$(1); \
		fi; \
		if [ "$(UBIFS)" = "y" ]; then \
			sed -i "/CONFIG_UBIFS_FS/d" $(1); \
			echo "CONFIG_UBIFS_FS=y" >>$(1); \
			sed -i "/CONFIG_UBIFS_FS_XATTR/d" $(1); \
			echo "# CONFIG_UBIFS_FS_XATTR is not set" >>$(1); \
			sed -i "/CONFIG_UBIFS_FS_AUTHENTICATION/d" $(1); \
			echo "# CONFIG_UBIFS_FS_AUTHENTICATION is not set" >>$(1); \
			sed -i "/CONFIG_UBIFS_FS_ADVANCED_COMPR/d" $(1); \
			echo "CONFIG_UBIFS_FS_ADVANCED_COMPR=y" >>$(1); \
			sed -i "/CONFIG_UBIFS_FS_LZO/d" $(1); \
			echo "CONFIG_UBIFS_FS_LZO=y" >>$(1); \
			sed -i "/CONFIG_UBIFS_FS_ZLIB/d" $(1); \
			echo "CONFIG_UBIFS_FS_ZLIB=y" >>$(1); \
			sed -i "/CONFIG_UBIFS_FS_XZ/d" $(1); \
			echo "CONFIG_UBIFS_FS_XZ=y" >>$(1); \
			sed -i "/CONFIG_UBIFS_FS_ZSTD/d" $(1); \
			echo "# CONFIG_UBIFS_FS_ZSTD is not set" >>$(1); \
			sed -i "/CONFIG_UBIFS_FS_DEBUG/d" $(1); \
			echo "# CONFIG_UBIFS_FS_DEBUG is not set" >>$(1); \
		else \
			sed -i "/CONFIG_UBIFS_FS/d" $(1); \
			echo "# CONFIG_UBIFS_FS is not set" >>$(1); \
		fi; \
	fi; \
	if [ "$(DUMP_OOPS_MSG)" = "y" ]; then \
		echo "CONFIG_DUMP_PREV_OOPS_MSG=y" >>$(1); \
		echo "CONFIG_DUMP_PREV_OOPS_MSG_BUF_ADDR=0x4C800000" >>$(1); \
		echo "CONFIG_DUMP_PREV_OOPS_MSG_BUF_LEN=0x2000" >>$(1); \
	else \
		sed -i "/CONFIG_DUMP_PREV_OOPS_MSG/d" $(1); \
		echo "# CONFIG_DUMP_PREV_OOPS_MSG is not set" >>$(1); \
	fi; \
	if [ "$(IPV6SUPP)" = "y" ]; then \
		sed -i "/CONFIG_IPV6_MULTIPLE_TABLES/d" $(1); \
		echo "# CONFIG_IPV6_MULTIPLE_TABLES is not set" >>$(1); \
	fi; \
	if [ "$(NFCM)" = "y" ]; then \
		sed -i "/CONFIG_MTK_HNAT_FORCE_CT_ACCOUNTING/d" $(1); \
		echo "CONFIG_MTK_HNAT_FORCE_CT_ACCOUNTING=y" >>$(1); \
	else \
		sed -i "/CONFIG_MTK_HNAT_FORCE_CT_ACCOUNTING/d" $(1); \
		echo "# CONFIG_MTK_HNAT_FORCE_CT_ACCOUNTING is not set" >>$(1); \
	fi; \
	if [ "$(WIREGUARD)" = "y" ]; then \
		echo "CONFIG_CRYPTO_SHA256_ARM64=y" >>$(1); \
		echo "CONFIG_CRYPTO_SHA512_ARM64=y" >>$(1); \
		echo "CONFIG_CRYPTO_SHA1_ARM64_CE=y" >>$(1); \
		echo "CONFIG_CRYPTO_SHA2_ARM64_CE=y" >>$(1); \
		echo "CONFIG_CRYPTO_SHA512_ARM64_CE=y" >>$(1); \
		echo "CONFIG_CRYPTO_SHA3_ARM64=y" >>$(1); \
		echo "CONFIG_CRYPTO_SM3_ARM64_CE=y" >>$(1); \
		echo "CONFIG_CRYPTO_SM4_ARM64_CE=y" >>$(1); \
		echo "CONFIG_CRYPTO_GHASH_ARM64_CE=y" >>$(1); \
		echo "CONFIG_CRYPTO_AES_ARM64=y" >>$(1); \
		echo "CONFIG_CRYPTO_AES_ARM64_CE=y" >>$(1); \
		echo "CONFIG_CRYPTO_AES_ARM64_CE_CCM=y" >>$(1); \
		echo "CONFIG_CRYPTO_AES_ARM64_CE_BLK=y" >>$(1); \
		echo "CONFIG_CRYPTO_AES_ARM64_NEON_BLK=y" >>$(1); \
		echo "CONFIG_CRYPTO_CHACHA20_NEON=y" >>$(1); \
		echo "CONFIG_CRYPTO_POLY1305_NEON=y" >>$(1); \
		echo "CONFIG_CRYPTO_NHPOLY1305_NEON=y" >>$(1); \
		echo "CONFIG_CRYPTO_AES_ARM64_BS=y" >>$(1); \
	fi; \
	)
	$(call platformL2Kernel, $(1))
endef
