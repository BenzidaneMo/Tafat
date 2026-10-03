/*
 * ClassList.h - list of the students expected in a lesson
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

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

struct ClassListEntry
{
	QString name;
	QString group;

	bool operator==( const ClassListEntry& other ) const
	{
		return name == other.name && group == other.group;
	}
};

using ClassList = QList<ClassListEntry>;


namespace ClassListUtils
{

// reads a class list exported from a spreadsheet: one student per line with the
// name in the first column and optionally the class in the second column;
// separators ",", ";" or tab, quoted values, UTF-8 with or without BOM and a
// header line are supported
ClassList parseCsv( const QByteArray& data );

// a form of the name for comparisons: case, spaces, Arabic diacritics and
// letter variants and the order of the words do not matter
QString matchKey( const QString& name );

bool isSameStudent( const QString& a, const QString& b );

// students of the class list without a matching name in registeredNames
ClassList absentStudents( const ClassList& classList, const QStringList& registeredNames );

// whether name is on the class list
bool contains( const ClassList& classList, const QString& name );

QStringList toStringList( const ClassList& classList );
ClassList fromStringList( const QStringList& list );

}
