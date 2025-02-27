#!/bin/bash

if command -v ccache &> /dev/null; then
	CROSS_COMPILE="${CROSS_COMPILE:-ccache mipsel-linux-}"
	HOSTCC="${HOSTCC:-ccache gcc}"
else
	CROSS_COMPILE="${CROSS_COMPILE:-mipsel-linux-}"
	HOSTCC="${HOSTCC:-gcc}"
fi

OUTPUT_DIR="./uboot_build"
JOBS=$(nproc)

while [ "$1" != "" ]; do
	case $1 in
		-single)
			JOBS=1
			shift
			;;
		*)
			soc=$1
			shift
			;;
	esac
done

if command -v resize; then
	eval $(resize)
	size="$(( LINES - 4 )) $(( COLUMNS -4 )) $(( LINES - 12 ))"
else
	size="20 76 12"
fi

pick_a_soc() {
	eval `resize`
	soc=$(whiptail --title "U-Boot SoC selection" \
		--menu "Choose a SoC model" $size \
		"isvp_a1_all_sfc0nor"		"Ingenic A1 ALL NOR"		\
		"isvp_a1_all_lzma_sfc0nor"	"Ingenic A1 ALL LZMA NOR"		\
		"isvp_a1_all_sfc0nand"		"Ingenic A1 ALL NAND"		\
		"isvp_a1_all_msc0"		"Ingenic A1 ALL MSC"		\
		"isvp_a1_nt_sfc0nor"		"Ingenic A1NT SFC0 NOR"		\
		"isvp_a1_nt_lzma_sfc0nor"	"Ingenic A1NT SFC0 NOR LZMA"		\
		"isvp_a1_nt_sfc01nand"		"Ingenic A1NT SFC0 NAND"		\
		"isvp_a1_nt_sfc1nand"		"Ingenic A1NT SFC1 NAND"		\
		"isvp_a1_nt_msc0"		"Ingenic A1NT MSC0"		\
		--notags 3>&1 1>&2 2>&3)
}

# Function to build a specific version
build_version() {
	# Start timer
	SECONDS=0

	local soc=$1
	echo "Building U-Boot for ${soc}"

	make CROSS_COMPILE="$CROSS_COMPILE" distclean
	mkdir -p "${OUTPUT_DIR}" >/dev/null
	make CROSS_COMPILE="$CROSS_COMPILE" $soc -j${JOBS} HOSTCC="$HOSTCC"
	if [ -f u-boot-lzo-with-spl.bin ]; then
		echo "u-boot-lzo-with-spl.bin exists, copying..."
		cp u-boot-lzo-with-spl.bin "${OUTPUT_DIR}/u-boot-${soc}.bin"
	elif [ -f u-boot-with-spl.bin ]; then
		echo "u-boot-with-spl.bin exists, copying..."
		cp u-boot-with-spl.bin "${OUTPUT_DIR}/u-boot-${soc}.bin"
	fi
}

[ -z "$soc" ] && pick_a_soc
[ -z "$soc" ] && echo No SoC && exit 1
build_version "$soc"

# End timer and report
duration=$SECONDS
echo "Done"
echo "Total build time: $(($duration / 60)) minutes and $(($duration % 60)) seconds."
exit 0
