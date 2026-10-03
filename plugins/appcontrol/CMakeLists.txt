include(BuildVeyonPlugin)

build_veyon_plugin(appcontrol
	NAME AppControl
	SOURCES
	AppControlFeaturePlugin.cpp
	AppControlFeaturePlugin.h
	AppControlDialog.cpp
	AppControlDialog.h
	ProcessControl.cpp
	ProcessControl.h
	PrintBlocker.cpp
	PrintBlocker.h
	RunningAppsWindow.cpp
	RunningAppsWindow.h
	UsbStorageBlocker.cpp
	UsbStorageBlocker.h
	appcontrol.qrc
	)
