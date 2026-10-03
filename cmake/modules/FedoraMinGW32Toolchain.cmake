# Toolchain for cross-compiling 32-bit Windows binaries with Fedora's
# mingw32-* packages (sys-root layout below /usr/i686-w64-mingw32/sys-root/mingw).
# usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/modules/FedoraMinGW32Toolchain.cmake ...

include(/usr/share/mingw/toolchain-mingw32.cmake)

set(MINGW_TARGET i686-w64-mingw32)
set(CMAKE_SYSTEM_PROCESSOR i686)

set(MINGW_PREFIX /usr/${MINGW_TARGET}/sys-root/mingw)
set(MINGW_TOOL_PREFIX /usr/bin/${MINGW_TARGET}-)
set(MINGW_DLL_DIRS ${MINGW_PREFIX}/bin)
set(STRIP ${MINGW_TOOL_PREFIX}strip)
