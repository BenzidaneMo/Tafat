/*
 * ReturnWorkMatcher.cpp - finds the folder of collected files of a student
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

#include <functional>

#include <QRegularExpression>

#include "ReturnWorkMatcher.h"


QString ReturnWorkMatcher::sanitized( const QString& name )
{
	// same characters as in FileCollectController::outputFilePath()
	static const QRegularExpression invalidFileNameCharacters( QStringLiteral("[\\x00-\\x1F<>:\"/\\|?*\\x7F]") );
	return QString( name ).replace( invalidFileNameCharacters, QString{} ).trimmed();
}



QString ReturnWorkMatcher::matchFolder( const QStringList& folders, const QString& computerName, const QString& userName )
{
	const auto computer = sanitized( computerName );
	const auto user = sanitized( userName );

	const auto find = [&folders]( const std::function<bool(const QString&)>& matches ) {
		for( const auto& folder : folders )
		{
			if( matches( folder ) )
			{
				return folder;
			}
		}
		return QString{};
	};

	const auto equals = []( const QString& a, const QString& b ) {
		return a.compare( b, Qt::CaseInsensitive ) == 0;
	};

	if( computer.isEmpty() == false && user.isEmpty() == false )
	{
		const auto folder = find( [&]( const QString& f ) {
			return equals( f, user + QLatin1Char('_') + computer );
		} );
		if( folder.isEmpty() == false )
		{
			return folder;
		}
	}

	if( computer.isEmpty() == false )
	{
		const auto folder = find( [&]( const QString& f ) {
			return equals( f, computer ) ||
				   f.endsWith( QLatin1Char('_') + computer, Qt::CaseInsensitive );
		} );
		if( folder.isEmpty() == false )
		{
			return folder;
		}
	}

	if( user.isEmpty() == false )
	{
		return find( [&]( const QString& f ) {
			return equals( f, user ) || f.startsWith( user + QLatin1Char('_'), Qt::CaseInsensitive );
		} );
	}

	return {};
}
