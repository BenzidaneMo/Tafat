/*
 * HandInTest.cpp - tests for handing in files to the teacher
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

#include <QTemporaryDir>
#include <QtTest>

#include "HandIn.h"

class HandInTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void splitAndAssemble()
	{
		QByteArray data;
		for( int i = 0; i < 1000; ++i )
		{
			data.append( QByteArray::number( i ) );
		}

		const auto chunks = HandIn::split( QStringLiteral("devoir.docx"), data, 100 );
		QCOMPARE( chunks.size(), int( ( data.size() + 99 ) / 100 ) );
		QCOMPARE( chunks.last().count, chunks.size() );

		// parts may arrive in any order
		HandInAssembler assembler;
		for( int i = chunks.size() - 1; i > 0; --i )
		{
			QVERIFY( assembler.add( QStringLiteral("pc1"), chunks[i] ).has_value() == false );
		}
		const auto file = assembler.add( QStringLiteral("pc1"), chunks.first() );
		QVERIFY( file.has_value() );
		QCOMPARE( file->fileName, QStringLiteral("devoir.docx") );
		QCOMPARE( file->data, data );
		QCOMPARE( assembler.pendingFiles(), 0 );
	}

	void emptyFile()
	{
		const auto chunks = HandIn::split( QStringLiteral("empty.txt"), {} );
		QCOMPARE( chunks.size(), 1 );
		HandInAssembler assembler;
		const auto file = assembler.add( QStringLiteral("pc1"), chunks.first() );
		QVERIFY( file.has_value() );
		QVERIFY( file->data.isEmpty() );
	}

	void computersAreKeptApart()
	{
		const auto chunks = HandIn::split( QStringLiteral("a.txt"), QByteArray( 250, 'x' ), 100 );
		HandInAssembler assembler;
		assembler.add( QStringLiteral("pc1"), chunks[0] );
		assembler.add( QStringLiteral("pc2"), chunks[1] );
		assembler.add( QStringLiteral("pc2"), chunks[2] );
		QCOMPARE( assembler.pendingFiles(), 2 );
	}

	void invalidChunksAreIgnored()
	{
		HandInAssembler assembler;
		QVERIFY( assembler.add( QStringLiteral("pc1"), { QUuid::createUuid(), QStringLiteral("x"), 3, 2, {} } ).has_value() == false );
		QVERIFY( assembler.add( QStringLiteral("pc1"), { QUuid::createUuid(), QStringLiteral("x"), 0, 0, {} } ).has_value() == false );
		QVERIFY( assembler.add( QStringLiteral("pc1"), { QUuid::createUuid(), QStringLiteral("x"), 0, 1000000, {} } ).has_value() == false );
		QCOMPARE( assembler.pendingFiles(), 0 );
	}

	void safeFileNames()
	{
		QCOMPARE( HandIn::safeFileName( QStringLiteral("../../etc/passwd") ), QStringLiteral("passwd") );
		QCOMPARE( HandIn::safeFileName( QStringLiteral("C:\\Users\\eleve\\Documents\\devoir.docx") ), QStringLiteral("devoir.docx") );
		QCOMPARE( HandIn::safeFileName( QStringLiteral("..") ), QStringLiteral("file") );
		QCOMPARE( HandIn::safeFileName( QStringLiteral(".hidden") ), QStringLiteral("hidden") );
		QCOMPARE( HandIn::safeFileName( QStringLiteral("a<b>c?.txt.") ), QStringLiteral("abc.txt") );
		QCOMPARE( HandIn::safeFileName( QStringLiteral("فرض الرياضيات.pdf") ), QStringLiteral("فرض الرياضيات.pdf") );
		QVERIFY( HandIn::safeFileName( QString( 300, QLatin1Char('a') ) + QStringLiteral(".pdf") ).size() <= 150 );
		QVERIFY( HandIn::safeFileName( QString( 300, QLatin1Char('a') ) + QStringLiteral(".pdf") ).endsWith( QStringLiteral(".pdf") ) );
	}

	void uniqueFilePaths()
	{
		QTemporaryDir dir;
		QVERIFY( dir.isValid() );
		const auto first = HandIn::uniqueFilePath( dir.path(), QStringLiteral("devoir.docx") );
		QVERIFY( first.endsWith( QStringLiteral("/devoir.docx") ) );
		QFile( first ).open( QFile::WriteOnly );
		const auto second = HandIn::uniqueFilePath( dir.path(), QStringLiteral("devoir.docx") );
		QVERIFY( second.endsWith( QStringLiteral("/devoir (2).docx") ) );
	}

	void queueKeepsOrderAndLimits()
	{
		HandInQueue queue;
		const auto start = QDateTime( QDate( 2026, 10, 3 ), QTime( 8, 0 ), Qt::UTC );
		for( int i = 0; i < 5; ++i )
		{
			queue.append( { QUuid::createUuid(), QStringLiteral("f"), 0, 1, QByteArray( 10, 'x' ) }, start );
		}
		QCOMPARE( queue.lastSequence(), qint64( 5 ) );
		QCOMPARE( queue.chunksAfter( 0, 3 ).size(), 3 );
		QCOMPARE( queue.chunksAfter( 3, 10 ).size(), 2 );
		QCOMPARE( queue.chunksAfter( 3, 10 ).first().first, qint64( 4 ) );
		QCOMPARE( queue.queuedBytes(), qint64( 50 ) );

		// old parts are dropped
		queue.purge( start.addSecs( HandInQueue::KeepSeconds + 1 ) );
		QCOMPARE( queue.chunksAfter( 0, 10 ).size(), 0 );
		QCOMPARE( queue.queuedBytes(), qint64( 0 ) );
	}

	void limitsFitTheQueue()
	{
		QVERIFY( HandIn::MaxTotalSize < HandInQueue::MaxQueuedBytes );
		QVERIFY( HandIn::MaxFileSize <= HandIn::MaxTotalSize );
	}
};

QTEST_GUILESS_MAIN(HandInTest)
#include "HandInTest.moc"
