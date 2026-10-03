/*
 * ClassChatTest.cpp - tests for the chat log of the hand raise and chat feature
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

#include "ChatLog.h"

class ClassChatTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void appendAssignsIncreasingIds()
	{
		ChatLog log;
		const auto first = log.append( false, QStringLiteral("  Hello  ") );
		const auto second = log.append( true, QStringLiteral("مرحبا") );

		QCOMPARE( first.id, qint64( 1 ) );
		QCOMPARE( second.id, qint64( 2 ) );
		QCOMPARE( first.text, QStringLiteral("Hello") );
		QVERIFY( second.fromTeacher );
		QCOMPARE( log.lastId(), qint64( 2 ) );
	}

	void blankMessagesAreIgnored()
	{
		ChatLog log;
		QCOMPARE( log.append( false, QStringLiteral(" \n\t ") ).id, qint64( 0 ) );
		QVERIFY( log.messages().isEmpty() );
		QCOMPARE( log.lastId(), qint64( 0 ) );
	}

	void longMessagesAreShortened()
	{
		const auto text = ChatLog::cleanText( QString( ChatLog::MaxTextLength + 50, QLatin1Char('x') ) );
		QCOMPARE( text.size(), ChatLog::MaxTextLength );
	}

	void messagesAfter()
	{
		ChatLog log;
		for( int i = 0; i < 5; ++i )
		{
			log.append( i % 2 == 1, QString::number( i ) );
		}

		const auto newer = log.messagesAfter( 3 );
		QCOMPARE( newer.size(), 2 );
		QCOMPARE( newer.first().id, qint64( 4 ) );
		QCOMPARE( log.messagesAfter( 5 ).size(), 0 );
		QCOMPARE( log.messagesAfter( 0 ).size(), 5 );
	}

	void oldMessagesAreDropped()
	{
		ChatLog log;
		for( int i = 0; i < ChatLog::MaxMessages + 10; ++i )
		{
			log.append( false, QStringLiteral("message") );
		}

		QCOMPARE( log.messages().size(), ChatLog::MaxMessages );
		QCOMPARE( log.messages().first().id, qint64( 11 ) );
		QCOMPARE( log.lastId(), qint64( ChatLog::MaxMessages + 10 ) );
	}

	void jsonRoundTrip()
	{
		ChatLog log;
		log.append( false, QStringLiteral("ⵜⴰⴼⴰⵜ"), QDateTime( QDate( 2026, 10, 2 ), QTime( 8, 30 ) ) );
		log.append( true, QStringLiteral("Line 1\nLine 2") );

		const auto messages = ChatLog::fromJson( ChatLog::toJson( log.messages() ) );
		QCOMPARE( messages, log.messages() );
		QCOMPARE( messages.first().time, QDateTime( QDate( 2026, 10, 2 ), QTime( 8, 30 ) ) );
	}

	void invalidJsonEntriesAreSkipped()
	{
		QJsonArray array;
		array.append( QJsonObject{ { QStringLiteral("id"), QStringLiteral("0") }, { QStringLiteral("text"), QStringLiteral("x") } } );
		array.append( QJsonObject{ { QStringLiteral("id"), QStringLiteral("3") }, { QStringLiteral("text"), QStringLiteral("  ") } } );
		array.append( QStringLiteral("garbage") );
		array.append( QJsonObject{ { QStringLiteral("id"), QStringLiteral("4") }, { QStringLiteral("text"), QStringLiteral("ok") } } );

		const auto messages = ChatLog::fromJson( array );
		QCOMPARE( messages.size(), 1 );
		QCOMPARE( messages.first().id, qint64( 4 ) );
	}

	void addKeepsIdsAndRejectsDuplicates()
	{
		ChatLog server;
		server.append( false, QStringLiteral("a") );
		server.append( true, QStringLiteral("b") );

		ChatLog master;
		for( const auto& message : server.messages() )
		{
			QVERIFY( master.add( message ) );
		}
		QVERIFY( master.add( server.messages().first() ) == false );
		QCOMPARE( master.lastId(), qint64( 2 ) );
		QCOMPARE( master.messages().size(), 2 );
	}
};

QTEST_GUILESS_MAIN(ClassChatTest)
#include "ClassChatTest.moc"
