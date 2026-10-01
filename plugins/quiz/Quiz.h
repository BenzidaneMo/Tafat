/*
 * Quiz.h - quiz and survey data model
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

#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QStringList>

struct QuizQuestion
{
	enum class Type
	{
		SingleChoice,
		MultipleChoice,
		Text
	};

	QString id;
	Type type{Type::SingleChoice};
	QString text;
	QStringList options;
	QList<int> correctOptions;		// for choice questions
	QStringList acceptedAnswers;	// for text questions
	int points{1};

	// whether the question has a solution and counts for the score
	bool isGraded() const;
};

struct QuizAnswer
{
	QList<int> selectedOptions;
	QString text;

	bool isEmpty() const
	{
		return selectedOptions.isEmpty() && text.trimmed().isEmpty();
	}
};

// answers of one student, by question ID
using QuizAnswers = QMap<QString, QuizAnswer>;

class Quiz
{
public:
	QString id;
	QString title;
	int timeLimitMinutes{0};
	QList<QuizQuestion> questions;

	static Quiz fromJson( const QJsonObject& json );
	QJsonObject toJson() const;

	// the quiz as sent to students: without solutions
	Quiz withoutSolutions() const;

	int maximumScore() const;
	bool isGraded() const;

	int score( const QuizAnswers& answers ) const;
	bool isCorrect( const QuizQuestion& question, const QuizAnswer& answer ) const;
	int answeredCount( const QuizAnswers& answers ) const;

	// number of students who selected each option of a choice question
	QList<int> optionTally( const QuizQuestion& question, const QList<QuizAnswers>& allAnswers ) const;

	static QJsonObject answersToJson( const QuizAnswers& answers );
	static QuizAnswers answersFromJson( const QJsonObject& json );

	// a one-question survey to start from
	static Quiz pollTemplate( const QString& question, const QStringList& options );

};
