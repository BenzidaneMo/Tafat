/*
 * QuizTest.cpp - tests for the quiz data model
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

#include <QtTest>

#include "Quiz.h"

using Type = QuizQuestion::Type;

class QuizTest : public QObject
{
	Q_OBJECT
private:
	static Quiz sampleQuiz()
	{
		Quiz quiz;
		quiz.id = QStringLiteral("quiz");
		quiz.title = QStringLiteral("Geography");
		quiz.timeLimitMinutes = 10;
		quiz.questions = {
			{ QStringLiteral("q1"), Type::SingleChoice, QStringLiteral("Capital of Algeria?"),
			  { QStringLiteral("Oran"), QStringLiteral("Algiers"), QStringLiteral("Annaba") }, { 1 }, {}, 2 },
			{ QStringLiteral("q2"), Type::MultipleChoice, QStringLiteral("Mediterranean countries?"),
			  { QStringLiteral("Algeria"), QStringLiteral("Mali"), QStringLiteral("Tunisia") }, { 0, 2 }, {}, 1 },
			{ QStringLiteral("q3"), Type::Text, QStringLiteral("Longest river of Africa?"),
			  {}, {}, { QStringLiteral("Nile"), QStringLiteral("النيل") }, 1 },
			{ QStringLiteral("q4"), Type::Text, QStringLiteral("Your opinion?"), {}, {}, {}, 1 },
		};
		return quiz;
	}

private Q_SLOTS:
	void jsonRoundTrip()
	{
		const auto quiz = sampleQuiz();
		const auto copy = Quiz::fromJson( quiz.toJson() );

		QCOMPARE( copy.toJson(), quiz.toJson() );
		QCOMPARE( copy.questions.size(), 4 );
		QCOMPARE( copy.questions[2].acceptedAnswers[1], QStringLiteral("النيل") );
	}

	void solutionsAreRemovedForStudents()
	{
		const auto studentQuiz = sampleQuiz().withoutSolutions();
		for( const auto& question : studentQuiz.questions )
		{
			QVERIFY( question.correctOptions.isEmpty() );
			QVERIFY( question.acceptedAnswers.isEmpty() );
		}
		QCOMPARE( studentQuiz.questions[0].options.size(), 3 );
	}

	void maximumScoreCountsOnlyGradedQuestions()
	{
		QCOMPARE( sampleQuiz().maximumScore(), 4 );
		QVERIFY( sampleQuiz().isGraded() );
		QVERIFY( Quiz::pollTemplate( QStringLiteral("Ready?"), { QStringLiteral("Yes"), QStringLiteral("No") } ).isGraded() == false );
	}

	void grading()
	{
		const auto quiz = sampleQuiz();

		QuizAnswers allCorrect{
			{ QStringLiteral("q1"), { { 1 }, {} } },
			{ QStringLiteral("q2"), { { 2, 0 }, {} } },
			{ QStringLiteral("q3"), { {}, QStringLiteral("  nile ") } },
			{ QStringLiteral("q4"), { {}, QStringLiteral("I liked it") } },
		};
		QCOMPARE( quiz.score( allCorrect ), 4 );
		QCOMPARE( quiz.answeredCount( allCorrect ), 4 );

		QuizAnswers someWrong{
			{ QStringLiteral("q1"), { { 0 }, {} } },
			{ QStringLiteral("q2"), { { 0 }, {} } },	// incomplete selection
			{ QStringLiteral("q3"), { {}, QStringLiteral("النيل") } },
		};
		QCOMPARE( quiz.score( someWrong ), 1 );
		QCOMPARE( quiz.answeredCount( someWrong ), 3 );

		QCOMPARE( quiz.score( {} ), 0 );
	}

	void answersJsonRoundTrip()
	{
		const QuizAnswers answers{
			{ QStringLiteral("q1"), { { 1 }, {} } },
			{ QStringLiteral("q3"), { {}, QStringLiteral("Nile") } },
		};
		const auto copy = Quiz::answersFromJson( Quiz::answersToJson( answers ) );
		QCOMPARE( copy.value( QStringLiteral("q1") ).selectedOptions, QList<int>{ 1 } );
		QCOMPARE( copy.value( QStringLiteral("q3") ).text, QStringLiteral("Nile") );
	}

	void optionTally()
	{
		const auto quiz = sampleQuiz();
		const QList<QuizAnswers> allAnswers{
			{ { QStringLiteral("q2"), { { 0, 2 }, {} } } },
			{ { QStringLiteral("q2"), { { 0 }, {} } } },
			{ { QStringLiteral("q2"), { { 7 }, {} } } },	// invalid option index is ignored
			{},
		};
		QCOMPARE( quiz.optionTally( quiz.questions[1], allAnswers ), QList<int>( { 2, 0, 1 } ) );
	}
};

QTEST_GUILESS_MAIN(QuizTest)

#include "QuizTest.moc"
