/*
 * ReturnWorkTest.cpp - tests for returning work to the students
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

#include "FileTransferPlugin.h"
#include "ReturnWorkMatcher.h"
#include "ReturnWorkTransfer.h"

// the messages are handled by the upstream file transfer plugin on the student computers
static_assert( int(ReturnWorkProtocol::Command::StartFileTransfer) == int(FileTransferPlugin::FeatureCommand::StartFileTransfer) );
static_assert( int(ReturnWorkProtocol::Command::ContinueFileTransfer) == int(FileTransferPlugin::FeatureCommand::ContinueFileTransfer) );
static_assert( int(ReturnWorkProtocol::Command::CancelFileTransfer) == int(FileTransferPlugin::FeatureCommand::CancelFileTransfer) );
static_assert( int(ReturnWorkProtocol::Command::FinishFileTransfer) == int(FileTransferPlugin::FeatureCommand::FinishFileTransfer) );
static_assert( int(ReturnWorkProtocol::Command::StopWorker) == int(FileTransferPlugin::FeatureCommand::StopWorker) );
static_assert( int(ReturnWorkProtocol::Argument::TransferId) == int(FileTransferPlugin::Argument::TransferId) );
static_assert( int(ReturnWorkProtocol::Argument::FileName) == int(FileTransferPlugin::Argument::FileName) );
static_assert( int(ReturnWorkProtocol::Argument::DataChunk) == int(FileTransferPlugin::Argument::DataChunk) );
static_assert( int(ReturnWorkProtocol::Argument::OpenFileInApplication) == int(FileTransferPlugin::Argument::OpenFileInApplication) );
static_assert( int(ReturnWorkProtocol::Argument::OverwriteExistingFile) == int(FileTransferPlugin::Argument::OverwriteExistingFile) );
static_assert( int(ReturnWorkProtocol::Argument::DestinationDirectory) == int(FileTransferPlugin::Argument::DestinationDirectory) );

class ReturnWorkTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void distributeFeatureUid()
	{
		// must be the UID of the "Distribute" feature in plugins/filetransfer/FileTransferPlugin.cpp
		QFile source( QStringLiteral(SOURCE_DIR "/plugins/filetransfer/FileTransferPlugin.cpp") );
		QVERIFY( source.open( QFile::ReadOnly ) );
		const auto text = QString::fromUtf8( source.readAll() );
		const auto distribute = text.indexOf( QStringLiteral("m_distributeFilesFeature(") );
		QVERIFY( distribute >= 0 );
		QVERIFY( text.indexOf( ReturnWorkProtocol::DistributeFilesFeatureUid.toString( QUuid::WithoutBraces ), distribute ) > distribute );
	}

	void matchesStudentAndComputer()
	{
		const QStringList folders{ QStringLiteral("Amina Benali_PC-01"), QStringLiteral("Yacine Kaci_PC-02"),
								   QStringLiteral("Lina_PC-03") };
		QCOMPARE( ReturnWorkMatcher::matchFolder( folders, QStringLiteral("PC-02"), QStringLiteral("Yacine Kaci") ),
				  QStringLiteral("Yacine Kaci_PC-02") );
		// case does not matter
		QCOMPARE( ReturnWorkMatcher::matchFolder( folders, QStringLiteral("pc-01"), QStringLiteral("amina benali") ),
				  QStringLiteral("Amina Benali_PC-01") );
	}

	void fallsBackToComputerThenStudent()
	{
		const QStringList folders{ QStringLiteral("Amina Benali_PC-01"), QStringLiteral("Yacine Kaci_PC-02") };
		// another student sits at PC-01 now
		QCOMPARE( ReturnWorkMatcher::matchFolder( folders, QStringLiteral("PC-01"), QStringLiteral("Someone Else") ),
				  QStringLiteral("Amina Benali_PC-01") );
		// Yacine moved to another computer
		QCOMPARE( ReturnWorkMatcher::matchFolder( folders, QStringLiteral("PC-09"), QStringLiteral("Yacine Kaci") ),
				  QStringLiteral("Yacine Kaci_PC-02") );
		QVERIFY( ReturnWorkMatcher::matchFolder( folders, QStringLiteral("PC-09"), QStringLiteral("Nobody") ).isEmpty() );
		QVERIFY( ReturnWorkMatcher::matchFolder( folders, QString{}, QString{} ).isEmpty() );
	}

	void computerOnlyFolders()
	{
		const QStringList folders{ QStringLiteral("PC-1"), QStringLiteral("PC-10") };
		QCOMPARE( ReturnWorkMatcher::matchFolder( folders, QStringLiteral("PC-1"), {} ), QStringLiteral("PC-1") );
		QCOMPARE( ReturnWorkMatcher::matchFolder( folders, QStringLiteral("PC-10"), {} ), QStringLiteral("PC-10") );
	}

	void sanitizesLikeTheFileCollection()
	{
		QCOMPARE( ReturnWorkMatcher::sanitized( QStringLiteral("a<b>:c\"d/e|f?g*h") ), QStringLiteral("abcdefgh") );
		const QStringList folders{ QStringLiteral("Ali Haddad_LAB1PC2") };
		QCOMPARE( ReturnWorkMatcher::matchFolder( folders, QStringLiteral("LAB1:PC2"), QStringLiteral("Ali Haddad") ),
				  QStringLiteral("Ali Haddad_LAB1PC2") );
	}
};

QTEST_GUILESS_MAIN(ReturnWorkTest)
#include "ReturnWorkTest.moc"
