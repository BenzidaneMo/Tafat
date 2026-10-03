#!/usr/bin/env bash
# Fail if user-visible upstream "Veyon" branding shows up in places the
# runtime branding (BrandingTranslator, cmake/modules/Branding.cmake) does
# not cover. Run from anywhere; exits non-zero with a list of offending lines.
#
# Translatable texts (tr(), .ui files) may keep saying "Veyon": the
# BrandingTranslator replaces it at runtime.

set -uo pipefail

cd "$(dirname "$0")/.."

SOURCE_DIRS=(core master server service worker cli configurator plugins)
status=0

report() { # <title> <matches>
	if [ -n "$2" ]; then
		echo "::error::$1"
		echo "$2"
		echo
		status=1
	fi
}

# 1. untranslated string literals naming the product
literals=$(grep -rnE '(QStringLiteral|QLatin1String|QString::fromUtf8) *\( *"[^"]*Veyon[^"]*"' \
		--include='*.cpp' --include='*.h' "${SOURCE_DIRS[@]}" |
	grep -vE '"Veyon Community"|_iid|IID' |
	grep -vE '^core/src/(AboutDialog|BrandingTranslator)\.cpp:' |
	grep -vE '"(VeyonMaster|VeyonSessionManager|VeyonServiceDataManager)"')
report "Untranslated literals mention Veyon; use VeyonCore::productName()" "$literals"

# 2. hard-coded executable names
executables=$(grep -rnE '"veyon-(master|server|service|worker|cli|wcli|configurator|auth-helper|input-helper)' \
		--include='*.cpp' --include='*.h' --include='*.c' "${SOURCE_DIRS[@]}")
report "Hard-coded veyon-* executable names; use VeyonCore::executableName()" "$executables"

# 3. configured templates (desktop files, services, policies, installer)
templates=$(grep -rn 'Veyon\|veyon-\(master\|server\|service\|configurator\)' \
		--include='*.desktop.in' --include='*.service.in' --include='*.policy.in' --include='*.nsi.in' . |
	grep -v '^\./3rdparty/' |
	grep -v 'Veyon Solutions / Tobias Junghans')
report "Templates mention Veyon; use the @BRANDING_*@ variables" "$templates"

# 4. Veyon links that are not documentation links
links=$(grep -rnE 'https?://(www\.)?veyon\.io' --include='*.cpp' --include='*.h' --include='*.in' "${SOURCE_DIRS[@]}" nsis |
	grep -vE '^[^:]+:[0-9]+:[[:space:]]*(\*|//|/\*)' |
	grep -vE '^core/src/AboutDialog\.cpp:|^master/src/DocumentationFigureCreator\.cpp:')
report "Links to veyon.io; use VEYON_WEBSITE / @BRANDING_WEBSITE@" "$links"

if [ $status -eq 0 ]; then
	echo "Branding check passed."
fi
exit $status
