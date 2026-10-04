/*
 * RewardBook.cpp - stars given to the students
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

#include <QCollator>

#include "RewardBook.h"


// sorts names case-insensitively, with numbers in their natural order ("2AS2" before
// "2AS10"); QCollator's numeric mode is not available on all platforms
static QStringList sortedNames( QStringList names )
{
	QCollator collator;
	collator.setCaseSensitivity( Qt::CaseInsensitive );

	const auto naturalLess = [&collator]( const QString& a, const QString& b ) {
		int i = 0;
		int j = 0;
		while( i < a.size() && j < b.size() )
		{
			const auto digits = []( const QString& text, int start ) {
				auto end = start;
				while( end < text.size() && text[end].isDigit() )
				{
					++end;
				}
				return end;
			};
			const auto aEnd = digits( a, i );
			const auto bEnd = digits( b, j );
			if( aEnd > i && bEnd > j )
			{
				const auto aNumber = a.mid( i, aEnd - i ).toLongLong();
				const auto bNumber = b.mid( j, bEnd - j ).toLongLong();
				if( aNumber != bNumber )
				{
					return aNumber < bNumber;
				}
				i = aEnd;
				j = bEnd;
				continue;
			}

			const auto result = collator.compare( a.mid( i, 1 ), b.mid( j, 1 ) );
			if( result != 0 )
			{
				return result < 0;
			}
			++i;
			++j;
		}
		if( a.size() - i != b.size() - j )
		{
			return a.size() - i < b.size() - j;
		}
		return collator.compare( a, b ) < 0;
	};

	std::sort( names.begin(), names.end(), naturalLess );
	return names;
}



QString RewardBook::key( const QString& studentName, const QString& computerName )
{
	const auto name = studentName.simplified();
	return name.isEmpty() ? computerName.simplified() : name;
}



int RewardBook::add( const QString& student, int stars )
{
	if( student.isEmpty() )
	{
		return 0;
	}
	auto& count = m_stars[student];
	count = qBound( 0, count + stars, MaximumStars );
	return count;
}



int RewardBook::remove( const QString& student, int stars )
{
	if( m_stars.contains( student ) == false )
	{
		return 0;
	}
	const auto count = add( student, -stars );
	if( count == 0 )
	{
		m_stars.remove( student );
	}
	return count;
}



int RewardBook::stars( const QString& student ) const
{
	return m_stars.value( student, 0 );
}



void RewardBook::reset()
{
	m_stars.clear();
}



QStringList RewardBook::students() const
{
	return sortedNames( m_stars.keys() );
}



QVariantMap RewardBook::toVariantMap() const
{
	QVariantMap map;
	for( auto it = m_stars.constBegin(); it != m_stars.constEnd(); ++it )
	{
		map[it.key()] = it.value();
	}
	return map;
}



RewardBook RewardBook::fromVariantMap( const QVariantMap& map )
{
	RewardBook book;
	for( auto it = map.constBegin(); it != map.constEnd(); ++it )
	{
		book.add( it.key(), it.value().toInt() );
		if( book.stars( it.key() ) == 0 )
		{
			book.m_stars.remove( it.key() );
		}
	}
	return book;
}



QString RewardBook::toCsv( const QString& studentHeader, const QString& starsHeader ) const
{
	const auto quote = []( QString field ) {
		return QLatin1Char('"') + field.replace( QLatin1Char('"'), QStringLiteral("\"\"") ) + QLatin1Char('"');
	};

	QStringList lines{ quote( studentHeader ) + QLatin1Char(',') + quote( starsHeader ) };
	for( const auto& student : students() )
	{
		lines.append( quote( student ) + QLatin1Char(',') + QString::number( stars( student ) ) );
	}
	return lines.join( QLatin1Char('\n') ) + QLatin1Char('\n');
}



RewardClasses::RewardClasses( const QString& defaultClass ) :
	m_current( defaultClass.simplified() )
{
	m_books[m_current];
}



QStringList RewardClasses::classes() const
{
	return sortedNames( m_books.keys() );
}



bool RewardClasses::setCurrentClass( const QString& name )
{
	const auto className = name.simplified();
	if( className.isEmpty() ||
		( m_books.contains( className ) == false && m_books.size() >= MaximumClasses ) )
	{
		return false;
	}

	m_books[className];
	m_current = className;
	return true;
}



bool RewardClasses::removeClass( const QString& name )
{
	if( m_books.size() <= 1 || m_books.remove( name ) == 0 )
	{
		return false;
	}

	if( m_current == name )
	{
		m_current = classes().constFirst();
	}
	return true;
}



QVariantMap RewardClasses::toVariantMap() const
{
	QVariantMap books;
	for( auto it = m_books.constBegin(); it != m_books.constEnd(); ++it )
	{
		books[it.key()] = it.value().toVariantMap();
	}
	return { { QStringLiteral("Classes"), books }, { QStringLiteral("Current"), m_current } };
}



RewardClasses RewardClasses::fromVariantMap( const QVariantMap& map, const QString& defaultClass )
{
	RewardClasses result( defaultClass );

	const auto books = map.value( QStringLiteral("Classes") ).toMap();
	for( auto it = books.constBegin(); it != books.constEnd(); ++it )
	{
		if( result.setCurrentClass( it.key() ) )
		{
			result.book() = RewardBook::fromVariantMap( it.value().toMap() );
		}
	}

	// the default class only stays if nothing else was stored
	if( result.m_books.size() > 1 && books.contains( defaultClass.simplified() ) == false )
	{
		result.m_books.remove( defaultClass.simplified() );
	}

	const auto current = map.value( QStringLiteral("Current") ).toString().simplified();
	result.m_current = result.m_books.contains( current ) ? current : result.classes().constFirst();

	return result;
}
