/*
 * ClassList.cpp - list of the students expected in a lesson
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

#include <QSet>

#include "ClassList.h"


namespace
{

QStringList splitCsvLine( const QString& line, QChar separator )
{
	QStringList values;
	QString value;
	bool quoted = false;

	for( int i = 0; i < line.size(); ++i )
	{
		const auto c = line.at( i );
		if( quoted )
		{
			if( c == QLatin1Char('"') )
			{
				if( i + 1 < line.size() && line.at( i + 1 ) == QLatin1Char('"') )
				{
					value += c;
					++i;
				}
				else
				{
					quoted = false;
				}
			}
			else
			{
				value += c;
			}
		}
		else if( c == QLatin1Char('"') )
		{
			quoted = true;
		}
		else if( c == separator )
		{
			values.append( value.simplified() );
			value.clear();
		}
		else
		{
			value += c;
		}
	}

	values.append( value.simplified() );

	return values;
}



QChar detectSeparator( const QStringList& lines )
{
	for( const auto& line : lines )
	{
		if( line.trimmed().isEmpty() )
		{
			continue;
		}
		for( const auto separator : { QLatin1Char('\t'), QLatin1Char(';'), QLatin1Char(',') } )
		{
			if( line.contains( separator ) )
			{
				return separator;
			}
		}
		break;
	}

	return QLatin1Char(',');
}



bool isHeader( const QStringList& values )
{
	static const QSet<QString> headerWords{
		QStringLiteral("name"), QStringLiteral("names"), QStringLiteral("student"), QStringLiteral("students"),
		QStringLiteral("full name"), QStringLiteral("nom"), QStringLiteral("nom et prénom"),
		QStringLiteral("nom prénom"), QStringLiteral("prénom"), QStringLiteral("élève"), QStringLiteral("eleve"),
		QStringLiteral("الاسم"), QStringLiteral("الإسم"), QStringLiteral("الاسم واللقب"), QStringLiteral("اللقب والاسم"),
		QStringLiteral("التلميذ"), QStringLiteral("isem"), QStringLiteral("ⵉⵙⵎ")
	};

	return values.isEmpty() == false && headerWords.contains( values.first().toLower() );
}

}



ClassList ClassListUtils::parseCsv( const QByteArray& data )
{
	auto text = QString::fromUtf8( data );
	if( text.startsWith( QChar( 0xFEFF ) ) )
	{
		text.remove( 0, 1 );
	}
	text.replace( QStringLiteral("\r\n"), QStringLiteral("\n") );
	text.replace( QLatin1Char('\r'), QLatin1Char('\n') );

	const auto lines = text.split( QLatin1Char('\n') );
	const auto separator = detectSeparator( lines );

	ClassList classList;
	QSet<QString> seen;
	bool firstLine = true;

	for( const auto& line : lines )
	{
		if( line.trimmed().isEmpty() )
		{
			continue;
		}

		const auto values = splitCsvLine( line, separator );
		if( firstLine )
		{
			firstLine = false;
			if( isHeader( values ) )
			{
				continue;
			}
		}

		ClassListEntry entry{ values.value( 0 ), values.value( 1 ) };
		const auto key = matchKey( entry.name );
		if( key.isEmpty() || seen.contains( key ) )
		{
			continue;
		}

		seen.insert( key );
		classList.append( entry );
	}

	return classList;
}



QString ClassListUtils::matchKey( const QString& name )
{
	QString normalized;
	normalized.reserve( name.size() );

	for( const auto c : name.toCaseFolded() )
	{
		const auto code = c.unicode();
		// Arabic diacritics (tashkeel) and tatweel
		if( ( code >= 0x064B && code <= 0x0652 ) || code == 0x0670 || code == 0x0640 )
		{
			continue;
		}
		// Arabic letter variants written differently by students
		if( code == 0x0623 || code == 0x0625 || code == 0x0622 || code == 0x0671 )
		{
			normalized += QChar( 0x0627 ); // alef
		}
		else if( code == 0x0629 )
		{
			normalized += QChar( 0x0647 ); // teh marbuta → heh
		}
		else if( code == 0x0649 )
		{
			normalized += QChar( 0x064A ); // alef maksura → yeh
		}
		else if( c.isLetterOrNumber() )
		{
			normalized += c;
		}
		else
		{
			normalized += QLatin1Char(' ');
		}
	}

	// remove Latin accents (é → e) so that names typed without them still match
	normalized = normalized.normalized( QString::NormalizationForm_D );
	QString withoutMarks;
	withoutMarks.reserve( normalized.size() );
	for( const auto c : std::as_const( normalized ) )
	{
		if( c.category() != QChar::Mark_NonSpacing )
		{
			withoutMarks += c;
		}
	}

	auto words = withoutMarks.split( QLatin1Char(' '), Qt::SkipEmptyParts );
	words.sort();

	return words.join( QLatin1Char(' ') );
}



bool ClassListUtils::isSameStudent( const QString& a, const QString& b )
{
	const auto keyA = matchKey( a );
	return keyA.isEmpty() == false && keyA == matchKey( b );
}



ClassList ClassListUtils::absentStudents( const ClassList& classList, const QStringList& registeredNames )
{
	QSet<QString> registeredKeys;
	for( const auto& name : registeredNames )
	{
		registeredKeys.insert( matchKey( name ) );
	}

	ClassList absent;
	for( const auto& entry : classList )
	{
		if( registeredKeys.contains( matchKey( entry.name ) ) == false )
		{
			absent.append( entry );
		}
	}

	return absent;
}



bool ClassListUtils::contains( const ClassList& classList, const QString& name )
{
	const auto key = matchKey( name );
	for( const auto& entry : classList )
	{
		if( matchKey( entry.name ) == key )
		{
			return true;
		}
	}
	return false;
}



QStringList ClassListUtils::toStringList( const ClassList& classList )
{
	QStringList list;
	for( const auto& entry : classList )
	{
		list.append( entry.name + QLatin1Char('\t') + entry.group );
	}
	return list;
}



ClassList ClassListUtils::fromStringList( const QStringList& list )
{
	ClassList classList;
	for( const auto& item : list )
	{
		const auto values = item.split( QLatin1Char('\t') );
		if( values.value( 0 ).trimmed().isEmpty() == false )
		{
			classList.append( { values.value( 0 ), values.value( 1 ) } );
		}
	}
	return classList;
}
