/*
 * QuizEditorDialogs.h - dialogs for managing and editing quizzes
 *
 * Copyright (c) 2026 Tafat contributors
 *
 * This file is part of Tafat, which is based on Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include <QDialog>

#include "Quiz.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QSpinBox;

// the teacher's saved quizzes, one JSON file per quiz
class QuizLibrary
{
public:
	static QString directory();
	static QList<Quiz> quizzes();
	static bool save( const Quiz& quiz );
	static bool remove( const Quiz& quiz );
};


class QuestionDialog : public QDialog
{
	Q_OBJECT
public:
	QuestionDialog( const QuizQuestion& question, QWidget* parent = nullptr );

	QuizQuestion question() const;

	void accept() override;

private:
	void updateHint();

	QuizQuestion m_question;
	QPlainTextEdit* m_textEdit;
	QComboBox* m_typeComboBox;
	QPlainTextEdit* m_answersEdit;
	QSpinBox* m_pointsSpinBox;
	QLabel* m_answersHint;

};


class QuizEditorDialog : public QDialog
{
	Q_OBJECT
public:
	QuizEditorDialog( const Quiz& quiz, QWidget* parent = nullptr );

	Quiz quiz() const;

	void accept() override;

private:
	void updateQuestionList();
	void editQuestion( int index );

	Quiz m_quiz;
	QLineEdit* m_titleEdit;
	QSpinBox* m_timeLimitSpinBox;
	QListWidget* m_questionList;

};


// entry point for the teacher: choose, create or edit the quiz to start
class QuizLauncherDialog : public QDialog
{
	Q_OBJECT
public:
	explicit QuizLauncherDialog( QWidget* parent = nullptr );

	Quiz selectedQuiz() const;

private:
	void reload( const QString& selectId = {} );
	void edit( const Quiz& quiz );

	QList<Quiz> m_quizzes;
	QListWidget* m_quizList;

};
