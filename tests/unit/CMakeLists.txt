if(WITH_TESTS AND VEYON_BUILD_LINUX)

	find_package(X11 REQUIRED)

	#
	# LinuxKeyboardMappingTest: tests keysym → linux keycode mapping tables
	# and UTF-8 → linux keycode conversion.
	#
	add_executable(LinuxKeyboardMappingTest
		LinuxKeyboardMappingTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/linux/LinuxKeyboardInput.cpp
		${CMAKE_SOURCE_DIR}/3rdparty/libfakekey/src/libfakekey.c
	)

	target_include_directories(LinuxKeyboardMappingTest PRIVATE
		${CMAKE_SOURCE_DIR}/plugins/platform/linux
		${CMAKE_SOURCE_DIR}/3rdparty/libfakekey
		${CMAKE_SOURCE_DIR}/core/src
		${CMAKE_SOURCE_DIR}/core/src/Configuration
	)

	target_link_libraries(LinuxKeyboardMappingTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		veyon-core
		${X11_LIBRARIES}
		${X11_XTest_LIB}
	)

	add_test(NAME LinuxKeyboardMappingTest COMMAND LinuxKeyboardMappingTest)

	#
	# PipeWireVncServerTest: tests BGRx→RGB conversion, damage rect clamping,
	# and portal session token/path generation.
	#
	add_executable(PipeWireVncServerTest
		PipeWireVncServerTest.cpp
	)

	target_link_libraries(PipeWireVncServerTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
	)

	add_test(NAME PipeWireVncServerTest COMMAND PipeWireVncServerTest)

	add_executable(VncProtocolRobustnessTest
		VncProtocolRobustnessTest.cpp
	)

	target_link_libraries(VncProtocolRobustnessTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		veyon-core
	)

	add_test(NAME VncProtocolRobustnessTest COMMAND VncProtocolRobustnessTest)

	#
	# ProcessControlTest: which applications the application control closes
	#
	add_executable(ProcessControlTest
		ProcessControlTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/appcontrol/ProcessControl.cpp
	)

	target_include_directories(ProcessControlTest PRIVATE
		${CMAKE_SOURCE_DIR}/plugins/appcontrol
	)

	target_link_libraries(ProcessControlTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		veyon-core
	)

	add_test(NAME ProcessControlTest COMMAND ProcessControlTest)

	#
	# WebPolicyTest: browser policies of the website control
	#
	add_executable(WebPolicyTest
		WebPolicyTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/webcontrol/WebPolicy.cpp
		${CMAKE_SOURCE_DIR}/plugins/webcontrol/PolicyStore.cpp
	)

	target_include_directories(WebPolicyTest PRIVATE
		${CMAKE_SOURCE_DIR}/plugins/webcontrol
	)

	target_link_libraries(WebPolicyTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		veyon-core
	)

	add_test(NAME WebPolicyTest COMMAND WebPolicyTest)

	#
	# QuizTest: quiz data model, grading and tallies
	#
	add_executable(QuizTest
		QuizTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/quiz/Quiz.cpp
	)

	target_include_directories(QuizTest PRIVATE
		${CMAKE_SOURCE_DIR}/plugins/quiz
	)

	target_link_libraries(QuizTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
	)

	add_test(NAME QuizTest COMMAND QuizTest)

	#
	# ClassChatTest: chat log of the hand raise and chat feature
	#
	add_executable(ClassChatTest
		ClassChatTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/classchat/ChatLog.cpp
	)

	target_include_directories(ClassChatTest PRIVATE
		${CMAKE_SOURCE_DIR}/plugins/classchat
	)

	target_link_libraries(ClassChatTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
	)

	add_test(NAME ClassChatTest COMMAND ClassChatTest)

	#
	# ClassListTest: class list import and name matching of the register
	#
	add_executable(ClassListTest
		ClassListTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/register/ClassList.cpp
	)

	target_include_directories(ClassListTest PRIVATE
		${CMAKE_SOURCE_DIR}/plugins/register
	)

	target_link_libraries(ClassListTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
	)

	add_test(NAME ClassListTest COMMAND ClassListTest)

endif()
