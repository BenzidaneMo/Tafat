/*
 * AppHistory.cpp - which applications a student used during the session
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

#include <algorithm>

#include "AppHistory.h"


void AppHistory::update( const QStringList& openApplications, qint64 now )
{
	for( const auto& application : openApplications )
	{
		const auto it = std::find_if( m_entries.begin(), m_entries.end(),
									  [&]( const Entry& entry ) { return entry.name == application; } );
		if( it != m_entries.end() )
		{
			it->lastSeen = now;
		}
		else
		{
			m_entries.append( { application, now, now } );
		}
	}

	if( m_entries.size() > MaxEntries )
	{
		// forget the applications that were not seen for the longest time
		std::stable_sort( m_entries.begin(), m_entries.end(),
						  []( const Entry& a, const Entry& b ) { return a.lastSeen > b.lastSeen; } );
		m_entries = m_entries.mid( 0, MaxEntries );
	}
}



void AppHistory::clear()
{
	m_entries.clear();
}



QVariantList AppHistory::toVariant( const Entries& entries )
{
	QVariantList list;
	list.reserve( entries.size() );
	for( const auto& entry : entries )
	{
		list.append( QVariantMap{ { QStringLiteral("n"), entry.name },
								  { QStringLiteral("f"), entry.firstSeen },
								  { QStringLiteral("l"), entry.lastSeen } } );
	}
	return list;
}



AppHistory::Entries AppHistory::fromVariant( const QVariantList& list )
{
	Entries entries;
	for( const auto& item : list )
	{
		const auto map = item.toMap();
		const auto name = map.value( QStringLiteral("n") ).toString();
		if( name.isEmpty() == false && entries.size() < MaxEntries )
		{
			entries.append( { name, map.value( QStringLiteral("f") ).toLongLong(),
							  map.value( QStringLiteral("l") ).toLongLong() } );
		}
	}
	return entries;
}
