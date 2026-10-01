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
	$M-gcc-c++ $M-openssl $M-libjpeg-turbo $M-libpng $M-zlib $M-nsis
	$M-qt$QT-qtbase $M-qt$QT-qttools $M-qt$QT-qttranslations $M-qt$QT-qtwebsockets
)
if [ "$QT" = "6" ]; then
	packages+=( $M-qt6-qt5compat qt6-qtbase-devel qt6-qttools-devel qt6-linguist )
else
	packages+=( $M-qca-qt5 )
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

# QCA (Fedora only packages it for Qt 5)
if [ "$QT" = "6" ]; then
	git clone -q --depth 1 -b v2.3.10 https://invent.kde.org/libraries/qca.git
	$M-cmake -S qca -B qca-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DQT6=ON -DQT_HOST_PATH=/usr \
		-DBUILD_TESTS=OFF -DBUILD_TOOLS=OFF -DBUILD_PLUGINS=ossl >/dev/null
	ninja -C qca-build install >/dev/null
fi

rm -rf "$WORK"
echo "Windows $ARCH build environment (Qt $QT) ready in $PREFIX"
