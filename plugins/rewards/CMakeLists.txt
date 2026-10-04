include(BuildVeyonPlugin)

build_veyon_plugin(rewards
	NAME Rewards
	SOURCES
	RewardBook.cpp
	RewardBook.h
	RewardPopup.cpp
	RewardPopup.h
	RewardsFeaturePlugin.cpp
	RewardsFeaturePlugin.h
	RewardsWindow.cpp
	RewardsWindow.h
	rewards.qrc
	)
