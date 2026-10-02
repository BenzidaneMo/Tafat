#!/usr/bin/env bash
# Build the Windows installer with the Fedora MinGW environment
# (see fedora-deps.sh). Run from the source directory.
# usage: build-fedora.sh <i686|x86_64> [<qt major version, default 6>]
# Qt 5 builds are legacy builds for Windows 7 and 8.1.

set -euo pipefail

ARCH=$1
QT=${2:-6}

BASEDIR=$(pwd)
BUILDDIR=/tmp/build-$ARCH-qt$QT
if [ "$ARCH" = "i686" ]; then BITS=32; else BITS=64; fi
if [ "$QT" = "6" ]; then WITH_QT6=ON; LEGACY=OFF; else WITH_QT6=OFF; LEGACY=ON; fi

"$BASEDIR/.ci/common/strip-ultravnc-sources.sh"

rm -rf "$BUILDDIR"
cmake -S "$BASEDIR" -B "$BUILDDIR" -G Ninja \
	-DCMAKE_TOOLCHAIN_FILE="$BASEDIR/cmake/modules/FedoraMinGW${BITS}Toolchain.cmake" \
	-DCMAKE_BUILD_TYPE=Release \
	-DQT_HOST_PATH=/usr \
	-DWITH_QT6=$WITH_QT6 \
	-DWITH_LEGACY_WINDOWS=$LEGACY \
	-DWITH_BUNDLED_LIBVNC=ON \
	-DWITH_LDAP=OFF \
	-DWITH_WEBAPI=OFF \
	-DWITH_LTO=OFF \
	-DWITH_WERROR=OFF \
	${CMAKE_FLAGS:-}

# keep going after errors and list all of them at the end, so one CI run
# shows every problem of the build
if ! ninja -C "$BUILDDIR" -k 0 create-windows-installer 2>&1 | tee "$BUILDDIR/build.log"; then
	echo "==== build errors ===="
	grep -E "error:|Error [0-9]|undefined reference" "$BUILDDIR/build.log" | sort -u | head -80
	exit 1
fi

mv "$BUILDDIR"/*-setup.exe "$BASEDIR/"
