if(WITH_TESTS AND VEYON_BUILD_LINUX)

	# the code under test needs the C++ standard of the other targets (GCC 10 defaults to C++14)
	set(CMAKE_CXX_STANDARD 20)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)

	#
	# IntegrationTest: a master client talks to a real server (Xvfb, key file
	# authentication); needs root, run with "ctest -L integration"
	#
	add_executable(FeatureRoundTripTest
		FeatureRoundTripTest.cpp
	)

	# one level below the build directory, like the applications, so that
	# VEYON_PLUGIN_DIR ("../<lib dir>") finds the same plugin directory
	set_target_properties(FeatureRoundTripTest PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/integration
	)

	target_link_libraries(FeatureRoundTripTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Widgets
		veyon-core
	)

	add_test(NAME IntegrationTest
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run-integration-test.sh
			${CMAKE_BINARY_DIR}
			${VEYON_LIB_DIR}
			$<TARGET_FILE:veyon-server>
			$<TARGET_FILE:veyon-cli>
			$<TARGET_FILE:FeatureRoundTripTest>
	)

	set_tests_properties(IntegrationTest PROPERTIES
		LABELS integration
		TIMEOUT 300
		SKIP_RETURN_CODE 77
	)

endif()
