#!/usr/bin/env bash
# Render all icon and installer images from the SVG sources in artwork/.
# Requires rsvg-convert (librsvg2-bin) and ImageMagick (convert).
#
# The output files keep the upstream Veyon file names so that the build
# system needs no changes; only their content is replaced.

set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
ART=$ROOT/artwork
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

png() { # <svg> <size> <output>
	rsvg-convert -w "$2" -h "$2" "$ART/$1" -o "$3"
}

ico() { # <name> <output>: small variant for 16-32 px, full icon above
	local files=()
	for size in 16 24 32; do
		png "$1-small.svg" $size "$TMP/$1-$size.png"; files+=("$TMP/$1-$size.png")
	done
	for size in 48 64 128 256; do
		png "$1.svg" $size "$TMP/$1-$size.png"; files+=("$TMP/$1-$size.png")
	done
	convert "${files[@]}" "$2"
}

# applications
for app in master configurator; do
	dir=$ROOT/$app/data
	png tafat-$app.svg 48 "$dir/veyon-$app.png"
	png tafat-$app-small.svg 32 "$TMP/$app-32.png"
	convert "$TMP/$app-32.png" "$dir/veyon-$app.xpm"
	ico tafat-$app "$dir/veyon-$app.ico"
	cp "$ART/tafat-$app.svg" "$dir/veyon-$app.svg"
done
png tafat-configurator.svg 128 "$ROOT/configurator/resources/veyon-configurator.png"

# window, tray and screenshot icons
for size in 16 22 32; do
	png tafat-master-small.svg $size "$ROOT/core/resources/icon$size.png"
done
for size in 64 128; do
	png tafat-master.svg $size "$ROOT/core/resources/icon$size.png"
done

# installer
ico tafat-master "$ROOT/nsis/installer.ico"
cp "$ROOT/nsis/installer.ico" "$ROOT/nsis/uninstaller.ico"
png tafat-logo.svg 52 "$TMP/logo-52.png"
convert -size 150x57 xc:'#faf4ea' "$TMP/logo-52.png" -gravity center -composite \
	-alpha remove -type TrueColor BMP3:"$ROOT/nsis/header.bmp"
png tafat-logo.svg 140 "$TMP/logo-140.png"
convert -size 164x314 xc:'#faf4ea' "$TMP/logo-140.png" -gravity north -geometry +0+40 -composite \
	-alpha remove -type TrueColor BMP3:"$ROOT/nsis/welcome-page.bmp"

echo "Artwork rendered."
