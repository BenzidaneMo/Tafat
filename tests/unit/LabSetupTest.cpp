/*
 * LabSetupTest.cpp - files of the student setup exported by the configurator
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

#include <QtTest>

#include "StudentSetup.h"
#include "VeyonCore.h"

class LabSetupTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void keyNames()
	{
		QVERIFY( StudentSetup::isValidKeyName( QStringLiteral("teacher") ) );
		QVERIFY( StudentSetup::isValidKeyName( QStringLiteral("lab-2_b") ) );
		// would break the batch script
		QVERIFY( StudentSetup::isValidKeyName( QStringLiteral("a&b") ) == false );
		QVERIFY( StudentSetup::isValidKeyName( QStringLiteral("a b") ) == false );
		QVERIFY( StudentSetup::isValidKeyName( QStringLiteral("%PATH%") ) == false );
		QVERIFY( StudentSetup::isValidKeyName( QString::fromUtf8( "أستاذ" ) ) == false );
		QVERIFY( StudentSetup::isValidKeyName( QString() ) == false );

		QCOMPARE( StudentSetup::keyFileName( QStringLiteral("teacher") ), QStringLiteral("teacher_public_key.pem") );
	}

	void publicKeyNames()
	{
		QTemporaryDir dir;
		QVERIFY( QDir( dir.path() ).mkpath( QStringLiteral("teacher") ) );
		QVERIFY( QDir( dir.path() ).mkpath( QStringLiteral("empty") ) );
		QFile key( dir.filePath( QStringLiteral("teacher/key") ) );
		QVERIFY( key.open( QFile::WriteOnly ) );
		key.close();

		QCOMPARE( StudentSetup::publicKeyNames( dir.path() ), QStringList{ QStringLiteral("teacher") } );
		QVERIFY( StudentSetup::publicKeyNames( dir.filePath( QStringLiteral("missing") ) ).isEmpty() );
	}

	void removeInstallationId()
	{
		QTemporaryDir dir;
		const auto fileName = dir.filePath( QStringLiteral("config.json") );
		QFile file( fileName );
		QVERIFY( file.open( QFile::WriteOnly ) );
		file.write( R"({"Core":{"InstallationID":"1234","ApplicationVersion":11},"Master":{"X":1}})" );
		file.close();

		QVERIFY( StudentSetup::removeInstallationId( fileName ) );

		QVERIFY( file.open( QFile::ReadOnly ) );
		const auto root = QJsonDocument::fromJson( file.readAll() ).object();
		QVERIFY( root[QStringLiteral("Core")].toObject().contains( QStringLiteral("InstallationID") ) == false );
		QCOMPARE( root[QStringLiteral("Core")].toObject()[QStringLiteral("ApplicationVersion")].toInt(), 11 );
		QCOMPARE( root[QStringLiteral("Master")].toObject()[QStringLiteral("X")].toInt(), 1 );

		QVERIFY( StudentSetup::removeInstallationId( dir.filePath( QStringLiteral("missing.json") ) ) == false );
	}

	void installScript()
	{
		const auto script = StudentSetup::installScript( QStringLiteral("teacher") );

		// batch files need CRLF line endings and plain ASCII
		QVERIFY( script.endsWith( "\r\n" ) );
		QCOMPARE( script.count( '\n' ), script.count( "\r\n" ) );
		for( const auto c : script )
		{
			QVERIFY( uchar( c ) < 128 );
		}

		const auto text = QString::fromLatin1( script );
		const auto slug = VeyonCore::productSlug();
		QVERIFY( text.contains( QStringLiteral("\"%SETUP%\" /S /NoMaster /ApplyConfig=\"%~dp0%1\" "
											   "/ImportPublicKey=\"%~dp0teacher_public_key.pem\" /PublicKeyName=teacher")
									.arg( StudentSetup::configFileName() ) ) );
		QVERIFY( text.contains( QStringLiteral("for %%f in (%1-*-%~1-setup.exe) do set SETUP=%%f").arg( slug ) ) );
		QVERIFY( text.contains( QStringLiteral("set LEGACY=-legacy") ) );
		QVERIFY( text.contains( QStringLiteral("net session") ) );
		QVERIFY( StudentSetup::configFileName().startsWith( slug ) );
	}
};

QTEST_GUILESS_MAIN(LabSetupTest)
#include "LabSetupTest.moc"
