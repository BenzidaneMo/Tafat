/*
 * AppHistory.h - which applications a student used during the session
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

#include <QStringList>
#include <QVariant>

// Remembers when each application was first and last seen open. The server
// samples the open applications regularly; the teacher sees the history next
// to the applications that are open now.
class AppHistory
{
public:
	static constexpr int MaxEntries = 100;

	struct Entry
	{
		QString name;
		qint64 firstSeen{0}; // msecs since epoch
		qint64 lastSeen{0};
	};
	using Entries = QList<Entry>;

	void update( const QStringList& openApplications, qint64 now );
	void clear();

	const Entries& entries() const
	{
		return m_entries;
	}

	static QVariantList toVariant( const Entries& entries );
	static Entries fromVariant( const QVariantList& list );

private:
	Entries m_entries;

};
