/*
 * StudentSetup.cpp - files for installing the student computers
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
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include "Configuration/JsonStore.h"
#include "StudentSetup.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"


QString StudentSetup::scriptFileName()
{
	return QStringLiteral("install-students.bat");
}



QString StudentSetup::configFileName()
{
	return VeyonCore::productSlug() + QStringLiteral("-config.json");
}



QString StudentSetup::keyFileName( const QString& keyName )
{
	// same name as "authkeys export" uses
	return QStringLiteral("%1_public_key.pem").arg( keyName );
}



bool StudentSetup::isValidKeyName( const QString& keyName )
{
	static const QRegularExpression keyNameRX{ QStringLiteral("^[A-Za-z0-9_-]+$") };
	return keyNameRX.match( keyName ).hasMatch();
}



QStringList StudentSetup::publicKeyNames( const QString& publicKeyBaseDir )
{
	QStringList names;
	const auto entries = QDir( publicKeyBaseDir ).entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );
	for( const auto& entry : entries )
	{
		if( QFileInfo::exists( QDir( publicKeyBaseDir ).filePath( entry + QStringLiteral("/key") ) ) )
		{
			names.append( entry );
		}
	}
	return names;
}



QString StudentSetup::teacherKeyName( const QString& publicKeyBaseDir )
{
	QStringList names;
	for( const auto& name : publicKeyNames( publicKeyBaseDir ) )
	{
		if( isValidKeyName( name ) )
		{
			names.append( name );
		}
	}
	if( names.contains( QStringLiteral("teacher") ) )
	{
		return QStringLiteral("teacher");
	}
	return names.value( 0 );
}



QByteArray StudentSetup::installScript( const QString& keyName )
{
	const auto slug = VeyonCore::productSlug();
	const auto product = VeyonCore::productName();

	const QStringList lines{
		QStringLiteral("@echo off"),
		QStringLiteral("rem Installs %1 on a student computer. Created by the %1 Configurator.").arg( product ),
		QStringLiteral("rem Copy the %1 installers (%2-...-setup.exe) into this folder and run").arg( product, slug ),
		QStringLiteral("rem this script as administrator on each student computer. It picks the"),
		QStringLiteral("rem installer for the Windows version and installs without the teacher program."),
		QStringLiteral("setlocal enableextensions"),
		QStringLiteral("cd /d \"%~dp0\""),
		QString(),
		QStringLiteral("net session >nul 2>&1"),
		QStringLiteral("if errorlevel 1 ("),
		QStringLiteral("  echo Please run this script as administrator ^(right click, Run as administrator^)."),
		QStringLiteral("  pause"),
		QStringLiteral("  exit /b 1"),
		QStringLiteral(")"),
		QString(),
		QStringLiteral("set ARCH=win32"),
		QStringLiteral("if /i \"%PROCESSOR_ARCHITECTURE%\"==\"AMD64\" set ARCH=win64"),
		QStringLiteral("if defined PROCESSOR_ARCHITEW6432 set ARCH=win64"),
		QString(),
		QStringLiteral("rem Windows 7, 8 and 8.1 (6.1 - 6.3) need the legacy installers"),
		QStringLiteral("set LEGACY="),
		QStringLiteral("for /f \"tokens=2 delims=[]\" %%v in ('ver') do set VERSIONTEXT=%%v"),
		QStringLiteral("for /f \"tokens=2\" %%n in (\"%VERSIONTEXT%\") do set WINVER=%%n"),
		QStringLiteral("if \"%WINVER:~0,4%\"==\"6.1.\" set LEGACY=-legacy"),
		QStringLiteral("if \"%WINVER:~0,4%\"==\"6.2.\" set LEGACY=-legacy"),
		QStringLiteral("if \"%WINVER:~0,4%\"==\"6.3.\" set LEGACY=-legacy"),
		QString(),
		QStringLiteral("set SETUP="),
		QStringLiteral("call :find %ARCH%%LEGACY%"),
		QStringLiteral("if not defined SETUP call :find %ARCH%-legacy"),
		QStringLiteral("if not defined SETUP call :find win32%LEGACY%"),
		QStringLiteral("if not defined SETUP call :find win32-legacy"),
		QStringLiteral("if not defined SETUP ("),
		QStringLiteral("  echo No installer for this computer found. Copy %1-...-%ARCH%%LEGACY%-setup.exe into this folder.").arg( slug ),
		QStringLiteral("  pause"),
		QStringLiteral("  exit /b 1"),
		QStringLiteral(")"),
		QString(),
		QStringLiteral("echo Installing %SETUP% ..."),
		QStringLiteral("\"%SETUP%\" /S /NoMaster /ApplyConfig=\"%~dp0%1\" /ImportPublicKey=\"%~dp0%2\" /PublicKeyName=%3")
			.arg( configFileName(), keyFileName( keyName ), keyName ),
		QStringLiteral("set RESULT=%errorlevel%"),
		QStringLiteral("if not \"%RESULT%\"==\"0\" ("),
		QStringLiteral("  echo The installation failed ^(error %RESULT%^)."),
		QStringLiteral("  pause"),
		QStringLiteral("  exit /b 1"),
		QStringLiteral(")"),
		QStringLiteral("echo %1 is installed.").arg( product ),
		QStringLiteral("exit /b 0"),
		QString(),
		QStringLiteral(":find"),
		QStringLiteral("for %%f in (%1-*-%~1-setup.exe) do set SETUP=%%f").arg( slug ),
		QStringLiteral("goto :eof"),
	};

	return lines.join( QStringLiteral("\r\n") ).toLatin1() + "\r\n";
}



bool StudentSetup::removeInstallationId( const QString& configFile )
{
	QFile file( configFile );
	if( file.open( QFile::ReadOnly ) == false )
	{
		return false;
	}
	auto root = QJsonDocument::fromJson( file.readAll() ).object();
	file.close();

	// every computer keeps its own installation ID
	auto core = root.value( QStringLiteral("Core") ).toObject();
	core.remove( QStringLiteral("InstallationID") );
	root[QStringLiteral("Core")] = core;

	return file.open( QFile::WriteOnly | QFile::Truncate ) &&
		   file.write( QJsonDocument( root ).toJson() ) >= 0;
}



StudentSetup::Result StudentSetup::write( const QString& folder, const QString& keyName, const QString& publicKeyFile )
{
	Result result;

	if( isValidKeyName( keyName ) == false )
	{
		result.error = QStringLiteral("invalid key name");
		return result;
	}

	const QDir dir( folder );
	if( dir.exists() == false && QDir().mkpath( folder ) == false )
	{
		result.error = folder;
		return result;
	}

	const auto keyFile = dir.filePath( keyFileName( keyName ) );
	QFile::remove( keyFile );
	if( QFile::copy( publicKeyFile, keyFile ) == false )
	{
		result.error = keyFile;
		return result;
	}
	result.files.append( keyFileName( keyName ) );

	const auto configFile = dir.filePath( configFileName() );
	QFile::remove( configFile );
	Configuration::JsonStore( Configuration::JsonStore::Scope::System, configFile ).flush( &VeyonCore::config() );
	if( removeInstallationId( configFile ) == false )
	{
		result.error = configFile;
		return result;
	}
	result.files.append( configFileName() );

	QFile script( dir.filePath( scriptFileName() ) );
	if( script.open( QFile::WriteOnly | QFile::Truncate ) == false ||
		script.write( installScript( keyName ) ) < 0 )
	{
		result.error = script.fileName();
		return result;
	}
	result.files.append( scriptFileName() );

	result.success = true;
	return result;
}
