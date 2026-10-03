include(BuildVeyonPlugin)

build_veyon_plugin(quiz
	NAME Quiz
	SOURCES
	QuizFeaturePlugin.cpp
	QuizFeaturePlugin.h
	Quiz.cpp
	Quiz.h
	QuizEditorDialogs.cpp
	QuizEditorDialogs.h
	QuizResultsWindow.cpp
	QuizResultsWindow.h
	QuizWindow.cpp
	QuizWindow.h
	quiz.qrc
	)
