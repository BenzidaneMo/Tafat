include(BuildVeyonPlugin)

build_veyon_plugin(webcontrol
	NAME WebControl
	SOURCES
	WebControlFeaturePlugin.cpp
	WebControlFeaturePlugin.h
	WebControlDialog.cpp
	WebControlDialog.h
	WebPolicy.cpp
	WebPolicy.h
	InternetBlocker.cpp
	InternetBlocker.h
	PolicyStore.cpp
	PolicyStore.h
	webcontrol.qrc
	)
