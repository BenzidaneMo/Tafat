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
	StudentPackage.cpp
	StudentPackage.h
	StudentInstallerDialog.cpp
	StudentInstallerDialog.h
	AddComputersDialog.cpp
	AddComputersDialog.h
	ComputerScanner.cpp
	ComputerScanner.h
	labsetup.qrc
	)

if(VEYON_BUILD_WINDOWS)
	# LookupAccountSid()
	target_link_libraries(labsetup PRIVATE advapi32)
endif()
