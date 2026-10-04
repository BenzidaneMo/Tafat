/*
 * RewardBook.h - stars given to the students
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

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariantMap>

// Stars per student, kept by the teacher. A student is identified by the name
// entered in "Register" or, without it, by the computer name.
class RewardBook
{
public:
	static QString key( const QString& studentName, const QString& computerName );

	// returns the new number of stars
	int add( const QString& student, int stars = 1 );
	int remove( const QString& student, int stars = 1 );
	int stars( const QString& student ) const;
	void reset();

	QStringList students() const;
	bool isEmpty() const
	{
		return m_stars.isEmpty();
	}

	QVariantMap toVariantMap() const;
	static RewardBook fromVariantMap( const QVariantMap& map );

	// "Student","Stars" lines, sorted by name, without byte order mark
	QString toCsv( const QString& studentHeader, const QString& starsHeader ) const;

	static constexpr int MaximumStars = 999;

private:
	QMap<QString, int> m_stars;

};



// The books of all classes taught on these computers, so that the next class
// does not see the stars given to the previous one on the same computers.
// There is always at least one class.
class RewardClasses
{
public:
	explicit RewardClasses( const QString& defaultClass );

	QStringList classes() const;
	QString currentClass() const
	{
		return m_current;
	}

	// selects the class and creates it if it does not exist yet
	bool setCurrentClass( const QString& name );
	// removes the class unless it is the last one
	bool removeClass( const QString& name );

	RewardBook& book()
	{
		return m_books[m_current];
	}
	const RewardBook& book() const
	{
		// the current class always exists
		return *m_books.constFind( m_current );
	}

	QVariantMap toVariantMap() const;
	static RewardClasses fromVariantMap( const QVariantMap& map, const QString& defaultClass );

	static constexpr int MaximumClasses = 100;

private:
	QMap<QString, RewardBook> m_books;
	QString m_current;

};
