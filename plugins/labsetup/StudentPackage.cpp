/*
 * StudentPackage.cpp - student installer with the teacher key and settings inside
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

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include "Configuration/JsonStore.h"
#include "StudentPackage.h"
#include "StudentSetup.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"


QByteArray StudentPackage::marker()
{
	// read by the installer (fixed length of 24 characters)
	return QByteArrayLiteral("TAFAT-STUDENT-PACKAGE-v1");
}



bool StudentPackage::Content::isValid() const
{
	return StudentSetup::isValidKeyName( keyName ) &&
		   publicKey.contains( "PUBLIC KEY" ) &&
		   QJsonDocument::fromJson( config ).isObject();
}



QByteArray StudentPackage::encode( const Content& content )
{
	const QJsonObject json{
		{ QStringLiteral("version"), 1 },
		{ QStringLiteral("keyName"), content.keyName },
		{ QStringLiteral("publicKey"), QString::fromLatin1( content.publicKey ) },
		{ QStringLiteral("config"), QJsonDocument::fromJson( content.config ).object() }
	};
	const auto data = QJsonDocument( json ).toJson( QJsonDocument::Compact );

	QByteArray length( LengthSize, 0 );
	auto size = quint64( data.size() );
	for( int i = 0; i < LengthSize; ++i )
	{
		length[i] = char( size & 0xff );
		size >>= 8;
	}

	return data + length + marker();
}



bool StudentPackage::hasPackage( const QString& file )
{
	return read( file ).isValid();
}



StudentPackage::Content StudentPackage::read( const QString& file )
{
	QFile input( file );
	const auto trailerSize = qint64( LengthSize + marker().size() );
	if( input.open( QFile::ReadOnly ) == false || input.size() < trailerSize )
	{
		return {};
	}

	input.seek( input.size() - trailerSize );
	const auto size = contentSize( input.read( trailerSize ), input.size() );
	if( size <= 0 )
	{
		return {};
	}

	input.seek( input.size() - trailerSize - size );
	const auto json = QJsonDocument::fromJson( input.read( size ) ).object();
	if( json.value( QStringLiteral("version") ).toInt() != 1 )
	{
		return {};
	}

	Content content;
	content.keyName = json.value( QStringLiteral("keyName") ).toString();
	content.publicKey = json.value( QStringLiteral("publicKey") ).toString().toLatin1();
	content.config = QJsonDocument( json.value( QStringLiteral("config") ).toObject() ).toJson();
	return content;
}



qint64 StudentPackage::contentSize( const QByteArray& trailer, qint64 fileSize )
{
	const auto trailerSize = qint64( LengthSize + marker().size() );
	if( trailer.size() != trailerSize || trailer.mid( LengthSize ) != marker() )
	{
		return 0;
	}

	quint64 size = 0;
	for( int i = LengthSize - 1; i >= 0; --i )
	{
		size = ( size << 8 ) | quint8( trailer[i] );
	}
	if( size == 0 || size > quint64( MaximumContentSize ) || qint64( size ) > fileSize - trailerSize )
	{
		return 0;
	}

	return qint64( size );
}



bool StudentPackage::create( const QString& installer, const QString& output, const Content& content, QString* error )
{
	const auto fail = [error]( const QString& message ) {
		if( error )
		{
			*error = message;
		}
		return false;
	};

	if( content.isValid() == false )
	{
		return fail( QStringLiteral("invalid key or settings") );
	}

	QFile input( installer );
	if( input.open( QFile::ReadOnly ) == false )
	{
		return fail( installer );
	}

	auto installerData = input.readAll();
	// a package made from a package: keep only the installer
	const auto trailerSize = LengthSize + marker().size();
	const auto existingSize = contentSize( installerData.right( trailerSize ), installerData.size() );
	if( existingSize > 0 )
	{
		installerData.chop( int( existingSize ) + trailerSize );
	}

	QFile out( output );
	if( out.open( QFile::WriteOnly | QFile::Truncate ) == false ||
		out.write( installerData ) != installerData.size() )
	{
		return fail( output );
	}

	const auto block = encode( content );
	if( out.write( block ) != block.size() )
	{
		return fail( output );
	}

	return true;
}



QStringList StudentPackage::extract( const QString& file, const QString& folder, QString* error )
{
	const auto content = read( file );
	if( content.isValid() == false )
	{
		if( error )
		{
			*error = QStringLiteral("%1 does not contain a student package").arg( file );
		}
		return {};
	}

	QDir().mkpath( folder );
	const QDir dir( folder );

	const QList<QPair<QString, QByteArray>> files{
		{ StudentSetup::keyFileName( content.keyName ), content.publicKey },
		{ StudentSetup::configFileName(), content.config }
	};

	QStringList written;
	for( const auto& entry : files )
	{
		QFile out( dir.filePath( entry.first ) );
		if( out.open( QFile::WriteOnly | QFile::Truncate ) == false || out.write( entry.second ) != entry.second.size() )
		{
			if( error )
			{
				*error = out.fileName();
			}
			return {};
		}
		written.append( out.fileName() );
	}

	return written;
}



StudentPackage::Content StudentPackage::fromCurrentConfiguration( const QString& keyName, const QString& publicKeyFile,
																  QString* error )
{
	Content content;
	content.keyName = keyName;

	QFile key( publicKeyFile );
	if( key.open( QFile::ReadOnly ) == false )
	{
		if( error )
		{
			*error = publicKeyFile;
		}
		return {};
	}
	content.publicKey = key.readAll();

	QTemporaryDir tempDir;
	const auto configFile = tempDir.filePath( StudentSetup::configFileName() );
	Configuration::JsonStore( Configuration::JsonStore::Scope::System, configFile ).flush( &VeyonCore::config() );
	QFile config( configFile );
	if( StudentSetup::removeInstallationId( configFile ) == false || config.open( QFile::ReadOnly ) == false )
	{
		if( error )
		{
			*error = configFile;
		}
		return {};
	}
	content.config = config.readAll();

	return content;
}



QString StudentPackage::windowsVersions( const QString& installerFileName )
{
	const auto name = QFileInfo( installerFileName ).fileName().toLower();
	if( name.endsWith( QLatin1String("-win64-legacy-setup.exe") ) )
	{
		return QStringLiteral("Windows 7-8.1 64-bit");
	}
	if( name.endsWith( QLatin1String("-win32-legacy-setup.exe") ) )
	{
		return QStringLiteral("Windows 7-8.1 32-bit");
	}
	if( name.endsWith( QLatin1String("-win64-setup.exe") ) )
	{
		return QStringLiteral("Windows 10-11 64-bit");
	}
	if( name.endsWith( QLatin1String("-win32-setup.exe") ) )
	{
		return QStringLiteral("Windows 10 32-bit");
	}
	return {};
}



QString StudentPackage::packageFileName( const QString& installerFileName )
{
	const auto versions = windowsVersions( installerFileName );
	if( versions.isEmpty() )
	{
		return QStringLiteral("Install %1 - student.exe").arg( VeyonCore::productName() );
	}
	return QStringLiteral("Install %1 - student (%2).exe").arg( VeyonCore::productName(), versions );
}
