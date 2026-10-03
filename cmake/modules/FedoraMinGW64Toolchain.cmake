# Toolchain for cross-compiling 64-bit Windows binaries with Fedora's
# mingw64-* packages (sys-root layout below /usr/x86_64-w64-mingw32/sys-root/mingw).
# usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/modules/FedoraMinGW64Toolchain.cmake ...

include(/usr/share/mingw/toolchain-mingw64.cmake)

set(MINGW_TARGET x86_64-w64-mingw32)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(WIN64 TRUE)

set(MINGW_PREFIX /usr/${MINGW_TARGET}/sys-root/mingw)
set(MINGW_TOOL_PREFIX /usr/bin/${MINGW_TARGET}-)
set(MINGW_DLL_DIRS ${MINGW_PREFIX}/bin)
set(STRIP ${MINGW_TOOL_PREFIX}strip)
