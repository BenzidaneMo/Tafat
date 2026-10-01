find_program(XDG_DESKTOP_MENU_EXECUTABLE xdg-desktop-menu)
set(XDG_APPS_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/applications)

# NAME is the installed base name of the desktop file and icons
macro(xdg_install NAME DESKTOP_FILE ICON_XPM ICON_PNG ICON_SVG)
	install(FILES ${DESKTOP_FILE} DESTINATION ${XDG_APPS_INSTALL_DIR} RENAME ${NAME}.desktop)
	install(FILES ${ICON_XPM} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pixmaps RENAME ${NAME}.xpm)
	install(FILES ${ICON_PNG} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps RENAME ${NAME}.png)
	install(FILES ${ICON_SVG} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps RENAME ${NAME}.svg)
	#if(XDG_DESKTOP_MENU_EXECUTABLE)
	#	install(CODE "execute_process(COMMAND ${XDG_DESKTOP_MENU_EXECUTABLE} install --novendor ${XDG_APPS_INSTALL_DIR}/${DESKTOP_FILE})")
	#endif()
endmacro()
