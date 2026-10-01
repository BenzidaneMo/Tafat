include(BuildVeyonPlugin)

find_package(PipeWire)
find_package(Qt${QT_MAJOR_VERSION} COMPONENTS DBus)

if(PipeWire_FOUND AND Qt${QT_MAJOR_VERSION}DBus_FOUND)

	qt_add_dbus_interface(DBUS_GENERATED_SOURCES
		dbus/org.freedesktop.portal.RemoteDesktop.xml
		OrgFreedesktopPortalRemoteDesktop
	)
	qt_add_dbus_interface(DBUS_GENERATED_SOURCES
		dbus/org.freedesktop.portal.ScreenCast.xml
		OrgFreedesktopPortalScreenCast
	)
	qt_add_dbus_interface(DBUS_GENERATED_SOURCES
		dbus/org.freedesktop.portal.Request.xml
		OrgFreedesktopPortalRequest
	)
	qt_add_dbus_interface(DBUS_GENERATED_SOURCES
		dbus/org.freedesktop.portal.Session.xml
		OrgFreedesktopPortalSession
	)

	build_veyon_plugin(pipewire-vnc-server
		NAME PipeWireVncServer
		SOURCES
		PipeWireVncServer.cpp
		PipeWireVncServer.h
		PipeWireFramebuffer.cpp
		PipeWireFramebuffer.h
		PortalSession.cpp
		PortalSession.h
		${DBUS_GENERATED_SOURCES}
	)

	target_link_libraries(pipewire-vnc-server PRIVATE
		PipeWire::PipeWire
		Qt${QT_MAJOR_VERSION}::DBus
	)

	if(WITH_BUNDLED_LIBVNC)
		target_link_libraries(pipewire-vnc-server PRIVATE vncserver)
	else()
		target_link_libraries(pipewire-vnc-server PRIVATE LibVNC::LibVNCServer)
	endif()

	target_compile_options(pipewire-vnc-server PRIVATE -Wno-parentheses)

	# use the branded server app ID in the pre-authorization script
	file(READ "${CMAKE_CURRENT_SOURCE_DIR}/veyon-preauth-kde.sh" preauth_script)
	string(REPLACE "io.veyon.veyon-server" "${BRANDING_SERVER_APP_ID}" preauth_script "${preauth_script}")
	set(preauth_script_file "${CMAKE_CURRENT_BINARY_DIR}/${BRANDING_PRODUCT_SLUG}-preauth-kde.sh")
	file(WRITE "${preauth_script_file}" "${preauth_script}")

	install(
		PROGRAMS "${preauth_script_file}"
		DESTINATION "${VEYON_INSTALL_DATA_DIR}"
		COMPONENT pipewire-vnc-server
	)
endif()
