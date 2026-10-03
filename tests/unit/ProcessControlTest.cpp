/*
 * ProcessControlTest.cpp - tests for application control decisions
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

#include "ProcessControl.h"
#include "UsbStorageBlocker.h"

using Process = ProcessControl::Process;
using Policy = ProcessControl::Policy;

class ProcessControlTest : public QObject
{
	Q_OBJECT
private:
	static QStringList names( const QList<Process>& processes )
	{
		QStringList result;
		for( const auto& process : processes )
		{
			result.append( process.name );
		}
		return result;
	}

	static QList<Process> sampleProcesses()
	{
		return {
			{ 1, QStringLiteral("chrome"), true },
			{ 2, QStringLiteral("notepad"), true },
			{ 3, QStringLiteral("backgroundservice"), false },
			{ 4, QStringLiteral("explorer"), true },
			{ 5, QStringLiteral("tafat-worker"), true },
			{ 6, QStringLiteral("game"), true },
		};
	}

private Q_SLOTS:
	void normalizedName_data()
	{
		QTest::addColumn<QString>("executable");
		QTest::addColumn<QString>("expected");

		QTest::newRow("windows path") << QStringLiteral("C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe") << QStringLiteral("chrome");
		QTest::newRow("unix path") << QStringLiteral("/usr/lib/firefox/firefox") << QStringLiteral("firefox");
		QTest::newRow("upper case with spaces") << QStringLiteral("  Game.EXE ") << QStringLiteral("game");
		QTest::newRow("plain name") << QStringLiteral("notepad") << QStringLiteral("notepad");
		QTest::newRow("empty") << QString() << QString();
	}

	void normalizedName()
	{
		QFETCH(QString, executable);
		QFETCH(QString, expected);

		QCOMPARE( ProcessControl::normalizedName( executable ), expected );
	}

	void protectedProcesses()
	{
		QVERIFY( ProcessControl::isProtected( QStringLiteral("tafat-worker") ) );
		QVERIFY( ProcessControl::isProtected( QStringLiteral("explorer") ) );
		QVERIFY( ProcessControl::isProtected( QStringLiteral("chrome") ) == false );
	}

	void blockListedClosesOnlyListedApplications()
	{
		const auto closed = ProcessControl::processesToClose( sampleProcesses(), Policy::BlockListed,
															  { QStringLiteral("chrome"), QStringLiteral("explorer"),
																QStringLiteral("backgroundservice") }, true );

		// the protected explorer survives, listed processes without window are closed too
		QCOMPARE( names( closed ), QStringList( { QStringLiteral("chrome"), QStringLiteral("backgroundservice") } ) );
	}

	void allowListedOnlyClosesOtherWindowedApplications()
	{
		const auto closed = ProcessControl::processesToClose( sampleProcesses(), Policy::AllowListedOnly,
															  { QStringLiteral("notepad") }, true );

		QCOMPARE( names( closed ), QStringList( { QStringLiteral("chrome"), QStringLiteral("game") } ) );
	}

	void allowListedOnlyNeedsWindowDetection()
	{
		const auto closed = ProcessControl::processesToClose( sampleProcesses(), Policy::AllowListedOnly,
															  { QStringLiteral("notepad") }, false );

		QVERIFY( closed.isEmpty() );
	}

	void usbStoragePolicyIsRestored()
	{
		// a value an administrator set before is written back
		QCOMPARE( UsbStorageBlocker::restoredValue( { true, true, 0 } ), QVariant( 0 ) );
		QCOMPARE( UsbStorageBlocker::restoredValue( { true, true, 1 } ), QVariant( 1 ) );
		// without a previous value the policy value is removed
		QVERIFY( UsbStorageBlocker::restoredValue( { true, false, {} } ).isValid() == false );
	}

	void openApplications()
	{
		const QList<ProcessControl::Process> processes{
			{ 1, QStringLiteral("firefox"), true },
			{ 2, QStringLiteral("firefox"), true },
			{ 3, QStringLiteral("svchost"), false },
			{ 4, QStringLiteral("explorer"), true },
			{ 5, QStringLiteral("tafat-worker"), true },
			{ 6, QStringLiteral("game"), true },
		};
		QCOMPARE( ProcessControl::openApplications( processes, true ),
				  ( QStringList{ QStringLiteral("firefox"), QStringLiteral("game") } ) );
		// without window detection all processes count, protected ones still not
		QCOMPARE( ProcessControl::openApplications( processes, false ),
				  ( QStringList{ QStringLiteral("firefox"), QStringLiteral("game"), QStringLiteral("svchost") } ) );
	}
};

QTEST_GUILESS_MAIN(ProcessControlTest)

#include "ProcessControlTest.moc"
