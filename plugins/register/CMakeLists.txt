include(BuildVeyonPlugin)

build_veyon_plugin(register
	NAME Register
	SOURCES
	RegisterFeaturePlugin.cpp
	RegisterFeaturePlugin.h
	RegisterWindows.cpp
	RegisterWindows.h
	register.qrc
	)
