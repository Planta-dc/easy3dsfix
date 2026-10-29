#!/bin/sh
# Install makerom and bannertool for packaging a Nintendo 3DS CIA.
# The published Linux binaries require a newer glibc than the devkitPro
# container provides, so build pinned source releases instead.
set -eu

MAKEROM_VERSION="${MAKEROM_VERSION:-0.19.0}"
BANNERTOOL_VERSION="${BANNERTOOL_VERSION:-1.2.3}"
DEST="${DEST:-/usr/local/bin}"
MAKEROM_REPO="${MAKEROM_REPO:-https://github.com/3DSGuy/Project_CTR}"
BANNERTOOL_REPO="${BANNERTOOL_REPO:-https://github.com/carstene1ns/3ds-bannertool}"

mkdir -p "$DEST"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

if ! command -v makerom >/dev/null 2>&1; then
	echo "Building makerom ${MAKEROM_VERSION} from source"
	git clone -q --depth 1 --branch "makerom-v${MAKEROM_VERSION}" "$MAKEROM_REPO" "$work/Project_CTR"
	(
		cd "$work/Project_CTR/makerom"
		make --no-print-directory deps
		make --no-print-directory -j"$(nproc 2>/dev/null || echo 2)"
	)
	install -m 0755 "$work/Project_CTR/makerom/bin/makerom" "$DEST/makerom"
fi

if ! command -v bannertool >/dev/null 2>&1; then
	echo "Building bannertool ${BANNERTOOL_VERSION} from source"
	git clone -q --depth 1 --branch "$BANNERTOOL_VERSION" "$BANNERTOOL_REPO" "$work/3ds-bannertool"
	(
		cd "$work/3ds-bannertool"
		"${CXX:-g++}" -std=c++17 -O2 \
			-DVERSION="\"${BANNERTOOL_VERSION}\"" \
			-Isource -Isource/pc \
			-DSTBI_ONLY_PNG -DSTBI_NO_LINEAR -DSTBI_NO_STDIO \
			-DDR_WAV_NO_STDIO \
			-DSTB_VORBIS_NO_PUSHDATA_API -DSTB_VORBIS_NO_STDIO \
			-o "$work/bannertool" \
			source/main.cpp source/log.cpp source/types.cpp \
			source/utils.cpp source/3ds/cbmd.cpp source/3ds/cwav.cpp \
			source/3ds/lz11.cpp source/pc/stb_image.cpp \
			source/pc/stb_vorbis.cpp source/pc/dr_wav.cpp
	)
	install -m 0755 "$work/bannertool" "$DEST/bannertool"
fi

# Confirm both tools execute before the (much longer) player build.
makerom -help >/dev/null 2>&1 || true
bannertool --version
command -v makerom
command -v bannertool
