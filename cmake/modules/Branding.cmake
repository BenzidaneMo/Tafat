# Branding.cmake - product identity of this Veyon-based distribution
#
# This is the single place to change the product name. Internal identifiers
# (VeyonCore, VEYON_* macros, CMake target names) intentionally keep the
# upstream "veyon" naming so upstream Veyon releases can still be merged.

# Name shown to users
set(BRANDING_PRODUCT_NAME "Tafat")
# Lowercase name used for executables, directories and packages
set(BRANDING_PRODUCT_SLUG "tafat")
# Organization name, used e.g. for configuration storage locations
set(BRANDING_ORGANIZATION "Tafat")
# Organization domain, used e.g. for macOS configuration storage locations
set(BRANDING_DOMAIN "benzidanemo.github.io")

# Name an executable target "veyon-<component>" as "<slug>-<component>" on disk
function(set_branded_output_name TARGET)
	string(REGEX REPLACE "^veyon-" "${BRANDING_PRODUCT_SLUG}-" output_name "${TARGET}")
	set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME "${output_name}")
endfunction()
