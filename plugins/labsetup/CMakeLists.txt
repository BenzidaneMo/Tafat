include(BuildVeyonPlugin)

build_veyon_plugin(labsetup
	NAME LabSetup
	SOURCES
	LabSetupPlugin.cpp
	LabSetupPlugin.h
	LabSetupPage.cpp
	LabSetupPage.h
	StudentSetup.cpp
	StudentSetup.h
	labsetup.qrc
	)
