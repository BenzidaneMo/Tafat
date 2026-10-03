/*
 * Quiz.cpp - quiz and survey data model
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

#include <QJsonArray>
#include <QSet>
#include <QUuid>

#include "Quiz.h"


static QString normalizedText( const QString& text )
{
	return text.simplified().toCaseFolded();
}



static QJsonArray intArray( const QList<int>& values )
{
	QJsonArray array;
	for( const auto value : values )
	{
		array.append( value );
	}
	return array;
}



static QList<int> intList( const QJsonValue& value )
{
	QList<int> result;
	for( const auto& item : value.toArray() )
	{
		result.append( item.toInt() );
	}
	return result;
}



static QStringList stringList( const QJsonValue& value )
{
	QStringList result;
	for( const auto& item : value.toArray() )
	{
		result.append( item.toString() );
	}
	return result;
}



bool QuizQuestion::isGraded() const
{
	return type == Type::Text ? acceptedAnswers.isEmpty() == false : correctOptions.isEmpty() == false;
}



Quiz Quiz::fromJson( const QJsonObject& json )
{
	Quiz quiz;
	quiz.id = json[QStringLiteral("id")].toString();
	quiz.title = json[QStringLiteral("title")].toString();
	quiz.timeLimitMinutes = json[QStringLiteral("timeLimitMinutes")].toInt();

	for( const auto& item : json[QStringLiteral("questions")].toArray() )
	{
		const auto object = item.toObject();
		QuizQuestion question;
		question.id = object[QStringLiteral("id")].toString();
		question.type = QuizQuestion::Type( object[QStringLiteral("type")].toInt() );
		question.text = object[QStringLiteral("text")].toString();
		question.options = stringList( object[QStringLiteral("options")] );
		question.correctOptions = intList( object[QStringLiteral("correctOptions")] );
		question.acceptedAnswers = stringList( object[QStringLiteral("acceptedAnswers")] );
		question.points = object[QStringLiteral("points")].toInt( 1 );
		quiz.questions.append( question );
	}

	if( quiz.id.isEmpty() )
	{
		quiz.id = QUuid::createUuid().toString( QUuid::WithoutBraces );
	}

	return quiz;
}



QJsonObject Quiz::toJson() const
{
	QJsonArray questionArray;
	for( const auto& question : questions )
	{
		QJsonObject object;
		object[QStringLiteral("id")] = question.id;
		object[QStringLiteral("type")] = int( question.type );
		object[QStringLiteral("text")] = question.text;
		object[QStringLiteral("options")] = QJsonArray::fromStringList( question.options );
		object[QStringLiteral("correctOptions")] = intArray( question.correctOptions );
		object[QStringLiteral("acceptedAnswers")] = QJsonArray::fromStringList( question.acceptedAnswers );
		object[QStringLiteral("points")] = question.points;
		questionArray.append( object );
	}

	QJsonObject json;
	json[QStringLiteral("id")] = id;
	json[QStringLiteral("title")] = title;
	json[QStringLiteral("timeLimitMinutes")] = timeLimitMinutes;
	json[QStringLiteral("questions")] = questionArray;
	return json;
}



Quiz Quiz::withoutSolutions() const
{
	auto quiz = *this;
	for( auto& question : quiz.questions )
	{
		question.correctOptions.clear();
		question.acceptedAnswers.clear();
	}
	return quiz;
}



int Quiz::maximumScore() const
{
	int score = 0;
	for( const auto& question : questions )
	{
		if( question.isGraded() )
		{
			score += question.points;
		}
	}
	return score;
}



bool Quiz::isGraded() const
{
	return maximumScore() > 0;
}



bool Quiz::isCorrect( const QuizQuestion& question, const QuizAnswer& answer ) const
{
	if( question.isGraded() == false )
	{
		return false;
	}

	if( question.type == QuizQuestion::Type::Text )
	{
		const auto given = normalizedText( answer.text );
		for( const auto& accepted : question.acceptedAnswers )
		{
			if( given == normalizedText( accepted ) )
			{
				return true;
			}
		}
		return false;
	}

	const auto selected = QSet<int>( answer.selectedOptions.begin(), answer.selectedOptions.end() );
	const auto correct = QSet<int>( question.correctOptions.begin(), question.correctOptions.end() );

	if( question.type == QuizQuestion::Type::SingleChoice )
	{
		return selected.size() == 1 && correct.contains( *selected.begin() );
	}

	return selected == correct;
}



int Quiz::score( const QuizAnswers& answers ) const
{
	int score = 0;
	for( const auto& question : questions )
	{
		if( isCorrect( question, answers.value( question.id ) ) )
		{
			score += question.points;
		}
	}
	return score;
}



int Quiz::answeredCount( const QuizAnswers& answers ) const
{
	int count = 0;
	for( const auto& question : questions )
	{
		if( answers.value( question.id ).isEmpty() == false )
		{
			++count;
		}
	}
	return count;
}



QList<int> Quiz::optionTally( const QuizQuestion& question, const QList<QuizAnswers>& allAnswers ) const
{
	QList<int> tally;
	tally.reserve( question.options.size() );
	for( int i = 0; i < question.options.size(); ++i )
	{
		tally.append( 0 );
	}

	for( const auto& answers : allAnswers )
	{
		for( const auto option : answers.value( question.id ).selectedOptions )
		{
			if( option >= 0 && option < tally.size() )
			{
				++tally[option];
			}
		}
	}

	return tally;
}



QJsonObject Quiz::answersToJson( const QuizAnswers& answers )
{
	QJsonObject json;
	for( auto it = answers.constBegin(); it != answers.constEnd(); ++it )
	{
		QJsonObject answer;
		answer[QStringLiteral("selectedOptions")] = intArray( it->selectedOptions );
		answer[QStringLiteral("text")] = it->text;
		json[it.key()] = answer;
	}
	return json;
}



QuizAnswers Quiz::answersFromJson( const QJsonObject& json )
{
	QuizAnswers answers;
	for( auto it = json.constBegin(); it != json.constEnd(); ++it )
	{
		const auto object = it.value().toObject();
		answers[it.key()] = { intList( object[QStringLiteral("selectedOptions")] ),
							  object[QStringLiteral("text")].toString() };
	}
	return answers;
}



Quiz Quiz::pollTemplate( const QString& question, const QStringList& options )
{
	Quiz quiz;
	quiz.id = QUuid::createUuid().toString( QUuid::WithoutBraces );
	quiz.title = question;
	quiz.questions.append( { QUuid::createUuid().toString( QUuid::WithoutBraces ),
							 QuizQuestion::Type::SingleChoice, question, options, {}, {}, 1 } );
	return quiz;
}
