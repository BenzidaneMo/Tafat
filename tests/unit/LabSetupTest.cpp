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

#include <QNetworkAddressEntry>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

#include "ComputerScanner.h"
#include "StudentPackage.h"
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

	void studentPackage()
	{
		QTemporaryDir dir;
		const auto installer = dir.filePath( QStringLiteral("tafat-1.0.1.0-win64-setup.exe") );
		const QByteArray installerData = QByteArray( "MZ" ) + QByteArray( 5000, 'x' ) + "end";
		QFile file( installer );
		QVERIFY( file.open( QFile::WriteOnly ) );
		file.write( installerData );
		file.close();

		// a plain installer is no package
		QVERIFY( StudentPackage::hasPackage( installer ) == false );

		StudentPackage::Content content;
		content.keyName = QStringLiteral("teacher");
		content.publicKey = "-----BEGIN PUBLIC KEY-----\nabc\n-----END PUBLIC KEY-----\n";
		content.config = R"({"Authentication":{"Method":1},"Network":{"VeyonServerPort":11100}})";
		QVERIFY( content.isValid() );

		QString error;
		const auto package = dir.filePath( QStringLiteral("student.exe") );
		QVERIFY2( StudentPackage::create( installer, package, content, &error ), qPrintable( error ) );

		// the installer stays unchanged in front, the trailer ends with the marker read by NSIS
		QFile packageFile( package );
		QVERIFY( packageFile.open( QFile::ReadOnly ) );
		const auto packageData = packageFile.readAll();
		packageFile.close();
		QVERIFY( packageData.startsWith( installerData ) );
		QVERIFY( packageData.endsWith( StudentPackage::marker() ) );
		QCOMPARE( StudentPackage::marker().size(), 24 );

		const auto read = StudentPackage::read( package );
		QVERIFY( read.isValid() );
		QCOMPARE( read.keyName, content.keyName );
		QCOMPARE( read.publicKey, content.publicKey );
		QCOMPARE( QJsonDocument::fromJson( read.config ), QJsonDocument::fromJson( content.config ) );

		// a package made from a package replaces the old content instead of adding to it
		auto other = content;
		other.keyName = QStringLiteral("lab2");
		const auto repackaged = dir.filePath( QStringLiteral("student2.exe") );
		QVERIFY( StudentPackage::create( package, repackaged, other, &error ) );
		QCOMPARE( StudentPackage::read( repackaged ).keyName, QStringLiteral("lab2") );
		QCOMPARE( QFileInfo( repackaged ).size(), qint64( installerData.size() + StudentPackage::encode( other ).size() ) );

		const auto folder = dir.filePath( QStringLiteral("extracted") );
		const auto files = StudentPackage::extract( package, folder, &error );
		QCOMPARE( files.size(), 2 );
		QFile key( QDir( folder ).filePath( StudentSetup::keyFileName( QStringLiteral("teacher") ) ) );
		QVERIFY( key.open( QFile::ReadOnly ) );
		QCOMPARE( key.readAll(), content.publicKey );
		QFile config( QDir( folder ).filePath( StudentSetup::configFileName() ) );
		QVERIFY( config.open( QFile::ReadOnly ) );
		QCOMPARE( QJsonDocument::fromJson( config.readAll() )[QStringLiteral("Authentication")][QStringLiteral("Method")].toInt(), 1 );

		QVERIFY( StudentPackage::extract( installer, folder, &error ).isEmpty() );
		QVERIFY( error.isEmpty() == false );

		// invalid content is refused
		auto invalid = content;
		invalid.keyName = QStringLiteral("a b");
		QVERIFY( StudentPackage::create( installer, dir.filePath( QStringLiteral("x.exe") ), invalid, &error ) == false );
	}

	void damagedPackage()
	{
		QTemporaryDir dir;
		const auto fileName = dir.filePath( QStringLiteral("damaged.exe") );
		QFile file( fileName );
		QVERIFY( file.open( QFile::WriteOnly ) );
		// marker with a length larger than the file
		file.write( QByteArray( "MZ" ) + QByteArray( "\xff\xff\x00\x00\x00\x00\x00\x00", 8 ) + StudentPackage::marker() );
		file.close();
		QVERIFY( StudentPackage::hasPackage( fileName ) == false );
	}

	void packageNames()
	{
		QCOMPARE( StudentPackage::windowsVersions( QStringLiteral("C:/x/tafat-1.0.0.0-win64-setup.exe") ),
				  QStringLiteral("Windows 10-11 64-bit") );
		QCOMPARE( StudentPackage::windowsVersions( QStringLiteral("tafat-1.0.0.0-win32-setup.exe") ),
				  QStringLiteral("Windows 10 32-bit") );
		QCOMPARE( StudentPackage::windowsVersions( QStringLiteral("tafat-1.0.0.0-win32-legacy-setup.exe") ),
				  QStringLiteral("Windows 7-8.1 32-bit") );
		QCOMPARE( StudentPackage::windowsVersions( QStringLiteral("tafat-1.0.0.0-win64-legacy-setup.exe") ),
				  QStringLiteral("Windows 7-8.1 64-bit") );
		QVERIFY( StudentPackage::windowsVersions( QStringLiteral("setup.exe") ).isEmpty() );
		QVERIFY( StudentPackage::packageFileName( QStringLiteral("tafat-1.0.0.0-win64-setup.exe") )
					 .endsWith( QStringLiteral(" - student (Windows 10-11 64-bit).exe") ) );
	}

	void candidateHosts()
	{
		QNetworkAddressEntry lab;
		lab.setIp( QHostAddress( QStringLiteral("192.168.1.10") ) );
		lab.setPrefixLength( 24 );

		QNetworkAddressEntry publicEntry;
		publicEntry.setIp( QHostAddress( QStringLiteral("8.8.4.4") ) );
		publicEntry.setPrefixLength( 24 );

		const auto own = QList<QHostAddress>{ QHostAddress( QStringLiteral("192.168.1.10") ) };
		const auto hosts = ComputerScanner::candidateHosts( { lab, publicEntry, lab }, own );
		// .1 to .254 without the own address and without duplicates, no public addresses
		QCOMPARE( hosts.size(), 253 );
		QVERIFY( hosts.contains( QHostAddress( QStringLiteral("192.168.1.1") ) ) );
		QVERIFY( hosts.contains( QHostAddress( QStringLiteral("192.168.1.254") ) ) );
		QVERIFY( hosts.contains( QHostAddress( QStringLiteral("192.168.1.10") ) ) == false );
		QVERIFY( hosts.contains( QHostAddress( QStringLiteral("192.168.1.255") ) ) == false );

		// a /16 network is cut to the /24 around this computer
		QNetworkAddressEntry large;
		large.setIp( QHostAddress( QStringLiteral("10.20.30.40") ) );
		large.setPrefixLength( 16 );
		const auto largeHosts = ComputerScanner::candidateHosts( { large }, {} );
		QCOMPARE( largeHosts.size(), 254 );
		QCOMPARE( largeHosts.first(), QHostAddress( QStringLiteral("10.20.30.1") ) );

		// the limit holds over several networks
		QNetworkAddressEntry second;
		second.setIp( QHostAddress( QStringLiteral("172.16.5.1") ) );
		second.setPrefixLength( 24 );
		QCOMPARE( ComputerScanner::candidateHosts( { lab, second }, own, 300 ).size(), 300 );

		QVERIFY( ComputerScanner::isPrivateAddress( QHostAddress( QStringLiteral("172.31.255.1") ) ) );
		QVERIFY( ComputerScanner::isPrivateAddress( QHostAddress( QStringLiteral("172.32.0.1") ) ) == false );
		QVERIFY( ComputerScanner::isPrivateAddress( QHostAddress( QStringLiteral("fe80::1") ) ) == false );
	}

	void rangeHosts()
	{
		const auto subnet = ComputerScanner::rangeHosts( QStringLiteral("10.0.5.77/24") );
		QCOMPARE( subnet.size(), 254 );
		QCOMPARE( subnet.first(), QHostAddress( QStringLiteral("10.0.5.1") ) );
		QCOMPARE( subnet.last(), QHostAddress( QStringLiteral("10.0.5.254") ) );

		QCOMPARE( ComputerScanner::rangeHosts( QStringLiteral("172.16.0.0/22") ).size(), 1022 );
		QVERIFY( ComputerScanner::rangeHosts( QStringLiteral("10.0.0.0/21") ).isEmpty() );	// too large

		const auto shortRange = ComputerScanner::rangeHosts( QStringLiteral(" 192.168.1.10-80 ") );
		QCOMPARE( shortRange.size(), 71 );
		QCOMPARE( shortRange.last(), QHostAddress( QStringLiteral("192.168.1.80") ) );
		QCOMPARE( ComputerScanner::rangeHosts( QStringLiteral("192.168.1.250-192.168.2.5") ).size(), 12 );
		QCOMPARE( ComputerScanner::rangeHosts( QStringLiteral("192.168.3.7") ).size(), 1 );

		QVERIFY( ComputerScanner::rangeHosts( QStringLiteral("192.168.1.80-10") ).isEmpty() );
		QVERIFY( ComputerScanner::rangeHosts( QStringLiteral("8.8.8.0/24") ).isEmpty() );	// public
		QVERIFY( ComputerScanner::rangeHosts( QStringLiteral("pc-01") ).isEmpty() );
		QVERIFY( ComputerScanner::rangeHosts( QStringLiteral("10.0.0.1/abc") ).isEmpty() );
		QVERIFY( ComputerScanner::rangeHosts( QString() ).isEmpty() );
	}

	void serverGreeting()
	{
		QVERIFY( ComputerScanner::isServerGreeting( "RFB 003.008\n" ) );
		QVERIFY( ComputerScanner::isServerGreeting( "SSH-2.0-OpenSSH" ) == false );
		QVERIFY( ComputerScanner::isServerGreeting( "HTTP/1.1 400" ) == false );
	}

	void scanner()
	{
		// one fake server answering like a VNC server, one with another greeting
		QTcpServer vnc;
		QTcpServer other;
		QVERIFY( vnc.listen( QHostAddress::LocalHost ) );
		QVERIFY( other.listen( QHostAddress::LocalHost ) );
		connect( &vnc, &QTcpServer::newConnection, &vnc, [&vnc]() {
			vnc.nextPendingConnection()->write( "RFB 003.008\n" );
		} );
		connect( &other, &QTcpServer::newConnection, &other, [&other]() {
			other.nextPendingConnection()->write( "SSH-2.0-test\r\n" );
		} );

		const QList<QHostAddress> hosts{ QHostAddress::LocalHost };
		for( const auto server : { &vnc, &other } )
		{
			ComputerScanner scanner( server->serverPort() );
			int found = 0;
			connect( &scanner, &ComputerScanner::found, &scanner, [&found]() { ++found; } );
			QSignalSpy finished( &scanner, &ComputerScanner::finished );
			scanner.start( hosts );
			QVERIFY( finished.wait( 5000 ) );
			QCOMPARE( found, server == &vnc ? 1 : 0 );
			QVERIFY( scanner.isRunning() == false );
		}
	}

	void importData()
	{
		QCOMPARE( ComputerScanner::importData( { { QStringLiteral("PC-01"), QStringLiteral("192.168.1.11") },
												 { QStringLiteral(" a;b "), QStringLiteral("pc2.lab ") } } ),
				  QByteArray( "PC-01;192.168.1.11\na b;pc2.lab" ) );
	}
};

QTEST_GUILESS_MAIN(LabSetupTest)
#include "LabSetupTest.moc"
