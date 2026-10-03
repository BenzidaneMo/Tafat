/*
 * StudentPackage.h - student installer with the teacher key and settings inside
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
#include <QString>
#include <QStringList>

// A student package is a copy of the installer with the teacher's public key
// and the lab settings appended:
//   <installer> <JSON> <JSON length: 8 bytes, little endian> <marker>
// The installer finds the marker at the end of its own file, installs without
// the teacher program and imports key and settings ("labsetup extractpackage").
class StudentPackage
{
public:
	static QByteArray marker();

	struct Content
	{
		QString keyName;
		QByteArray publicKey;
		QByteArray config;

		bool isValid() const;
	};

	// block appended to the installer
	static QByteArray encode( const Content& content );

	static bool hasPackage( const QString& file );
	static Content read( const QString& file );

	// copies the installer to output and appends the content
	static bool create( const QString& installer, const QString& output, const Content& content, QString* error );

	// writes <key>_public_key.pem and the configuration file into folder
	static QStringList extract( const QString& file, const QString& folder, QString* error );

	// the teacher's public key and the current configuration (without the installation ID)
	static Content fromCurrentConfiguration( const QString& keyName, const QString& publicKeyFile, QString* error );

	// "Windows 10-11 64-bit" etc. from the installer file name, empty if unknown
	static QString windowsVersions( const QString& installerFileName );
	// "Install Tafat - student (Windows 10-11 64-bit).exe"
	static QString packageFileName( const QString& installerFileName );

private:
	static constexpr int LengthSize = 8;
	static constexpr qint64 MaximumContentSize = 4 * 1024 * 1024;

	// size of the JSON block before a trailer (length + marker), 0 if there is no package
	static qint64 contentSize( const QByteArray& trailer, qint64 fileSize );

};
