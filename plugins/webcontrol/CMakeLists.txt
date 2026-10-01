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
	PolicyStore.cpp
	PolicyStore.h
	webcontrol.qrc
	)
