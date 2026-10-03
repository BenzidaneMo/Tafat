/*
 * StudentSetup.h - files for installing the student computers
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

// Builds the folder that sets up the student computers: the teacher's public
// key, the exported configuration and a script that picks the matching
// installer for the Windows version and installs silently.
class StudentSetup
{
public:
	static QString scriptFileName();
	static QString configFileName();
	static QString keyFileName( const QString& keyName );

	// key names are written into the script, so only allow plain ASCII names
	static bool isValidKeyName( const QString& keyName );

	// names of the keys below the public key directory
	static QStringList publicKeyNames( const QString& publicKeyBaseDir );

	// Windows batch script with CRLF line endings
	static QByteArray installScript( const QString& keyName );

	// writes the key, the configuration and the script, returns the written
	// file names or an error message
	struct Result
	{
		bool success{false};
		QStringList files;
		QString error;
	};
	static Result write( const QString& folder, const QString& keyName, const QString& publicKeyFile );

	// removes the ID of this installation from an exported configuration file
	static bool removeInstallationId( const QString& configFile );

};
