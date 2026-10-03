/*
 * FeatureMessageLimitsTest.cpp - payloads of the Tafat plugins fit into feature messages
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

#include <QBuffer>
#include <QJsonDocument>
#include <QtTest>

#include "AppHistory.h"
#include "ChatLog.h"
#include "FeatureMessage.h"
#include "SystemInventory.h"

// Feature messages are checked by VariantStream when they are received:
// strings may have at most 32768 characters, containers at most 1024
// entries, nesting is limited and only some types are allowed. A message
// that breaks a limit is dropped silently, so the plugins must keep their
// payloads within these limits.
class FeatureMessageLimitsTest : public QObject
{
	Q_OBJECT
private:
	enum class Command { Test = 1 };

	static QVariantMap roundTrip( const QVariantMap& arguments )
	{
		FeatureMessage message{ Feature::Uid::createUuid(), Command::Test };
		for( auto it = arguments.constBegin(); it != arguments.constEnd(); ++it )
		{
			message.addArgument( it.key().toInt(), it.value() );
		}

		QBuffer buffer;
		buffer.open( QBuffer::ReadWrite );
		if( message.sendPlain( &buffer ) == false )
		{
			return { { QStringLiteral("send failed"), true } };
		}
		buffer.seek( 0 );

		FeatureMessage received;
		if( received.receive( &buffer ) == false )
		{
			return { { QStringLiteral("receive failed"), true } };
		}
		return received.arguments();
	}

	static bool survives( const QVariant& value )
	{
		const QVariantMap arguments{ { QStringLiteral("0"), value } };
		return roundTrip( arguments ) == arguments;
	}

private Q_SLOTS:
	void limits()
	{
		// documents the limits the plugins work around
		QVERIFY( survives( QString( 32768, QLatin1Char('x') ) ) );
		QVERIFY( survives( QString( 32769, QLatin1Char('x') ) ) == false );
		QVERIFY( survives( QByteArray( 4 * 1024 * 1024, 'x' ) ) );
		QVERIFY( survives( QStringList( 1024, QStringLiteral("app") ) ) );
		QVERIFY( survives( QStringList( 1025, QStringLiteral("app") ) ) == false );
		QVERIFY( survives( 1.5 ) == false );
		QVERIFY( survives( QDateTime::currentDateTime() ) == false );
		QVERIFY( survives( qint64( 1 ) << 40 ) );
	}

	void chatLog()
	{
		// a full log of long Arabic messages, as sent to a new connection
		ChatLog log;
		const auto text = QString::fromUtf8( "رسالة طويلة من التلميذ " ).repeated( 100 ).left( ChatLog::MaxTextLength );
		for( int i = 0; i < ChatLog::MaxMessages; ++i )
		{
			log.append( i % 2 == 0, text );
		}
		const auto json = QJsonDocument( ChatLog::toJson( log.messagesAfter( 0 ) ) ).toJson( QJsonDocument::Compact );
		QVERIFY( json.size() > 32768 );

		const auto received = roundTrip( { { QStringLiteral("0"), json } } );
		QCOMPARE( ChatLog::fromJson( QJsonDocument::fromJson( received.value( QStringLiteral("0") ).toByteArray() ).array() ).size(),
				  ChatLog::MaxMessages );
	}

	void appHistory()
	{
		AppHistory history;
		for( int i = 0; i < AppHistory::MaxEntries; ++i )
		{
			history.update( { QStringLiteral("application%1").arg( i ) }, 1000 + i );
		}
		QVERIFY( survives( AppHistory::toVariant( history.entries() ) ) );
	}

	void inventory()
	{
		QVERIFY( survives( SystemInventory::collect() ) );
	}
};

QTEST_GUILESS_MAIN(FeatureMessageLimitsTest)
#include "FeatureMessageLimitsTest.moc"
