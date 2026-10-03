include(BuildVeyonPlugin)

build_veyon_plugin(inventory
	NAME Inventory
	SOURCES
	InventoryFeaturePlugin.cpp
	InventoryFeaturePlugin.h
	InventoryWindow.cpp
	InventoryWindow.h
	SystemInventory.cpp
	SystemInventory.h
	inventory.qrc
	)
