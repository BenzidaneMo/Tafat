/*
 * QuizResultsWindow.h - teacher window with the live results of a quiz
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

#include <QWidget>

#include "Quiz.h"

class QTableWidget;
class QTreeWidget;
class QLabel;

class QuizResultsWindow : public QWidget
{
	Q_OBJECT
public:
	explicit QuizResultsWindow( const Quiz& quiz, QWidget* parent = nullptr );

	const Quiz& quiz() const
	{
		return m_quiz;
	}

	// key identifies the computer, e.g. its host address
	void addParticipant( const QString& key, const QString& computerName, const QString& studentName );
	void updateAnswers( const QString& key, const QuizAnswers& answers, bool finished );

	void setEnded();

private:
	struct Participant
	{
		QString computerName;
		QString studentName;
		QuizAnswers answers;
		bool finished{false};
	};

	void refresh();
	void exportCsv();
	QString answerText( const QuizQuestion& question, const QuizAnswer& answer ) const;

	Quiz m_quiz;
	QStringList m_order;
	QMap<QString, Participant> m_participants;
	bool m_ended{false};

	QLabel* m_summaryLabel;
	QTableWidget* m_table;
	QTreeWidget* m_questionTree;

};
