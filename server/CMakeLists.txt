include(BuildVeyonApplication)

build_veyon_application(veyon-server
	NAME Server
	SOURCES
	src/ComputerControlClient.cpp
	src/ComputerControlClient.h
	src/ComputerControlServer.cpp
	src/ComputerControlServer.h
	src/main.cpp
	src/ServerAccessControlManager.cpp
	src/ServerAccessControlManager.h
	src/ServerAuthenticationManager.cpp
	src/ServerAuthenticationManager.h
	src/VeyonServerProtocol.cpp
	src/VeyonServerProtocol.h
	src/VncProxyConnection.cpp
	src/VncProxyConnectionFactory.h
	src/VncProxyConnection.h
	src/VncProxyServer.cpp
	src/VncProxyServer.h
	src/VncServer.cpp
	src/VncServer.h)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/data/io.veyon.veyon-server.desktop.in"
	"${CMAKE_CURRENT_BINARY_DIR}/data/${BRANDING_SERVER_APP_ID}.desktop"
	@ONLY
)
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/data/io.veyon.veyon-server.service.in"
	"${CMAKE_CURRENT_BINARY_DIR}/data/${BRANDING_SERVER_APP_ID}.service"
	@ONLY
)

install(
	FILES "${CMAKE_CURRENT_BINARY_DIR}/data/${BRANDING_SERVER_APP_ID}.desktop"
	DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
	COMPONENT pipewire-vnc-server
)

install(
	FILES "${CMAKE_CURRENT_BINARY_DIR}/data/${BRANDING_SERVER_APP_ID}.service"
	DESTINATION "${CMAKE_INSTALL_DATADIR}/dbus-1/services"
	COMPONENT pipewire-vnc-server
)
