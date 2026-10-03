include(BuildVeyonPlugin)

build_veyon_plugin(returnwork
	NAME ReturnWork
	SOURCES
	ReturnWorkFeaturePlugin.cpp
	ReturnWorkFeaturePlugin.h
	ReturnWorkDialog.cpp
	ReturnWorkDialog.h
	ReturnWorkMatcher.cpp
	ReturnWorkMatcher.h
	ReturnWorkTransfer.cpp
	ReturnWorkTransfer.h
	returnwork.qrc
	)
