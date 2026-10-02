include(BuildVeyonPlugin)

build_veyon_plugin(classchat
	NAME ClassChat
	SOURCES
	ChatLog.cpp
	ChatLog.h
	ClassChatFeaturePlugin.cpp
	ClassChatFeaturePlugin.h
	ClassChatWindows.cpp
	ClassChatWindows.h
	classchat.qrc
	)
