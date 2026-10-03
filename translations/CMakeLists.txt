include(CreateTranslations)
include(ImportQtTranslations)

file(GLOB veyon_translations ${CMAKE_CURRENT_SOURCE_DIR}/veyon_*.ts)
file(GLOB_RECURSE veyon_sources ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.h ${CMAKE_SOURCE_DIR}/*.ui ${CMAKE_SOURCE_DIR}/*.qml)
string(REGEX REPLACE "${CMAKE_SOURCE_DIR}/3rdparty[^;]+;?" "" veyon_sources "${veyon_sources}")
string(REGEX REPLACE "${CMAKE_SOURCE_DIR}/addons[^;]+;?" "" veyon_sources "${veyon_sources}")

# texts of the plugins added by this distribution have their own catalogs
# (tafat_*.ts) so that the upstream catalogs stay unchanged
set(tafat_plugins appcontrol classchat quiz register returnwork webcontrol)
set(tafat_sources "")
foreach(plugin ${tafat_plugins})
	file(GLOB_RECURSE plugin_sources ${CMAKE_SOURCE_DIR}/plugins/${plugin}/*.cpp ${CMAKE_SOURCE_DIR}/plugins/${plugin}/*.h ${CMAKE_SOURCE_DIR}/plugins/${plugin}/*.ui)
	list(APPEND tafat_sources ${plugin_sources})
	list(REMOVE_ITEM veyon_sources ${plugin_sources})
endforeach()
file(GLOB tafat_translations ${CMAKE_CURRENT_SOURCE_DIR}/tafat_*.ts)

create_translations(veyon "${veyon_translations}" "${veyon_sources}")
create_translations(tafat "${tafat_translations}" "${tafat_sources}")
import_qt_translations()
