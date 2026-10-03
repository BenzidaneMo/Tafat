#!/usr/bin/env bash
# Install the Fedora MinGW build environment for Windows builds and build the
# libraries Fedora does not package.
# usage: fedora-deps.sh <i686|x86_64> [<qt major version, default 6>]

set -euo pipefail

ARCH=$1
QT=${2:-6}

if [ "$ARCH" = "i686" ]; then M=mingw32; else M=mingw64; fi
TARGET=$ARCH-w64-mingw32
PREFIX=/usr/$TARGET/sys-root/mingw
WORK=$(mktemp -d)

packages=(
	cmake ninja-build git make autoconf automake libtool wget xz dos2unix findutils
	$M-gcc-c++ $M-openssl $M-libjpeg-turbo $M-libpng $M-zlib
	# the installer itself is a 32-bit program also for 64-bit builds
	mingw32-nsis
	$M-qt$QT-qtbase $M-qt$QT-qttools $M-qt$QT-qttranslations $M-qt$QT-qtwebsockets
)
if [ "$QT" = "6" ]; then
	packages+=( $M-qt6-qt5compat qt6-qtbase-devel qt6-qttools-devel qt6-linguist )
else
	# -tools: native lrelease etc. referenced by the Qt5LinguistTools CMake package
	packages+=( $M-qca-qt5 $M-qt5-qttools-tools )
fi
dnf -y install "${packages[@]}"

cd "$WORK"

# LZO (used by the bundled LibVNCServer)
LZO_VERSION=2.10
wget -q https://www.oberhumer.com/opensource/lzo/download/lzo-$LZO_VERSION.tar.gz
tar xzf lzo-$LZO_VERSION.tar.gz
( cd lzo-$LZO_VERSION && $M-configure --enable-shared --disable-static >/dev/null && make -j"$(nproc)" >/dev/null && make install >/dev/null )

# Interception (input device blocking)
git clone -q --depth 1 https://github.com/oblitum/Interception.git
( cd Interception/library &&
  $TARGET-gcc -O2 -shared -DINTERCEPTION_EXPORT -o interception.dll interception.c \
	-Wl,--out-implib,libinterception.dll.a &&
  install -m 644 interception.h "$PREFIX/include/" &&
  install -m 755 interception.dll "$PREFIX/bin/" &&
  install -m 644 libinterception.dll.a "$PREFIX/lib/" )

# QCA with its OpenSSL plugin (Fedora only packages it for Qt 5, and maybe
# without the plugin)
if [ "$QT" = "6" ] || [ -z "$(find "$PREFIX" -name libqca-ossl.dll -print -quit)" ]; then
	if [ "$QT" = "6" ]; then QCA_QT6=ON; else QCA_QT6=OFF; fi
	git clone -q --depth 1 -b v2.3.10 https://invent.kde.org/libraries/qca.git
	$M-cmake -S qca -B qca-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DQT6=$QCA_QT6 -DQT_HOST_PATH=/usr \
		-DBUILD_TESTS=OFF -DBUILD_TOOLS=OFF -DBUILD_PLUGINS=ossl >/dev/null
	ninja -C qca-build install >/dev/null
fi
echo "QCA OpenSSL plugin: $(find "$PREFIX" -name libqca-ossl.dll)"

rm -rf "$WORK"
rpm -q $M-openssl $M-qt$QT-qtbase $M-gcc-c++ $M-crt 2>/dev/null || true
echo "Windows $ARCH build environment (Qt $QT) ready in $PREFIX"
