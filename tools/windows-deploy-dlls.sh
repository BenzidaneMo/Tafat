#!/usr/bin/env bash
# Copy all DLLs that the Windows binaries in a directory depend on.
#
# usage: windows-deploy-dlls.sh <objdump> <dest-dir> <search-dir>...
#
# Scans every .exe and .dll below <dest-dir> with objdump, looks up each
# imported DLL that is not yet present in <dest-dir> in the search
# directories (case-insensitive) and copies it to <dest-dir>. Repeats
# until no new DLLs are needed. DLLs not found in the search directories
# (Windows system DLLs) are skipped.

set -euo pipefail

objdump=$1
dest=$2
shift 2
search_dirs=("$@")

find_dll() {
	local name=$1 dir match
	for dir in "${search_dirs[@]}"; do
		[ -d "$dir" ] || continue
		match=$(find "$dir" -maxdepth 1 -iname "$name" -print -quit)
		if [ -n "$match" ]; then
			echo "$match"
			return
		fi
	done
}

while true; do
	added=0
	while read -r dll; do
		if [ -z "$(find "$dest" -maxdepth 1 -iname "$dll" -print -quit)" ]; then
			src=$(find_dll "$dll")
			if [ -n "$src" ]; then
				echo "deploying $(basename "$src")"
				cp "$src" "$dest/"
				added=1
			fi
		fi
	done < <(find "$dest" \( -iname '*.exe' -o -iname '*.dll' \) -exec "$objdump" -p {} \; 2>/dev/null |
				sed -n 's/^[[:space:]]*DLL Name: //p' | sort -u)
	[ $added -eq 1 ] || break
done
