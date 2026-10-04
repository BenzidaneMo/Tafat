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
	auto names = m_stars.keys();
	QCollator collator;
	collator.setCaseSensitivity( Qt::CaseInsensitive );
	std::sort( names.begin(), names.end(), [&collator]( const QString& a, const QString& b ) {
		return collator.compare( a, b ) < 0;
	} );
	return names;
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
