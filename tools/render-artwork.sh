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

# splash screen of the teacher application
rsvg-convert -w 640 -h 320 "$ART/tafat-splash.svg" -o "$ROOT/master/resources/splash.png"

# feature icons
png feature-app-control.svg 128 "$ROOT/plugins/appcontrol/application-control.png"
png feature-running-apps.svg 128 "$ROOT/plugins/appcontrol/running-apps.png"
png feature-website-control.svg 128 "$ROOT/plugins/webcontrol/website-control.png"
png feature-quiz.svg 128 "$ROOT/plugins/quiz/quiz.png"
png feature-register.svg 128 "$ROOT/plugins/register/register.png"
png feature-chat.svg 128 "$ROOT/plugins/classchat/classchat.png"
png feature-hand-raised.svg 128 "$ROOT/plugins/classchat/hand-raised.png"
png feature-hand-in.svg 128 "$ROOT/plugins/classchat/hand-in.png"
png feature-return-work.svg 128 "$ROOT/plugins/returnwork/return-work.png"
png feature-lab-setup.svg 128 "$ROOT/plugins/labsetup/lab-setup.png"
png feature-add-computers.svg 128 "$ROOT/plugins/labsetup/add-computers.png"
png feature-settings.svg 128 "$ROOT/plugins/labsetup/settings.png"
png feature-inventory.svg 128 "$ROOT/plugins/inventory/inventory.png"

echo "Artwork rendered."

# upstream toolbar and bottom-bar icons, redrawn in the Tafat style
while read -r svg out; do
	png "$svg.svg" 128 "$ROOT/$out"
done <<'MAP'
feature-monitoring core/resources/presentation-none.png
feature-demo plugins/demo/demo.png
feature-demo-fullscreen plugins/demo/presentation-fullscreen.png
feature-demo-window plugins/demo/presentation-window.png
feature-lock plugins/screenlock/system-lock-screen.png
feature-remote-view plugins/remoteaccess/kmag.png
feature-remote-control plugins/remoteaccess/krdc.png
feature-power-on plugins/powercontrol/preferences-system-power-management.png
feature-reboot plugins/powercontrol/system-reboot.png
feature-power-down plugins/powercontrol/system-shutdown.png
feature-log-in plugins/usersessioncontrol/login-user.png
feature-log-off plugins/usersessioncontrol/logout-user.png
feature-text-message plugins/textmessage/dialog-information.png
feature-start-app plugins/desktopservices/preferences-desktop-launch-feedback.png
feature-open-website plugins/desktopservices/internet-web-browser.png
feature-distribute plugins/filetransfer/distribute-files.png
feature-distribute plugins/filetransfer/distribute-files-dark.png
feature-collect plugins/filetransfer/collect-files.png
feature-screenshot plugins/screenshot/camera-photo.png
feature-screenshot plugins/remoteaccess/camera-photo.png
feature-screenshot master/resources/camera-photo.png
panel-computers master/resources/computers.png
panel-slideshow master/resources/computer-slideshow.png
panel-spotlight master/resources/spotlight.png
button-powered-on master/resources/powered-on.png
button-powered-on-dark master/resources/powered-on-dark.png
button-align-grid master/resources/align-grid.png
button-align-grid-dark master/resources/align-grid-dark.png
button-zoom-fit master/resources/zoom-fit-best.png
button-zoom-fit-dark master/resources/zoom-fit-best-dark.png
button-exchange master/resources/exchange-positions-zorder.png
button-exchange-dark master/resources/exchange-positions-zorder-dark.png
button-about core/resources/help-about.png
button-about-dark core/resources/help-about-dark.png
button-user-group core/resources/user-group-new.png
button-user-group-dark core/resources/user-group-new-dark.png
MAP
