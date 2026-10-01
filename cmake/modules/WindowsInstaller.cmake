set(WINDOWS_INSTALL_FILES "${BRANDING_PRODUCT_SLUG}-${VEYON_WINDOWS_ARCH}-${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_BUILD}")

# Directories with the runtime DLLs of the MinGW environment. Toolchain files
# may preset them (e.g. for Fedora's sys-root layout).
string(REGEX MATCH "^[^.]+" GCC_VERSION_MAJOR ${CMAKE_CXX_COMPILER_VERSION})
if(NOT MINGW_DLL_DIRS)
	set(MINGW_DLL_DIRS "${MINGW_PREFIX}/bin" "${MINGW_PREFIX}/lib" "/usr/lib/gcc/${MINGW_TARGET}/${GCC_VERSION_MAJOR}-posix")
endif()
if(NOT MINGW_QT_PLUGINS_DIR)
	foreach(dir "${MINGW_PREFIX}/lib/qt${QT_MAJOR_VERSION}/plugins" "${MINGW_PREFIX}/plugins")
		if(EXISTS "${dir}/platforms")
			set(MINGW_QT_PLUGINS_DIR "${dir}")
			break()
		endif()
	endforeach()
endif()
if(NOT MINGW_QCA_PLUGINS_DIR)
	set(MINGW_QCA_PLUGINS_DIR "${MINGW_PREFIX}/lib/qca-qt${QT_MAJOR_VERSION}/crypto")
endif()
find_program(UNIX2DOS NAMES unix2dos todos REQUIRED)

if(NOT MINGW_OBJDUMP)
	set(MINGW_OBJDUMP "${MINGW_TOOL_PREFIX}objdump")
endif()

if(VEYON_BUILD_WIN64)
	set(DLL_DDENGINE "ddengine64.dll")
else()
	set(DLL_DDENGINE "ddengine.dll")
endif()

add_custom_target(windows-binaries
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config $<CONFIGURATION>
	COMMAND rm -rf ${WINDOWS_INSTALL_FILES}*
	COMMAND mkdir -p ${WINDOWS_INSTALL_FILES}/interception
	COMMAND cp ${CMAKE_SOURCE_DIR}/3rdparty/interception/* ${WINDOWS_INSTALL_FILES}/interception
	COMMAND cp ${CMAKE_SOURCE_DIR}/3rdparty/ddengine/${DLL_DDENGINE} ${WINDOWS_INSTALL_FILES}
	COMMAND cp core/veyon-core.dll ${WINDOWS_INSTALL_FILES}
	COMMAND find . -mindepth 2 -name '${BRANDING_PRODUCT_SLUG}-*.exe' -exec cp '{}' ${WINDOWS_INSTALL_FILES}/ '\;'
	COMMAND find 3rdparty -name 'libvnc*.dll' -exec cp '{}' ${WINDOWS_INSTALL_FILES}/ '\;'
	COMMAND mkdir -p ${WINDOWS_INSTALL_FILES}/plugins
	COMMAND find plugins/ -name '*.dll' -exec cp '{}' ${WINDOWS_INSTALL_FILES}/plugins/ '\;'
	COMMAND find ${WINDOWS_INSTALL_FILES}/plugins -name 'lib*.dll' -exec mv '{}' ${WINDOWS_INSTALL_FILES} '\;'
	COMMAND mv ${WINDOWS_INSTALL_FILES}/plugins/vnchooks.dll ${WINDOWS_INSTALL_FILES}
	COMMAND mkdir -p ${WINDOWS_INSTALL_FILES}/translations
	COMMAND find translations -name '*.qm' -exec cp '{}' ${WINDOWS_INSTALL_FILES}/translations/ '\;'
	COMMAND mkdir -p ${WINDOWS_INSTALL_FILES}/crypto
	COMMAND cp ${MINGW_QCA_PLUGINS_DIR}/libqca-ossl.dll ${WINDOWS_INSTALL_FILES}/crypto
	COMMAND mkdir -p ${WINDOWS_INSTALL_FILES}/imageformats ${WINDOWS_INSTALL_FILES}/platforms ${WINDOWS_INSTALL_FILES}/styles ${WINDOWS_INSTALL_FILES}/tls
	COMMAND cp ${MINGW_QT_PLUGINS_DIR}/imageformats/qjpeg.dll ${WINDOWS_INSTALL_FILES}/imageformats
	COMMAND cp ${MINGW_QT_PLUGINS_DIR}/platforms/qwindows.dll ${WINDOWS_INSTALL_FILES}/platforms
	COMMAND cp ${MINGW_QT_PLUGINS_DIR}/styles/*.dll ${WINDOWS_INSTALL_FILES}/styles
	COMMAND cp ${MINGW_QT_PLUGINS_DIR}/tls/qopensslbackend.dll ${WINDOWS_INSTALL_FILES}/tls
	# copy all DLLs the binaries and plugins depend on from the MinGW environment
	COMMAND ${CMAKE_SOURCE_DIR}/tools/windows-deploy-dlls.sh ${MINGW_OBJDUMP} ${WINDOWS_INSTALL_FILES} ${MINGW_DLL_DIRS}
	COMMAND ${MINGW_TOOL_PREFIX}strip ${WINDOWS_INSTALL_FILES}/*.dll ${WINDOWS_INSTALL_FILES}/*.exe ${WINDOWS_INSTALL_FILES}/plugins/*.dll ${WINDOWS_INSTALL_FILES}/platforms/*.dll ${WINDOWS_INSTALL_FILES}/styles/*.dll ${WINDOWS_INSTALL_FILES}/crypto/*.dll
	COMMAND cp ${CMAKE_SOURCE_DIR}/COPYING ${WINDOWS_INSTALL_FILES}
	COMMAND cp ${CMAKE_SOURCE_DIR}/COPYING ${WINDOWS_INSTALL_FILES}/LICENSE.TXT
	COMMAND cp ${CMAKE_SOURCE_DIR}/README.md ${WINDOWS_INSTALL_FILES}/README.TXT
	COMMAND cp ${CMAKE_SOURCE_DIR}/core/resources/fonts/NotoSansTifinagh-OFL.txt ${WINDOWS_INSTALL_FILES}/NotoSansTifinagh-OFL.TXT
	COMMAND ${UNIX2DOS} ${WINDOWS_INSTALL_FILES}/*.TXT
	COMMAND cp -ra ${CMAKE_SOURCE_DIR}/nsis ${WINDOWS_INSTALL_FILES}
	COMMAND cp ${CMAKE_BINARY_DIR}/nsis/veyon.nsi ${WINDOWS_INSTALL_FILES}
	COMMAND find ${WINDOWS_INSTALL_FILES} -ls
)

add_custom_target(create-windows-installer
	COMMAND makensis ${WINDOWS_INSTALL_FILES}/veyon.nsi
	COMMAND mv ${WINDOWS_INSTALL_FILES}/${BRANDING_PRODUCT_SLUG}-*setup.exe .
	COMMAND rm -rf ${WINDOWS_INSTALL_FILES}
	DEPENDS windows-binaries
)

add_custom_target(prepare-dev-nsi
	COMMAND sed -i ${WINDOWS_INSTALL_FILES}/veyon.nsi -e "s,/SOLID lzma,zlib,g"
	DEPENDS windows-binaries)

add_custom_target(dev-nsi
	DEPENDS prepare-dev-nsi create-windows-installer
)

