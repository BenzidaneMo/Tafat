#!/usr/bin/env bash
# Report Windows binaries that cannot be loaded on Windows 7.
#
# usage: check-windows7-imports.sh <objdump> <dir>
#
# Lists the imports of every .exe and .dll below <dir> (objdump -p) and reports
# imported DLLs and functions that only exist on Windows 8 or newer. Windows 7
# refuses to start a program, or to load a plugin, with such an import, even
# if the code never calls it. Exits with 1 if anything was found.

set -uo pipefail

objdump=$1
dir=$2

# DLLs that do not exist on Windows 7 (SP1 with the usual updates)
win8_dlls='^(shcore|dcomp|d3d12|api-ms-win-core-synch-l1-2-0|api-ms-win-core-path-l1-1-0|api-ms-win-core-winrt-.*|api-ms-win-shcore-.*|api-ms-win-core-realtime-l1-1-1|api-ms-win-appmodel-.*|windows[.]ui.*|coremessaging|cfgmgr32-l1-.*)[.]dll$'

# functions of system DLLs that exist since Windows 8 or 10
win8_functions='^(GetSystemTimePreciseAsFileTime|CreateFile2|CopyFile2|GetCurrentPackageId|GetCurrentPackageFullName|GetPackageFullName|GetPackagePath|PrefetchVirtualMemory|SetThreadDescription|GetThreadDescription|GetProcessMitigationPolicy|SetProcessMitigationPolicy|GetCurrentThreadStackLimits|GetOverlappedResultEx|CreateFileMappingFromApp|MapViewOfFileFromApp|VirtualAllocFromApp|GetFirmwareType|QueryUnbiasedInterruptTimePrecise|AppPolicyGetProcessTerminationMethod|WaitOnAddress|WakeByAddressSingle|WakeByAddressAll|SetDefaultDllDirectoriesEx|GetPointerInfo|GetPointerType|GetPointerFrameInfo|GetPointerFrameInfoHistory|GetPointerPenInfo|GetPointerTouchInfo|GetPointerDevices|GetPointerDevice|GetPointerDeviceRects|EnableMouseInPointer|IsMouseInPointerEnabled|RegisterPointerDeviceNotifications|SkipPointerFrameMessages|InjectTouchInput|InitializeTouchInjection|CreateSyntheticPointerDevice|InjectSyntheticPointerInput|SetDisplayAutoRotationPreferences|GetDisplayAutoRotationPreferences|GetDpiForWindow|GetDpiForSystem|GetSystemMetricsForDpi|AdjustWindowRectExForDpi|SystemParametersInfoForDpi|SetProcessDpiAwarenessContext|SetThreadDpiAwarenessContext|GetThreadDpiAwarenessContext|GetWindowDpiAwarenessContext|AreDpiAwarenessContextsEqual|EnableNonClientDpiScaling|GetAwarenessFromDpiAwarenessContext|IsImmersiveProcess|SetCoalescableTimer|GetWindowFeedbackSetting|SetWindowFeedbackSetting|RegisterTouchHitTestingWindow|GetCurrentInputMessageSource|GetCIMSSM)$'

status=0
while IFS= read -r -d '' file; do
	report=$("$objdump" -p "$file" 2>/dev/null | awk -v dlls="$win8_dlls" -v functions="$win8_functions" '
		/DLL Name:/ { dll = $3; bad = (tolower(dll) ~ dlls); if (bad) print "  imports " dll; next }
		/^\t[0-9a-f]+[ \t]+[0-9]+[ \t]+[A-Za-z_]/ {
			if (bad) print "    " $3
			else if ($3 ~ functions) print "  imports " $3 " from " dll
		}
	')
	if [ -n "$report" ]; then
		echo "::warning::${file#"$dir"/} cannot be loaded on Windows 7:"
		echo "$report"
		status=1
	fi
done < <(find "$dir" \( -iname '*.exe' -o -iname '*.dll' \) -print0)

if [ $status -eq 0 ]; then
	echo "Windows 7 import check passed."
fi
exit $status
