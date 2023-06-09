#!/bin/sh
#
# Licensed under the terms of the GNU GPL License version 2 or later.
#
# Author: Peter Tyser <ptyser@xes-inc.com>
#
# U-Boot firmware supports the booting of images in the Flattened Image
# Tree (FIT) format.  The FIT format uses a device tree structure to
# describe a kernel image, device tree blob, ramdisk, etc.  This script
# creates an Image Tree Source (.its file) which can be passed to the
# 'mkimage' utility to generate an Image Tree Blob (.itb file).  The .itb
# file can then be booted by U-Boot (or other bootloaders which support
# FIT images).  See doc/uImage.FIT/howto.txt in U-Boot source code for
# additional information on FIT images.
#

usage() {
	printf "Usage: %s -A arch -C comp -a addr -e entry" "$(basename "$0")"
	printf " -v version -k kernel [-D name -n address -d dtb] -o its_file"
	printf " [-s script] [-S key_name_hint] [-r ar_ver]"

	printf "\n\t-A ==> set architecture to 'arch'"
	printf "\n\t-C ==> set compression type 'comp'"
	printf "\n\t-c ==> set config name 'config'"
	printf "\n\t-a ==> set load address to 'addr' (hex)"
	printf "\n\t-e ==> set entry point to 'entry' (hex)"
	printf "\n\t-v ==> set kernel version to 'version'"
	printf "\n\t-k ==> include kernel image 'kernel'"
	printf "\n\t-D ==> human friendly Device Tree Blob 'name'"
	printf "\n\t-n ==> fdt unit-address 'address'"
	printf "\n\t-d ==> include Device Tree Blob 'dtb'"
	printf "\n\t-o ==> create output file 'its_file'"
	printf "\n\t-s ==> include u-boot script 'script'"
	printf "\n\t-S ==> add signature at configurations and assign its key_name_hint by 'key_name_hint'"
	printf "\n\t-r ==> set anti-rollback version to 'fw_ar_ver' (dec)\n"
	printf "\n\t-R ==> set root filesystem image\n"
	exit 1
}

FDTNUM=1

while getopts ":A:a:c:C:D:d:e:k:n:o:v:s:S:r:R:" OPTION
do
	case $OPTION in
		A ) ARCH=$OPTARG;;
		a ) LOAD_ADDR=$OPTARG;;
		c ) CONFIG=$OPTARG;;
		C ) COMPRESS=$OPTARG;;
		D ) DEVICE=$OPTARG;;
		d ) DTB=$OPTARG;;
		e ) ENTRY_ADDR=$OPTARG;;
		k ) KERNEL=$OPTARG;;
		n ) FDTNUM=$OPTARG;;
		o ) OUTPUT=$OPTARG;;
		v ) VERSION=$OPTARG;;
		s ) UBOOT_SCRIPT=$OPTARG;;
		S ) KEY_NAME_HINT=$OPTARG;;
		r ) AR_VER=$OPTARG;;
		R ) ROOTFS=$OPTARG;;
		* ) echo "Invalid option passed to '$0' (options:$*)"
		usage;;
	esac
done

# Make sure user entered all required parameters
if [ -z "${ARCH}" ] || [ -z "${COMPRESS}" ] || [ -z "${LOAD_ADDR}" ] || \
	[ -z "${ENTRY_ADDR}" ] || [ -z "${VERSION}" ] || [ -z "${KERNEL}" ] || [ -z "${ROOTFS}" ] || \
	[ -z "${OUTPUT}" ] || [ -z "${CONFIG}" ]; then
	usage
fi

ARCH_UPPER=$(echo "$ARCH" | tr '[:lower:]' '[:upper:]')

# Conditionally create fdt information
if [ -n "${DTB}" ]; then
	FDT_NODE="
		fdt-$FDTNUM {
			description = \"${ARCH_UPPER} OpenWrt ${DEVICE} device tree blob\";
			data = /incbin/(\"${DTB}\");
			type = \"flat_dt\";
			arch = \"${ARCH}\";
			compression = \"none\";
			hash-1 {
				algo = \"crc32\";
			};
			hash-2 {
				algo = \"sha1\";
			};
		};
"
	FDT_PROP="fdt = \"fdt-$FDTNUM\";"
fi

# Conditionally create script information
if [ -n "${UBOOT_SCRIPT}" ]; then
	SCRIPT="\
		script-1 {
			description = \"U-Boot Script\";
			data = /incbin/(\"${UBOOT_SCRIPT}\");
			type = \"script\";
			arch = \"${ARCH}\";
			os = \"linux\";
			load = <0>;
			entry = <0>;
			compression = \"none\";
			hash-1 {
				algo = \"crc32\";
			};
			hash-2 {
				algo = \"sha1\";
			};
		};\
"
	LOADABLES="\
			loadables = \"script-1\";\
"
	SIGN_IMAGES="\
				sign-images = \"fdt\", \"kernel\", \"loadables\";\
"
else
	SIGN_IMAGES="\
				sign-images = \"fdt\", \"kernel\";\
"
fi

# Conditionally create signature information
if [ -n "${KEY_NAME_HINT}" ]; then
	SIGNATURE="\
			signature {
				algo = \"sha1,rsa2048\";
				key-name-hint = \"${KEY_NAME_HINT}\";
${SIGN_IMAGES}
			};\
"
fi

# Conditionally create anti-rollback version information
if [ -n "${AR_VER}" ]; then
	FW_AR_VER="\
			fw_ar_ver = <${AR_VER}>;\
"
fi

# Create a default, fully populated DTS file
DATA="/dts-v1/;

/ {
	description = \"${ARCH_UPPER} OpenWrt FIT (Flattened Image Tree)\";
	#address-cells = <1>;

	images {
		kernel-1 {
			description = \"${ARCH_UPPER} OpenWrt Linux-${VERSION}\";
			data = /incbin/(\"${KERNEL}\");
			type = \"kernel\";
			arch = \"${ARCH}\";
			os = \"linux\";
			compression = \"${COMPRESS}\";
			load = <${LOAD_ADDR}>;
			entry = <${ENTRY_ADDR}>;
			hash-1 {
				algo = \"crc32\";
			};
			hash-2 {
				algo = \"sha1\";
			};
		};
${FDT_NODE}
		rootfs-1 {
			description = \"Root File System\";
			data = /incbin/(\"${ROOTFS}\");
			type = \"ramdisk\";
			load = <0>;
			entry = <0>;
			hash-1 {
				algo = \"crc32\";
			};
			hash-2 {
				algo = \"sha1\";
			};
		};
${SCRIPT}
	};

	configurations {
		default = \"${CONFIG}\";
		${CONFIG} {
			description = \"OpenWrt\";
${FW_AR_VER}
${LOADABLES}
			kernel = \"kernel-1\";
			${FDT_PROP}
${SIGNATURE}
			ramdisk = \"rootfs-1\";
		};
	};
};"

# Write .its file to disk
echo "$DATA" > "${OUTPUT}"
