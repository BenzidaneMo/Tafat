/*
 * QuizWindow.h - student window for answering a quiz
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

#include <QTimer>
#include <QWidget>

#include "Quiz.h"

class QAbstractButton;
class QLabel;
class QLineEdit;
class QPushButton;

class QuizWindow : public QWidget
{
	Q_OBJECT
public:
	explicit QuizWindow( const Quiz& quiz, QWidget* parent = nullptr );

	QuizAnswers answers() const;

Q_SIGNALS:
	void answersChanged( const QuizAnswers& answers, bool finished );

protected:
	void closeEvent( QCloseEvent* event ) override;

private:
	struct QuestionWidgets
	{
		QList<QAbstractButton*> optionButtons;
		QLineEdit* textEdit{nullptr};
	};

	void submit( bool confirm );
	void updateRemainingTime();

	const Quiz m_quiz;
	QMap<QString, QuestionWidgets> m_widgets;
	QLabel* m_timeLabel;
	QPushButton* m_submitButton;
	QTimer m_timer;
	int m_remainingSeconds;
	bool m_submitted{false};

};
